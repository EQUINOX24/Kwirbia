#pragma once

#include <SDL3/SDL.h>
#include <cstdint>
#include <vector>
#include <string_view>

struct Cell {
    uint8_t glyph = ' ';
    SDL_Color fg{255, 255, 255, 255};
    SDL_Color bg{0, 0, 0, 255};
};

// CPU-side virtual console: what each cell shows. Drawing is done by Renderer.
class DisplayGrid {
public:
    DisplayGrid(int32_t width = 0, int32_t height = 0);

    void resize(int32_t width, int32_t height);

    int32_t width() const noexcept { return width_; }
    int32_t height() const noexcept { return height_; }
    int32_t size() const noexcept { return width_ * height_; }

    void clear(SDL_Color bg = {0, 0, 0, 255}) noexcept;

    void set_cell(int32_t x, int32_t y, uint8_t glyph, SDL_Color fg, SDL_Color bg) noexcept;
    void set_cell(int32_t x, int32_t y, const Cell& cell) noexcept;
    const Cell& get_cell(int32_t x, int32_t y) const noexcept;

    // Row-major cell storage (width * height), for the renderer
    const Cell* data() const noexcept { return cells_.data(); }

    void print(int32_t x, int32_t y, std::string_view text, SDL_Color fg, SDL_Color bg = {0, 0, 0, 255}) noexcept;
    void draw_box(int32_t x, int32_t y, int32_t w, int32_t h, SDL_Color fg, SDL_Color bg = {0, 0, 0, 255}) noexcept;
    void fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t glyph, SDL_Color fg, SDL_Color bg) noexcept;

private:
    int32_t width_ = 0;
    int32_t height_ = 0;
    std::vector<Cell> cells_;
};
