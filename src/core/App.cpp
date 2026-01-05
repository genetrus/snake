#include "core/App.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string_view>

#include "io/Paths.h"
#include "render/Animation.h"
#include "lua/Bindings.h"

namespace snake::core {

App::App() {
    try {
        InitSDL();
        sdl_initialized_ = true;
        CreateWindowAndRenderer();
        ui_.SetFont(&ui_font_);
        lua_ctx_.game = &game_;
        lua_ctx_.audio = &audio_;

        audio_.Init();
        sfx_.SetAudioSystem(&audio_);
        sfx_.LoadAll("./assets/sounds");

        const auto font_path = snake::io::AssetsPath("fonts/placeholder.ttf");
        if (std::filesystem::exists(font_path)) {
            if (!ui_font_.Load(font_path, 20)) {
                SDL_Log("Failed to load font: %s", font_path.string().c_str());
            }
        } else {
            SDL_Log("Font not found: %s", font_path.string().c_str());
        }

        const auto atlas_path = snake::io::AssetsPath("sprites/atlas.png");
        if (std::filesystem::exists(atlas_path)) {
            if (!atlas_.Load(renderer_, atlas_path)) {
                SDL_Log("Failed to load sprite atlas: %s", atlas_path.string().c_str());
            }
        } else {
            SDL_Log("Atlas not found: %s", atlas_path.string().c_str());
        }

        time_.Init();
        game_.ResetAll();

        lua_.Init();
        snake::lua::Bindings::Register(lua_.L());

        const auto rules_path = snake::io::AssetsPath("scripts/rules.lua");
        const auto config_path = snake::io::UserPath("config.lua");
        if (!lua_.LoadRules(rules_path)) {
            SDL_Log("Lua load rules failed");
        }
        if (!lua_.LoadConfig(config_path)) {
            SDL_Log("Lua load config failed");
        }
        lua_.CallWithCtx("on_app_init", &lua_ctx_);
    } catch (...) {
        ShutdownSDL();
        throw;
    }
}

App::~App() {
    if (sdl_initialized_) {
        ShutdownSDL();
    }
}

int App::Run() {
    try {
        auto prev_state = game_.State();
        int prev_score = game_.GetScore().Score();

        bool running = true;
        while (running) {
            input_.BeginFrame();

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                input_.ProcessEvent(event);
            }

            game_.HandleInput(input_);

            if (input_.KeyPressed(SDLK_F5)) {
                const auto rules_path = snake::io::AssetsPath("scripts/rules.lua");
                const auto config_path = snake::io::UserPath("config.lua");
                if (lua_.HotReload(rules_path, config_path)) {
                    snake::lua::Bindings::Register(lua_.L());
                    lua_.CallWithCtx("on_app_init", &lua_ctx_);
                    lua_error_.clear();
                } else if (lua_.LastError()) {
                    lua_error_ = lua_.LastError()->message;
                    SDL_Log("Lua reload failed: %s", lua_error_.c_str());
                }
            }

            if (game_.State() == snake::game::State::Menu && input_.KeyPressed(SDLK_ESCAPE)) {
                input_.RequestQuit();
            }

            time_.UpdateFrame();
            while (time_.ConsumeTick()) {
                UpdateTick();
                if (input_.QuitRequested()) {
                    running = false;
                    break;
                }
            }

            if (!running) {
                break;
            }

            const auto current_state = game_.State();
            const int current_score = game_.GetScore().Score();

            if (current_score > prev_score) {
                sfx_.Play(audio::SfxId::Eat);
            }

            if (current_state != prev_state) {
                if (prev_state == snake::game::State::Menu && current_state == snake::game::State::Playing) {
                    sfx_.Play(audio::SfxId::MenuClick);
                    lua_.CallWithCtx("on_round_start", &lua_ctx_);
                }
                if ((prev_state == snake::game::State::Playing || prev_state == snake::game::State::Paused) &&
                    current_state == snake::game::State::GameOver) {
                    sfx_.Play(audio::SfxId::GameOver);
                }
                if (prev_state == snake::game::State::Playing && current_state == snake::game::State::Paused) {
                    sfx_.Play(audio::SfxId::PauseOn);
                }
                if (prev_state == snake::game::State::Paused && current_state == snake::game::State::Playing) {
                    sfx_.Play(audio::SfxId::PauseOff);
                }
                if (prev_state == snake::game::State::GameOver && current_state == snake::game::State::Playing) {
                    lua_.CallWithCtx("on_round_reset", &lua_ctx_);
                }
            }

            prev_state = current_state;
            prev_score = current_score;

            RenderFrame();
            SDL_RenderPresent(renderer_);

            if (input_.QuitRequested()) {
                break;
            }
        }
    } catch (const std::exception&) {
        return 1;
    }

    return 0;
}

void App::InitSDL() {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        SDL_Log("SDL_Init (video|audio) failed: %s", SDL_GetError());
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            throw std::runtime_error(SDL_GetError());
        }
    }

    const int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
    }

    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
    }
}

