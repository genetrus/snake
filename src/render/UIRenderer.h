#pragma once

#include <SDL.h>

#include <string>
#include <string_view>
#include <vector>

#include "game/Game.h"
#include "game/StateMachine.h"
#include "io/Config.h"
#include "io/Highscores.h"
#include "render/Font.h"

namespace snake::render {

struct Layout {
    int window_w = 800;
    int window_h = 800;
    int panel_h = 96;  // top bar height (legacy; prefer panel_rect.h)
    SDL_Rect panel_rect{0, 0, 800, 96};
    bool panel_on_right = false;
    int padding = 12;
    int line_gap = 6;
};

struct UiFrameData {
    snake::game::Screen screen = snake::game::Screen::MainMenu;
    int menu_index = 0;
    int options_index = 0;
    bool rebinding = false;
    bool pending_round_restart = false;
    std::string ui_message;
    std::string lua_error;
    std::string game_over_reason;
    int final_score = 0;
    const snake::io::ConfigData* config = nullptr;
    const std::vector<snake::io::HighscoreEntry>* highscores = nullptr;
    std::vector<std::string> menu_items;
    std::vector<std::pair<std::string, std::string>> option_items;
};

class UIRenderer {
public:
    void SetFont(const Font* font);
    void Render(SDL_Renderer* r,
                const Layout& l,
                const snake::game::Game& game,
                double now_seconds,
                const UiFrameData& ui);

private:
    const Font* font_ = nullptr;
    int DrawTextLine(SDL_Renderer* r, int x, int y, std::string_view s);
    void RenderMenu(SDL_Renderer* r, const Layout& l, const UiFrameData& ui);
    void RenderOptions(SDL_Renderer* r, const Layout& l, const UiFrameData& ui);
    void RenderHighscores(SDL_Renderer* r, const Layout& l, const UiFrameData& ui);
    void RenderPaused(SDL_Renderer* r, const Layout& l);
    void RenderGameOver(SDL_Renderer* r, const Layout& l, const UiFrameData& ui);
};

}  // namespace snake::render
