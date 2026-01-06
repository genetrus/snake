#include "game/Spawner.h"

#include <algorithm>

namespace snake::game {

void Spawner::Reset() {
    food_.reset();
    bonuses_.clear();
}

void Spawner::EnsureFood(const Board& b, const Snake& s, std::mt19937& rng) {
    if (food_.has_value()) {
        return;
    }

    food_ = RandomFreeCell(b, s, rng);
}

void Spawner::EnsureBonuses(const Board& /*b*/, const Snake& /*s*/) {
    // Placeholder: keep bonuses empty for now while respecting max size.
    if (bonuses_.size() > 2) {
        bonuses_.resize(2);
    }
}

void Spawner::RespawnFood(const Board& b, const Snake& s, std::mt19937& rng) {
    std::optional<Pos> avoid = food_;
    food_ = RandomFreeCell(b, s, rng, avoid);
}

Pos Spawner::FoodPos() const {
    return food_.value_or(Pos{0, 0});
}

bool Spawner::HasFood() const {
    return food_.has_value();
}

const std::vector<Bonus>& Spawner::Bonuses() const {
    return bonuses_;
}

void Spawner::ConsumeFood() {
    food_.reset();
}

void Spawner::ConsumeBonusAt(Pos p) {
    bonuses_.erase(
        std::remove_if(
            bonuses_.begin(),
            bonuses_.end(),
            [p](const Bonus& b) { return b.pos == p; }),
        bonuses_.end());
}

std::optional<Pos> Spawner::RandomFreeCell(const Board& b,
                                           const Snake& s,
                                           std::mt19937& rng,
                                           std::optional<Pos> avoid) const {
    std::vector<Pos> free_cells;
    free_cells.reserve(static_cast<std::size_t>(b.W() * b.H()));

    for (int y = 0; y < b.H(); ++y) {
        for (int x = 0; x < b.W(); ++x) {
            Pos candidate{x, y};
            if (s.Occupies(candidate)) {
                continue;
            }
            if (avoid.has_value() && candidate == *avoid) {
                continue;
            }
            if (std::any_of(
                    bonuses_.begin(),
                    bonuses_.end(),
                    [candidate](const Bonus& bonus) { return bonus.pos == candidate; })) {
                continue;
            }
            free_cells.push_back(candidate);
        }
    }

    if (free_cells.empty()) {
        return std::nullopt;
    }

    std::uniform_int_distribution<std::size_t> dist(0, free_cells.size() - 1);
    return free_cells[dist(rng)];
}

}  // namespace snake::game
