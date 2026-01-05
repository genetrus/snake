#pragma once

namespace snake::game {

class ScoreSystem {
public:
    void Reset();
    int Score() const;
    void AddFood(int food_score = 10);
    void AddBonusScore();       // +50
    double StepsPerSecond() const;  // placeholder formula

private:
    int score_ = 0;
};
}  // namespace snake::game
