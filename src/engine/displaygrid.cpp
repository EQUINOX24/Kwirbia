#include "displaygrid.hpp"
#include <algorithm>

DisplayGrid::DisplayGrid(int32_t width, int32_t height)
{
    resize(width, height);
}

void DisplayGrid::resize(int32_t width, int32_t height)
{
    width_ = std::max(0, width);
    height_ = std::max(0, height);
    cells_.assign(static_cast<std::size_t>(width_ * height_), Cell{});
}

void DisplayGrid::clear(SDL_Color bg) noexcept
{
    for (auto& cell : cells_) {
        cell.glyph = ' ';
        cell.fg = {255, 255, 255, 255};
        cell.bg = bg;
    }
}

void DisplayGrid::set_cell(int32_t x, int32_t y, uint8_t glyph, SDL_Color fg, SDL_Color bg) noexcept
{
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    std::size_t idx = static_cast<std::size_t>(y * width_ + x);
    cells_[idx] = Cell{glyph, fg, bg};
}

void DisplayGrid::set_cell(int32_t x, int32_t y, const Cell& cell) noexcept
{
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return;
    std::size_t idx = static_cast<std::size_t>(y * width_ + x);
    cells_[idx] = cell;
}

const Cell& DisplayGrid::get_cell(int32_t x, int32_t y) const noexcept
{
    static const Cell default_cell{};
    if (x < 0 || x >= width_ || y < 0 || y >= height_) return default_cell;
    return cells_[static_cast<std::size_t>(y * width_ + x)];
}

void DisplayGrid::print(int32_t x, int32_t y, std::string_view text, SDL_Color fg, SDL_Color bg) noexcept
{
    if (y < 0 || y >= height_) return;
    for (std::size_t i = 0; i < text.size(); ++i) {
        int32_t cur_x = x + static_cast<int32_t>(i);
        if (cur_x < 0 || cur_x >= width_) continue;
        set_cell(cur_x, y, static_cast<uint8_t>(text[i]), fg, bg);
    }
}

void DisplayGrid::draw_box(int32_t x, int32_t y, int32_t w, int32_t h, SDL_Color fg, SDL_Color bg) noexcept
{
    if (w <= 1 || h <= 1) return;

    // CP437 box drawing characters:
    // ┌ = 218 (0xDA), ┐ = 191 (0xBF), └ = 192 (0xC0), ┘ = 217 (0xD9)
    // ─ = 196 (0xC4), │ = 179 (0xB3)
    constexpr uint8_t TL = 218;
    constexpr uint8_t TR = 191;
    constexpr uint8_t BL = 192;
    constexpr uint8_t BR = 217;
    constexpr uint8_t H_LINE = 196;
    constexpr uint8_t V_LINE = 179;

    set_cell(x, y, TL, fg, bg);
    set_cell(x + w - 1, y, TR, fg, bg);
    set_cell(x, y + h - 1, BL, fg, bg);
    set_cell(x + w - 1, y + h - 1, BR, fg, bg);

    for (int32_t i = x + 1; i < x + w - 1; ++i) {
        set_cell(i, y, H_LINE, fg, bg);
        set_cell(i, y + h - 1, H_LINE, fg, bg);
    }
    for (int32_t j = y + 1; j < y + h - 1; ++j) {
        set_cell(x, j, V_LINE, fg, bg);
        set_cell(x + w - 1, j, V_LINE, fg, bg);
    }
}

void DisplayGrid::fill_rect(int32_t x, int32_t y, int32_t w, int32_t h, uint8_t glyph, SDL_Color fg, SDL_Color bg) noexcept
{
    for (int32_t j = y; j < y + h; ++j) {
        for (int32_t i = x; i < x + w; ++i) {
            set_cell(i, j, glyph, fg, bg);
        }
    }
}
