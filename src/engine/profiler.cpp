#include "profiler.hpp"
#include "displaygrid.hpp"
#include "renderer.hpp"
#include <algorithm>
#include <cstdio>
#include <iostream>

namespace {
    constexpr auto PROFILER_INTERVAL = std::chrono::seconds{2};
}

Profiler::Profiler() noexcept
{
    reset_state();
}

void Profiler::toggle() noexcept
{
    set_enabled(!enabled_);
}

void Profiler::set_enabled(bool enabled) noexcept
{
    enabled_ = enabled;
    reset_state();
    
    if (enabled_) {
        frame_start_time_ = Clock::now();
    }
}

void Profiler::reset_state() noexcept
{
    interval_frames_ = 0;
    sum_world_ms_ = 0.0;
    sum_render_ms_ = 0.0;
    sum_cpu_ms_ = 0.0;
    interval_max_cpu_ms_ = 0.0;
    interval_max_render_ms_ = 0.0;

    display_fps_ = 0.0;
    display_avg_cpu_ms_ = 0.0;
    display_max_cpu_ms_ = 0.0;
    display_avg_world_ms_ = 0.0;
    display_avg_render_ms_ = 0.0;
    display_max_render_ms_ = 0.0;
    has_initial_data_ = false;

    interval_start_ = Clock::now();
}

void Profiler::start_frame() noexcept
{
    if (!enabled_) return;

    frame_start_time_ = Clock::now();
    if (interval_frames_ == 0 && !has_initial_data_) {
        interval_start_ = frame_start_time_;
    }
}

void Profiler::start_world_prep() noexcept
{
    if (!enabled_) return;
    world_start_time_ = Clock::now();
}

void Profiler::end_world_prep() noexcept
{
    if (!enabled_) return;
    const auto now = Clock::now();
    frame_world_ms_ = std::chrono::duration<double, std::milli>(now - world_start_time_).count();
}

void Profiler::start_render() noexcept
{
    if (!enabled_) return;
    render_start_time_ = Clock::now();
}

void Profiler::end_render() noexcept
{
    if (!enabled_) return;
    const auto now = Clock::now();
    frame_render_ms_ = std::chrono::duration<double, std::milli>(now - render_start_time_).count();
}

void Profiler::end_frame() noexcept
{
    if (!enabled_) return;

    const auto now = Clock::now();
    frame_cpu_ms_ = std::chrono::duration<double, std::milli>(now - frame_start_time_).count();

    interval_frames_++;
    sum_world_ms_ += frame_world_ms_;
    sum_render_ms_ += frame_render_ms_;
    sum_cpu_ms_ += frame_cpu_ms_;

    interval_max_cpu_ms_ = std::max(interval_max_cpu_ms_, frame_cpu_ms_);
    interval_max_render_ms_ = std::max(interval_max_render_ms_, frame_render_ms_);

    const double elapsed_sec = std::chrono::duration<double>(now - interval_start_).count();

    // Latch values every PROFILER_INTERVAL seconds
    if (now - interval_start_ >= PROFILER_INTERVAL) {
        if (interval_frames_ > 0) {
            display_fps_ = static_cast<double>(interval_frames_) / elapsed_sec;
            display_avg_cpu_ms_ = sum_cpu_ms_ / static_cast<double>(interval_frames_);
            display_max_cpu_ms_ = interval_max_cpu_ms_;
            display_avg_world_ms_ = sum_world_ms_ / static_cast<double>(interval_frames_);
            display_avg_render_ms_ = sum_render_ms_ / static_cast<double>(interval_frames_);
            display_max_render_ms_ = interval_max_render_ms_;
            has_initial_data_ = true;
        }

        // Reset accumulation for the next interval
        interval_frames_ = 0;
        sum_world_ms_ = 0.0;
        sum_render_ms_ = 0.0;
        sum_cpu_ms_ = 0.0;
        interval_max_cpu_ms_ = 0.0;
        interval_max_render_ms_ = 0.0;
        interval_start_ = now;
    } else if (!has_initial_data_ && interval_frames_ > 0) {
        // Live preview until the first 3-second window completes
        const double safe_sec = std::max(0.001, elapsed_sec);
        display_fps_ = static_cast<double>(interval_frames_) / safe_sec;
        display_avg_cpu_ms_ = sum_cpu_ms_ / static_cast<double>(interval_frames_);
        display_max_cpu_ms_ = interval_max_cpu_ms_;
        display_avg_world_ms_ = sum_world_ms_ / static_cast<double>(interval_frames_);
        display_avg_render_ms_ = sum_render_ms_ / static_cast<double>(interval_frames_);
        display_max_render_ms_ = interval_max_render_ms_;
    }
}

