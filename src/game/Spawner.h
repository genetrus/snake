#pragma once

#include <cstdint>
#include <random>
#include <string>
#include <vector>

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
    void Reset(std::uint32_t seed = 1337);
    void EnsureFood(const Board& b, const Snake& s);     // guarantee 1 food
    void EnsureBonuses(const Board& b, const Snake& s);  // keep <=2
    Pos FoodPos() const;
    const std::vector<Bonus>& Bonuses() const;

    void ConsumeFood();          // mark food missing
    void ConsumeBonusAt(Pos p);  // remove bonus if exists at p

private:
    std::mt19937 rng_;
    bool has_food_ = false;
    Pos food_{0, 0};
    std::vector<Bonus> bonuses_;

    Pos RandomFreeCell(const Board& b, const Snake& s);  // throws if no space
};
}  // namespace snake::game
