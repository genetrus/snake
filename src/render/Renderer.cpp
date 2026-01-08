#include "render/Renderer.h"

#include <SDL.h>
#include <SDL_image.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdio>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

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

bool Renderer::LoadSprite(SDL_Renderer* r,
                          const std::filesystem::path& path,
                          SpriteTexture& sprite,
                          std::string_view label) {
    if (r == nullptr) {
        return false;
    }

    if (!std::filesystem::exists(path)) {
        SDL_Log("Missing sprite '%.*s' at %s", static_cast<int>(label.size()), label.data(), path.string().c_str());
        return false;
    }

    SDL_Surface* surface = IMG_Load(path.string().c_str());
    if (surface == nullptr) {
        SDL_Log("Failed to load sprite '%.*s' (%s): %s",
                static_cast<int>(label.size()),
                label.data(),
                path.string().c_str(),
                IMG_GetError());
        return false;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surface);
    if (tex == nullptr) {
        SDL_Log("Failed to create texture for '%.*s': %s", static_cast<int>(label.size()), label.data(), SDL_GetError());
        SDL_FreeSurface(surface);
        return false;
    }

    sprite.texture = tex;
    sprite.w = surface->w;
    sprite.h = surface->h;
    SDL_FreeSurface(surface);

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
#if SNAKE_HAS_SDL_SCALE_MODE
    SDL_SetTextureScaleMode(tex, SDL_ScaleModeNearest);
#else
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
#endif

    return true;
}

void Renderer::ResetSprites() {
    auto destroy = [](SpriteTexture& sprite) {
        if (sprite.texture != nullptr) {
            SDL_DestroyTexture(sprite.texture);
            sprite.texture = nullptr;
        }
        sprite.w = 0;
        sprite.h = 0;
    };

    destroy(sprites_.snake_head);
    destroy(sprites_.snake_body);
    destroy(sprites_.food);
    destroy(sprites_.bonus_score);
    destroy(sprites_.bonus_slow);
}

bool Renderer::Init(SDL_Renderer* r) {
    bool ok = true;
    sprite_error_text_.clear();
    ResetSprites();

    std::vector<std::string> missing;
    auto load_sprite = [&](SpriteTexture& sprite, std::string_view label, std::string_view rel_path) {
        const auto path = snake::io::AssetsPath(std::string(rel_path));
        if (!LoadSprite(r, path, sprite, label)) {
            missing.emplace_back(label);
        }
    };

    load_sprite(sprites_.snake_head, "snake_head", "sprites/snake_head.png");
    load_sprite(sprites_.snake_body, "snake_body", "sprites/snake_body.png");
    load_sprite(sprites_.food, "food", "sprites/food.png");
    load_sprite(sprites_.bonus_score, "bonus_score", "sprites/bonus_score.png");
    load_sprite(sprites_.bonus_slow, "bonus_slow", "sprites/bonus_slow.png");

    if (!missing.empty()) {
        sprite_error_text_ = "Missing sprite";
        if (missing.size() > 1) {
            sprite_error_text_.append("s");
        }
        sprite_error_text_.append(": ");
        for (std::size_t i = 0; i < missing.size(); ++i) {
            if (i > 0) {
                sprite_error_text_.append(", ");
            }
            sprite_error_text_.append(missing[i]);
        }
    }

    std::vector<std::filesystem::path> font_paths;
    font_paths.push_back(snake::io::AssetsPath("fonts/Roboto-Regular.ttf"));
#if defined(_WIN32)
    font_paths.emplace_back("C:/Windows/Fonts/segoeui.ttf");
    font_paths.emplace_back("C:/Windows/Fonts/arial.ttf");
#elif defined(__APPLE__)
    font_paths.emplace_back("/Library/Fonts/Arial.ttf");
#else
    font_paths.emplace_back("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf");
    font_paths.emplace_back("/usr/share/fonts/truetype/freefont/FreeSans.ttf");
#endif

    text_renderer_.Init(font_paths, 16);
    ui_.SetTextRenderer(&text_renderer_);
    effects_.Init();
    return ok;
}

void Renderer::Shutdown() {
    DestroyFramebuffer();
    text_renderer_.Reset();
    ResetSprites();
    effects_.Reset();
    last_render_seconds_ = 0.0;
    sprite_error_text_.clear();
    ui_.SetTextRenderer(nullptr);
}

void Renderer::ResetEffects() {
    effects_.Reset();
}

