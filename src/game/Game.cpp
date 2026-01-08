#include "game/Game.h"

#include <SDL.h>
#include <optional>
#include <random>
#include <sstream>
#include <utility>

namespace snake::game {
namespace {

bool IsOpposite(Dir a, Dir b) {
    return (a == Dir::Up && b == Dir::Down) ||
        (a == Dir::Down && b == Dir::Up) ||
        (a == Dir::Left && b == Dir::Right) ||
        (a == Dir::Right && b == Dir::Left);
}

bool IsSame(Dir a, Dir b) {
    return a == b;
}

}  // namespace

void Game::ResetAll() {
    last_game_over_reason_ = "unknown";
    game_over_ = false;
    turn_queue_.clear();
    std::random_device rd;
    rng_.seed(rd());
    snake_.Reset(board_);
    spawner_.Reset();
    score_.Reset();
    effects_.Reset();
    tick_events_ = {};
    spawner_.EnsureFood(board_, snake_, rng_);

    std::ostringstream segments;
    segments << "[";
    const auto& body = snake_.Body();
    for (std::size_t i = 0; i < body.size(); ++i) {
        segments << "(" << body[i].x << "," << body[i].y << ")";
        if (i + 1 < body.size()) {
            segments << ", ";
        }
    }
    segments << "]";

    const char* dir = "right";
    switch (snake_.Direction()) {
        case Dir::Up:
            dir = "up";
            break;
        case Dir::Down:
            dir = "down";
            break;
        case Dir::Left:
            dir = "left";
            break;
        case Dir::Right:
            dir = "right";
            break;
        default:
            dir = "unknown";
            break;
    }

    SDL_Log("Round start: board=%dx%d segments=%s dir=%s wrap=%s",
            board_.W(), board_.H(), segments.str().c_str(), dir, wrap_mode_ ? "true" : "false");
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
    ApplyTurnQueue();

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
    auto match = [](const ActionKeys& keys, SDL_Keycode key) {
        return key == keys.primary || key == keys.secondary;
    };
    for (SDL_Keycode key : input.KeyPresses()) {
        if (match(controls_.up, key)) {
            EnqueueTurn(Dir::Up);
        } else if (match(controls_.down, key)) {
            EnqueueTurn(Dir::Down);
        } else if (match(controls_.left, key)) {
            EnqueueTurn(Dir::Left);
        } else if (match(controls_.right, key)) {
            EnqueueTurn(Dir::Right);
        }
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

int Game::FoodScore() const {
    return food_score_;
}

int Game::BonusScore() const {
    return bonus_score_;
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

void Game::EnqueueTurn(Dir d) {
    if (turn_queue_.size() >= kTurnQueueCapacity) {
        return;
    }
    const Dir reference = turn_queue_.empty() ? snake_.Direction() : turn_queue_.back();
    if (IsSame(reference, d) || IsOpposite(reference, d)) {
        return;
    }
    // Enqueue only if it isn't a duplicate or a 180-degree reversal.
    turn_queue_.push_back(d);
}

void Game::ApplyTurnQueue() {
    const Dir current = snake_.Direction();
    bool did_apply = false;

    // Apply at most one queued turn per tick; discard invalid reversals.
    while (!turn_queue_.empty() && !did_apply) {
        const Dir next = turn_queue_.front();
        turn_queue_.pop_front();
        if (IsOpposite(current, next) || IsSame(current, next)) {
            continue;
        }
        snake_.SetDirection(next);
        did_apply = true;
    }
}

}  // namespace snake::game
