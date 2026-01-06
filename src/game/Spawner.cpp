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

void Spawner::RespawnFood(const Board& b, const Snake& s, std::mt19937& rng) {
    std::optional<Pos> avoid = food_;
    food_ = RandomFreeCell(b, s, rng, avoid);
}

void Spawner::MaybeSpawnBonus(const Board& b, const Snake& s, std::mt19937& rng, int /*current_score*/) {
    if (bonuses_.size() >= 2) {
        return;
    }

    // TODO(genetrus): Make spawn probability configurable via Lua.
    std::uniform_real_distribution<double> chance_dist(0.0, 1.0);
    if (chance_dist(rng) >= 0.20) {
        return;
    }

    auto free_cell = RandomFreeCell(b, s, rng);
    if (!free_cell.has_value()) {
        return;
    }

    std::uniform_real_distribution<double> type_dist(0.0, 1.0);
    const BonusType type = type_dist(rng) < 0.50 ? BonusType::Score : BonusType::Slow;
    bonuses_.push_back(Bonus{*free_cell, type});

    if (bonuses_.size() > 2) {
        bonuses_.resize(2);
    }
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

int Spawner::BonusCount() const {
    return static_cast<int>(bonuses_.size());
}

bool Spawner::HasBonusAt(Pos p) const {
    return BonusTypeAt(p).has_value();
}

std::optional<BonusType> Spawner::BonusTypeAt(Pos p) const {
    for (const auto& bonus : bonuses_) {
        if (bonus.pos == p) {
            return bonus.type;
        }
    }
    return std::nullopt;
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
            if (CellOccupied(s, candidate)) {
                continue;
            }
            if (avoid.has_value() && candidate == *avoid) {
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

bool Spawner::CellOccupied(const Snake& s, Pos candidate) const {
    if (s.Occupies(candidate)) {
        return true;
    }
    if (food_.has_value() && candidate == *food_) {
        return true;
    }
    return std::any_of(
        bonuses_.begin(),
        bonuses_.end(),
        [candidate](const Bonus& bonus) { return bonus.pos == candidate; });
}

}  // namespace snake::game
