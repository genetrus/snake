#include "core/App.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

#include <algorithm>
#include <functional>
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
        config_path_ = snake::io::UserPath("config.lua");
        pending_config_.LoadFromFile(config_path_);
        active_config_ = pending_config_;
        const auto highscores_path = snake::io::UserPath("highscores.json");
        highscores_.Load(highscores_path);
        menu_items_ = {"Start", "Options", "Highscores", "Exit"};

        window_w_ = pending_config_.Data().window.width;
        window_h_ = pending_config_.Data().window.height;

        CreateWindowAndRenderer(pending_config_.Data().window.vsync);
        renderer_impl_.Init(renderer_);
        time_.Init();
        lua_ctx_.game = &game_;
        lua_ctx_.audio = &audio_;

        audio_.Init();
        ApplyConfig();
        ApplyAudioSettings();
        ApplyImmediateSettings(pending_config_.Data());
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
    rs.tile_px = active_config_.Data().grid.tile_size > 0 ? active_config_.Data().grid.tile_size : 32;
    rs.panel_mode = active_config_.Data().ui.panel_mode;

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
    ui.rebind_action = rebind_action_;
    ui.rebind_slot = rebind_slot_;
    ui.pending_round_restart = pending_round_restart_;
    ui.ui_message = ui_message_;
    ui.lua_error = lua_reload_error_;
    ui.game_over_reason = game_.GameOverReason();
    ui.final_score = game_.GetScore().Score();
    ui.config = &pending_config_.Data();
    ui.highscores = &highscores_.Entries();
    ui.menu_items = menu_items_;

    auto bool_label = [](bool on) { return on ? "On" : "Off"; };
    auto wrap_label = [](bool wrap) { return wrap ? "On" : "Off"; };
    auto keypair_to_text = [](const snake::io::KeyPair& kp) {
        std::string a = snake::io::Config::KeycodeToToken(kp.primary);
        std::string b = snake::io::Config::KeycodeToToken(kp.secondary);
        if (a.empty()) a = SDL_GetKeyName(kp.primary);
        if (b.empty()) b = SDL_GetKeyName(kp.secondary);
        if (a.empty()) a = "-";
        if (b.empty()) b = "-";
        return a + " / " + b;
    };

    ui.option_items = {
        {"Board Width:", std::to_string(pending_config_.Data().grid.board_w)},
        {"Board Height:", std::to_string(pending_config_.Data().grid.board_h)},
        {"Tile Size:", std::to_string(pending_config_.Data().grid.tile_size)},
        {"Wrap Mode:", wrap_label(pending_config_.Data().grid.wrap_mode)},
        {"Window Width:", std::to_string(pending_config_.Data().window.width)},
        {"Window Height:", std::to_string(pending_config_.Data().window.height)},
        {"Fullscreen Desktop:", bool_label(pending_config_.Data().window.fullscreen_desktop)},
        {"VSync:", bool_label(pending_config_.Data().window.vsync)},
        {"Audio Enabled:", bool_label(pending_config_.Data().audio.enabled)},
        {"Master Volume:", std::to_string(pending_config_.Data().audio.master_volume)},
        {"SFX Volume:", std::to_string(pending_config_.Data().audio.sfx_volume)},
        {"UI Panel Mode:", pending_config_.Data().ui.panel_mode},
        {"Keybind Up:", keypair_to_text(pending_config_.Data().keys.up)},
        {"Keybind Down:", keypair_to_text(pending_config_.Data().keys.down)},
        {"Keybind Left:", keypair_to_text(pending_config_.Data().keys.left)},
        {"Keybind Right:", keypair_to_text(pending_config_.Data().keys.right)},
        {"Keybind Pause:", keypair_to_text(pending_config_.Data().keys.pause)},
        {"Keybind Restart:", keypair_to_text(pending_config_.Data().keys.restart)},
        {"Keybind Menu:", keypair_to_text(pending_config_.Data().keys.menu)},
        {"Keybind Confirm:", keypair_to_text(pending_config_.Data().keys.confirm)},
        {"Back", ""},
    };

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
    const auto& data = active_config_.Data();
    game_.SetBoardSize(data.grid.board_w, data.grid.board_h);
    game_.SetWrapMode(data.grid.wrap_mode);
    game_.SetFoodScore(data.gameplay.food_score);
    const int bonus_score = data.gameplay.bonus_score_score > 0 ? data.gameplay.bonus_score_score : data.gameplay.bonus_score;
    game_.SetBonusScore(bonus_score);
    game_.SetSlowParams(data.gameplay.slow_multiplier, data.gameplay.slow_duration_sec);

    ApplyControlSettings();
}

