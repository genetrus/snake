#pragma once

namespace snake::game {

class Effects {
public:
    void Reset();
    void Tick(double tick_dt);  // called each fixed tick
    void ApplySlow();           // adds +6 seconds; activates if inactive
    bool SlowActive() const;
    double SlowMultiplier() const;  // 0.70
    double SlowRemaining() const;   // seconds

private:
    double slow_remaining_ = 0.0;
};
}  // namespace snake::game
