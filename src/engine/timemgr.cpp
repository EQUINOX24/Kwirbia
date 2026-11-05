#include "timemgr.hpp"
#include <SDL3/SDL.h>
#include <algorithm>

TimeManager::TimeManager(int32_t target_fps)
{
    set_target_fps(target_fps);
    fps_timer_start_ns_ = SDL_GetTicksNS();
}

void TimeManager::set_target_fps(int32_t fps) noexcept
{
    target_fps_ = std::max(1, fps);
    nspf_ = SDL_NS_PER_SECOND / static_cast<uint64_t>(target_fps_);
}

void TimeManager::start_frame() noexcept
{
    frame_start_ns_ = SDL_GetTicksNS();
    counter_++;
    frames_this_second_++;

    const uint64_t now = frame_start_ns_;
    if (now - fps_timer_start_ns_ >= SDL_NS_PER_SECOND) {
        avg_fps_ = static_cast<double>(frames_this_second_) * 1e9 / static_cast<double>(now - fps_timer_start_ns_);
        frames_this_second_ = 0;
        fps_timer_start_ns_ = now;
    }
}

void TimeManager::end_frame() noexcept
{
    uint64_t elapsed = SDL_GetTicksNS() - frame_start_ns_;

    if (limiter_enabled_ && elapsed < nspf_) {
        SDL_DelayPrecise(nspf_ - elapsed);
        elapsed = SDL_GetTicksNS() - frame_start_ns_;
    }

    frame_time_ms_ = static_cast<double>(elapsed) / 1e6;
    delta_time_ = frame_time_ms_ / 1000.0;
}
