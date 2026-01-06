#include "game/StateMachine.h"

namespace snake::game {

void StateMachine::ResetToMenu() {
    state_ = GameState::Menu;
}

GameState StateMachine::Current() const {
    return state_;
}

void StateMachine::StartGame() {
    if (state_ == GameState::Menu) {
        state_ = GameState::Playing;
    }
}

void StateMachine::Pause() {
    if (state_ == GameState::Playing) {
        state_ = GameState::Paused;
    }
}

void StateMachine::Resume() {
    if (state_ == GameState::Paused) {
        state_ = GameState::Playing;
    }
}

void StateMachine::GameOver() {
    if (state_ == GameState::Playing || state_ == GameState::Paused) {
        state_ = GameState::GameOver;
    }
}

void StateMachine::Restart() {
    if (state_ == GameState::GameOver) {
        state_ = GameState::Playing;
    }
}

void StateMachine::BackToMenu() {
    state_ = GameState::Menu;
}

}  // namespace snake::game
