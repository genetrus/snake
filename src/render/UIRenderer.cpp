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

void UIRenderer::Render(SDL_Renderer* r,
                        const Layout& l,
                        const snake::game::Game& game,
                        double /*now_seconds*/,
                        const UiFrameData& ui) {
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);

    switch (ui.screen) {
        case snake::game::Screen::MainMenu:
            RenderMenu(r, l, ui);
            break;
        case snake::game::Screen::Options:
            RenderOptions(r, l, ui);
            break;
        case snake::game::Screen::Highscores:
            RenderHighscores(r, l, ui);
            break;
        case snake::game::Screen::Paused:
            RenderPaused(r, l);
            break;
        case snake::game::Screen::GameOver:
            RenderGameOver(r, l, ui);
            break;
        case snake::game::Screen::NameEntry:
            RenderNameEntry(r, l, ui);
            break;
        case snake::game::Screen::Playing:
        default:
            break;
    }

    // gameplay panel (score/status)
    SDL_Rect panel_rect = l.panel_rect;
    SDL_SetRenderDrawColor(r, 24, 24, 32, 200);
    SDL_RenderFillRect(r, &panel_rect);

    int cursor_x = panel_rect.x + l.padding;
    int cursor_y = panel_rect.y + l.padding;

    std::ostringstream top_line;
    top_line << "State: ";
    switch (ui.screen) {
        case snake::game::Screen::MainMenu: top_line << "Menu"; break;
        case snake::game::Screen::Options: top_line << "Options"; break;
        case snake::game::Screen::Highscores: top_line << "Highscores"; break;
        case snake::game::Screen::Playing: top_line << "Playing"; break;
        case snake::game::Screen::Paused: top_line << "Paused"; break;
        case snake::game::Screen::GameOver: top_line << "Game Over"; break;
        case snake::game::Screen::NameEntry: top_line << "Name Entry"; break;
    }
    top_line << "   Score: " << game.GetScore().Score();

    const int top_h = DrawTextLine(r, cursor_x, cursor_y, top_line.str());
    cursor_y += top_h + l.line_gap;

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

    std::string hints = "Enter: Select  |  Esc: Back  |  P: Pause  |  R: Restart";
    DrawTextLine(r, cursor_x, cursor_y, hints);

    if (!ui.ui_message.empty()) {
        cursor_y += top_h + l.line_gap;
        DrawTextLine(r, cursor_x, cursor_y, ui.ui_message);
    }
    if (!ui.lua_error.empty()) {
        cursor_y += top_h + l.line_gap;
        DrawTextLine(r, cursor_x, cursor_y, "Lua: " + ui.lua_error);
    }
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
        SDL_Rect rect{x, y, static_cast<int>(s.size()) * 7, 16};
        SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
        SDL_RenderDrawRect(r, &rect);
        return rect.h;
    }
}

void UIRenderer::RenderMenu(SDL_Renderer* r, const Layout& l, const UiFrameData& ui) {
    SDL_Rect backdrop{0, 0, l.window_w, l.window_h};
    SDL_SetRenderDrawColor(r, 12, 12, 18, 220);
    SDL_RenderFillRect(r, &backdrop);

    const int item_w = 240;
    const int item_h = 32;
    const int gap = 12;
    const int total_h = static_cast<int>(ui.menu_items.size()) * (item_h + gap);
    int y = (l.window_h - total_h) / 2;
    int x = (l.window_w - item_w) / 2;

    for (std::size_t i = 0; i < ui.menu_items.size(); ++i) {
        SDL_Rect rect{x, y, item_w, item_h};
        const bool sel = static_cast<int>(i) == ui.menu_index;
        SDL_SetRenderDrawColor(r, sel ? 60 : 32, sel ? 80 : 32, sel ? 120 : 48, 230);
        SDL_RenderFillRect(r, &rect);
        DrawTextLine(r, x + 10, y + 6, ui.menu_items[i]);
        y += item_h + gap;
    }
}