void Renderer::SpawnFoodEat(snake::game::Pos pos, int score_delta) {
    effects_.SpawnFoodEat(pos);

    char text[16];
    std::snprintf(text, sizeof(text), "+%d", score_delta);
    effects_.SpawnFloatingText(pos, text, SDL_Color{240, 220, 140, 255});
    effects_.StartHeadFlash(0.12);
}

void Renderer::SpawnBonusPickup(snake::game::Pos pos, std::string_view bonus_type, int score_delta) {
    SDL_Color pulse_color{120, 170, 255, 42};
    SDL_Color text_color{180, 220, 255, 255};

    if (bonus_type == "bonus_score") {
        pulse_color = SDL_Color{255, 220, 140, 42};
        text_color = SDL_Color{255, 230, 150, 255};
    }

    effects_.SpawnBonusPulse(pulse_color);

    if (score_delta > 0) {
        char text[16];
        std::snprintf(text, sizeof(text), "+%d", score_delta);
        effects_.SpawnFloatingText(pos, text, text_color);
    } else if (bonus_type == "bonus_slow") {
        effects_.SpawnFloatingText(pos, "SLOW", text_color);
    }
}

void Renderer::RenderFrame(SDL_Renderer* r,
                           int window_w,
                           int window_h,
                           const RenderSettings& rs,
                           const snake::game::Game& game,
                           double now_seconds,
                           const std::string& overlay_error_text,
                           bool show_text_debug,
                           bool show_audio_debug,
                           const std::vector<std::string>& audio_debug_lines,
                           const snake::render::UiFrameData& ui_frame) {
    if (r == nullptr) {
        return;
    }

    double dt_seconds = 0.0;
    if (last_render_seconds_ > 0.0) {
        dt_seconds = now_seconds - last_render_seconds_;
    }
    last_render_seconds_ = now_seconds;
    effects_.Update(dt_seconds);

    std::string combined_error_text = overlay_error_text;
    if (!sprite_error_text_.empty()) {
        if (!combined_error_text.empty()) {
            combined_error_text.append(" | ");
        }
        combined_error_text.append(sprite_error_text_);
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
        ui_frame.screen == snake::game::Screen::Highscores ||
        ui_frame.screen == snake::game::Screen::NameEntry) {
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

    if (game.GetSpawner().HasFood()) {
        const snake::game::Pos food_pos = game.GetSpawner().FoodPos();
        const double food_scale = food_pulse_.Eval(now_seconds);
        const int food_size = static_cast<int>(tile_px * food_scale);
        SDL_Rect food_dst = TileRect(origin, tile_px, food_pos, food_size);
        if (sprites_.food.texture != nullptr) {
            SDL_Rect dst = food_dst;
            dst.w = tile_px;
            dst.h = tile_px;
            SDL_RenderCopy(r, sprites_.food.texture, nullptr, &dst);
        } else {
            RenderFallbackRect(r, food_dst, SDL_Color{200, 80, 80, 255});
        }
    }

    for (const auto& bonus : game.GetSpawner().Bonuses()) {
        SDL_Rect dst = TileRect(origin, tile_px, bonus.pos);
        SDL_Texture* bonus_tex = nullptr;
        switch (bonus.type) {
            case snake::game::BonusType::Score:
                bonus_tex = sprites_.bonus_score.texture;
                break;
            case snake::game::BonusType::Slow:
                bonus_tex = sprites_.bonus_slow.texture;
                break;
        }

        if (bonus_tex != nullptr) {
            SDL_RenderCopy(r, bonus_tex, nullptr, &dst);
        } else {
            RenderFallbackRect(r, dst, SDL_Color{80, 200, 120, 255});
        }
    }

    effects_.RenderFoodEats(r, sprites_.food.texture, origin, tile_px);

    const auto& snake = game.GetSnake();
    const auto& body = snake.Body();
    const double head_flash = effects_.HeadFlashStrength();
    for (std::size_t i = 0; i < body.size(); ++i) {
        const bool is_head = i == 0;
        SDL_Rect dst = TileRect(origin, tile_px, body[i]);
        SDL_Texture* tex = is_head ? sprites_.snake_head.texture : sprites_.snake_body.texture;
        if (tex != nullptr) {
            SDL_RenderCopy(r, tex, nullptr, &dst);
        } else {
            const SDL_Color color = is_head ? SDL_Color{240, 240, 120, 255} : SDL_Color{120, 200, 120, 255};
            RenderFallbackRect(r, dst, color);
        }
        if (is_head && head_flash > 0.0) {
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_ADD);
            SDL_SetRenderDrawColor(r, 255, 255, 255, static_cast<Uint8>(std::round(60.0 * head_flash)));
            SDL_RenderFillRect(r, &dst);
            SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        }
    }

    effects_.RenderFloatingText(r, text_renderer_, origin, tile_px);
    SDL_Rect viewport_rect{0, 0, virtual_w, virtual_h};
    effects_.RenderPulse(r, viewport_rect);

    Layout layout{};
    layout.window_w = virtual_w;
    layout.window_h = virtual_h;
    layout.panel_h = panel_rect.h;
    layout.panel_rect = panel_rect;
    layout.play_rect = play_rect;
    layout.panel_on_right = place_right;

    ui_.Render(r, layout, game, now_seconds, ui_frame);

    if (!combined_error_text.empty()) {
        const int padding = 8;
        SDL_Color text_color{255, 200, 200, 255};
        const auto metrics = text_renderer_.MeasureText(combined_error_text, 16);
        int text_w = metrics.w;
        int text_h = metrics.h;

        const int bg_w = text_w + padding * 2;
        const int bg_h = text_h + padding * 2;
        SDL_Rect bg{play_rect.x + padding, std::max(0, virtual_h - bg_h - padding), bg_w, bg_h};

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, 10, 10, 10, 190);
        SDL_RenderFillRect(r, &bg);

        text_renderer_.DrawText(r, bg.x + padding, bg.y + padding, combined_error_text, text_color, 16);

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    if (show_text_debug) {
        const int padding = 8;
        const int line_gap = 4;
        const SDL_Color text_color{220, 220, 220, 255};
        const SDL_Color bg_color{12, 12, 18, 220};

        const std::string ttf_status = text_renderer_.IsTtfReady() ? "TTF: OK" : "TTF: FAIL";
        const std::string font_path =
            text_renderer_.FontPath().empty() ? "n/a" : text_renderer_.FontPath().string();
        const std::string font_status = text_renderer_.IsFontLoaded()
                                            ? "Font: OK (" + font_path + ")"
                                            : "Font: FAIL (" + font_path + ")";
        const std::string last_error =
            text_renderer_.LastError().empty() ? "Last error: None" : "Last error: " + text_renderer_.LastError();

        const std::array<std::string, 3> lines = {ttf_status, font_status, last_error};
        int max_w = 0;
        int line_h = 0;
        for (const auto& line : lines) {
            const auto metrics = text_renderer_.MeasureText(line, 14, true);
            max_w = std::max(max_w, metrics.w);
            line_h = std::max(line_h, metrics.h);
        }

        const int total_h = static_cast<int>(lines.size()) * line_h + (static_cast<int>(lines.size()) - 1) * line_gap;
        SDL_Rect bg{padding, padding, max_w + padding * 2, total_h + padding * 2};
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(r, &bg);

        int cursor_y = bg.y + padding;
        for (const auto& line : lines) {
            text_renderer_.DrawText(r, bg.x + padding, cursor_y, line, text_color, 14, true);
            cursor_y += line_h + line_gap;
        }

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
    }

    if (show_audio_debug && !audio_debug_lines.empty()) {
        const int padding = 8;
        const int line_gap = 4;
        const SDL_Color text_color{220, 220, 220, 255};
        const SDL_Color bg_color{12, 12, 18, 220};

        int max_w = 0;
        int line_h = 0;
        for (const auto& line : audio_debug_lines) {
            const auto metrics = text_renderer_.MeasureText(line, 14, true);
            max_w = std::max(max_w, metrics.w);
            line_h = std::max(line_h, metrics.h);
        }

        const int total_h = static_cast<int>(audio_debug_lines.size()) * line_h +
                            (static_cast<int>(audio_debug_lines.size()) - 1) * line_gap;
        SDL_Rect bg{padding, padding * 2 + 120, max_w + padding * 2, total_h + padding * 2};
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        SDL_RenderFillRect(r, &bg);

        int cursor_y = bg.y + padding;
        for (const auto& line : audio_debug_lines) {
            text_renderer_.DrawText(r, bg.x + padding, cursor_y, line, text_color, 14, true);
            cursor_y += line_h + line_gap;
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
