#include "render/Renderer.h"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <string>

#if SDL_VERSION_ATLEAST(2, 0, 12)
#define SNAKE_HAS_SDL_SCALE_MODE 1
#else
#define SNAKE_HAS_SDL_SCALE_MODE 0
#endif

#include "game/Board.h"
#include "game/Game.h"
#include "game/Spawner.h"
#include "io/Paths.h"

namespace snake::render {
namespace {
constexpr int kFallbackTilePx = 32;
constexpr int kPanelH = 96;
constexpr int kPanelW = 280;

struct Viewport {
    SDL_Rect dst{0, 0, 0, 0};
    double scale = 1.0;
};

Viewport ComputeLetterboxViewport(int win_w, int win_h, int virtual_w, int virtual_h) {
    if (virtual_w <= 0 || virtual_h <= 0 || win_w <= 0 || win_h <= 0) {
        return {};
    }

    const double scale = std::min(static_cast<double>(win_w) / static_cast<double>(virtual_w),
                                  static_cast<double>(win_h) / static_cast<double>(virtual_h));
    const int dst_w = static_cast<int>(std::floor(static_cast<double>(virtual_w) * scale));
    const int dst_h = static_cast<int>(std::floor(static_cast<double>(virtual_h) * scale));
    const int dst_x = (win_w - dst_w) / 2;
    const int dst_y = (win_h - dst_h) / 2;

    Viewport vp;
    vp.dst = SDL_Rect{dst_x, dst_y, dst_w, dst_h};
    vp.scale = scale;
    return vp;
}

SDL_Rect TileRect(SDL_Point origin, int tile_px, snake::game::Pos p, int size_override = -1) {
    const int size = (size_override > 0) ? size_override : tile_px;
    const int offset = (tile_px - size) / 2;
    const int x = origin.x + p.x * tile_px + offset;
    const int y = origin.y + p.y * tile_px + offset;
    return SDL_Rect{x, y, size, size};
}

std::string NormalizePanelMode(std::string mode) {
    std::transform(mode.begin(), mode.end(), mode.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (mode != "top" && mode != "right") {
        mode = "auto";
    }
    return mode;
}

void RenderFallbackRect(SDL_Renderer* r, const SDL_Rect& rect, SDL_Color color) {
    SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(r, &rect);
}

SDL_Rect ClampRectToNonNegative(const SDL_Rect& rect) {
    SDL_Rect clamped = rect;
    if (clamped.w < 0) clamped.w = 0;
    if (clamped.h < 0) clamped.h = 0;
    return clamped;
}

}  // namespace

bool Renderer::EnsureFramebuffer(SDL_Renderer* r, int virtual_w, int virtual_h) {
    if (r == nullptr || virtual_w <= 0 || virtual_h <= 0) {
        return false;
    }

    if (framebuffer_ != nullptr && fb_w_ == virtual_w && fb_h_ == virtual_h) {
        return true;
    }

    DestroyFramebuffer();

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");  // nearest as a fallback for older SDL

    SDL_Texture* tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, virtual_w, virtual_h);
    if (tex == nullptr) {
        SDL_Log("Failed to create framebuffer: %s", SDL_GetError());
        return false;
    }

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

#if SNAKE_HAS_SDL_SCALE_MODE
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
#endif

