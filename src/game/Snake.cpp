#include "game/Snake.h"

#include <SDL.h>

#include <algorithm>

namespace snake::game {

void Snake::Reset(const Board& b) {
    body_.clear();
    dir_ = Dir::Right;

    const int w = b.W();
    const int h = b.H();

    if (w <= 0 || h <= 0) {
        body_.push_back(Pos{0, 0});
        SDL_Log("Snake spawn: board=%dx%d head=(0,0) dir=right (degenerate)", w, h);
        return;
    }

    const int cx = w / 2;
    const int cy = h / 2;

    const int max_x = w - 1;
    const int max_y = h - 1;

    const auto clamp_x = [max_x](int x) { return std::clamp(x, 0, max_x); };
    const auto clamp_y = [max_y](int y) { return std::clamp(y, 0, max_y); };

    if (w >= 3) {
        const int head_x = std::clamp(cx, 2, max_x);
        const int head_y = clamp_y(cy);
        body_.push_back(Pos{head_x, head_y});
        body_.push_back(Pos{head_x - 1, head_y});
        body_.push_back(Pos{head_x - 2, head_y});
    } else if (h >= 3) {
        const int head_x = clamp_x(cx);
        const int head_y = std::clamp(cy, 2, max_y);
        body_.push_back(Pos{head_x, head_y});
        body_.push_back(Pos{head_x, head_y - 1});
        body_.push_back(Pos{head_x, head_y - 2});
    } else {
        const int head_x = clamp_x(cx);
        const int head_y = clamp_y(cy);
        body_.push_back(Pos{head_x, head_y});
        body_.push_back(Pos{clamp_x(head_x - 1), head_y});
        body_.push_back(Pos{head_x, clamp_y(head_y - 1)});
    }

    if (!body_.empty()) {
        const Pos& head = body_.front();
        const Pos& tail = body_.back();
        const Pos& mid = body_.size() > 1 ? body_[1] : head;
        SDL_Log("Snake spawn: board=%dx%d head=(%d,%d) body=(%d,%d) tail=(%d,%d) dir=right",
                w, h, head.x, head.y, mid.x, mid.y, tail.x, tail.y);
    }
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
