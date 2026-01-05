#pragma once

namespace snake::game {
struct Pos {
    int x;
    int y;
};

inline bool operator==(const Pos&, const Pos&) = default;
}  // namespace snake::game
