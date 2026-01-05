#include "core/Time.h"

#include <SDL.h>

namespace snake::core {

void Time::Init() {
    const uint64_t counter = SDL_GetPerformanceCounter();
    start_counter_ = counter;
    last_counter_ = counter;

    const uint64_t frequency = SDL_GetPerformanceFrequency();
    frequency_inv_ = frequency > 0 ? 1.0 / static_cast<double>(frequency) : 0.0;
    frame_dt_ = 0.0;
    accumulator_ = 0.0;
}

void Time::UpdateFrame() {
    const uint64_t current = SDL_GetPerformanceCounter();
    const uint64_t delta = current - last_counter_;
    frame_dt_ = static_cast<double>(delta) * frequency_inv_;
    last_counter_ = current;

    if (frame_dt_ < 0.0) {
        frame_dt_ = 0.0;
    }

    accumulator_ += frame_dt_;
}

double Time::FrameDt() const {
    return frame_dt_;
}

double Time::Now() const {
    const uint64_t current = SDL_GetPerformanceCounter();
    const uint64_t delta = current - start_counter_;
    return static_cast<double>(delta) * frequency_inv_;
}

void Time::SetTickDt(double seconds) {
    if (seconds > 0.0) {
        tick_dt_ = seconds;
    }
}

bool Time::ConsumeTick() {
    if (accumulator_ >= tick_dt_) {
        accumulator_ -= tick_dt_;
        return true;
    }

    return false;
}

double Time::TickDt() const {
    return tick_dt_;
}

}  // namespace snake::core
