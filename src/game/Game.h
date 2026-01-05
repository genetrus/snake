#pragma once

#include "core/Input.h"
#include "game/Board.h"
#include "game/Effects.h"
#include "game/ScoreSystem.h"
#include "game/Snake.h"
#include "game/Spawner.h"
#include "game/StateMachine.h"

namespace snake::game {
class Game {
public:
    void ResetAll();   // menu + reset round data
    void ResetRound(); // reset snake/spawns/score/effects but keep board
    void Tick(double tick_dt);
    void HandleInput(const snake::core::Input& input);
    State State() const;

    const Board& GetBoard() const;
    const Snake& GetSnake() const;
    const Spawner& GetSpawner() const;
    const ScoreSystem& GetScore() const;
    const Effects& GetEffects() const;

private:
    Board board_;
    Snake snake_;
    Spawner spawner_;
    ScoreSystem score_;
    Effects effects_;
    StateMachine sm_;

    bool wrap_mode_ = false;  // walls kill (false) vs wrap (true)

    Pos NextHeadPos() const;
    Pos ApplyWrap(Pos p) const;
};
}  // namespace snake::game
