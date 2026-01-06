#pragma once

#include "core/Input.h"
#include "game/Board.h"
#include "game/Effects.h"
#include "game/ScoreSystem.h"
#include "game/Snake.h"
#include "game/Spawner.h"
#include "game/StateMachine.h"

#include <random>
#include <string>
#include <string_view>

namespace snake::game {
class Game {
public:
    struct TickEvents {
        bool food_eaten = false;
    };

    struct Controls {
        SDL_Keycode up = SDLK_UP;
        SDL_Keycode down = SDLK_DOWN;
        SDL_Keycode left = SDLK_LEFT;
        SDL_Keycode right = SDLK_RIGHT;
        SDL_Keycode pause = SDLK_p;
        SDL_Keycode restart = SDLK_r;
        SDL_Keycode menu = SDLK_ESCAPE;
        SDL_Keycode confirm = SDLK_RETURN;
    };

    void ResetAll();   // menu + reset round data
    void ResetRound(); // reset snake/spawns/score/effects but keep board
    void Tick(double tick_dt);
    void HandleInput(const snake::core::Input& input);
    GameState State() const;
    bool IsGameOver() const;
    std::string_view GameOverReason() const;

    const Board& GetBoard() const;
    const Snake& GetSnake() const;
    const Spawner& GetSpawner() const;
    const ScoreSystem& GetScore() const;
    const Effects& GetEffects() const;
    const TickEvents& Events() const;

    void SetBoardSize(int w, int h);
    void SetWrapMode(bool wrap);
    void SetFoodScore(int food);
    void SetBonusScore(int bonus);
    void SetSlowParams(double multiplier, double duration);
    void SetControls(const Controls& c);

private:
    Board board_;
    Snake snake_;
    Spawner spawner_;
    ScoreSystem score_;
    Effects effects_;
    StateMachine sm_;
    TickEvents tick_events_;

    std::mt19937 rng_;
    bool wrap_mode_ = false;  // walls kill (false) vs wrap (true)
    int food_score_ = 10;
    int bonus_score_ = 50;
    double slow_multiplier_ = 0.70;
    double slow_duration_ = 6.0;
    Controls controls_;

    std::string last_game_over_reason_ = "unknown";

    Pos NextHeadPos() const;
    void SetGameOver(std::string reason);
};
}  // namespace snake::game
