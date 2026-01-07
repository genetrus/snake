#include "game/StateMachine.h"

namespace snake::game {

Screen StateMachine::Current() const {
  return screen_;
}

void StateMachine::Set(Screen s) {
  screen_ = s;
}

bool StateMachine::Is(Screen s) const {
  return screen_ == s;
}

void StateMachine::StartGame() {
  screen_ = Screen::Playing;
}

void StateMachine::OpenOptions() {
  screen_ = Screen::Options;
}

void StateMachine::OpenHighscores() {
  screen_ = Screen::Highscores;
}

void StateMachine::BackToMenu() {
  screen_ = Screen::MainMenu;
}

void StateMachine::Pause() {
  if (screen_ == Screen::Playing) {
    screen_ = Screen::Paused;
  }
}

void StateMachine::Resume() {
  if (screen_ == Screen::Paused) {
    screen_ = Screen::Playing;
  }
}

void StateMachine::GameOver() {
  screen_ = Screen::GameOver;
}

void StateMachine::NameEntry() {
  screen_ = Screen::NameEntry;
}

}  // namespace snake::game
