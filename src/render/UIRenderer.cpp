#include "render/UIRenderer.h"

#include <SDL.h>

#include <sstream>
#include <string>

#include "game/Effects.h"
#include "game/Game.h"
#include "game/ScoreSystem.h"
#include "game/StateMachine.h"
#include "render/Font.h"

namespace snake::render {

void UIRenderer::SetFont(const Font* font) {
    font_ = font;
}

void UIRenderer::Render(SDL_Renderer* r, const Layout& l, const snake::game::Game& game, double /*now_seconds*/) {
    SDL_Rect panel_rect = l.panel_rect;
    SDL_SetRenderDrawColor(r, 24, 24, 32, 255);
    SDL_RenderFillRect(r, &panel_rect);

    int cursor_x = panel_rect.x + l.padding;
    int cursor_y = panel_rect.y + l.padding;

    // State + Score
    std::ostringstream top_line;
    top_line << "State: ";
    switch (game.State()) {
        case snake::game::State::Menu:
            top_line << "Menu";
            break;
        case snake::game::State::Playing:
            top_line << "Playing";
            break;
        case snake::game::State::Paused:
            top_line << "Paused";
            break;
        case snake::game::State::GameOver:
            top_line << "Game Over";
            break;
        default:
            top_line << "Unknown";
            break;
    }
    top_line << "   Score: " << game.GetScore().Score();

    const int top_h = DrawTextLine(r, cursor_x, cursor_y, top_line.str());
    cursor_y += top_h + l.line_gap;

    // Effects line
    std::ostringstream effects_line;
    const auto& effects = game.GetEffects();
    effects_line << "Slow: ";
    if (effects.SlowActive()) {
        effects_line.setf(std::ios::fixed);
        effects_line.precision(1);
        effects_line << effects.SlowRemaining() << "s remaining";
    } else {
        effects_line << "inactive";
    }

    const int effects_h = DrawTextLine(r, cursor_x, cursor_y, effects_line.str());
    cursor_y += effects_h + l.line_gap;

    // Hints
    std::string hints = "Enter: Start  |  P: Pause  |  R: Restart  |  Esc: Menu/Quit";
    DrawTextLine(r, cursor_x, cursor_y, hints);
}

int UIRenderer::DrawTextLine(SDL_Renderer* r, int x, int y, std::string_view s) {
    const SDL_Color color{230, 230, 230, 255};
    int w = 0;
    int h = 0;
    SDL_Texture* tex = nullptr;

    if (font_ != nullptr && font_->IsLoaded()) {
        tex = font_->RenderText(r, s, color, &w, &h);
    }

    if (tex != nullptr) {
        SDL_Rect dst{x, y, w, h};
        SDL_RenderCopy(r, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
        return dst.h;
    } else {
        // Fallback: draw a placeholder rectangle
        SDL_Rect rect{x, y, static_cast<int>(s.size()) * 7, 16};
        SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
        SDL_RenderDrawRect(r, &rect);
        return rect.h;
    }
}

}  // namespace snake::render
