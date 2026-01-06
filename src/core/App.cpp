#include "core/App.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <stdexcept>

#include "audio/AudioSystem.h"
#include "io/Paths.h"
#include "lua/Bindings.h"

namespace snake::core {

namespace {
constexpr int kDefaultWindowW = 800;
constexpr int kDefaultWindowH = 800;
constexpr int kMaxTicksPerFrame = 10;
}  // namespace

App::App() = default;

App::~App() {
    ShutdownSDL();
}

int App::Run() {
    try {
        InitSDL();
        const auto config_path = snake::io::UserPath("config.lua");
        config_.LoadFromFile(config_path);

        window_w_ = config_.Data().video.window_w;
        window_h_ = config_.Data().video.window_h;

        CreateWindowAndRenderer(config_.Data().video.vsync);
        renderer_impl_.Init(renderer_);
        time_.Init();
        lua_ctx_.game = &game_;
        lua_ctx_.audio = &audio_;

        audio_.Init();
        ApplyConfig();
        InitLua();

        bool running = true;
        while (running) {
            input_.BeginFrame();
            window_resized_ = false;

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    running = false;
                }

                if (event.type == SDL_WINDOWEVENT) {
                    switch (event.window.event) {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        case SDL_WINDOWEVENT_RESIZED:
                            window_w_ = event.window.data1;
                            window_h_ = event.window.data2;
                            window_resized_ = true;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_GAINED:
                            is_focused_ = true;
                            break;
                        case SDL_WINDOWEVENT_FOCUS_LOST:
                            is_focused_ = false;
                            break;
                        default:
                            break;
                    }
                }

                input_.HandleEvent(event);
            }

            if (input_.QuitRequested()) {
                running = false;
            }

            if (config_.Data().video.vsync != vsync_enabled_) {
                const bool recreated = RecreateRenderer(config_.Data().video.vsync);
                if (!recreated) {
                    config_.Data().video.vsync = vsync_enabled_;
                    if (!config_.SaveToFile(config_path)) {
                        SDL_Log("Failed to save reverted config after VSync error");
                    }
                } else if (!config_.SaveToFile(config_path)) {
                    SDL_Log("Failed to persist VSync change to config file");
                }
            }

            time_.UpdateFrame();

            // Per-frame input (direction/state changes)
            game_.HandleInput(input_);

            // Compute speed from Lua
            const int score = game_.GetScore().Score();
            double base_tps = last_base_ticks_per_sec_;
            if (lua_.IsReady()) {
                double lua_tps = 0.0;
                if (lua_.GetBaseTicksPerSec(score, &lua_tps)) {
                    base_tps = lua_tps;
                    last_base_ticks_per_sec_ = lua_tps;
                }
            }

            const bool slow_active = game_.GetEffects().SlowActive();
            const double slow_multiplier =
                slow_active ? game_.GetEffects().SlowMultiplier() : 1.0;
            const double effective_tps = base_tps * slow_multiplier;
            const double tick_dt = effective_tps > 0.0 ? 1.0 / effective_tps : 0.1;
            time_.SetTickDt(tick_dt);

            int ticks_done = 0;
            while (time_.ConsumeTick() && ticks_done < kMaxTicksPerFrame) {
                lua_.CallWithCtxIfExists("on_tick_begin", &lua_ctx_);

                const auto state_before = game_.State();
                game_.Tick(time_.TickDt());
                const auto events = game_.Events();
                const auto state_after = game_.State();

                if (events.food_eaten) {
                    lua_.CallWithCtxIfExists("on_food_eaten", &lua_ctx_);
                }
                if (events.bonus_picked) {
                    lua_.CallWithCtxIfExists("on_bonus_picked", &lua_ctx_, events.bonus_type);
                }

                if (state_after != snake::game::GameState::GameOver) {
                    lua_.CallWithCtxIfExists("on_tick_end", &lua_ctx_);
                } else if (state_before != snake::game::GameState::GameOver) {
                    // no end hook when switched to GameOver during tick
                }

                ++ticks_done;
            }

            if (ticks_done >= kMaxTicksPerFrame) {
                time_.DropAccumulatorToOneTick();
            }

            RenderFrame();
        }
    } catch (const std::exception& ex) {
        SDL_Log("App run failed: %s", ex.what());
        return 1;
    }

    return 0;
}

Input& App::GetInput() {
    return input_;
}

const Input& App::GetInput() const {
    return input_;
}

bool App::IsFocused() const {
    return is_focused_;
}

SDL_Point App::GetWindowSize() const {
    return SDL_Point{window_w_, window_h_};
}

bool App::WasResizedThisFrame() const {
    return window_resized_;
}

