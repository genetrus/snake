#include "game/Spawner.h"

#include <algorithm>
#include <stdexcept>

namespace snake::game {

void Spawner::Reset(std::uint32_t seed) {
    rng_.seed(seed);
    has_food_ = false;
    bonuses_.clear();
}

void Spawner::EnsureFood(const Board& b, const Snake& s) {
    if (has_food_) {
        return;
    }

    food_ = RandomFreeCell(b, s);
    has_food_ = true;
}

void Spawner::EnsureBonuses(const Board& /*b*/, const Snake& /*s*/) {
    // Placeholder: keep bonuses empty for now while respecting max size.
    if (bonuses_.size() > 2) {
        bonuses_.resize(2);
    }
}

Pos Spawner::FoodPos() const {
    return food_;
}

const std::vector<Bonus>& Spawner::Bonuses() const {
    return bonuses_;
}

void Spawner::ConsumeFood() {
    has_food_ = false;
}

void Spawner::ConsumeBonusAt(Pos p) {
    bonuses_.erase(
        std::remove_if(
            bonuses_.begin(),
            bonuses_.end(),
            [p](const Bonus& b) { return b.pos == p; }),
        bonuses_.end());
}

Pos Spawner::RandomFreeCell(const Board& b, const Snake& s) {
    std::vector<Pos> free_cells;
    free_cells.reserve(static_cast<std::size_t>(b.W() * b.H()));

    for (int y = 0; y < b.H(); ++y) {
        for (int x = 0; x < b.W(); ++x) {
            Pos candidate{x, y};
            if (s.Occupies(candidate)) {
                continue;
            }
            if (has_food_ && candidate == food_) {
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
        throw std::runtime_error("No free cell available for spawning");
    }

    std::uniform_int_distribution<std::size_t> dist(0, free_cells.size() - 1);
    return free_cells[dist(rng_)];
}

}  // namespace snake::game
