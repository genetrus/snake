#pragma once

namespace snake::game {

enum class Screen {
  MainMenu,
  Options,
  Highscores,
  Playing,
  Paused,    // overlay on top of Playing, but keep as separate screen for simplicity
  GameOver
};

class StateMachine {
public:
  Screen Current() const;
  void Set(Screen s);
  bool Is(Screen s) const;

  // For transitions
  void StartGame();      // -> Playing (also triggers round start/reset)
  void OpenOptions();    // -> Options
  void OpenHighscores(); // -> Highscores
  void BackToMenu();     // -> MainMenu
  void Pause();          // -> Paused (only from Playing)
  void Resume();         // -> Playing (only from Paused)
  void GameOver();       // -> GameOver
private:
  Screen screen_ = Screen::MainMenu;
};

} // namespace snake::game