void UIRenderer::RenderOptions(SDL_Renderer* r, const Layout& l, const UiFrameData& ui) {
    SDL_Rect backdrop{0, 0, l.window_w, l.window_h};
    SDL_SetRenderDrawColor(r, 8, 8, 12, 230);
    SDL_RenderFillRect(r, &backdrop);

    const int start_x = l.padding * 2;
    int y = l.padding * 2;
    const int row_h = 24;
    const int gap = 8;

    DrawTextLine(r, start_x, y, "Options");
    y += row_h + gap;

    for (std::size_t i = 0; i < ui.option_items.size(); ++i) {
        const auto& [label, value] = ui.option_items[i];
        const bool sel = static_cast<int>(i) == ui.options_index;
        if (sel) {
            SDL_Rect hilite{start_x - 6, y - 2, l.window_w - start_x * 2, row_h + 4};
            SDL_SetRenderDrawColor(r, 40, 60, 80, 180);
            SDL_RenderFillRect(r, &hilite);
        }
        DrawTextLine(r, start_x, y, label);
        DrawTextLine(r, start_x + 260, y, value);
        y += row_h + gap;
    }

    if (ui.rebinding) {
        std::ostringstream msg;
        msg << "Rebinding " << ui.rebind_action << " slot " << (ui.rebind_slot + 1) << " - press allowed key";
        DrawTextLine(r, start_x, y + gap, msg.str());
    } else {
        DrawTextLine(r, start_x, y + gap, "Up/Down: select  |  Left/Right: adjust  |  Confirm: toggle/edit  |  Esc/Menu: Back");
    }
    if (ui.pending_round_restart) {
        DrawTextLine(r, start_x, y + gap + row_h, "Applies on restart");
    }
}

void UIRenderer::RenderHighscores(SDL_Renderer* r, const Layout& l, const UiFrameData& ui) {
    SDL_Rect backdrop{0, 0, l.window_w, l.window_h};
    SDL_SetRenderDrawColor(r, 10, 10, 16, 230);
    SDL_RenderFillRect(r, &backdrop);

    const int start_x = l.padding * 2;
    int y = l.padding * 2;
    DrawTextLine(r, start_x, y, "Highscores");
    y += 30;

    if (ui.highscores != nullptr) {
        int rank = 1;
        for (const auto& e : *ui.highscores) {
            std::ostringstream line;
            line << rank << ") " << e.name << "  " << e.score << "  " << e.achieved_at;
            DrawTextLine(r, start_x, y, line.str());
            y += 24;
            ++rank;
        }
    } else {
        DrawTextLine(r, start_x, y, "No highscores yet.");
    }
}

void UIRenderer::RenderPaused(SDL_Renderer* r, const Layout& l) {
    SDL_Rect overlay{0, 0, l.window_w, l.window_h};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_RenderFillRect(r, &overlay);
    DrawTextLine(r, l.window_w / 2 - 40, l.window_h / 2 - 16, "PAUSED");
    DrawTextLine(r, l.window_w / 2 - 120, l.window_h / 2 + 8, "P: Resume   R: Restart   Esc: Menu");
}

void UIRenderer::RenderGameOver(SDL_Renderer* r, const Layout& l, const UiFrameData& ui) {
    SDL_Rect overlay{0, 0, l.window_w, l.window_h};
    SDL_SetRenderDrawColor(r, 0, 0, 0, 160);
    SDL_RenderFillRect(r, &overlay);
    DrawTextLine(r, l.window_w / 2 - 50, l.window_h / 2 - 30, "GAME OVER");
    DrawTextLine(r, l.window_w / 2 - 80, l.window_h / 2, "Reason: " + ui.game_over_reason);
    DrawTextLine(r, l.window_w / 2 - 60, l.window_h / 2 + 20, "Score: " + std::to_string(ui.final_score));
    DrawTextLine(r, l.window_w / 2 - 120, l.window_h / 2 + 44, "Enter/R: Restart   Esc: Menu");
}

void UIRenderer::RenderNameEntry(SDL_Renderer* r, const Layout& l, const UiFrameData& ui) {
    SDL_Rect backdrop{0, 0, l.window_w, l.window_h};
    SDL_SetRenderDrawColor(r, 6, 6, 10, 220);
    SDL_RenderFillRect(r, &backdrop);

    const int start_x = l.window_w / 2 - 160;
    int y = l.window_h / 2 - 80;
    DrawTextLine(r, start_x, y, "NEW HIGHSCORE!");
    y += 28;
    DrawTextLine(r, start_x, y, "Score: " + std::to_string(ui.final_score));
    y += 28;
    DrawTextLine(r, start_x, y, "Enter name (1-12):");
    y += 28;

    std::string display = ui.name_entry;
    if (display.size() < 12) {
        display.push_back('|');
    }
    SDL_Rect input_box{start_x - 6, y - 6, 320, 28};
    SDL_SetRenderDrawColor(r, 40, 60, 80, 200);
    SDL_RenderFillRect(r, &input_box);
    DrawTextLine(r, start_x, y - 2, display);
    y += 36;

    DrawTextLine(r, start_x, y, "Enter: Save   Esc: Cancel   Backspace: Delete");
}

}  // namespace snake::render
