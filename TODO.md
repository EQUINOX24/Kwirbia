# Rendering Roadmap

Goal: near-zero CPU cost for drawing the ASCII grid, with the GPU doing the work.
End state is a **GPU-resident world** (step 5): the CPU only sends the camera position
and small changes, and the shader turns tile data directly into pixels.

Constraints that stay in place throughout:

- **Zoom stays discrete**: 4 glyph sizes (8x8, 12x12, 18x18, 24x24), one atlas PNG each.
  Zooming means switching the active atlas and recomputing the grid size, exactly as now.
  No fractional zoom or scaling of glyphs.
- **Camera moves in whole cells** (no smooth or sub-cell scrolling).
- F3 perf overlay, WASD/arrows (+Shift sprint), Q/E / PgUp/PgDn / `<` `>` layer switching,
  Ctrl +/- zoom and the 2-row HUD must keep working after every step.

Benchmark setup used for the numbers below: window maximized at 2560x1440 (1379 px tall client area),
8x8 zoom -> grid 320x172 (55.0k cells), RTX 4090, 2 s averages from the F3 overlay.

| Stage | FPS | CPU work | World | Render |
|---|---|---|---|---|
| Original (no `-O`, SDL_Renderer through sdl2-compat) | 44.6 | 21.39 ms | 1.41 ms | 19.96 ms |
| Step 1 only (`-O2`) | 62.0 | 3.20 ms | 0.25 ms | 2.91 ms |
| Steps 1 + 2 (native SDL3 + OpenGL 3.3) | 60.0 | ~1.0 ms | 0.24 ms | ~0.75 ms |

---

## [x] Step 1: Quick wins

- [x] `-O2` by default in the Makefile (`make DEBUG=1` for `-O0 -g`).
- [x] Stop stacking `SDL_Delay` on top of VSync. `TimeManager` now uses nanosecond timing
      (`SDL_GetTicksNS`) and only sleeps (`SDL_DelayPrecise`) when VSync can't pace the frames.

## [x] Step 2: Native SDL3 + OpenGL 3.3 core

- [x] Port from SDL2 (which was the `sdl2-compat` layer over SDL3) to SDL3 directly.
      `SDL_image` dropped: SDL 3.4 loads PNGs itself (`SDL_LoadPNG`).
- [x] `SDLApp` owns the window + GL 3.3 core context. `SDL_Renderer` is gone.
- [x] `engine/gl_loader.*`: small function loader using `SDL_GL_GetProcAddress`
      (no GLEW: the installed GLEW is GLX-only and would break on Wayland/EGL).
      Add new GL functions to `GL_FUNCTION_LIST` as needed.
- [x] VSync picks a swap interval matching the 60 FPS target (interval 2 on a 120 Hz display).
      Falls back to the software limiter if the refresh rate isn't a multiple of 60.
- [x] `engine/renderer.*`: **interim** GL renderer. It still builds 2 quads per cell on the CPU,
      but into reused storage, with a static index buffer and a single draw call.
      Glyph atlases are uploaded as single-channel `GL_R8` coverage textures.
- [x] `DisplayGrid` is now pure CPU-side cell data (`data()` gives the renderer the cells).

---

## [ ] Step 3: Grid as a texture, no geometry (Option C)

Replace the interim quad renderer. The CPU uploads **cell data** (8 bytes per cell) and
a single full-screen triangle draws everything.

- [ ] **Packed cell format** matching the GPU layout so the upload needs no repacking:
      `struct PackedCell { uint32_t glyph_fg; uint32_t bg_flags; }`
      (`glyph` 8 bits + `fg` RGB 24 bits | `bg` RGB 24 bits + 8 flag bits, e.g. "transparent" for step 4).
      Make it `DisplayGrid`'s storage (keep the `SDL_Color`-based `set_cell`/`print`/`draw_box` API on top).
      Keep `glyph` widened to 16 bits in mind if fonts with >256 glyphs are planned.
- [ ] **Cell texture** `GL_RG32UI`, size = grid width x height.
      Reallocate (`glTexImage2D`) on grid resize (window resize / zoom change);
      otherwise one `glTexSubImage2D` per frame straight from `DisplayGrid::data()`
      (~440 KB at 320x172).
- [ ] **Glyph atlases**: keep the 4 separate `GL_R8` textures (sizes differ, so no texture array);
      bind the one for the current zoom level. The glyph size is a uniform.
- [ ] **Full-screen triangle** generated from `gl_VertexID` (empty VAO, no VBO).
- [ ] **Fragment shader**, per pixel:
      1. `p = ivec2(gl_FragCoord.xy)` flipped to top-left origin, minus the viewport offset (uniform).
      2. Outside the grid area -> clear color.
      3. `cell = p / glyph_size`, `inner = p % glyph_size`.
      4. `texelFetch` the cell -> glyph, fg, bg.
      5. `texelFetch` the atlas at `(glyph % 16, glyph / 16) * glyph_size + inner`
         (integer fetch = exact pixel-art result, no UV rounding issues).
      6. `out = mix(bg, fg, coverage)`.
- [ ] Remove the CPU vertex/index buffers from `Renderer`.
- [ ] Profiler: replace "Vertices" with "Upload: N KB / draw calls: 1".
- [ ] Target: Render stage **< 0.1 ms**, independent of window size and zoom level.

