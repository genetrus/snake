#pragma once

#include <deque>

#include "game/Board.h"
#include "game/Types.h"

namespace snake::game {

enum class Dir { Up, Down, Left, Right };

class Snake {
public:
    void Reset(const Board& b);  // length 3, centered
    const std::deque<Pos>& Body() const;
    Pos Head() const;
    Dir Direction() const;
    void SetDirection(Dir d);  // reject 180-degree reversal
    bool Occupies(Pos p) const;
    bool WouldCollideSelf(Pos next_head) const;  // if next_head in body (including tail)
    void Step(Pos next_head, bool grow);
    int Length() const;

private:
    std::deque<Pos> body_;
    Dir dir_ = Dir::Right;
};
}  // namespace snake::game
