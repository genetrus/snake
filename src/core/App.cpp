#include "core/App.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <stdexcept>
#include <lua.hpp>

#include "audio/AudioSystem.h"
#include "io/Paths.h"
#include "io/Highscores.h"
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
        const auto highscores_path = snake::io::UserPath("highscores.json");
        highscores_.Load(highscores_path);
        menu_items_ = {"Start", "Options", "Highscores", "Exit"};

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

            HandleMenus(running);

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
    rs.panel_mode = config_.Data().ui.panel_mode;

    std::string overlay_error_text = renderer_error_text_;
    if (const auto& err = lua_.LastError()) {
        if (!overlay_error_text.empty()) {
            overlay_error_text.append(" | ");
        }
        overlay_error_text.append(err->message);
    }

    snake::render::UiFrameData ui{};
    ui.screen = sm_.Current();
    ui.menu_index = menu_index_;
    ui.options_index = options_index_;
    ui.rebinding = rebinding_;
    ui.pending_round_restart = pending_round_restart_;
    ui.ui_message = ui_message_;
    ui.lua_error = lua_reload_error_;
    ui.game_over_reason = game_.GameOverReason();
    ui.final_score = game_.GetScore().Score();
    ui.config = &config_.Data();
    ui.highscores = &highscores_.Entries();
    ui.menu_items = menu_items_;

    ui.option_items = {
        {"window.width", std::to_string(config_.Data().video.window_w)},
        {"window.height", std::to_string(config_.Data().video.window_h)},
        {"window.fullscreen_desktop", config_.Data().video.fullscreen_desktop ? "true" : "false"},
        {"window.vsync", config_.Data().video.vsync ? "on" : "off"},
        {"ui.panel_mode", config_.Data().ui.panel_mode},
        {"grid.board_w", std::to_string(config_.Data().game.board_w)},
        {"grid.board_h", std::to_string(config_.Data().game.board_h)},
        {"grid.tile_size", std::to_string(config_.Data().video.tile_px)},
        {"grid.wrap_mode", config_.Data().game.walls == snake::io::WallMode::Wrap ? "wrap" : "death"},
        {"audio.enabled", config_.Data().audio.enabled ? "true" : "false"},
        {"audio.master_volume", std::to_string(config_.Data().audio.master_volume)},
        {"gameplay.food_score", std::to_string(config_.Data().game.food_score)},
        {"keybinds.up", "key"},
        {"keybinds.down", "key"},
        {"keybinds.left", "key"},
        {"keybinds.right", "key"},
        {"keybinds.pause", "key"},
        {"keybinds.restart", "key"},
        {"keybinds.menu", "key"},
        {"keybinds.confirm", "key"},
    };

    auto key_to_str = [](SDL_Keycode k) {
        return SDL_GetKeyName(k);
    };
    ui.option_items[12].second = key_to_str(config_.Data().keys.up);
    ui.option_items[13].second = key_to_str(config_.Data().keys.down);
    ui.option_items[14].second = key_to_str(config_.Data().keys.left);
    ui.option_items[15].second = key_to_str(config_.Data().keys.right);
    ui.option_items[16].second = key_to_str(config_.Data().keys.pause);
    ui.option_items[17].second = key_to_str(config_.Data().keys.restart);
    ui.option_items[18].second = key_to_str(config_.Data().keys.menu);
    ui.option_items[19].second = key_to_str(config_.Data().keys.confirm);

    renderer_impl_.RenderFrame(renderer_, window_w, window_h, rs, game_, time_.Now(), overlay_error_text, ui);
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

void App::PushUiMessage(std::string msg) {
    ui_message_ = std::move(msg);
}

