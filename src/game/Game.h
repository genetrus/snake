#pragma once

#include "core/Input.h"
#include "game/Board.h"
#include "game/Effects.h"
#include "game/ScoreSystem.h"
#include "game/Snake.h"
#include "game/Spawner.h"

#include <random>
#include <string>
#include <string_view>

namespace snake::game {
class Game {
public:
    struct TickEvents {
        bool food_eaten = false;
        bool bonus_picked = false;
        std::string bonus_type;
    };

    struct ActionKeys {
        SDL_Keycode primary = SDLK_UNKNOWN;
        SDL_Keycode secondary = SDLK_UNKNOWN;
    };

    struct Controls {
        ActionKeys up{SDLK_UP, SDLK_w};
        ActionKeys down{SDLK_DOWN, SDLK_s};
        ActionKeys left{SDLK_LEFT, SDLK_a};
        ActionKeys right{SDLK_RIGHT, SDLK_d};
        ActionKeys pause{SDLK_p, SDLK_p};
        ActionKeys restart{SDLK_r, SDLK_r};
        ActionKeys menu{SDLK_ESCAPE, SDLK_ESCAPE};
        ActionKeys confirm{SDLK_RETURN, SDLK_RETURN};
    };

    void ResetAll();   // reset round data
    void ResetRound(); // reset snake/spawns/score/effects but keep board
    void Tick(double tick_dt);
    void HandleInput(const snake::core::Input& input);
    bool IsGameOver() const;
    std::string_view GameOverReason() const;

    const Board& GetBoard() const;
    const Snake& GetSnake() const;
    const Spawner& GetSpawner() const;
    const ScoreSystem& GetScore() const;
    const Effects& GetEffects() const;
    const TickEvents& Events() const;
    int FoodScore() const;
    int BonusScore() const;

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
    TickEvents tick_events_;

    std::mt19937 rng_;
    bool wrap_mode_ = false;  // walls kill (false) vs wrap (true)
    int food_score_ = 10;
    int bonus_score_ = 50;
    double slow_multiplier_ = 0.70;
    double slow_duration_ = 6.0;
    Controls controls_;

    std::string last_game_over_reason_ = "unknown";
    bool game_over_ = false;

    Pos NextHeadPos() const;
    void SetGameOver(std::string reason);
};
}  // namespace snake::game
