#include "game/Game.h"

#include <SDL.h>

namespace snake::game {

void Game::ResetAll() {
    sm_.ResetToMenu();
    ResetRound();
}

void Game::ResetRound() {
    snake_.Reset(board_);
    spawner_.Reset();
    score_.Reset();
    effects_.Reset();
    spawner_.EnsureFood(board_, snake_);
    spawner_.EnsureBonuses(board_, snake_);
}

void Game::Tick(double tick_dt) {
    if (sm_.Current() != State::Playing) {
        return;
    }

    effects_.Tick(tick_dt);

    spawner_.EnsureFood(board_, snake_);
    spawner_.EnsureBonuses(board_, snake_);

    Pos next = NextHeadPos();
    if (wrap_mode_) {
        next = ApplyWrap(next);
    } else if (!board_.InBounds(next)) {
        sm_.GameOver();
        return;
    }

    if (snake_.WouldCollideSelf(next)) {
        sm_.GameOver();
        return;
    }

    const bool ate_food = (spawner_.FoodPos() == next);

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
        score_.AddFood(10);
        spawner_.ConsumeFood();
        spawner_.EnsureFood(board_, snake_);
    }

    if (got_bonus_score || got_bonus_slow) {
        if (got_bonus_score) {
            score_.AddBonusScore();
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
        if (input.KeyPressed(SDLK_UP) || input.KeyPressed(SDLK_w)) {
            snake_.SetDirection(Dir::Up);
        } else if (input.KeyPressed(SDLK_DOWN) || input.KeyPressed(SDLK_s)) {
            snake_.SetDirection(Dir::Down);
        } else if (input.KeyPressed(SDLK_LEFT) || input.KeyPressed(SDLK_a)) {
            snake_.SetDirection(Dir::Left);
        } else if (input.KeyPressed(SDLK_RIGHT) || input.KeyPressed(SDLK_d)) {
            snake_.SetDirection(Dir::Right);
        }
    };

    switch (state) {
        case State::Menu:
            if (input.KeyPressed(SDLK_RETURN)) {
                ResetRound();
                sm_.StartGame();
            }
            break;
        case State::Playing:
            if (input.KeyPressed(SDLK_p)) {
                sm_.Pause();
            }
            if (input.KeyPressed(SDLK_ESCAPE)) {
                sm_.BackToMenu();
                ResetRound();
            }
            handle_direction();
            break;
        case State::Paused:
            if (input.KeyPressed(SDLK_p)) {
                sm_.Resume();
            }
            if (input.KeyPressed(SDLK_r)) {
                ResetRound();
                sm_.Resume();
            }
            if (input.KeyPressed(SDLK_ESCAPE)) {
                sm_.BackToMenu();
                ResetRound();
            }
            break;
        case State::GameOver:
            if (input.KeyPressed(SDLK_r)) {
                ResetRound();
                sm_.Restart();
            }
            if (input.KeyPressed(SDLK_ESCAPE)) {
                sm_.BackToMenu();
                ResetRound();
            }
            break;
        default:
            break;
    }
}

State Game::State() const {
    return sm_.Current();
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

Pos Game::ApplyWrap(Pos p) const {
    const int w = board_.W();
    const int h = board_.H();

    if (w > 0) {
        while (p.x < 0) p.x += w;
        while (p.x >= w) p.x -= w;
    }
    if (h > 0) {
        while (p.y < 0) p.y += h;
        while (p.y >= h) p.y -= h;
    }
    return p;
}

}  // namespace snake::game
