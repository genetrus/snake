#pragma once

#include <compare>

namespace snake::game {
struct Pos {
    int x;
    int y;

    auto operator<=>(const Pos&) const = default;
};
}  // namespace snake::game
