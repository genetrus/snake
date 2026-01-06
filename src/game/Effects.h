#pragma once

namespace snake::game {

class Effects {
public:
    void Reset();
    void Update(double tick_dt);  // called each fixed tick
    void AddSlow(double sec);
    bool SlowActive() const;
    double SlowMultiplier() const;  // 0.70
    double SlowRemaining() const;   // seconds

private:
    double slow_remaining_sec_ = 0.0;
    static constexpr double kSlowMultiplier = 0.70;
};
}  // namespace snake::game
