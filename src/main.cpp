#include "engine/sdl_app.hpp"
#include "engine/displaygrid.hpp"
#include "engine/timemgr.hpp"
#include "engine/input_handler.hpp"
#include "engine/profiler.hpp"
#include "engine/renderer.hpp"
#include "terrain.hpp"

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>

int main() {
    try {
        SDLApp app("Kwirbia - A world, written cell by cell");
        Renderer renderer(SDLApp::FONT_SIZES); // Declared after app: GL objects die before the context
        DisplayGrid grid(app.grid_width(), app.grid_height());
        World world(1024, 1024, -1);
        TimeManager timer(60);
        timer.set_limiter_enabled(!app.configure_vsync(timer.target_fps()));
        InputHandler input;
        Profiler profiler;

        WorldLayer current_layer = WorldLayer::GROUND;
        Vec2i camera_pos{512, 512};
        bool should_quit = false;

        std::cout << "Engine started successfully. Window size: "
                  << app.window_width() << "x" << app.window_height()
                  << " | Grid: " << app.grid_width() << "x" << app.grid_height() << "\n";

        while (!should_quit) {
            profiler.start_frame();
            timer.start_frame();

            input.process_events(app, grid, current_layer, profiler, should_quit);

            // Update camera / cursor position
            Vec2i move_delta = input.get_movement_delta();
            camera_pos.x = std::clamp(camera_pos.x + move_delta.x, 0, world.width() - 1);
            camera_pos.y = std::clamp(camera_pos.y + move_delta.y, 0, world.height() - 1);

            // Measure Stage 1: World Tile Prep (virtual console population & HUD text)
            profiler.start_world_prep();

            // Render world to virtual console
            grid.clear({0, 0, 0, 255});
            int32_t gw = grid.width();
            int32_t gh = grid.height();

            // Reserve bottom 2 rows for HUD
            int32_t world_render_h = std::max(0, gh - 2);
            int32_t center_x = gw / 2;
            int32_t center_y = world_render_h / 2;

            for (int32_t sy = 0; sy < world_render_h; ++sy) {
                int32_t wy = camera_pos.y - center_y + sy;
                for (int32_t sx = 0; sx < gw; ++sx) {
                    int32_t wx = camera_pos.x - center_x + sx;

                    if (wx >= 0 && wx < world.width() && wy >= 0 && wy < world.height()) {
                        const TileVisual& visual = world.get_tile_visual(current_layer, wx, wy);
                        grid.set_cell(sx, sy, visual.glyph, visual.fg, visual.bg);
                    } else {
                        // Void beyond world boundaries
                        grid.set_cell(sx, sy, ' ', {50, 50, 50, 255}, {10, 10, 15, 255});
                    }
                }
            }

            // Draw cursor symbol '+' at center of screen
            if (center_x >= 0 && center_x < gw && center_y >= 0 && center_y < world_render_h) {
                const TileVisual& under_cursor = world.get_tile_visual(current_layer, camera_pos.x, camera_pos.y);
                grid.set_cell(center_x, center_y, '+', {255, 255, 255, 255}, under_cursor.bg);
            }

            // Draw HUD (Status bar & controls)
            const TileVisual& current_tile = world.get_tile_visual(current_layer, camera_pos.x, camera_pos.y);

            std::ostringstream hud_status;
            hud_status << "[Layer: " << layer_to_string(current_layer) << "] "
                       << "Pos: (" << camera_pos.x << ", " << camera_pos.y << ") | "
                       << "Tile: " << current_tile.name << " | "
                       << "Zoom: " << app.current_fontsize() << "x" << app.current_fontsize() << " | "
                       << "FPS: " << static_cast<int>(timer.average_fps() + 0.5);

            std::string hud_controls = "Controls: WASD=Move | Shift=Sprint | </> (Q/E)=Layer | Ctrl +/-=Zoom | F3=Perf";

            SDL_Color hud_bg{20, 25, 35, 255};
            SDL_Color hud_fg_label{220, 230, 240, 255};
            SDL_Color hud_fg_sub{140, 160, 180, 255};

            if (gh >= 2) {
                grid.fill_rect(0, gh - 2, gw, 1, ' ', hud_fg_label, hud_bg);
                grid.print(1, gh - 2, hud_status.str(), hud_fg_label, hud_bg);

                grid.fill_rect(0, gh - 1, gw, 1, ' ', hud_fg_sub, hud_bg);
                grid.print(1, gh - 1, hud_controls, hud_fg_sub, hud_bg);
            }

            profiler.end_world_prep();

            // Draw dedicated performance overlay box if enabled
            if (profiler.is_enabled()) {
                profiler.draw_overlay(grid, renderer, timer.target_fps());
            }

            // Measure Stage 2: Geometry & Render (vertex packing, GPU upload & draw submission)
            profiler.start_render();
            renderer.begin_frame(app.window_width(), app.window_height(), {0, 0, 0, 255});
            renderer.draw_grid(
                grid,
                app.fontsize_index(),
                app.viewport_offset_x(),
                app.viewport_offset_y()
            );
            profiler.end_render();

            // End active CPU frame work measurement before VSync/Delay
            profiler.end_frame();

            app.present();

            timer.end_frame();
        }

        std::cout << "Engine shut down cleanly.\n";
        return EXIT_SUCCESS;
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal engine error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
}