    framebuffer_ = tex;
    fb_w_ = virtual_w;
    fb_h_ = virtual_h;
    return true;
}

void Renderer::DestroyFramebuffer() {
    if (framebuffer_ != nullptr) {
        SDL_DestroyTexture(framebuffer_);
        framebuffer_ = nullptr;
    }
    fb_w_ = 0;
    fb_h_ = 0;
}

bool Renderer::Init(SDL_Renderer* r) {
    bool ok = true;

    const auto atlas_path = snake::io::AssetsPath("sprites/atlas.png");
    if (std::filesystem::exists(atlas_path)) {
        ok = atlas_.Load(r, atlas_path) && ok;
    }

    const auto font_path = snake::io::AssetsPath("fonts/Roboto-Regular.ttf");
    if (std::filesystem::exists(font_path)) {
        ok = font_.Load(font_path, 16) && ok;
    }

    ui_.SetFont(&font_);
    return ok;
}

void Renderer::Shutdown() {
    DestroyFramebuffer();
    font_.Reset();
    atlas_.SetTexture(nullptr);
    ui_.SetFont(nullptr);
}

void Renderer::RenderFrame(SDL_Renderer* r,
                           int window_w,
                           int window_h,
                           const RenderSettings& rs,
                           const snake::game::Game& game,
                           double now_seconds,
                           const std::string& overlay_error_text,
                           const snake::render::UiFrameData& ui_frame) {
    if (r == nullptr) {
        return;
    }

    const std::string mode = NormalizePanelMode(rs.panel_mode);
    bool place_right = (mode == "right");
    const int tile_px = rs.tile_px > 0 ? rs.tile_px : kFallbackTilePx;
    const int board_w = game.GetBoard().W();
    const int board_h = game.GetBoard().H();

    const int board_px_w = board_w * tile_px;
    const int board_px_h = board_h * tile_px;
    const int panel_px_h = kPanelH;
    const int panel_px_w = kPanelW;

    if (mode == "auto") {
        place_right = window_w >= window_h;
    }

    if (!place_right && window_w >= window_h + 200) {
        place_right = true;  // legacy wide preference
    }

    const int virtual_w = place_right ? board_px_w + panel_px_w : board_px_w;
    const int virtual_h = place_right ? std::max(board_px_h, panel_px_h) : board_px_h + panel_px_h;

    const bool have_fb = EnsureFramebuffer(r, virtual_w, virtual_h);

    SDL_Texture* target = have_fb ? framebuffer_ : nullptr;
    SDL_SetRenderTarget(r, target);

    SDL_SetRenderDrawColor(r, 4, 4, 6, 255);
    SDL_RenderClear(r);

    SDL_Rect panel_rect{};
    SDL_Rect play_rect{};
    if (place_right) {
        panel_rect = SDL_Rect{board_px_w, 0, panel_px_w, std::max(panel_px_h, board_px_h)};
        play_rect = SDL_Rect{0, 0, board_px_w, board_px_h};
    } else {
        panel_rect = SDL_Rect{0, 0, board_px_w, panel_px_h};
        play_rect = SDL_Rect{0, panel_px_h, board_px_w, board_px_h};
    }

    panel_rect = ClampRectToNonNegative(panel_rect);
    play_rect = ClampRectToNonNegative(play_rect);

    SDL_Point origin{play_rect.x, play_rect.y};

    SDL_Rect board_rect{origin.x, origin.y, board_w * tile_px, board_h * tile_px};
    if (ui_frame.screen == snake::game::Screen::MainMenu ||
        ui_frame.screen == snake::game::Screen::Options ||
        ui_frame.screen == snake::game::Screen::Highscores) {
        RenderFallbackRect(r, SDL_Rect{0, 0, virtual_w, virtual_h}, SDL_Color{8, 8, 12, 255});
    } else {
        RenderFallbackRect(r, board_rect, SDL_Color{32, 32, 42, 255});
    }

    SDL_SetRenderDrawColor(r, 48, 48, 58, 255);
    for (int x = 0; x <= board_w; ++x) {
        const int px = origin.x + x * tile_px;
        SDL_RenderDrawLine(r, px, origin.y, px, origin.y + board_h * tile_px);
    }
    for (int y = 0; y <= board_h; ++y) {
        const int py = origin.y + y * tile_px;
        SDL_RenderDrawLine(r, origin.x, py, origin.x + board_w * tile_px, py);
    }

    SDL_Texture* atlas_tex = atlas_.Texture();

    if (game.GetSpawner().HasFood()) {
        const snake::game::Pos food_pos = game.GetSpawner().FoodPos();
        const double food_scale = food_pulse_.Eval(now_seconds);
        const int food_size = static_cast<int>(tile_px * food_scale);
        SDL_Rect food_dst = TileRect(origin, tile_px, food_pos, food_size);
        if (atlas_tex != nullptr) {
            if (const SDL_Rect* src = atlas_.Get("food")) {
                SDL_Rect dst = food_dst;
                dst.w = tile_px;
                dst.h = tile_px;
                SDL_RenderCopy(r, atlas_tex, src, &dst);
            }
        }
        if (atlas_tex == nullptr || atlas_.Get("food") == nullptr) {
            RenderFallbackRect(r, food_dst, SDL_Color{200, 80, 80, 255});
        }
    }

    for (const auto& bonus : game.GetSpawner().Bonuses()) {
        SDL_Rect dst = TileRect(origin, tile_px, bonus.pos);
        const SDL_Rect* src = nullptr;
        if (atlas_tex != nullptr) {
            switch (bonus.type) {
                case snake::game::BonusType::Score:
                    src = atlas_.Get("bonus_score");
                    break;
                case snake::game::BonusType::Slow:
                    src = atlas_.Get("bonus_slow");
                    break;
            }
        }

        if (atlas_tex != nullptr && src != nullptr) {
            SDL_RenderCopy(r, atlas_tex, src, &dst);
        } else {
            RenderFallbackRect(r, dst, SDL_Color{80, 200, 120, 255});
        }
    }

    const auto& snake = game.GetSnake();
    const auto& body = snake.Body();
    for (std::size_t i = 0; i < body.size(); ++i) {
        const bool is_head = i == 0;
        SDL_Rect dst = TileRect(origin, tile_px, body[i]);

        const SDL_Rect* src = nullptr;
        if (atlas_tex != nullptr) {
            if (is_head) {
                switch (snake.Direction()) {
                    case snake::game::Dir::Up:
                        src = atlas_.Get("snake_head_up");
                        break;
                    case snake::game::Dir::Down:
                        src = atlas_.Get("snake_head_down");
                        break;
                    case snake::game::Dir::Left:
                        src = atlas_.Get("snake_head_left");
                        break;
                    case snake::game::Dir::Right:
                        src = atlas_.Get("snake_head_right");
                        break;
                }
            } else {
                src = atlas_.Get("snake_body");
            }
        }

        if (atlas_tex != nullptr && src != nullptr) {
            SDL_RenderCopy(r, atlas_tex, src, &dst);
        } else {
            const SDL_Color color = is_head ? SDL_Color{240, 240, 120, 255} : SDL_Color{120, 200, 120, 255};
            RenderFallbackRect(r, dst, color);
        }
    }

    Layout layout{};
    layout.window_w = virtual_w;
    layout.window_h = virtual_h;
    layout.panel_h = panel_rect.h;
    layout.panel_rect = panel_rect;
    layout.panel_on_right = place_right;

    ui_.Render(r, layout, game, now_seconds, ui_frame);

    if (!overlay_error_text.empty()) {
        const int padding = 8;
        SDL_Color text_color{255, 200, 200, 255};
        int text_w = 0;
        int text_h = 0;
        SDL_Texture* tex = nullptr;
        if (font_.IsLoaded()) {
            tex = font_.RenderText(r, overlay_error_text, text_color, &text_w, &text_h);
        } else {
            text_w = static_cast<int>(overlay_error_text.size()) * 7;
            text_h = 16;
        }

        const int bg_w = text_w + padding * 2;
        const int bg_h = text_h + padding * 2;
        SDL_Rect bg{play_rect.x + padding, std::max(0, virtual_h - bg_h - padding), bg_w, bg_h};

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 10, 10, 10, 190);
        SDL_RenderFillRect(r, &bg);

        if (tex != nullptr) {
            SDL_Rect dst{bg.x + padding, bg.y + padding, text_w, text_h};
            SDL_RenderCopy(r, tex, nullptr, &dst);
            SDL_DestroyTexture(tex);
        } else {
            SDL_Rect dst{bg.x + padding, bg.y + padding, text_w, text_h};
            SDL_SetRenderDrawColor(r, text_color.r, text_color.g, text_color.b, text_color.a);
            SDL_RenderDrawRect(r, &dst);
        }

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    SDL_SetRenderTarget(r, nullptr);

    if (have_fb) {
        SDL_SetRenderDrawColor(r, 0, 0, 0, 255);
        SDL_RenderClear(r);

        const Viewport vp = ComputeLetterboxViewport(window_w, window_h, virtual_w, virtual_h);
        SDL_RenderCopy(r, framebuffer_, nullptr, &vp.dst);
    }

    SDL_RenderPresent(r);
}

}  // namespace snake::render
