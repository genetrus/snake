#pragma once

#include <SDL.h>

#include <string>

#include "game/Game.h"
#include "render/Animation.h"
#include "render/Font.h"
#include "render/SpriteAtlas.h"
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

    void RenderFrame(SDL_Renderer* r,
                     int window_w,
                     int window_h,
                     const RenderSettings& rs,
                     const snake::game::Game& game,
                     double now_seconds,
                     const std::string& overlay_error_text,
                     const snake::render::UiFrameData& ui_frame);

private:
    bool EnsureFramebuffer(SDL_Renderer* r, int virtual_w, int virtual_h);
    void DestroyFramebuffer();

    SpriteAtlas atlas_;
    Font font_;
    UIRenderer ui_;
    Pulse food_pulse_;

    SDL_Texture* framebuffer_ = nullptr;
    int fb_w_ = 0;
    int fb_h_ = 0;
};

}  // namespace snake::render