void Profiler::draw_overlay(DisplayGrid& grid, const Renderer& renderer, int32_t target_fps) noexcept
{
    if (!enabled_) return;

    const int32_t gw = grid.width();
    const int32_t gh = grid.height();

    constexpr int32_t box_w = 36;
    constexpr int32_t box_h = 8;

    if (gw < box_w + 2 || gh < box_h + 4) return;

    const int32_t box_x = gw - box_w - 1;
    const int32_t box_y = 1;

    // Themed UI colors
    const SDL_Color bg_color{14, 20, 32, 245};
    const SDL_Color border_color{75, 135, 190, 255};
    const SDL_Color title_color{120, 215, 255, 255};
    const SDL_Color label_color{225, 235, 245, 255};
    const SDL_Color sub_color{155, 185, 215, 255};
    const SDL_Color dim_color{115, 140, 170, 255};

    // Draw box background and borders
    grid.fill_rect(box_x, box_y, box_w, box_h, ' ', label_color, bg_color);
    grid.draw_box(box_x, box_y, box_w, box_h, border_color, bg_color);

    char line[48];

    // Title banner on upper border
    // std::cout << PROFILER_INTERVAL.count() << std::endl;
    std::snprintf(line, sizeof(line), " Perf Stats (%lds avg) [F3] ", PROFILER_INTERVAL.count());
    grid.print(box_x + 2, box_y, line, title_color, bg_color);

    // Row 1: Framerate
    std::snprintf(line, sizeof(line), "FPS:      %5.1f (target %d)", display_fps_, target_fps);
    grid.print(box_x + 2, box_y + 1, line, label_color, bg_color);

    // Row 2: Total CPU Work
    std::snprintf(line, sizeof(line), "CPU Work: %5.2f ms (max %5.2f)", display_avg_cpu_ms_, display_max_cpu_ms_);
    grid.print(box_x + 2, box_y + 2, line, label_color, bg_color);

    // Row 3: World Prep stage
    std::snprintf(line, sizeof(line), " - World: %5.2f ms", display_avg_world_ms_);
    grid.print(box_x + 2, box_y + 3, line, sub_color, bg_color);

    // Row 4: Geometry/Render stage
    std::snprintf(line, sizeof(line), " - Render:%5.2f ms (max %5.2f)", display_avg_render_ms_, display_max_render_ms_);
    grid.print(box_x + 2, box_y + 4, line, sub_color, bg_color);

    // Row 5: Grid dimensions & total cells
    const int32_t total_cells = gw * gh;
    if (total_cells >= 1000) {
        std::snprintf(line, sizeof(line), "Grid:     %dx%d (%.1fk cells)", gw, gh, static_cast<double>(total_cells) / 1000.0);
    } else {
        std::snprintf(line, sizeof(line), "Grid:     %dx%d (%d cells)", gw, gh, total_cells);
    }
    grid.print(box_x + 2, box_y + 5, line, dim_color, bg_color);

    // Row 6: Rendered vertex count
    const std::size_t total_verts = renderer.vertex_count();
    if (total_verts >= 1000) {
        std::snprintf(line, sizeof(line), "Vertices: %.1fk (bg+fg)", static_cast<double>(total_verts) / 1000.0);
    } else {
        std::snprintf(line, sizeof(line), "Vertices: %zu (bg+fg)", total_verts);
    }
    grid.print(box_x + 2, box_y + 6, line, dim_color, bg_color);
}
