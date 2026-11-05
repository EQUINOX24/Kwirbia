#pragma once

#include "sdl_app.hpp"
#include "displaygrid.hpp"
#include "profiler.hpp"
#include "../terrain.hpp"
#include <SDL3/SDL.h>
#include <array>
#include <iostream>

struct Vec2i {
    int32_t x = 0;
    int32_t y = 0;
};

class InputHandler {
public:
    InputHandler() = default;

    void process_events(SDLApp& app, DisplayGrid& grid, WorldLayer& current_layer, Profiler& profiler, bool& should_quit) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT:
                    should_quit = true;
                    break;

                // Pixel size (not logical size): the GL viewport and grid are measured in pixels
                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    app.handle_resize(event.window.data1, event.window.data2);
                    grid.resize(app.grid_width(), app.grid_height());
                    break;

                case SDL_EVENT_KEY_DOWN:
                    handle_key_down(event.key, app, grid, current_layer, profiler);
                    break;

                case SDL_EVENT_KEY_UP:
                    handle_key_up(event.key);
                    break;

                default:
                    break;
            }
        }

        update_modifiers();
    }

    Vec2i get_movement_delta() const noexcept {
        Vec2i delta{0, 0};
        int32_t step = shiftdown_ ? 5 : 1;

        if (keys_[SDL_SCANCODE_W] || keys_[SDL_SCANCODE_UP])    delta.y -= step;
        if (keys_[SDL_SCANCODE_S] || keys_[SDL_SCANCODE_DOWN])  delta.y += step;
        if (keys_[SDL_SCANCODE_A] || keys_[SDL_SCANCODE_LEFT])  delta.x -= step;
        if (keys_[SDL_SCANCODE_D] || keys_[SDL_SCANCODE_RIGHT]) delta.x += step;

        return delta;
    }

    bool is_shift_down() const noexcept { return shiftdown_; }
    bool is_ctrl_down() const noexcept { return ctrldown_; }

private:
    void handle_key_down(const SDL_KeyboardEvent& key, SDLApp& app, DisplayGrid& grid, WorldLayer& current_layer, Profiler& profiler) {
        keys_[key.scancode] = true;
        update_modifiers();

        // Performance Profiler toggle: F3
        if (key.scancode == SDL_SCANCODE_F3 && !key.repeat) {
            profiler.toggle();
        }

        // Zoom In: Ctrl + = or Ctrl + keypad +
        if ((key.scancode == SDL_SCANCODE_EQUALS || key.scancode == SDL_SCANCODE_KP_PLUS) && ctrldown_) {
            if (app.zoom_in()) {
                grid.resize(app.grid_width(), app.grid_height());
            }
        }
        // Zoom Out: Ctrl + - or Ctrl + keypad -
        else if ((key.scancode == SDL_SCANCODE_MINUS || key.scancode == SDL_SCANCODE_KP_MINUS) && ctrldown_) {
            if (app.zoom_out()) {
                grid.resize(app.grid_width(), app.grid_height());
            }
        }

        // Layer switching:
        // UP layer (Underground -> Ground -> Sky): PageUp, Q, or '<' (Shift + Comma)
        if (key.scancode == SDL_SCANCODE_PAGEUP || key.scancode == SDL_SCANCODE_Q ||
            (key.scancode == SDL_SCANCODE_COMMA && shiftdown_)) {
            if (current_layer == WorldLayer::UNDERGROUND) {
                current_layer = WorldLayer::GROUND;
            } else if (current_layer == WorldLayer::GROUND) {
                current_layer = WorldLayer::SKY;
            }
        }
        // DOWN layer (Sky -> Ground -> Underground): PageDown, E, or '>' (Shift + Period)
        else if (key.scancode == SDL_SCANCODE_PAGEDOWN || key.scancode == SDL_SCANCODE_E ||
                 (key.scancode == SDL_SCANCODE_PERIOD && shiftdown_)) {
            if (current_layer == WorldLayer::SKY) {
                current_layer = WorldLayer::GROUND;
            } else if (current_layer == WorldLayer::GROUND) {
                current_layer = WorldLayer::UNDERGROUND;
            }
        }
    }

    void handle_key_up(const SDL_KeyboardEvent& key) noexcept {
        keys_[key.scancode] = false;
        update_modifiers();
    }

    void update_modifiers() noexcept {
        shiftdown_ = keys_[SDL_SCANCODE_LSHIFT] || keys_[SDL_SCANCODE_RSHIFT];
        ctrldown_  = keys_[SDL_SCANCODE_LCTRL]  || keys_[SDL_SCANCODE_RCTRL];
    }

    std::array<bool, SDL_SCANCODE_COUNT> keys_{};
    bool shiftdown_ = false;
    bool ctrldown_ = false;
};
