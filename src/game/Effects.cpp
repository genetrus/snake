#include "game/Effects.h"

namespace snake::game {

void Effects::Reset() {
    slow_remaining_sec_ = 0.0;
}

void Effects::Update(double tick_dt) {
    if (slow_remaining_sec_ <= 0.0) {
        return;
    }

    slow_remaining_sec_ -= tick_dt;
    if (slow_remaining_sec_ < 0.0) {
        slow_remaining_sec_ = 0.0;
    }
}

void Effects::AddSlow(double sec) {
    slow_remaining_sec_ += sec;
}

bool Effects::SlowActive() const {
    return slow_remaining_sec_ > 0.0;
}

double Effects::SlowMultiplier() const {
    return kSlowMultiplier;
}

double Effects::SlowRemaining() const {
    return slow_remaining_sec_;
}

}  // namespace snake::game