void App::HandleMenus(bool& running) {
    const auto config_path = snake::io::UserPath("config.lua");
    const auto highscores_path = snake::io::UserPath("highscores.json");

    auto start_round = [&]() {
        ApplyConfig();
        game_.ResetAll();
        sm_.StartGame();
        pending_round_restart_ = false;
        lua_.CallWithCtxIfExists("on_round_start", &lua_ctx_);
    };

    if (rebinding_) {
        HandleRebind();
        return;
    }

    auto up_pressed = input_.KeyPressed(SDLK_UP) || input_.KeyPressed(SDLK_w);
    auto down_pressed = input_.KeyPressed(SDLK_DOWN) || input_.KeyPressed(SDLK_s);
    auto left_pressed = input_.KeyPressed(SDLK_LEFT) || input_.KeyPressed(SDLK_a);
    auto right_pressed = input_.KeyPressed(SDLK_RIGHT) || input_.KeyPressed(SDLK_d);
    const bool enter_pressed = input_.KeyPressed(SDLK_RETURN);
    const bool esc_pressed = input_.KeyPressed(SDLK_ESCAPE);
    const bool restart_pressed = input_.KeyPressed(config_.Data().keys.restart);
    const bool pause_pressed = input_.KeyPressed(config_.Data().keys.pause);

    switch (sm_.Current()) {
        case snake::game::Screen::MainMenu: {
            if (up_pressed) {
                menu_index_ = (menu_index_ + static_cast<int>(menu_items_.size()) - 1) % static_cast<int>(menu_items_.size());
            } else if (down_pressed) {
                menu_index_ = (menu_index_ + 1) % static_cast<int>(menu_items_.size());
            }
            if (enter_pressed) {
                switch (menu_index_) {
                    case 0:
                        start_round();
                        break;
                    case 1:
                        sm_.OpenOptions();
                        break;
                    case 2:
                        sm_.OpenHighscores();
                        break;
                    case 3:
                        running = false;
                        break;
                    default:
                        break;
                }
            }
            if (esc_pressed) {
                running = false;
            }
            break;
        }
        case snake::game::Screen::Options:
            HandleOptionsInput();
            if (esc_pressed) {
                sm_.BackToMenu();
            }
            break;
        case snake::game::Screen::Highscores:
            if (esc_pressed) {
                sm_.BackToMenu();
            }
            break;
        case snake::game::Screen::Playing: {
            game_.HandleInput(input_);
            if (pause_pressed) {
                sm_.Pause();
            }
            if (restart_pressed) {
                start_round();
            }
            if (esc_pressed) {
                sm_.BackToMenu();
                game_.ResetAll();
            }
            break;
        }
        case snake::game::Screen::Paused:
            if (pause_pressed) {
                sm_.Resume();
            }
            if (restart_pressed) {
                start_round();
            }
            if (esc_pressed) {
                sm_.BackToMenu();
                game_.ResetAll();
            }
            break;
        case snake::game::Screen::GameOver:
            if (restart_pressed || enter_pressed) {
                start_round();
            }
            if (esc_pressed) {
                sm_.BackToMenu();
                game_.ResetAll();
            }
            break;
    }

    if (sm_.Is(snake::game::Screen::Playing)) {
        time_.UpdateFrame();

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

            game_.Tick(time_.TickDt());
            const auto events = game_.Events();

            if (events.food_eaten) {
                lua_.CallWithCtxIfExists("on_food_eaten", &lua_ctx_);
            }
            if (events.bonus_picked) {
                lua_.CallWithCtxIfExists("on_bonus_picked", &lua_ctx_, events.bonus_type);
            }
            if (!game_.IsGameOver()) {
                lua_.CallWithCtxIfExists("on_tick_end", &lua_ctx_);
            }

            ++ticks_done;
        }

        if (ticks_done >= kMaxTicksPerFrame) {
            time_.DropAccumulatorToOneTick();
        }

        if (game_.IsGameOver()) {
            sm_.GameOver();
            highscores_.TryAdd(config_.Data().player_name, game_.GetScore().Score(), snake::io::Highscores::NowIsoUtc());
            highscores_.Save(highscores_path);
            lua_.CallWithCtxIfExists("on_game_over", &lua_ctx_, game_.GameOverReason());
        }
    } else {
        time_.UpdateFrame();
    }
}

