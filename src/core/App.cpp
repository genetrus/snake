#include "core/App.h"

#include <SDL.h>

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
        CreateWindowAndRenderer();
        time_.Init();
        lua_ctx_.game = &game_;
        lua_ctx_.audio = &audio_;

        audio_.Init();
        config_.LoadFromFile(snake::io::UserPath("config.lua"));
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

            time_.UpdateFrame();

            // Per-frame input (direction/state changes)
            game_.HandleInput(input_);

            // Compute speed from Lua
            double base_tps = last_ticks_per_sec_;
            if (lua_.IsReady()) {
                double lua_tps = 0.0;
                if (lua_.GetSpeedTicksPerSec(game_.GetScore().Score(), &lua_tps)) {
                    base_tps = lua_tps;
                    last_ticks_per_sec_ = lua_tps;
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
                const auto state_after = game_.State();

                if (state_after != snake::game::State::GameOver) {
                    lua_.CallWithCtxIfExists("on_tick_end", &lua_ctx_);
                } else if (state_before != snake::game::State::GameOver) {
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
}

void App::CreateWindowAndRenderer() {
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

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (renderer_ == nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GetWindowSize(window_, &window_w_, &window_h_);
}

void App::ShutdownSDL() {
    audio_.Shutdown();

    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }

    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    if (sdl_initialized_) {
        SDL_Quit();
        sdl_initialized_ = false;
    }
}

void App::RenderFrame() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderPresent(renderer_);
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
