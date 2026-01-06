#pragma once

#include "game/Types.h"

namespace snake::game {
class Board {
public:
    Board(int w = 20, int h = 20);
    int W() const;
    int H() const;
    bool InBounds(Pos p) const;
    Pos Wrap(Pos p) const;
    void SetSize(int w, int h);

private:
    int w_;
    int h_;
};
}  // namespace snake::game
