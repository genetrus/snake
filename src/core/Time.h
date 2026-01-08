#pragma once

#include <cstdint>

namespace snake::core {
class Time {
public:
    void Init();
    void UpdateFrame();
    void UpdateFrameNoAccum();

    double FrameDt() const;
    double Now() const;

    void SetTickDt(double seconds);
    bool ConsumeTick();
    double TickDt() const;

    // Returns true if accumulator has enough time for at least one tick.
    bool HasPendingTick() const;
    // Drops the accumulator down to at most one tick to avoid runaway loops.
    void DropAccumulatorToOneTick();

private:
    uint64_t start_counter_ = 0;
    uint64_t last_counter_ = 0;
    double frequency_inv_ = 0.0;
    double frame_dt_ = 0.0;
    double accumulator_ = 0.0;
    double tick_dt_ = 1.0 / 10.0;
};
}  // namespace snake::core