void App::CreateWindowAndRenderer() {
    window_ = SDL_CreateWindow(
        "Snake",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        800,
        SDL_WINDOW_SHOWN);

    if (window_ == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (renderer_ == nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        throw std::runtime_error(SDL_GetError());
    }
}

void App::ShutdownSDL() {
    sfx_.Reset();
    audio_.Shutdown();
    atlas_.SetTexture(nullptr);
    ui_font_.Reset();

    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (sdl_initialized_) {
        TTF_Quit();
        IMG_Quit();
        SDL_Quit();
        sdl_initialized_ = false;
    }
}

void App::UpdateTick() {
    game_.Tick(time_.TickDt());
}

void App::RenderFrame() {
    if (renderer_ == nullptr) {
        return;
    }

    SDL_GetRendererOutputSize(renderer_, &window_w_, &window_h_);

    render::Layout layout;
    layout.window_w = window_w_;
    layout.window_h = window_h_;

    SDL_SetRenderDrawColor(renderer_, 10, 10, 12, 255);
    SDL_RenderClear(renderer_);

    SDL_Rect play_rect{0, layout.panel_h, window_w_, std::max(0, window_h_ - layout.panel_h)};
    SDL_SetRenderDrawColor(renderer_, 12, 16, 20, 255);
    SDL_RenderFillRect(renderer_, &play_rect);

    const auto& board = game_.GetBoard();
    const int tile_size = 32;
    const int board_px_w = board.W() * tile_size;
    const int board_px_h = board.H() * tile_size;

    int origin_x = (window_w_ - board_px_w) / 2;
    if (origin_x < 0) origin_x = 0;
    int origin_y = layout.panel_h + (play_rect.h - board_px_h) / 2;
    if (origin_y < layout.panel_h) origin_y = layout.panel_h;

    SDL_Rect board_rect{origin_x, origin_y, board_px_w, board_px_h};
    SDL_SetRenderDrawColor(renderer_, 30, 34, 38, 255);
    SDL_RenderFillRect(renderer_, &board_rect);
    SDL_SetRenderDrawColor(renderer_, 60, 64, 68, 255);
    SDL_RenderDrawRect(renderer_, &board_rect);

    SDL_SetRenderDrawColor(renderer_, 50, 54, 58, 120);
    for (int x = 1; x < board.W(); ++x) {
        const int px = origin_x + x * tile_size;
        SDL_RenderDrawLine(renderer_, px, origin_y, px, origin_y + board_px_h);
    }
    for (int y = 1; y < board.H(); ++y) {
        const int py = origin_y + y * tile_size;
        SDL_RenderDrawLine(renderer_, origin_x, py, origin_x + board_px_w, py);
    }

    const double now = time_.Now();

    auto draw_sprite = [&](SDL_Rect dest, std::string_view name, SDL_Color fallback) {
        if (atlas_.Texture() != nullptr) {
            if (const SDL_Rect* src = atlas_.Get(name)) {
                SDL_RenderCopy(renderer_, atlas_.Texture(), src, &dest);
                return;
            }
        }
        SDL_SetRenderDrawColor(renderer_, fallback.r, fallback.g, fallback.b, fallback.a);
        SDL_RenderFillRect(renderer_, &dest);
    };

    // Food
    const auto& spawner = game_.GetSpawner();
    const auto food_pos = spawner.FoodPos();
    render::Pulse pulse;
    const double food_scale = pulse.Eval(now);
    const int food_w = static_cast<int>(tile_size * food_scale);
    const int food_h = static_cast<int>(tile_size * food_scale);
    SDL_Rect food_rect{
        origin_x + food_pos.x * tile_size + (tile_size - food_w) / 2,
        origin_y + food_pos.y * tile_size + (tile_size - food_h) / 2,
        food_w,
        food_h};
    draw_sprite(food_rect, "food", SDL_Color{200, 80, 50, 255});

    // Bonuses
    for (const auto& bonus : spawner.Bonuses()) {
        SDL_Color color{120, 180, 255, 255};
        std::string_view sprite = "bonus_score";
        if (bonus.type == "bonus_score") {
            color = SDL_Color{220, 200, 60, 255};
            sprite = "bonus_score";
        } else if (bonus.type == "bonus_slow") {
            color = SDL_Color{80, 200, 220, 255};
            sprite = "bonus_slow";
        }

        SDL_Rect bonus_rect{
            origin_x + bonus.pos.x * tile_size + 4,
            origin_y + bonus.pos.y * tile_size + 4,
            tile_size - 8,
            tile_size - 8};
        draw_sprite(bonus_rect, sprite, color);
    }

    // Snake
    const auto& snake = game_.GetSnake();
    const auto& body = snake.Body();
    for (std::size_t i = 0; i < body.size(); ++i) {
        const auto pos = body[i];
        SDL_Rect dest{
            origin_x + pos.x * tile_size,
            origin_y + pos.y * tile_size,
            tile_size,
            tile_size};

        if (i == 0) {
            std::string_view head_sprite = "snake_head_right";
            switch (snake.Direction()) {
                case snake::game::Dir::Up:
                    head_sprite = "snake_head_up";
                    break;
                case snake::game::Dir::Down:
                    head_sprite = "snake_head_down";
                    break;
                case snake::game::Dir::Left:
                    head_sprite = "snake_head_left";
                    break;
                case snake::game::Dir::Right:
                    head_sprite = "snake_head_right";
                    break;
                default:
                    break;
            }
            draw_sprite(dest, head_sprite, SDL_Color{120, 240, 140, 255});
        } else if (i == body.size() - 1) {
            draw_sprite(dest, "snake_tail", SDL_Color{80, 180, 110, 255});
        } else {
            draw_sprite(dest, "snake_body", SDL_Color{70, 140, 90, 255});
        }
    }

    ui_.Render(renderer_, layout, game_, now);
}

}  // namespace snake::core
