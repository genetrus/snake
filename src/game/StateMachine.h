#pragma once

namespace snake::game {

enum class GameState { Menu, Playing, Paused, GameOver };

class StateMachine {
public:
    void ResetToMenu();
    GameState Current() const;

    void StartGame();  // Menu -> Playing and reset round
    void Pause();      // Playing -> Paused
    void Resume();     // Paused -> Playing
    void GameOver();   // Playing/Paused -> GameOver
    void Restart();    // GameOver -> Playing and reset round
    void BackToMenu(); // any -> Menu

private:
    GameState state_ = GameState::Menu;
};
}  // namespace snake::game
