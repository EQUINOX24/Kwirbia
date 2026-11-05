#pragma once

#include <chrono>
#include <cstdint>
#include <string_view>

class DisplayGrid;
class Renderer;

class Profiler {
public:
    Profiler() noexcept;

    void toggle() noexcept;
    void set_enabled(bool enabled) noexcept;
    bool is_enabled() const noexcept { return enabled_; }

    // Frame lifecycle timing markers
    void start_frame() noexcept;
    void start_world_prep() noexcept;
    void end_world_prep() noexcept;
    void start_render() noexcept;
    void end_render() noexcept;
    void end_frame() noexcept;

    // Dedicated top-right overlay box
    void draw_overlay(DisplayGrid& grid, const Renderer& renderer, int32_t target_fps = 60) noexcept;

    // Metrics accessors
    double avg_fps() const noexcept { return display_fps_; }
    double avg_cpu_work_ms() const noexcept { return display_avg_cpu_ms_; }
    double max_cpu_work_ms() const noexcept { return display_max_cpu_ms_; }
    double avg_world_prep_ms() const noexcept { return display_avg_world_ms_; }
    double avg_render_ms() const noexcept { return display_avg_render_ms_; }
    double max_render_ms() const noexcept { return display_max_render_ms_; }

private:
    void reset_state() noexcept;

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    bool enabled_ = false;

    // Per-frame time points
    TimePoint frame_start_time_{};
    TimePoint world_start_time_{};
    TimePoint render_start_time_{};

    // Measured frame intervals (in milliseconds)
    double frame_world_ms_ = 0.0;
    double frame_render_ms_ = 0.0;
    double frame_cpu_ms_ = 0.0;

    // 3-second aggregation window
    TimePoint interval_start_{};
    uint64_t interval_frames_ = 0;
    double sum_world_ms_ = 0.0;
    double sum_render_ms_ = 0.0;
    double sum_cpu_ms_ = 0.0;
    double interval_max_cpu_ms_ = 0.0;
    double interval_max_render_ms_ = 0.0;

    // Latched display values (refreshed every 3 seconds)
    double display_fps_ = 0.0;
    double display_avg_cpu_ms_ = 0.0;
    double display_max_cpu_ms_ = 0.0;
    double display_avg_world_ms_ = 0.0;
    double display_avg_render_ms_ = 0.0;
    double display_max_render_ms_ = 0.0;
    bool has_initial_data_ = false;
};
