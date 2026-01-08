#include "render/Effects.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "render/TextRenderer.h"

namespace snake::render {
namespace {
constexpr std::size_t kMaxFoodEat = 24;
constexpr std::size_t kMaxFloatingText = 24;
constexpr std::size_t kMaxPulses = 4;
constexpr double kMaxFrameDt = 0.25;

double Clamp01(double v) {
    return std::max(0.0, std::min(1.0, v));
}

Uint8 ScaleAlpha(Uint8 base, double intensity) {
    const double scaled = static_cast<double>(base) * Clamp01(intensity);
    return static_cast<Uint8>(std::round(std::clamp(scaled, 0.0, 255.0)));
}

SDL_Rect TileRect(SDL_Point origin, int tile_px, snake::game::Pos p, int size_override = -1) {
    const int size = (size_override > 0) ? size_override : tile_px;
    const int offset = (tile_px - size) / 2;
    const int x = origin.x + p.x * tile_px + offset;
    const int y = origin.y + p.y * tile_px + offset;
    return SDL_Rect{x, y, size, size};
}

}  // namespace

void Effects::Init() {
    Reserve();
    Reset();
}

void Effects::Reset() {
    food_eats_.clear();
    floating_texts_.clear();
    pulses_.clear();
    head_flash_elapsed_ = 0.0;
    head_flash_duration_ = 0.0;
}

void Effects::Update(double dt_seconds) {
    if (dt_seconds < 0.0) {
        dt_seconds = 0.0;
    } else if (dt_seconds > kMaxFrameDt) {
        dt_seconds = kMaxFrameDt;
    }

    if (head_flash_duration_ > 0.0) {
        head_flash_elapsed_ = std::min(head_flash_elapsed_ + dt_seconds, head_flash_duration_);
        if (head_flash_elapsed_ >= head_flash_duration_) {
            head_flash_duration_ = 0.0;
            head_flash_elapsed_ = 0.0;
        }
    }

    auto update_list = [dt_seconds](auto& effects) {
        std::size_t i = 0;
        while (i < effects.size()) {
            effects[i].elapsed += dt_seconds;
            if (effects[i].elapsed >= effects[i].duration) {
                effects[i] = effects.back();
                effects.pop_back();
                continue;
            }
            ++i;
        }
    };

    update_list(food_eats_);
    update_list(floating_texts_);
    update_list(pulses_);
}

void Effects::RenderFoodEats(SDL_Renderer* r, SDL_Texture* food_tex, SDL_Point origin, int tile_px) {
    if (r == nullptr || food_eats_.empty()) {
        return;
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    Uint8 restore_alpha = 255;
    if (food_tex != nullptr) {
        SDL_GetTextureAlphaMod(food_tex, &restore_alpha);
    }

    for (const auto& effect : food_eats_) {
        if (effect.duration <= 0.0) {
            continue;
        }
        const double t = Clamp01(effect.elapsed / effect.duration);
        const double scale = 1.0 - t;
        const double alpha = 1.0 - t;
        const int size = std::max(1, static_cast<int>(std::round(tile_px * scale)));
        SDL_Rect dst = TileRect(origin, tile_px, effect.pos, size);
        if (food_tex != nullptr) {
            SDL_SetTextureAlphaMod(food_tex, ScaleAlpha(255, alpha));
            SDL_RenderCopy(r, food_tex, nullptr, &dst);
        } else {
            SDL_Color color{200, 80, 80, ScaleAlpha(255, alpha)};
            SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(r, &dst);
        }
    }

    if (food_tex != nullptr) {
        SDL_SetTextureAlphaMod(food_tex, restore_alpha);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void Effects::RenderFloatingText(SDL_Renderer* r,
                                 const TextRenderer& text_renderer,
                                 SDL_Point origin,
                                 int tile_px) {
    if (r == nullptr || floating_texts_.empty()) {
        return;
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    for (const auto& effect : floating_texts_) {
        if (effect.duration <= 0.0 || effect.text_len == 0) {
            continue;
        }
        const double t = Clamp01(effect.elapsed / effect.duration);
        const double rise = static_cast<double>(tile_px) * effect.rise_tiles * EaseOutQuad(t);
        const double fade = t > 0.65 ? (1.0 - (t - 0.65) / 0.35) : 1.0;
        const SDL_Color color{effect.color.r, effect.color.g, effect.color.b, ScaleAlpha(effect.color.a, fade)};

        const std::string_view text(effect.text.data(), effect.text_len);
        const auto metrics = text_renderer.MeasureText(text, 16);
        const int base_x = origin.x + effect.pos.x * tile_px + tile_px / 2;
        const int base_y = origin.y + effect.pos.y * tile_px - static_cast<int>(std::round(rise)) - tile_px / 4;

        bool drew = false;
        if (metrics.w > 0 && metrics.h > 0) {
            const int draw_x = base_x - metrics.w / 2;
            const int draw_y = base_y - metrics.h / 2;
            drew = text_renderer.DrawText(r, draw_x, draw_y, text, color, 16) > 0;
        }

        if (!drew) {
            const int size = std::max(4, tile_px / 4);
            SDL_Rect fallback{base_x - size / 2, base_y - size / 2, size, size / 2};
            SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
            SDL_RenderFillRect(r, &fallback);
        }
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void Effects::RenderPulse(SDL_Renderer* r, const SDL_Rect& viewport_rect) {
    if (r == nullptr || pulses_.empty()) {
        return;
    }

    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    for (const auto& effect : pulses_) {
        if (effect.duration <= 0.0) {
            continue;
        }
        const double t = Clamp01(effect.elapsed / effect.duration);
        const double intensity = 1.0 - EaseOutQuad(t);
        SDL_Color color = effect.color;
        color.a = ScaleAlpha(effect.color.a, intensity);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(r, &viewport_rect);
    }
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
}

void Effects::SpawnFoodEat(snake::game::Pos pos) {
    if (!settings_.enable_food_eat) {
        return;
    }

    FoodEatEffect effect;
    effect.pos = pos;
    effect.duration = food_duration_dist_(rng_);
    if (food_eats_.size() >= kMaxFoodEat) {
        food_eats_[0] = effect;
        return;
    }
    food_eats_.push_back(effect);
}

void Effects::SpawnFloatingText(snake::game::Pos pos, std::string_view text, SDL_Color color) {
    if (!settings_.enable_floating_text || text.empty()) {
        return;
    }

    FloatingTextEffect effect;
    effect.pos = pos;
    effect.duration = 0.75;
    effect.rise_tiles = 0.85f;
    effect.color = color;
    const std::size_t len = std::min(text.size(), effect.text.size() - 1);
    std::memcpy(effect.text.data(), text.data(), len);
    effect.text[len] = '\0';
    effect.text_len = len;

    if (floating_texts_.size() >= kMaxFloatingText) {
        floating_texts_[0] = effect;
        return;
    }
    floating_texts_.push_back(effect);
}

void Effects::SpawnBonusPulse(SDL_Color color) {
    if (!settings_.enable_bonus_pulse) {
        return;
    }

    ScreenPulseEffect effect;
    effect.duration = 0.22;
    effect.color = color;

    if (pulses_.size() >= kMaxPulses) {
        pulses_[0] = effect;
        return;
    }
    pulses_.push_back(effect);
}

void Effects::StartHeadFlash(double duration_sec) {
    if (!settings_.enable_head_flash) {
        return;
    }

    if (duration_sec <= 0.0) {
        return;
    }
    head_flash_duration_ = duration_sec;
    head_flash_elapsed_ = 0.0;
}

double Effects::HeadFlashStrength() const {
    if (head_flash_duration_ <= 0.0) {
        return 0.0;
    }
    const double t = Clamp01(head_flash_elapsed_ / head_flash_duration_);
    return 1.0 - EaseOutQuad(t);
}

void Effects::SetSettings(const Settings& settings) {
    settings_ = settings;
}

void Effects::Reserve() {
    food_eats_.reserve(kMaxFoodEat);
    floating_texts_.reserve(kMaxFloatingText);
    pulses_.reserve(kMaxPulses);
}

double Effects::EaseOutQuad(double t) {
    const double clamped = Clamp01(t);
    return 1.0 - (1.0 - clamped) * (1.0 - clamped);
}

}  // namespace snake::render