void App::HandleOptionsInput() {
    const auto config_path = snake::io::UserPath("config.lua");
    auto& data = config_.Data();

    auto save_and_notify = [&](const std::string& key) {
        config_.Sanitize();
        config_.SaveToFile(config_path);
        NotifySettingChanged(key);
    };

    auto apply_next_round = [&]() {
        pending_round_restart_ = true;
        PushUiMessage("Will apply on Restart");
    };

    auto apply_video_now = [&]() { ApplyNowVideoSettings(); };
    auto apply_audio_now = [&]() { ApplyAudioSettings(); };

    auto up_pressed = input_.KeyPressed(SDLK_UP) || input_.KeyPressed(SDLK_w);
    auto down_pressed = input_.KeyPressed(SDLK_DOWN) || input_.KeyPressed(SDLK_s);
    auto left_pressed = input_.KeyPressed(SDLK_LEFT) || input_.KeyPressed(SDLK_a);
    auto right_pressed = input_.KeyPressed(SDLK_RIGHT) || input_.KeyPressed(SDLK_d);
    const bool enter_pressed = input_.KeyPressed(SDLK_RETURN);

    const int options_count = 20;
    if (up_pressed) {
        options_index_ = (options_index_ + options_count - 1) % options_count;
    } else if (down_pressed) {
        options_index_ = (options_index_ + 1) % options_count;
    }

    auto adjust_int = [&](int& value, int delta, int min_v, int max_v, const std::string& key, bool apply_now) {
        const int before = value;
        value = std::clamp(value + delta, min_v, max_v);
        if (value != before) {
            save_and_notify(key);
            if (apply_now) {
                apply_video_now();
            } else {
                apply_next_round();
            }
        }
    };

    switch (options_index_) {
        case 0:  // window.width
            if (left_pressed) adjust_int(data.video.window_w, -16, 320, 3840, "window.width", true);
            if (right_pressed) adjust_int(data.video.window_w, 16, 320, 3840, "window.width", true);
            break;
        case 1:  // window.height
            if (left_pressed) adjust_int(data.video.window_h, -16, 320, 3840, "window.height", true);
            if (right_pressed) adjust_int(data.video.window_h, 16, 320, 3840, "window.height", true);
            break;
        case 2:  // fullscreen
            if (enter_pressed) {
                data.video.fullscreen_desktop = !data.video.fullscreen_desktop;
                apply_video_now();
                save_and_notify("window.fullscreen_desktop");
            }
            break;
        case 3:  // vsync
            if (enter_pressed) {
                data.video.vsync = !data.video.vsync;
                const bool recreated = RecreateRenderer(data.video.vsync);
                if (!recreated) {
                    data.video.vsync = !data.video.vsync;
                }
                save_and_notify("window.vsync");
            }
            break;
        case 4:  // ui.panel_mode
            if (enter_pressed) {
                data.ui.panel_mode = (data.ui.panel_mode == "auto") ? "top" : (data.ui.panel_mode == "top" ? "right" : "auto");
                save_and_notify("ui.panel_mode");
            }
            break;
        case 5:  // board_w
            if (left_pressed) adjust_int(data.game.board_w, -1, 5, 60, "grid.board_w", false);
            if (right_pressed) adjust_int(data.game.board_w, 1, 5, 60, "grid.board_w", false);
            break;
        case 6:  // board_h
            if (left_pressed) adjust_int(data.game.board_h, -1, 5, 60, "grid.board_h", false);
            if (right_pressed) adjust_int(data.game.board_h, 1, 5, 60, "grid.board_h", false);
            break;
        case 7:  // tile_size
            if (left_pressed) adjust_int(data.video.tile_px, -1, 8, 128, "grid.tile_size", true);
            if (right_pressed) adjust_int(data.video.tile_px, 1, 8, 128, "grid.tile_size", true);
            break;
        case 8:  // wrap_mode
            if (enter_pressed) {
                data.game.walls = (data.game.walls == snake::io::WallMode::Wrap) ? snake::io::WallMode::Death : snake::io::WallMode::Wrap;
                save_and_notify("grid.wrap_mode");
                apply_next_round();
            }
            break;
        case 9:  // audio enabled
            if (enter_pressed) {
                data.audio.enabled = !data.audio.enabled;
                apply_audio_now();
                save_and_notify("audio.enabled");
            }
            break;
        case 10:  // master volume
            if (left_pressed) adjust_int(data.audio.master_volume, -8, 0, 128, "audio.master_volume", true);
            if (right_pressed) adjust_int(data.audio.master_volume, 8, 0, 128, "audio.master_volume", true);
            apply_audio_now();
            break;
        case 11:  // food score
            if (left_pressed) adjust_int(data.game.food_score, -1, 1, 1000, "gameplay.food_score", false);
            if (right_pressed) adjust_int(data.game.food_score, 1, 1, 1000, "gameplay.food_score", false);
            break;
        case 12: if (enter_pressed) BeginRebind("up"); break;
        case 13: if (enter_pressed) BeginRebind("down"); break;
        case 14: if (enter_pressed) BeginRebind("left"); break;
        case 15: if (enter_pressed) BeginRebind("right"); break;
        case 16: if (enter_pressed) BeginRebind("pause"); break;
        case 17: if (enter_pressed) BeginRebind("restart"); break;
        case 18: if (enter_pressed) BeginRebind("menu"); break;
        case 19: if (enter_pressed) BeginRebind("confirm"); break;
        default:
            break;
    }
}

