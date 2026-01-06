#include "game/Game.h"

#include <SDL.h>
#include <utility>
#include <random>

namespace snake::game {

void Game::ResetAll() {
    sm_.ResetToMenu();
    ResetRound();
}

void Game::ResetRound() {
    last_game_over_reason_ = "unknown";
    std::random_device rd;
    rng_.seed(rd());
    snake_.Reset(board_);
    spawner_.Reset();
    score_.Reset();
    effects_.Reset();
    tick_events_ = {};
    spawner_.EnsureFood(board_, snake_, rng_);
    spawner_.EnsureBonuses(board_, snake_);
}

void Game::Tick(double tick_dt) {
    if (sm_.Current() != GameState::Playing) {
        return;
    }

    tick_events_ = {};
    effects_.Tick(tick_dt);

    spawner_.EnsureFood(board_, snake_, rng_);
    spawner_.EnsureBonuses(board_, snake_);

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

    bool got_bonus_score = false;
    bool got_bonus_slow = false;
    for (const auto& bonus : spawner_.Bonuses()) {
        if (bonus.pos == next) {
            if (bonus.type == "bonus_score") {
                got_bonus_score = true;
            } else if (bonus.type == "bonus_slow") {
                got_bonus_slow = true;
            }
        }
    }

    snake_.Step(next, ate_food);

    if (ate_food) {
        score_.AddFood(food_score_);
        tick_events_.food_eaten = true;
        spawner_.RespawnFood(board_, snake_, rng_);
    }

    if (got_bonus_score || got_bonus_slow) {
        if (got_bonus_score) {
            score_.AddBonusScore(bonus_score_);
        }
        if (got_bonus_slow) {
            effects_.ApplySlow();
        }
        spawner_.ConsumeBonusAt(next);
    }

    spawner_.EnsureBonuses(board_, snake_);
}

void Game::HandleInput(const snake::core::Input& input) {
    const auto state = sm_.Current();

    const auto handle_direction = [this, &input]() {
        if (input.KeyPressed(controls_.up)) {
            snake_.SetDirection(Dir::Up);
        } else if (input.KeyPressed(controls_.down)) {
            snake_.SetDirection(Dir::Down);
        } else if (input.KeyPressed(controls_.left)) {
            snake_.SetDirection(Dir::Left);
        } else if (input.KeyPressed(controls_.right)) {
            snake_.SetDirection(Dir::Right);
        }
    };

    switch (state) {
        case GameState::Menu:
            if (input.KeyPressed(controls_.confirm)) {
                ResetRound();
                sm_.StartGame();
            }
            break;
        case GameState::Playing:
            if (input.KeyPressed(controls_.pause)) {
                sm_.Pause();
            }
            if (input.KeyPressed(controls_.menu)) {
                sm_.BackToMenu();
                ResetRound();
            }
            handle_direction();
            break;
        case GameState::Paused:
            if (input.KeyPressed(controls_.pause)) {
                sm_.Resume();
            }
            if (input.KeyPressed(controls_.restart)) {
                ResetRound();
                sm_.Resume();
            }
            if (input.KeyPressed(controls_.menu)) {
                sm_.BackToMenu();
                ResetRound();
            }
            break;
        case GameState::GameOver:
            if (input.KeyPressed(controls_.restart)) {
                ResetRound();
                sm_.Restart();
            }
            if (input.KeyPressed(controls_.menu)) {
                sm_.BackToMenu();
                ResetRound();
            }
            break;
        default:
            break;
    }
}

GameState Game::State() const {
    return sm_.Current();
}

bool Game::IsGameOver() const {
    return sm_.Current() == GameState::GameOver;
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
    effects_.SetSlowParams(multiplier, duration);
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
    sm_.GameOver();
}

}  // namespace snake::game
