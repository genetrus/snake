#include "game/Effects.h"

namespace snake::game {

void Effects::Reset() {
    slow_remaining_ = 0.0;
}

void Effects::Tick(double tick_dt) {
    if (slow_remaining_ <= 0.0) {
        return;
    }

    slow_remaining_ -= tick_dt;
    if (slow_remaining_ < 0.0) {
        slow_remaining_ = 0.0;
    }
}

void Effects::ApplySlow() {
    slow_remaining_ += slow_duration_;
}

void Effects::SetSlowParams(double multiplier, double duration) {
    if (multiplier > 0.0) {
        slow_multiplier_ = multiplier;
    }
    if (duration > 0.0) {
        slow_duration_ = duration;
    }
}

bool Effects::SlowActive() const {
    return slow_remaining_ > 0.0;
}

double Effects::SlowMultiplier() const {
    return slow_multiplier_;
}

double Effects::SlowRemaining() const {
    return slow_remaining_;
}

}  // namespace snake::game
