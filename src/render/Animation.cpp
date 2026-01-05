#include "render/Animation.h"

namespace snake::render {

double Pulse::Eval(double t) const {
    if (period <= 0.0) {
        return 1.0;
    }
    const double phase = (t / period) * 2.0 * 3.14159265358979323846;
    const double s = (std::sin(phase) + 1.0) * 0.5;  // 0..1
    return Lerp(min_scale, max_scale, s);
}

double Slide::Alpha(double elapsed) const {
    if (duration <= 0.0) {
        return 1.0;
    }
    return Clamp01(elapsed / duration);
}

}  // namespace snake::render