void App::InitSDL() {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }
    sdl_initialized_ = true;

    if ((IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) == 0) {
        SDL_Log("IMG_Init failed: %s", IMG_GetError());
    }
    if (TTF_Init() != 0) {
        SDL_Log("TTF_Init failed: %s", TTF_GetError());
    }
}

void App::CreateWindowAndRenderer(bool want_vsync) {
    if (window_w_ <= 0) window_w_ = kDefaultWindowW;
    if (window_h_ <= 0) window_h_ = kDefaultWindowH;

    window_ = SDL_CreateWindow(
        "snake",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        window_w_,
        window_h_,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

    if (window_ == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (want_vsync) {
        flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    renderer_ = SDL_CreateRenderer(window_, -1, flags);
    if (renderer_ == nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        throw std::runtime_error(SDL_GetError());
    }

    vsync_enabled_ = want_vsync;
    SDL_GetWindowSize(window_, &window_w_, &window_h_);
}

void App::ShutdownSDL() {
    audio_.Shutdown();
    renderer_impl_.Shutdown();

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

void App::RenderFrame() {
    if (renderer_ == nullptr) {
        return;
    }

    int window_w = 0;
    int window_h = 0;
    SDL_GetWindowSize(window_, &window_w, &window_h);

    snake::render::RenderSettings rs{};
    rs.tile_px = config_.Data().video.tile_px > 0 ? config_.Data().video.tile_px : 32;
    rs.panel_mode = "auto";

    std::string overlay_error_text = renderer_error_text_;
    if (const auto& err = lua_.LastError()) {
        if (!overlay_error_text.empty()) {
            overlay_error_text.append(" | ");
        }
        overlay_error_text.append(err->message);
    }

    renderer_impl_.RenderFrame(renderer_, window_w, window_h, rs, game_, time_.Now(), overlay_error_text);
}

bool App::RecreateRenderer(bool want_vsync) {
    if (renderer_ == nullptr || window_ == nullptr) {
        SDL_Log("RecreateRenderer called before window/renderer were initialized");
        renderer_error_text_ = "Failed to apply VSync setting: renderer not ready";
        return false;
    }

    if (want_vsync == vsync_enabled_) {
        return true;
    }

    Uint32 flags = SDL_RENDERER_ACCELERATED;
    if (want_vsync) {
        flags |= SDL_RENDERER_PRESENTVSYNC;
    }

    SDL_Renderer* new_renderer = SDL_CreateRenderer(window_, -1, flags);
    if (new_renderer == nullptr) {
        const char* err = SDL_GetError();
        SDL_Log("Failed to recreate renderer: %s", err);
        renderer_error_text_ = std::string("Failed to apply VSync setting: ") + (err ? err : "unknown error");
        return false;
    }

    renderer_impl_.Shutdown();

    if (!renderer_impl_.Init(new_renderer)) {
        SDL_Log("Failed to initialize render resources after recreating SDL_Renderer");
        if (!renderer_impl_.Init(renderer_)) {
            SDL_Log("Failed to restore render resources on previous renderer");
        }
        SDL_DestroyRenderer(new_renderer);
        renderer_error_text_ = "Failed to apply VSync setting: renderer init failed";
        return false;
    }

    SDL_Renderer* old_renderer = renderer_;
    renderer_ = new_renderer;
    vsync_enabled_ = want_vsync;
    renderer_error_text_.clear();

    if (old_renderer != nullptr) {
        SDL_DestroyRenderer(old_renderer);
    }

    return true;
}

void App::ApplyConfig() {
    const auto& data = config_.Data();
    game_.SetBoardSize(data.game.board_w, data.game.board_h);
    game_.SetWrapMode(data.game.walls == snake::io::WallMode::Wrap);
    game_.SetFoodScore(data.game.food_score);
    game_.SetBonusScore(data.game.bonus_score);
    game_.SetSlowParams(data.game.slow_multiplier, data.game.slow_duration_sec);

    snake::game::Game::Controls controls{};
    controls.up = data.keys.up;
    controls.down = data.keys.down;
    controls.left = data.keys.left;
    controls.right = data.keys.right;
    controls.pause = data.keys.pause;
    controls.restart = data.keys.restart;
    controls.menu = data.keys.menu;
    controls.confirm = data.keys.confirm;
    game_.SetControls(controls);

    game_.ResetAll();
}

void App::InitLua() {
    if (!lua_.Init()) {
        SDL_Log("Failed to init Lua runtime");
        return;
    }

    snake::lua::Bindings::Register(lua_.L());

    const auto rules_path = snake::io::AssetsPath("scripts/rules.lua");
    const auto config_path = snake::io::UserPath("config.lua");
    if (!lua_.LoadRules(rules_path)) {
        SDL_Log("Failed to load Lua rules");
    }
    if (!lua_.LoadConfig(config_path)) {
        SDL_Log("Failed to load Lua config");
    }
}

}  // namespace snake::core
