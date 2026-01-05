#pragma once

#include <algorithm>
#include <cmath>

namespace snake::render {

struct Pulse {
    double period = 0.6;     // seconds
    double min_scale = 0.85;
    double max_scale = 1.10;
    double Eval(double t) const;  // returns scale
};

inline double Lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

inline double Clamp01(double t) {
    return std::max(0.0, std::min(1.0, t));
}

struct Slide {
    double duration = 0.10;  // seconds
    double Alpha(double elapsed) const;  // 0..1
};

}  // namespace snake::render
