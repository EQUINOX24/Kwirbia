# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository layout

The top level is not a git repository. `src/` has its own `.git`, so source commits go inside `src/`.

## Commands

Run all `make` commands from the repo root. The Makefile globs `src/*.cpp`, `src/random/*.cpp` and `src/engine/*.cpp`, and the binary is `./output`.

- Build (optimised, `-O2`): `make`
- Debug build (`-O0 -g`): `make clean && make DEBUG=1`. Object files are not tracked per configuration, so always `make clean` when switching.
- Run: `make test` (or `./output`). This only launches the game. There is no test suite, so there is no way to run a single test.
- Lint: no linter is configured. The build uses `-Wall -Wextra -Wpedantic`, so keep it warning-free.
- Clean: `make clean`

Dependencies: SDL 3 via `pkg-config sdl3` (SDL 3.4 is what the project targets). At runtime it needs an OpenGL 3.3 core context. Object and `.d` files are written next to their sources in `src/`.

Assets are loaded from `media/` relative to the working directory, with a fallback to `src/media/`. Run the binary from the repo root.

## Architecture

**Frame loop (`src/main.cpp`).** A single loop does, in order: poll input, move the camera, fill a CPU-side `DisplayGrid` with the visible world, draw the HUD and optional profiler overlay into that same grid, call `Renderer::draw_grid`, then `SDLApp::present()`, and finally `TimeManager` pacing. Creation order matters: `SDLApp` must outlive `Renderer` because the GL objects die with the context.

**World model (`src/terrain.*`).** `World` stores three full-size `TileType` layers (Sky, Ground, Underground), 1024x1024 by default. They are generated from seeded Perlin noise (`src/random/`). `TILE_VISUALS` in `terrain.cpp` is indexed by the `TileType` enum value, so new tile types must be added in enum order in both `terrain.hpp` and that table. `get_tile_visual()` is the single lookup used for rendering and walkability.

**Virtual console and renderer (`src/engine/displaygrid.*`, `renderer.*`).** `DisplayGrid` holds `Cell` values (glyph, fg, bg) and knows nothing about GL. `Renderer` turns the grid into quads, one background quad and one glyph quad per cell, and submits them in a single draw call. Glyphs come from `media/BlockyBold_{8,12,18,24}x*.png`, which are 16x16 CP437-style atlases (glyph `g` sits at column `g % 16`, row `g / 16`). Each atlas is uploaded as an `R8` coverage texture. This is the interim step-2 renderer, and `TODO.md` describes the planned replacement.

**Zoom and grid size (`src/engine/sdl_app.*`).** `SDLApp` owns the window and the GL 3.3 context. Zoom is discrete, chosen from `FONT_SIZES = {8, 12, 18, 24}`. The grid dimensions are always derived as window pixels divided by the current font size. `InputHandler` calls `grid.resize()` after any zoom or window-size change.

**OpenGL loading (`src/engine/gl_loader.*`).** libGL is not linked. Every GL function is resolved at runtime through `SDL_GL_GetProcAddress` from the `GL_FUNCTION_LIST` macro. To use a new GL function, add it to that list first, or it will not compile.

**Input and timing (`src/engine/input_handler.hpp`, `timemgr.*`).** `InputHandler` keeps a scancode state table. Movement is applied once per frame, not scaled by elapsed time, so speed depends on frame rate. `TimeManager` either relies on VSync (`SDLApp::configure_vsync` picks a swap interval for 60 FPS) or uses its own software limiter. `profiler.*` draws the F3 overlay into the grid and reads `Renderer::vertex_count()`.

## Roadmap constraints

`TODO.md` is the rendering roadmap. Steps 1 and 2 are done. Steps 3–5 move drawing to a GPU-resident grid and tile texture. Any step must keep these working: discrete zoom (4 glyph sizes), camera movement in whole cells, the F3 overlay, WASD/arrow movement with Shift sprint, layer switching (Q/E, PgUp/PgDn, `<` `>`), Ctrl +/- zoom, and the 2-row HUD.
