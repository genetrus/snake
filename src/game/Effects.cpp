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
    slow_remaining_ += 6.0;
}

bool Effects::SlowActive() const {
    return slow_remaining_ > 0.0;
}

double Effects::SlowMultiplier() const {
    return 0.70;
}

double Effects::SlowRemaining() const {
    return slow_remaining_;
}

}  // namespace snake::game