void App::InitLua() {
    if (!lua_.Init()) {
        SDL_Log("Failed to init Lua runtime");
        return;
    }

    snake::lua::Bindings::Register(lua_.L());

    const auto rules_path = snake::io::AssetsPath("scripts/rules.lua");
    const auto config_path = config_path_.empty() ? snake::io::UserPath("config.lua") : config_path_;
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
    const auto highscores_path = snake::io::UserPath("highscores.json");

    auto start_round = [&]() {
        ApplyRoundSettingsOnRestart();
        ApplyConfig();
        game_.ResetAll();
        sm_.StartGame();
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
    auto action_pressed = [&](const snake::io::KeyPair& keys) {
        return input_.KeyPressed(keys.primary) || input_.KeyPressed(keys.secondary);
    };
    const bool confirm_pressed = action_pressed(active_config_.Data().keys.confirm);
    const bool menu_pressed = action_pressed(active_config_.Data().keys.menu);
    const bool restart_pressed = action_pressed(active_config_.Data().keys.restart);
    const bool pause_pressed = action_pressed(active_config_.Data().keys.pause);

    switch (sm_.Current()) {
        case snake::game::Screen::MainMenu: {
            if (up_pressed) {
                menu_index_ = (menu_index_ + static_cast<int>(menu_items_.size()) - 1) % static_cast<int>(menu_items_.size());
            } else if (down_pressed) {
                menu_index_ = (menu_index_ + 1) % static_cast<int>(menu_items_.size());
            }
            if (confirm_pressed) {
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
            if (menu_pressed) {
                running = false;
            }
            break;
        }
        case snake::game::Screen::Options:
            HandleOptionsInput();
            if (!rebinding_ && menu_pressed) {
                sm_.BackToMenu();
            }
            break;
        case snake::game::Screen::Highscores:
            if (menu_pressed) {
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
            if (menu_pressed) {
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
            if (menu_pressed) {
                sm_.BackToMenu();
                game_.ResetAll();
            }
            break;
        case snake::game::Screen::GameOver:
            if (restart_pressed || confirm_pressed) {
                start_round();
            }
            if (menu_pressed) {
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
            highscores_.TryAdd(active_config_.Data().player_name, game_.GetScore().Score(), snake::io::Highscores::NowIsoUtc());
            highscores_.Save(highscores_path);
            lua_.CallWithCtxIfExists("on_game_over", &lua_ctx_, game_.GameOverReason());
        }
    } else {
        time_.UpdateFrame();
    }
}

void App::HandleOptionsInput() {
    auto& data = pending_config_.Data();

    auto persist = [&]() {
        pending_config_.Sanitize();
        if (!pending_config_.SaveToFile(config_path_)) {
            SDL_Log("Failed to save config to %s", config_path_.string().c_str());
        }
    };

    auto notify_and_apply = [&](const std::string& key, const std::function<void()>& apply_fn, bool sync_before_apply) {
        persist();
        if (sync_before_apply) {
            SyncActiveWithPendingPreserveRound();
        }
        if (apply_fn) {
            apply_fn();
        }
        if (!sync_before_apply) {
            SyncActiveWithPendingPreserveRound();
        }
        NotifySettingChanged(key);
    };

    auto apply_next_round = [&]() {
        RefreshPendingRoundRestartFlag();
        PushUiMessage("Applies on restart");
    };

    auto up_pressed = input_.KeyPressed(SDLK_UP) || input_.KeyPressed(SDLK_w);
    auto down_pressed = input_.KeyPressed(SDLK_DOWN) || input_.KeyPressed(SDLK_s);
    auto left_pressed = input_.KeyPressed(SDLK_LEFT) || input_.KeyPressed(SDLK_a);
    auto right_pressed = input_.KeyPressed(SDLK_RIGHT) || input_.KeyPressed(SDLK_d);
    auto action_pressed = [&](const snake::io::KeyPair& keys) {
        return input_.KeyPressed(keys.primary) || input_.KeyPressed(keys.secondary);
    };
    const bool confirm_pressed = action_pressed(active_config_.Data().keys.confirm);
    const bool menu_pressed = action_pressed(active_config_.Data().keys.menu);

    if (menu_pressed) {
        sm_.BackToMenu();
        return;
    }

    const int options_count = 21;
    if (up_pressed) {
        options_index_ = (options_index_ + options_count - 1) % options_count;
    } else if (down_pressed) {
        options_index_ = (options_index_ + 1) % options_count;
    }

    auto adjust_int = [&](int& value, int delta, int min_v, int max_v, const std::string& key, bool apply_now, const std::function<void()>& apply_fn = {}, bool sync_before_apply = false) {
        const int before = value;
        value = std::clamp(value + delta, min_v, max_v);
        if (value != before) {
            if (apply_now) {
                notify_and_apply(key, apply_fn, sync_before_apply);
            } else {
                persist();
                apply_next_round();
                NotifySettingChanged(key);
            }
        }
    };

    switch (options_index_) {
        case 0:  // board_w
            if (left_pressed) adjust_int(data.grid.board_w, -1, 5, 60, "grid.board_w", false);
            if (right_pressed) adjust_int(data.grid.board_w, 1, 5, 60, "grid.board_w", false);
            break;
        case 1:  // board_h
            if (left_pressed) adjust_int(data.grid.board_h, -1, 5, 60, "grid.board_h", false);
            if (right_pressed) adjust_int(data.grid.board_h, 1, 5, 60, "grid.board_h", false);
            break;
        case 2:  // tile_size
            if (left_pressed) adjust_int(data.grid.tile_size, -2, 8, 128, "grid.tile_size", true, [&]() { ApplyImmediateSettings(pending_config_.Data()); });
            if (right_pressed) adjust_int(data.grid.tile_size, 2, 8, 128, "grid.tile_size", true, [&]() { ApplyImmediateSettings(pending_config_.Data()); });
            break;
        case 3:  // wrap_mode
            if (confirm_pressed) {
                data.grid.wrap_mode = !data.grid.wrap_mode;
                persist();
                apply_next_round();
                NotifySettingChanged("grid.wrap_mode");
            }
            break;
        case 4:  // window.width
            if (left_pressed) adjust_int(data.window.width, -16, 320, 3840, "window.width", true, [&]() { ApplyImmediateSettings(pending_config_.Data()); });
            if (right_pressed) adjust_int(data.window.width, 16, 320, 3840, "window.width", true, [&]() { ApplyImmediateSettings(pending_config_.Data()); });
            break;
        case 5:  // window.height
            if (left_pressed) adjust_int(data.window.height, -16, 320, 3840, "window.height", true, [&]() { ApplyImmediateSettings(pending_config_.Data()); });
            if (right_pressed) adjust_int(data.window.height, 16, 320, 3840, "window.height", true, [&]() { ApplyImmediateSettings(pending_config_.Data()); });
            break;
        case 6:  // fullscreen
            if (confirm_pressed) {
                data.window.fullscreen_desktop = !data.window.fullscreen_desktop;
                notify_and_apply("window.fullscreen_desktop", [&]() { ApplyImmediateSettings(pending_config_.Data()); }, false);
            }
            break;
        case 7:  // vsync
            if (confirm_pressed) {
                data.window.vsync = !data.window.vsync;
                notify_and_apply("window.vsync", [&]() { ApplyImmediateSettings(pending_config_.Data()); }, false);
            }
            break;
        case 8:  // audio enabled
            if (confirm_pressed) {
                data.audio.enabled = !data.audio.enabled;
                notify_and_apply("audio.enabled", [&]() { ApplyAudioSettings(); }, true);
            }
            break;
        case 9:  // master volume
            if (left_pressed) adjust_int(data.audio.master_volume, -8, 0, 128, "audio.master_volume", true, [&]() { ApplyAudioSettings(); }, true);
            if (right_pressed) adjust_int(data.audio.master_volume, 8, 0, 128, "audio.master_volume", true, [&]() { ApplyAudioSettings(); }, true);
            break;
        case 10:  // sfx volume
            if (left_pressed) adjust_int(data.audio.sfx_volume, -8, 0, 128, "audio.sfx_volume", true, [&]() { ApplyAudioSettings(); }, true);
            if (right_pressed) adjust_int(data.audio.sfx_volume, 8, 0, 128, "audio.sfx_volume", true, [&]() { ApplyAudioSettings(); }, true);
            break;
        case 11:  // ui.panel_mode
            if (confirm_pressed || left_pressed || right_pressed) {
                if (left_pressed) {
                    if (data.ui.panel_mode == "auto") data.ui.panel_mode = "right";
                    else if (data.ui.panel_mode == "right") data.ui.panel_mode = "top";
                    else data.ui.panel_mode = "auto";
                } else {
                    data.ui.panel_mode = (data.ui.panel_mode == "auto") ? "top" : (data.ui.panel_mode == "top" ? "right" : "auto");
                }
                notify_and_apply("ui.panel_mode", {}, true);
            }
            break;
        case 12: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("up", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 13: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("down", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 14: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("left", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 15: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("right", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 16: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("pause", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 17: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("restart", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 18: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("menu", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 19: if (confirm_pressed || left_pressed || right_pressed) BeginRebind("confirm", left_pressed ? 0 : (right_pressed ? 1 : 0)); break;
        case 20:
            if (confirm_pressed) {
                sm_.BackToMenu();
            }
            break;
        default:
            break;
    }
}

void App::BeginRebind(const std::string& action, int slot) {
    rebinding_ = true;
    rebind_action_ = action;
    rebind_slot_ = (slot == 1) ? 1 : 0;
}

void App::HandleRebind() {
    if (input_.KeyPressed(SDLK_RETURN)) {
        rebind_slot_ = 1 - rebind_slot_;
    }
    if (input_.KeyPressed(SDLK_LEFT)) {
        rebind_slot_ = 0;
    } else if (input_.KeyPressed(SDLK_RIGHT)) {
        rebind_slot_ = 1;
    }
    const std::vector<SDL_Keycode> allowed{SDLK_UP, SDLK_DOWN, SDLK_LEFT, SDLK_RIGHT, SDLK_w, SDLK_a, SDLK_s, SDLK_d,
                                           SDLK_RETURN, SDLK_ESCAPE, SDLK_p, SDLK_r};
    for (auto key : allowed) {
        if (input_.KeyPressed(key)) {
            if (pending_config_.SetBind(rebind_action_, key, rebind_slot_)) {
                pending_config_.Sanitize();
                if (!pending_config_.SaveToFile(config_path_)) {
                    SDL_Log("Failed to save config after rebinding");
                }
                SyncActiveWithPendingPreserveRound();
                ApplyControlSettings();
                NotifySettingChanged("keybinds." + rebind_action_);
            }
            rebinding_ = false;
            rebind_action_.clear();
            rebind_slot_ = 0;
            return;
        }
    }
}

void App::ApplyImmediateSettings(const snake::io::ConfigData& cfg) {
    SDL_SetWindowSize(window_, cfg.window.width, cfg.window.height);
    if (cfg.window.fullscreen_desktop) {
        SDL_SetWindowFullscreen(window_, SDL_WINDOW_FULLSCREEN_DESKTOP);
    } else {
        SDL_SetWindowFullscreen(window_, 0);
    }
    SDL_GetWindowSize(window_, &window_w_, &window_h_);

    const bool prev_vsync = active_config_.Data().window.vsync;
    if (cfg.window.vsync != prev_vsync) {
        if (!RecreateRenderer(cfg.window.vsync)) {
            pending_config_.Data().window.vsync = prev_vsync;
            pending_config_.Sanitize();
            if (!pending_config_.SaveToFile(config_path_)) {
                SDL_Log("Failed to save config after reverting VSync");
            }
        }
    }

    SyncActiveWithPendingPreserveRound();
}

void App::ApplyRoundSettingsOnRestart() {
    active_config_.Data().grid.board_w = pending_config_.Data().grid.board_w;
    active_config_.Data().grid.board_h = pending_config_.Data().grid.board_h;
    active_config_.Data().grid.wrap_mode = pending_config_.Data().grid.wrap_mode;
    RefreshPendingRoundRestartFlag();
}

void App::ApplyAudioSettings() {
    audio_.SetEnabled(active_config_.Data().audio.enabled);
    audio_.SetMasterVolume(active_config_.Data().audio.master_volume);
    audio_.SetSfxVolume(active_config_.Data().audio.sfx_volume);
}

void App::ApplyControlSettings() {
    const auto& keys = active_config_.Data().keys;
    snake::game::Game::Controls controls{};
    controls.up.primary = keys.up.primary;
    controls.up.secondary = keys.up.secondary;
    controls.down.primary = keys.down.primary;
    controls.down.secondary = keys.down.secondary;
    controls.left.primary = keys.left.primary;
    controls.left.secondary = keys.left.secondary;
    controls.right.primary = keys.right.primary;
    controls.right.secondary = keys.right.secondary;
    controls.pause.primary = keys.pause.primary;
    controls.pause.secondary = keys.pause.secondary;
    controls.restart.primary = keys.restart.primary;
    controls.restart.secondary = keys.restart.secondary;
    controls.menu.primary = keys.menu.primary;
    controls.menu.secondary = keys.menu.secondary;
    controls.confirm.primary = keys.confirm.primary;
    controls.confirm.secondary = keys.confirm.secondary;
    game_.SetControls(controls);
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

    const auto& d = pending_config_.Data();
    auto push_keypair = [&](const snake::io::KeyPair& kp) {
        const std::string a = snake::io::Config::KeycodeToToken(kp.primary);
        const std::string b = snake::io::Config::KeycodeToToken(kp.secondary);
        lua_createtable(L, 2, 0);
        lua_pushlstring(L, a.c_str(), a.size());
        lua_rawseti(L, -2, 1);
        lua_pushlstring(L, b.c_str(), b.size());
        lua_rawseti(L, -2, 2);
    };

    if (key == "window.width") {
        lua_pushinteger(L, d.window.width);
    } else if (key == "window.height") {
        lua_pushinteger(L, d.window.height);
    } else if (key == "window.fullscreen_desktop") {
        lua_pushboolean(L, d.window.fullscreen_desktop);
    } else if (key == "window.vsync") {
        lua_pushboolean(L, d.window.vsync);
    } else if (key == "ui.panel_mode") {
        lua_pushlstring(L, d.ui.panel_mode.c_str(), d.ui.panel_mode.size());
    } else if (key == "grid.board_w") {
        lua_pushinteger(L, d.grid.board_w);
    } else if (key == "grid.board_h") {
        lua_pushinteger(L, d.grid.board_h);
    } else if (key == "grid.tile_size") {
        lua_pushinteger(L, d.grid.tile_size);
    } else if (key == "grid.wrap_mode") {
        lua_pushboolean(L, d.grid.wrap_mode);
    } else if (key == "audio.enabled") {
        lua_pushboolean(L, d.audio.enabled);
    } else if (key == "audio.master_volume") {
        lua_pushinteger(L, d.audio.master_volume);
    } else if (key == "audio.sfx_volume") {
        lua_pushinteger(L, d.audio.sfx_volume);
    } else if (key.rfind("keybinds.", 0) == 0) {
        const std::string action = key.substr(std::string("keybinds.").size());
        if (action == "up") push_keypair(d.keys.up);
        else if (action == "down") push_keypair(d.keys.down);
        else if (action == "left") push_keypair(d.keys.left);
        else if (action == "right") push_keypair(d.keys.right);
        else if (action == "pause") push_keypair(d.keys.pause);
        else if (action == "restart") push_keypair(d.keys.restart);
        else if (action == "menu") push_keypair(d.keys.menu);
        else if (action == "confirm") push_keypair(d.keys.confirm);
        else lua_pushnil(L);
    } else {
        lua_pushnil(L);
    }

    if (lua_pcall(L, 3, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        SDL_Log("on_setting_changed failed: %s", err ? err : "unknown error");
        lua_pop(L, 1);
    }
}

void App::SyncActiveWithPendingPreserveRound() {
    const bool keep_round_settings = HasPendingRoundChanges();
    const int board_w = active_config_.Data().grid.board_w;
    const int board_h = active_config_.Data().grid.board_h;
    const bool wrap_mode = active_config_.Data().grid.wrap_mode;

    active_config_.Data() = pending_config_.Data();
    if (keep_round_settings) {
        active_config_.Data().grid.board_w = board_w;
        active_config_.Data().grid.board_h = board_h;
        active_config_.Data().grid.wrap_mode = wrap_mode;
    }
    RefreshPendingRoundRestartFlag();
}

bool App::HasPendingRoundChanges() const {
    const auto& a = active_config_.Data().grid;
    const auto& p = pending_config_.Data().grid;
    return a.board_w != p.board_w || a.board_h != p.board_h || a.wrap_mode != p.wrap_mode;
}

void App::RefreshPendingRoundRestartFlag() {
    pending_round_restart_ = HasPendingRoundChanges();
}

}  // namespace snake::core