## [ ] Step 4: Overlay layer + honest GPU timing

- [ ] **Overlay grid**: a second `DisplayGrid` and cell texture for HUD, perf overlay and cursor,
      transparent by default (flag bit). The shader draws an overlay cell over the world cell when the
      cell isn't transparent. Same grid size and glyph size as the world grid.
- [ ] Upload the overlay only when it changed (dirty flag on `DisplayGrid`). The HUD changes when
      position/FPS changes and the profiler every 2 s, so most frames upload nothing.
- [ ] Draw HUD/profiler into the overlay instead of the world grid; `main.cpp` stops clearing
      and repainting them into the world grid.
- [ ] **GPU timer queries** (`GL_TIME_ELAPSED`): ring of 3-4 query objects, read results a few frames
      late so the CPU never stalls. Show "GPU: x.xx ms" next to the CPU Render time in F3.
      Needs loader entries: `glGenQueries`, `glDeleteQueries`, `glBeginQuery`, `glEndQuery`,
      `glGetQueryObjectiv`, `glGetQueryObjectui64v`.
- [ ] Look into the occasional render-stage spikes (max 3-9 ms vs ~0.75 ms average in step 2).
      Probably driver buffer reallocation; re-check after step 3, which no longer streams vertices.

## [ ] Step 5: GPU-resident world (Option D)

The shader reads tile data directly. The per-cell world loop in `main.cpp`
(the "World" stage) and the world-grid upload both go away.

- [ ] **World tile texture**: `GL_TEXTURE_2D_ARRAY`, `GL_R8UI`, 1024 x 1024 x 3 layers
      (Sky / Ground / Underground), uploaded once after `World::generate`. ~3 MB.
      (`TileType` is `uint8_t`; switch to `GL_R16UI` if it ever exceeds 256 types.)
- [ ] **Tile visual lookup**: `TILE_VISUALS` (TileType -> glyph, fg, bg) as a small
      `GL_RG32UI` texture (one texel per TileType, same packing as `PackedCell`), uploaded once.
- [ ] **Uniforms**: camera position, current layer, world size, world-area height
      (grid height minus the 2 HUD rows), cursor cell, void visual for out-of-bounds cells.
- [ ] **Shader**: `world_xy = camera - center + cell`; out of bounds -> void visual;
      otherwise `tile = texelFetch(world, ivec3(world_xy, layer))` -> lookup -> glyph/fg/bg.
      Then the overlay layer from step 4 on top. The cursor `+` becomes a uniform check
      (or stays an overlay cell).
- [ ] **Tile changes** (for future gameplay: digging, building, ...): `World::set_tile()` keeps the CPU
      copy authoritative (game logic reads it) and records dirty rectangles per layer.
      Each frame the renderer uploads only the dirty regions with `glTexSubImage2D`, then clears the list.
- [ ] `main.cpp` world-drawing loop deleted; per frame the CPU only updates uniforms, any dirty
      tiles and (rarely) the overlay. Target: CPU cost for drawing ≈ 0, with the World stage gone.
- [ ] Bigger worlds later: GPU max texture size is ≥ 16384 on current hardware, but for very large
      or streamed worlds split into chunk textures (e.g. 256x256) and upload only the chunks near the camera.

## [ ] Step 6: Later: extras that become cheap on the GPU

All of these work within whole-cell movement and discrete zoom:

- [ ] **Entities / items**: they occupy whole cells, so a sparse "entity" cell layer
      (another transparent cell texture, rewritten each frame from the entity list, or just written into
      the overlay) is enough. No sprite or instancing pipeline needed.
- [ ] **Animated tiles** (water shimmer, lava flicker): a `time` uniform plus an "animated" flag in the
      tile lookup; the shader alternates glyph/colour.
- [ ] **Lighting / fog of war / visibility**: per-cell `R8` textures multiplied into the colours.
- [ ] **Multi-level view** (Dwarf Fortress style): when the current layer's tile is "open"
      (e.g. Open Sky), sample the layer below and darken it, all in the shader.

---

## Housekeeping / notes

- [ ] **Fixed-timestep game logic**: movement currently happens once per frame, so speed depends on
      the frame rate (this is why VSync is paced to exactly 60 FPS). Decouple logic ticks from rendering
      before adding real gameplay.
- [ ] **Font is missing glyphs 128-255**: `BlockyBold_*.png` has nothing in the upper half of CP437,
      so box-drawing borders (F3 overlay frame) and the shade blocks 176/177 (Dense Clouds, Solid Rock)
      draw no glyph, only the background. This was the case before the GL port too. Fill in the atlases
      or switch to a full CP437 font.
- [ ] **Portability**: `gl_loader.hpp` includes the system `<GL/glcorearb.h>` (Mesa headers on Linux).
      Copy `glcorearb.h` + `KHR/khrplatform.h` into the repo for Windows/macOS builds. macOS also needs
      `SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG` for a 3.3 core context.
- [ ] Optional, later: SDL3 GPU API (Vulkan/Metal/D3D12) instead of OpenGL, if consoles or modern-API
      features ever matter. The shader logic from steps 3-5 ports directly; it only adds shader
      cross-compilation tooling (SDL_shadercross).
