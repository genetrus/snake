#include "game/StateMachine.h"

namespace snake::game {

void StateMachine::ResetToMenu() {
    state_ = State::Menu;
}

State StateMachine::Current() const {
    return state_;
}

void StateMachine::StartGame() {
    if (state_ == State::Menu) {
        state_ = State::Playing;
    }
}

void StateMachine::Pause() {
    if (state_ == State::Playing) {
        state_ = State::Paused;
    }
}

void StateMachine::Resume() {
    if (state_ == State::Paused) {
        state_ = State::Playing;
    }
}

void StateMachine::GameOver() {
    if (state_ == State::Playing || state_ == State::Paused) {
        state_ = State::GameOver;
    }
}

void StateMachine::Restart() {
    if (state_ == State::GameOver) {
        state_ = State::Playing;
    }
}

void StateMachine::BackToMenu() {
    state_ = State::Menu;
}

}  // namespace snake::game
