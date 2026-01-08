#pragma once

#include <SDL.h>

#include <array>
#include <random>
#include <string_view>
#include <vector>

#include "game/Types.h"

namespace snake::render {

class TextRenderer;

class Effects {
public:
    struct Settings {
        bool enable_food_eat = true;
        bool enable_head_flash = true;
        bool enable_bonus_pulse = true;
        bool enable_floating_text = true;
    };

    void Init();
    void Reset();
    void Update(double dt_seconds);

    void RenderFoodEats(SDL_Renderer* r, SDL_Texture* food_tex, SDL_Point origin, int tile_px);
    void RenderFloatingText(SDL_Renderer* r, const TextRenderer& text_renderer, SDL_Point origin, int tile_px);
    void RenderPulse(SDL_Renderer* r, const SDL_Rect& viewport_rect);

    void SpawnFoodEat(snake::game::Pos pos);
    void SpawnFloatingText(snake::game::Pos pos, std::string_view text, SDL_Color color);
    void SpawnBonusPulse(SDL_Color color);
    void StartHeadFlash(double duration_sec);

    double HeadFlashStrength() const;
    void SetSettings(const Settings& settings);

private:
    struct FoodEatEffect {
        snake::game::Pos pos{};
        double elapsed = 0.0;
        double duration = 0.0;
    };

    struct FloatingTextEffect {
        snake::game::Pos pos{};
        double elapsed = 0.0;
        double duration = 0.0;
        float rise_tiles = 0.0f;
        SDL_Color color{255, 255, 255, 255};
        std::array<char, 16> text{};
        std::size_t text_len = 0;
    };

    struct ScreenPulseEffect {
        double elapsed = 0.0;
        double duration = 0.0;
        SDL_Color color{255, 255, 255, 255};
    };

    void Reserve();

    static double EaseOutQuad(double t);

    std::vector<FoodEatEffect> food_eats_{};
    std::vector<FloatingTextEffect> floating_texts_{};
    std::vector<ScreenPulseEffect> pulses_{};

    double head_flash_elapsed_ = 0.0;
    double head_flash_duration_ = 0.0;

    Settings settings_{};
    std::mt19937 rng_{std::random_device{}()};
    std::uniform_real_distribution<double> food_duration_dist_{0.12, 0.18};
};

}  // namespace snake::render
