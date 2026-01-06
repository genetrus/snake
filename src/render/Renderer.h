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
                     const std::string& overlay_error_text);

private:
    SpriteAtlas atlas_;
    Font font_;
    UIRenderer ui_;
    Pulse food_pulse_;
};

}  // namespace snake::render
