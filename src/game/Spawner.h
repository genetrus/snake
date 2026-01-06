#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>
#include <optional>

#include "game/Board.h"
#include "game/Snake.h"
#include "game/Types.h"

namespace snake::game {

struct Bonus {
    std::string type;
    Pos pos;
};

class Spawner {
public:
    void Reset();
    void EnsureFood(const Board& b, const Snake& s, std::mt19937& rng);     // guarantee 1 food
    void EnsureBonuses(const Board& b, const Snake& s);  // keep <=2
    void RespawnFood(const Board& b, const Snake& s, std::mt19937& rng);
    Pos FoodPos() const;
    bool HasFood() const;
    const std::vector<Bonus>& Bonuses() const;

    void ConsumeFood();          // mark food missing
    void ConsumeBonusAt(Pos p);  // remove bonus if exists at p

private:
    std::optional<Pos> food_;
    std::vector<Bonus> bonuses_;

    std::optional<Pos> RandomFreeCell(const Board& b,
                                      const Snake& s,
                                      std::mt19937& rng,
                                      std::optional<Pos> avoid = std::nullopt) const;
};
}  // namespace snake::game
