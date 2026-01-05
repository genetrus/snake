#pragma once

#include <SDL.h>

#include <string_view>

#include "game/Game.h"
#include "render/Font.h"

namespace snake::render {

struct Layout {
    int window_w = 800;
    int window_h = 800;
    int panel_h = 96;        // top bar height
    int padding = 12;
    int line_gap = 6;
};

class UIRenderer {
public:
    void SetFont(const Font* font);
    void Render(SDL_Renderer* r, const Layout& l, const snake::game::Game& game, double now_seconds);

private:
    const Font* font_ = nullptr;
    int DrawTextLine(SDL_Renderer* r, int x, int y, std::string_view s);
};

}  // namespace snake::render
