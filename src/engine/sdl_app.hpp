#pragma once

#include <SDL3/SDL.h>
#include <array>
#include <string_view>

// Owns the SDL window and its OpenGL 3.3 core context. All drawing goes through Renderer,
// which must be created after (and destroyed before) the SDLApp.
class SDLApp {
public:
    static constexpr std::size_t NUM_FONTSIZES = 4;
    static constexpr std::array<int32_t, NUM_FONTSIZES> FONT_SIZES = {8, 12, 18, 24};

    SDLApp(std::string_view title = "AsciiEngine", int32_t width = 1400, int32_t height = 800);
    ~SDLApp();

    // Non-copyable
    SDLApp(const SDLApp&) = delete;
    SDLApp& operator=(const SDLApp&) = delete;

    // Movable
    SDLApp(SDLApp&& other) noexcept;
    SDLApp& operator=(SDLApp&& other) noexcept;

    // Window & Viewport (in pixels)
    void handle_resize(int32_t width, int32_t height) noexcept;
    int32_t window_width() const noexcept { return window_width_; }
    int32_t window_height() const noexcept { return window_height_; }

    // Dynamic grid dimensions
    int32_t current_fontsize() const noexcept { return FONT_SIZES[current_fontsize_idx_]; }
    int32_t grid_width() const noexcept { return window_width_ / current_fontsize(); }
    int32_t grid_height() const noexcept { return window_height_ / current_fontsize(); }
    int32_t viewport_offset_x() const noexcept { return (window_width_ - (grid_width() * current_fontsize())) / 2; }
    int32_t viewport_offset_y() const noexcept { return (window_height_ - (grid_height() * current_fontsize())) / 2; }

    // Zooming
    bool zoom_in() noexcept;
    bool zoom_out() noexcept;
    std::size_t fontsize_index() const noexcept { return current_fontsize_idx_; }

    // Picks a swap interval so VSync paces frames at target_fps (e.g. every 2nd refresh on 120 Hz).
    // If the refresh rate is not a multiple of target_fps, VSync is turned off instead.
    // Returns vsync_enabled(): when false, the caller must limit the frame rate itself.
    bool configure_vsync(int32_t target_fps) noexcept;
    bool vsync_enabled() const noexcept { return vsync_enabled_; }

    void present() noexcept;

    SDL_Window* window() const noexcept { return window_; }

private:
    void cleanup() noexcept;

    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;
    std::size_t current_fontsize_idx_ = 1; // Default: 12x12
    int32_t window_width_ = 1400;
    int32_t window_height_ = 800;
    bool vsync_enabled_ = false;
};
