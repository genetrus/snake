#include "game/ScoreSystem.h"

namespace snake::game {

void ScoreSystem::Reset() {
    score_ = 0;
}

int ScoreSystem::Score() const {
    return score_;
}

void ScoreSystem::AddFood(int food_score) {
    score_ += food_score;
}

void ScoreSystem::AddBonusScore() {
    score_ += 50;
}

double ScoreSystem::StepsPerSecond() const {
    return 10.0 + static_cast<double>(score_) * 0.05;
}

}  // namespace snake::game
