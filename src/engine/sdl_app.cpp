#include "sdl_app.hpp"
#include "gl_loader.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>

SDLApp::SDLApp(std::string_view title, int32_t width, int32_t height)
    : window_width_(width), window_height_(height)
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        throw std::runtime_error(std::string("SDL_Init error: ") + SDL_GetError());
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    window_ = SDL_CreateWindow(
        std::string(title).c_str(),
        window_width_,
        window_height_,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
    );
    if (!window_) {
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_CreateWindow error: ") + SDL_GetError());
    }

    gl_context_ = SDL_GL_CreateContext(window_);
    if (!gl_context_) {
        SDL_DestroyWindow(window_);
        SDL_Quit();
        throw std::runtime_error(std::string("SDL_GL_CreateContext error (OpenGL 3.3 core required): ") + SDL_GetError());
    }
    SDL_GL_MakeCurrent(window_, gl_context_);

    try {
        gl_load_functions();
    } catch (...) {
        cleanup();
        throw;
    }

    // The window manager may have adjusted the requested size; work in actual pixels
    int pw = 0, ph = 0;
    if (SDL_GetWindowSizeInPixels(window_, &pw, &ph)) {
        window_width_ = pw;
        window_height_ = ph;
    }

    std::cout << "OpenGL: " << reinterpret_cast<const char*>(glGetString(GL_VERSION))
              << " | " << reinterpret_cast<const char*>(glGetString(GL_RENDERER)) << "\n";
}

SDLApp::~SDLApp()
{
    cleanup();
}

SDLApp::SDLApp(SDLApp&& other) noexcept
    : window_(other.window_),
      gl_context_(other.gl_context_),
      current_fontsize_idx_(other.current_fontsize_idx_),
      window_width_(other.window_width_),
      window_height_(other.window_height_),
      vsync_enabled_(other.vsync_enabled_)
{
    other.window_ = nullptr;
    other.gl_context_ = nullptr;
}

SDLApp& SDLApp::operator=(SDLApp&& other) noexcept
{
    if (this != &other) {
        cleanup();

        window_ = other.window_;
        gl_context_ = other.gl_context_;
        current_fontsize_idx_ = other.current_fontsize_idx_;
        window_width_ = other.window_width_;
        window_height_ = other.window_height_;
        vsync_enabled_ = other.vsync_enabled_;

        other.window_ = nullptr;
        other.gl_context_ = nullptr;
    }
    return *this;
}

void SDLApp::cleanup() noexcept
{
    // A moved-from object owns nothing and must not shut SDL down
    if (!window_ && !gl_context_) return;

    if (gl_context_) {
        SDL_GL_DestroyContext(gl_context_);
        gl_context_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}

void SDLApp::handle_resize(int32_t width, int32_t height) noexcept
{
    window_width_ = width;
    window_height_ = height;
}

bool SDLApp::zoom_in() noexcept
{
    if (current_fontsize_idx_ + 1 < NUM_FONTSIZES) {
        current_fontsize_idx_++;
        return true;
    }
    return false;
}

bool SDLApp::zoom_out() noexcept
{
    if (current_fontsize_idx_ > 0) {
        current_fontsize_idx_--;
        return true;
    }
    return false;
}

bool SDLApp::configure_vsync(int32_t target_fps) noexcept
{
    vsync_enabled_ = false;

    float refresh = 0.0f;
    if (const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(SDL_GetDisplayForWindow(window_))) {
        refresh = mode->refresh_rate;
    }

    if (refresh > 0.0f && target_fps > 0) {
        const int interval = std::max(1, static_cast<int>(std::lround(refresh / static_cast<float>(target_fps))));
        const float paced_fps = refresh / static_cast<float>(interval);
        const bool matches_target = std::abs(paced_fps - static_cast<float>(target_fps)) <= 0.05f * static_cast<float>(target_fps);

        if (matches_target && SDL_GL_SetSwapInterval(interval)) {
            vsync_enabled_ = true;
            std::cout << "VSync: " << refresh << " Hz display, swap interval " << interval
                      << " (" << paced_fps << " FPS)\n";
            return true;
        }
    }

    SDL_GL_SetSwapInterval(0);
    std::cout << "VSync: off (display " << refresh << " Hz), software limiter at " << target_fps << " FPS\n";
    return false;
}

void SDLApp::present() noexcept
{
    SDL_GL_SwapWindow(window_);
}
