#include "render/UIRenderer.h"

#include <SDL.h>

#include <algorithm>
#include <sstream>
#include <string>

#include "game/Effects.h"
#include "game/Game.h"
#include "game/ScoreSystem.h"
#include "game/StateMachine.h"
#include "render/TextRenderer.h"

namespace snake::render {
namespace {

std::string FormatIsoDate(std::string_view iso) {
    if (iso.size() >= 16) {
        std::string out(iso.substr(0, 16));
        const auto pos = out.find('T');
        if (pos != std::string::npos) {
            out[pos] = ' ';
        }
        return out;
    }
    return std::string(iso);
}

int MeasureTextWidth(TextRenderer* renderer, std::string_view text, int size) {
    if (renderer == nullptr) {
        return static_cast<int>(text.size()) * 7;
    }
    return renderer->MeasureText(text, size).w;
}

int MeasureTextHeight(TextRenderer* renderer, int size) {
    if (renderer == nullptr) {
        return size;
    }
    return renderer->MeasureText("Ag", size).h;
}

}  // namespace

void UIRenderer::SetTextRenderer(TextRenderer* text_renderer) {
    text_renderer_ = text_renderer;
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
    if (text_renderer_ != nullptr) {
        return text_renderer_->DrawText(r, x, y, s, color, 16);
    }

    SDL_Rect rect{x, y, static_cast<int>(s.size()) * 7, 16};
    SDL_SetRenderDrawColor(r, 180, 180, 180, 255);
    SDL_RenderDrawRect(r, &rect);
    return rect.h;
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
    const int row_h = 24;
    const int gap = 8;
    const int title_h = row_h;
    const int footer_lines = 1 + (ui.pending_round_restart ? 1 : 0);
    const int footer_h = footer_lines * row_h + (footer_lines - 1) * gap;
    const int list_top = l.padding * 2 + title_h + gap;
    const int list_bottom = l.window_h - l.padding * 2 - footer_h - gap;
    const int list_area_h = std::max(0, list_bottom - list_top);
    const int item_h = row_h + gap;
    const int visible_rows = std::max(1, list_area_h / item_h);
    const int max_scroll = std::max(0, static_cast<int>(ui.option_items.size()) - visible_rows);
    const int center_row = visible_rows / 2;
    const int scroll_index =
        std::clamp(ui.options_index - center_row, 0, max_scroll);
    int y = l.padding * 2;

    DrawTextLine(r, start_x, y, "Options");
    y = list_top;

    const int end_index =
        std::min(static_cast<int>(ui.option_items.size()),
                 scroll_index + visible_rows);
    for (int i = scroll_index; i < end_index; ++i) {
        const auto& [label, value] = ui.option_items[i];
        const bool sel = i == ui.options_index;
        if (sel) {
            SDL_Rect hilite{start_x - 6, y - 2, l.window_w - start_x * 2, row_h + 4};
            SDL_SetRenderDrawColor(r, 40, 60, 80, 180);
            SDL_RenderFillRect(r, &hilite);
        }
        DrawTextLine(r, start_x, y, label);
        DrawTextLine(r, start_x + 260, y, value);
        y += item_h;
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

    SDL_Rect content_rect = l.play_rect;
    if (content_rect.w <= 0 || content_rect.h <= 0) {
        content_rect = SDL_Rect{0, 0, l.window_w, l.window_h};
    }

    const int font_size = 16;
    const int line_h = MeasureTextHeight(text_renderer_, font_size);
    const int row_h = line_h + 6;
    const int title_h = line_h + 8;
    const int header_h = line_h + 6;
    const int footer_h = line_h + 6;
    const int inner_pad = 16;
    const int section_gap = 10;
    const int margin = 24;

    const int avail_w = std::max(0, content_rect.w - margin * 2);
    const int avail_h = std::max(0, content_rect.h - margin * 2);
    int panel_w = std::min(avail_w, 600);
    if (panel_w < 360) {
        panel_w = avail_w;
    }
    panel_w = std::max(0, panel_w);

    int rows = ui.highscores != nullptr ? static_cast<int>(ui.highscores->size()) : 0;
    int base_h = inner_pad * 2 + title_h + header_h + footer_h + section_gap * 2;
    if (rows == 0) {
        rows = 1;
    }
    int max_rows = rows;
    int total_h = base_h + rows * row_h;
    if (avail_h > 0 && total_h > avail_h) {
        max_rows = std::max(0, (avail_h - base_h) / row_h);
        total_h = base_h + max_rows * row_h;
    }
    if (total_h <= 0) {
        total_h = base_h + row_h;
    }

    const int panel_h = total_h;
    const int panel_x = content_rect.x + std::max(0, (content_rect.w - panel_w) / 2);
    const int panel_y = content_rect.y + std::max(0, (content_rect.h - panel_h) / 2);
    SDL_Rect panel{panel_x, panel_y, panel_w, panel_h};

    SDL_SetRenderDrawColor(r, 18, 18, 26, 220);
    SDL_RenderFillRect(r, &panel);
    SDL_SetRenderDrawColor(r, 70, 80, 96, 200);
    SDL_RenderDrawRect(r, &panel);

    int cursor_x = panel.x + inner_pad;
    int cursor_y = panel.y + inner_pad;

    DrawTextLine(r, cursor_x, cursor_y, "Highscores");
    cursor_y += title_h + section_gap;

    const int table_x = cursor_x;
    const int table_w = panel.w - inner_pad * 2;
    const int gap = 12;

    int rank_w = std::max(24, MeasureTextWidth(text_renderer_, "#", font_size));
    int score_w = std::max(70, MeasureTextWidth(text_renderer_, "Score", font_size) + 6);
    int date_w = std::max(120, MeasureTextWidth(text_renderer_, "YYYY-MM-DD HH:MM", font_size));
    int name_w = table_w - rank_w - score_w - date_w - gap * 3;
    if (name_w < 120) {
        const int target = 120;
        int deficit = target - name_w;
        const int reduce_date = std::min(deficit, date_w - 100);
        date_w -= reduce_date;
        deficit -= reduce_date;
        const int reduce_score = std::min(deficit, score_w - 60);
        score_w -= reduce_score;
        name_w = table_w - rank_w - score_w - date_w - gap * 3;
    }
    name_w = std::max(0, name_w);

    const int x_rank = table_x;
    const int x_name = x_rank + rank_w + gap;
    const int x_score = x_name + name_w + gap;
    const int x_date = x_score + score_w + gap;

    auto draw_cell = [&](int x, int y, int width, std::string_view text, bool right_align) {
        int draw_x = x;
        if (right_align) {
            const int text_w = MeasureTextWidth(text_renderer_, text, font_size);
            draw_x = x + std::max(0, width - text_w);
        }
        DrawTextLine(r, draw_x, y, text);
    };

    SDL_Rect header_bg{table_x - 4, cursor_y - 4, table_w + 8, header_h + 6};
    SDL_SetRenderDrawColor(r, 30, 34, 48, 200);
    SDL_RenderFillRect(r, &header_bg);
    draw_cell(x_rank, cursor_y, rank_w, "#", true);
    draw_cell(x_name, cursor_y, name_w, "Name", false);
    draw_cell(x_score, cursor_y, score_w, "Score", true);
    draw_cell(x_date, cursor_y, date_w, "Date", false);
    cursor_y += header_h + section_gap;

    if (ui.highscores == nullptr || ui.highscores->empty() || max_rows == 0) {
        DrawTextLine(r, cursor_x, cursor_y, "No highscores yet.");
        cursor_y += row_h;
    } else {
        int rank = 1;
        const int rows_to_render = std::min(max_rows, static_cast<int>(ui.highscores->size()));
        for (int i = 0; i < rows_to_render; ++i) {
            const auto& e = ui.highscores->at(i);
            SDL_Rect row_rect{table_x - 4, cursor_y - 2, table_w + 8, row_h + 2};
            if (rank == 1) {
                SDL_SetRenderDrawColor(r, 50, 70, 100, 210);
                SDL_RenderFillRect(r, &row_rect);
            } else if (rank <= 3) {
                SDL_SetRenderDrawColor(r, 36, 48, 70, 190);
                SDL_RenderFillRect(r, &row_rect);
            } else if (i % 2 == 1) {
                SDL_SetRenderDrawColor(r, 24, 24, 34, 180);
                SDL_RenderFillRect(r, &row_rect);
            }

            draw_cell(x_rank, cursor_y, rank_w, std::to_string(rank), true);
            draw_cell(x_name, cursor_y, name_w, e.name, false);
            draw_cell(x_score, cursor_y, score_w, std::to_string(e.score), true);
            draw_cell(x_date, cursor_y, date_w, FormatIsoDate(e.achieved_at), false);
            cursor_y += row_h;
            ++rank;
        }
    }

    cursor_y = panel.y + panel.h - inner_pad - footer_h;
    DrawTextLine(r, cursor_x, cursor_y, "Enter: Select  |  Esc: Back");
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
