#pragma once

#include "gl_loader.hpp"
#include <SDL3/SDL.h>
#include <cstdint>
#include <span>
#include <vector>

class DisplayGrid;

constexpr SDL_Color make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
    return SDL_Color{r, g, b, a};
}

constexpr SDL_Color color_from_hex(uint32_t hex, uint8_t a = 255) {
    return SDL_Color{
        static_cast<uint8_t>((hex >> 16) & 0xFF),
        static_cast<uint8_t>((hex >> 8) & 0xFF),
        static_cast<uint8_t>(hex & 0xFF),
        a
    };
}

// OpenGL renderer for the DisplayGrid.
//
// Interim implementation (TODO.md step 2): the CPU still builds one background quad and one glyph
// quad per cell, but writes them straight into a reused buffer, uploads them once and draws
// everything with a single glDrawElements call using a static index buffer.
// TODO.md step 3 replaces this with the zero-geometry "grid as texture" pipeline.
//
// Requires a current OpenGL 3.3 core context (created by SDLApp) for its whole lifetime.
class Renderer {
public:
    explicit Renderer(std::span<const int32_t> font_sizes);
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    // Sets the viewport (window size in pixels) and clears the framebuffer.
    void begin_frame(int32_t viewport_w, int32_t viewport_h, SDL_Color clear_color) noexcept;

    // Draws every cell of the grid using the glyph atlas at font_index (index into font_sizes).
    void draw_grid(const DisplayGrid& grid, std::size_t font_index, int32_t offset_x, int32_t offset_y);

    // Vertices submitted by the last draw_grid call
    std::size_t vertex_count() const noexcept { return last_vertex_count_; }

private:
    struct Vertex {
        float x, y;
        float u, v;       // u < 0 marks an untextured (solid background) vertex
        SDL_Color color;
    };

    void load_font_atlases(std::span<const int32_t> font_sizes);
    void ensure_index_capacity(std::size_t quads);

    GLuint program_ = 0;
    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
    GLint u_viewport_ = -1;
    GLint u_atlas_ = -1;

    std::vector<GLuint> font_textures_; // 0 where the atlas failed to load
    std::vector<int32_t> font_sizes_;

    // CPU-side vertex staging (reused across frames, only grows)
    std::vector<Vertex> bg_vertices_;
    std::vector<Vertex> fg_vertices_;
    std::size_t index_capacity_quads_ = 0;
    std::size_t vbo_capacity_bytes_ = 0;
    std::size_t last_vertex_count_ = 0;
};
