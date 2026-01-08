#pragma once

#include <SDL.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "game/Game.h"
#include "render/Animation.h"
#include "render/Effects.h"
#include "render/TextRenderer.h"
#include "render/UIRenderer.h"

namespace snake::render {

struct RenderSettings {
    int tile_px = 32;
    std::string panel_mode = "auto";  // "auto"|"top"|"right"
};

class Renderer {
public:
    bool Init(SDL_Renderer* r);
    void Shutdown();
    void ResetEffects();
    void SpawnFoodEat(snake::game::Pos pos, int score_delta);
    void SpawnBonusPickup(snake::game::Pos pos, std::string_view bonus_type, int score_delta);

    void RenderFrame(SDL_Renderer* r,
                     int window_w,
                     int window_h,
                     const RenderSettings& rs,
                     const snake::game::Game& game,
                     double now_seconds,
                     const std::string& overlay_error_text,
                     bool show_text_debug,
                     bool show_audio_debug,
                     const std::vector<std::string>& audio_debug_lines,
                     const snake::render::UiFrameData& ui_frame);

private:
    struct SpriteTexture {
        SDL_Texture* texture = nullptr;
        int w = 0;
        int h = 0;
    };

    struct SpriteSet {
        SpriteTexture snake_head;
        SpriteTexture snake_body;
        SpriteTexture food;
        SpriteTexture bonus_score;
        SpriteTexture bonus_slow;
    };

    bool EnsureFramebuffer(SDL_Renderer* r, int virtual_w, int virtual_h);
    void DestroyFramebuffer();
    bool LoadSprite(SDL_Renderer* r,
                    const std::filesystem::path& path,
                    SpriteTexture& sprite,
                    std::string_view label);
    void ResetSprites();

    SpriteSet sprites_{};
    std::string sprite_error_text_;
    TextRenderer text_renderer_;
    UIRenderer ui_;
    Pulse food_pulse_;
    Effects effects_;

    SDL_Texture* framebuffer_ = nullptr;
    int fb_w_ = 0;
    int fb_h_ = 0;
    double last_render_seconds_ = 0.0;
};

}  // namespace snake::render
