#include "game/Board.h"

namespace snake::game {

Board::Board(int w, int h) : w_(w), h_(h) {}

int Board::W() const {
    return w_;
}

int Board::H() const {
    return h_;
}

bool Board::InBounds(Pos p) const {
    return p.x >= 0 && p.x < w_ && p.y >= 0 && p.y < h_;
}

Pos Board::Wrap(Pos p) const {
    const int w = w_;
    const int h = h_;
    if (w != 0) {
        p.x = (p.x % w + w) % w;
    }
    if (h != 0) {
        p.y = (p.y % h + h) % h;
    }
    return p;
}

void Board::SetSize(int w, int h) {
    w_ = w;
    h_ = h;
}

}  // namespace snake::game
