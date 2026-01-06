#pragma once

#include <optional>
#include <random>
#include <vector>

#include "game/Board.h"
#include "game/Snake.h"
#include "game/Types.h"

namespace snake::game {

enum class BonusType {
    Score,
    Slow
};

struct Bonus {
    Pos pos;
    BonusType type;
};

class Spawner {
public:
    void Reset();
    void EnsureFood(const Board& b, const Snake& s, std::mt19937& rng);     // guarantee 1 food
    void RespawnFood(const Board& b, const Snake& s, std::mt19937& rng);
    void MaybeSpawnBonus(const Board& b, const Snake& s, std::mt19937& rng, int current_score);
    Pos FoodPos() const;
    bool HasFood() const;
    const std::vector<Bonus>& Bonuses() const;
    int BonusCount() const;
    bool HasBonusAt(Pos p) const;
    std::optional<BonusType> BonusTypeAt(Pos p) const;

    void ConsumeFood();          // mark food missing
    void ConsumeBonusAt(Pos p);  // remove bonus if exists at p

private:
    std::optional<Pos> food_;
    std::vector<Bonus> bonuses_;

    std::optional<Pos> RandomFreeCell(const Board& b,
                                      const Snake& s,
                                      std::mt19937& rng,
                                      std::optional<Pos> avoid = std::nullopt) const;
    bool CellOccupied(const Snake& s, Pos candidate) const;
};
}  // namespace snake::game