void App::BeginRebind(const std::string& action) {
    rebinding_ = true;
    rebind_action_ = action;
}

void App::HandleRebind() {
    const auto config_path = snake::io::UserPath("config.lua");
    if (input_.KeyPressed(SDLK_ESCAPE)) {
        rebinding_ = false;
        rebind_action_.clear();
        return;
    }
    const std::vector<SDL_Keycode> allowed{SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_w, SDLK_a, SDLK_s, SDLK_d,
                                           SDLK_RETURN, SDLK_ESCAPE, SDLK_p, SDLK_r};
    for (auto key : allowed) {
        if (input_.KeyPressed(key)) {
            if (config_.SetBind(rebind_action_, key)) {
                config_.SaveToFile(config_path);
                NotifySettingChanged("keybinds." + rebind_action_);
            }
            rebinding_ = false;
            rebind_action_.clear();
            return;
        }
    }
}

void App::ApplyNowVideoSettings() {
    SDL_SetWindowSize(window_, config_.Data().video.window_w, config_.Data().video.window_h);
    if (config_.Data().video.fullscreen_desktop) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, 0);
    }
    SDL_GetWindowSize(window_, &window_w_, &window_h_);
}

void App::ApplyAudioSettings() {
    if (!config_.Data().audio.enabled) {
        audio_.SetMasterVolume(0);
    } else {
        audio_.SetMasterVolume(config_.Data().audio.master_volume);
    }
}

void App::NotifySettingChanged(const std::string& key) {
    if (!lua_.IsReady()) {
        return;
    }
    lua_State* L = lua_.L();
    if (L == nullptr) {
        return;
    }
    lua_getglobal(L, "on_setting_changed");
    if (!lua_isfunction(L, -1)) {
        lua_pop(L, 1);
        return;
    }
    lua_pushlightuserdata(L, &lua_ctx_);
    lua_pushlstring(L, key.data(), key.size());

    const auto& d = config_.Data();
    if (key == "window.width") {
        lua_pushinteger(L, d.video.window_w);
    } else if (key == "window.height") {
        lua_pushinteger(L, d.video.window_h);
    } else if (key == "window.fullscreen_desktop") {
        lua_pushboolean(L, d.video.fullscreen_desktop);
    } else if (key == "window.vsync") {
        lua_pushboolean(L, d.video.vsync);
    } else if (key == "ui.panel_mode") {
        lua_pushlstring(L, d.ui.panel_mode.c_str(), d.ui.panel_mode.size());
    } else if (key == "grid.board_w") {
        lua_pushinteger(L, d.game.board_w);
    } else if (key == "grid.board_h") {
        lua_pushinteger(L, d.game.board_h);
    } else if (key == "grid.tile_size") {
        lua_pushinteger(L, d.video.tile_px);
    } else if (key == "grid.wrap_mode") {
        lua_pushboolean(L, d.game.walls == snake::io::WallMode::Wrap);
    } else if (key == "audio.enabled") {
        lua_pushboolean(L, d.audio.enabled);
    } else if (key == "audio.master_volume") {
        lua_pushinteger(L, d.audio.master_volume);
    } else if (key == "gameplay.food_score") {
        lua_pushinteger(L, d.game.food_score);
    } else if (key == "keybinds.up") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.up));
    } else if (key == "keybinds.down") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.down));
    } else if (key == "keybinds.left") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.left));
    } else if (key == "keybinds.right") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.right));
    } else if (key == "keybinds.pause") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.pause));
    } else if (key == "keybinds.restart") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.restart));
    } else if (key == "keybinds.menu") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.menu));
    } else if (key == "keybinds.confirm") {
        lua_pushstring(L, SDL_GetKeyName(d.keys.confirm));
    } else {
        lua_pushnil(L);
    }
    lua_pcall(L, 3, 0, 0);
}

}  // namespace snake::core
