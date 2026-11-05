#pragma once

#include <cstdint>

class TimeManager {
public:
    explicit TimeManager(int32_t target_fps = 60);

    void start_frame() noexcept;
    void end_frame() noexcept;

    void set_target_fps(int32_t fps) noexcept;
    int32_t target_fps() const noexcept { return target_fps_; }

    // When VSync paces the frames (buffer swap blocks), the software limiter must stay off,
    // otherwise both wait and the frame time drifts past the display refresh interval.
    void set_limiter_enabled(bool enabled) noexcept { limiter_enabled_ = enabled; }
    bool limiter_enabled() const noexcept { return limiter_enabled_; }

    double delta_time() const noexcept { return delta_time_; }       // In seconds
    double frame_time_ms() const noexcept { return frame_time_ms_; } // In milliseconds
    double average_fps() const noexcept { return avg_fps_; }
    uint64_t frame_count() const noexcept { return counter_; }

private:
    int32_t target_fps_ = 60;
    uint64_t nspf_ = 16'666'667;
    bool limiter_enabled_ = true;
    uint64_t frame_start_ns_ = 0;
    double delta_time_ = 0.016;
    double frame_time_ms_ = 16.0;
    double avg_fps_ = 60.0;
    uint64_t counter_ = 0;

    uint64_t fps_timer_start_ns_ = 0;
    uint32_t frames_this_second_ = 0;
};
