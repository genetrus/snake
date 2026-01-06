#include "game/Game.h"

#include <SDL.h>
#include <random>
#include <optional>
#include <utility>

namespace snake::game {

void Game::ResetAll() {
    last_game_over_reason_ = "unknown";
    game_over_ = false;
    std::random_device rd;
    rng_.seed(rd());
    snake_.Reset(board_);
    spawner_.Reset();
    score_.Reset();
    effects_.Reset();
    tick_events_ = {};
    spawner_.EnsureFood(board_, snake_, rng_);
}

void Game::ResetRound() {
    ResetAll();
}

void Game::Tick(double tick_dt) {
    if (game_over_) {
        return;
    }
    tick_events_ = {};
    effects_.Update(tick_dt);

    spawner_.EnsureFood(board_, snake_, rng_);

    Pos next = NextHeadPos();
    if (wrap_mode_) {
        next = board_.Wrap(next);
    } else if (!board_.InBounds(next)) {
        SetGameOver("wall_collision");
        return;
    }

    if (snake_.WouldCollideSelf(next)) {
        SetGameOver("self_collision");
        return;
    }

    const bool ate_food = spawner_.HasFood() && (spawner_.FoodPos() == next);

    const std::optional<BonusType> bonus_at_next = spawner_.BonusTypeAt(next);

    snake_.Step(next, ate_food);

    if (ate_food) {
        score_.AddFood(food_score_);
        tick_events_.food_eaten = true;
        spawner_.RespawnFood(board_, snake_, rng_);
        spawner_.MaybeSpawnBonus(board_, snake_, rng_, score_.Score());
    }

    if (bonus_at_next.has_value()) {
        if (*bonus_at_next == BonusType::Score) {
            score_.AddBonusScore(bonus_score_);
            tick_events_.bonus_picked = true;
            tick_events_.bonus_type = "bonus_score";
        } else if (*bonus_at_next == BonusType::Slow) {
            effects_.AddSlow(6.0);
            tick_events_.bonus_picked = true;
            tick_events_.bonus_type = "bonus_slow";
        }
        spawner_.ConsumeBonusAt(next);
    }
}

void Game::HandleInput(const snake::core::Input& input) {
    if (game_over_) {
        return;
    }
    auto pressed = [&](const ActionKeys& keys) {
        return input.KeyPressed(keys.primary) || input.KeyPressed(keys.secondary);
    };
    if (pressed(controls_.up)) {
        snake_.SetDirection(Dir::Up);
    } else if (pressed(controls_.down)) {
        snake_.SetDirection(Dir::Down);
    } else if (pressed(controls_.left)) {
        snake_.SetDirection(Dir::Left);
    } else if (pressed(controls_.right)) {
        snake_.SetDirection(Dir::Right);
    }
}

bool Game::IsGameOver() const {
    return game_over_;
}

std::string_view Game::GameOverReason() const {
    return last_game_over_reason_;
}

const Board& Game::GetBoard() const {
    return board_;
}

const Snake& Game::GetSnake() const {
    return snake_;
}

const Spawner& Game::GetSpawner() const {
    return spawner_;
}

const ScoreSystem& Game::GetScore() const {
    return score_;
}

const Effects& Game::GetEffects() const {
    return effects_;
}

const Game::TickEvents& Game::Events() const {
    return tick_events_;
}

void Game::SetBoardSize(int w, int h) {
    board_.SetSize(w, h);
}

void Game::SetWrapMode(bool wrap) {
    wrap_mode_ = wrap;
}

void Game::SetFoodScore(int food) {
    food_score_ = food;
}

void Game::SetBonusScore(int bonus) {
    bonus_score_ = bonus;
}

void Game::SetSlowParams(double multiplier, double duration) {
    slow_multiplier_ = multiplier;
    slow_duration_ = duration;
}

void Game::SetControls(const Controls& c) {
    controls_ = c;
}

Pos Game::NextHeadPos() const {
    Pos head = snake_.Head();
    switch (snake_.Direction()) {
        case Dir::Up:
            --head.y;
            break;
        case Dir::Down:
            ++head.y;
            break;
        case Dir::Left:
            --head.x;
            break;
        case Dir::Right:
            ++head.x;
            break;
        default:
            break;
    }
    return head;
}

void Game::SetGameOver(std::string reason) {
    last_game_over_reason_ = std::move(reason);
    game_over_ = true;
}

}  // namespace snake::game
