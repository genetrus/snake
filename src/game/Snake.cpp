#include "game/Snake.h"

#include <algorithm>

namespace snake::game {

void Snake::Reset(const Board& b) {
    body_.clear();
    dir_ = Dir::Right;

    const int cx = b.W() / 2;
    const int cy = b.H() / 2;

    body_.push_front(Pos{cx, cy});
    body_.push_back(Pos{cx - 1, cy});
    body_.push_back(Pos{cx - 2, cy});
}

const std::deque<Pos>& Snake::Body() const {
    return body_;
}

Pos Snake::Head() const {
    if (body_.empty()) {
        return Pos{0, 0};
    }
    return body_.front();
}

Dir Snake::Direction() const {
    return dir_;
}

void Snake::SetDirection(Dir d) {
    if (dir_ == Dir::Up && d == Dir::Down) return;
    if (dir_ == Dir::Down && d == Dir::Up) return;
    if (dir_ == Dir::Left && d == Dir::Right) return;
    if (dir_ == Dir::Right && d == Dir::Left) return;
    dir_ = d;
}

bool Snake::Occupies(Pos p) const {
    return std::find(body_.begin(), body_.end(), p) != body_.end();
}

bool Snake::WouldCollideSelf(Pos next_head) const {
    return Occupies(next_head);
}

void Snake::Step(Pos next_head, bool grow) {
    body_.push_front(next_head);
    if (!grow && !body_.empty()) {
        body_.pop_back();
    }
}

int Snake::Length() const {
    return static_cast<int>(body_.size());
}

}  // namespace snake::game
