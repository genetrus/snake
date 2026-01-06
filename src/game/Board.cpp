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

void Board::SetSize(int w, int h) {
    w_ = w;
    h_ = h;
}

}  // namespace snake::game
