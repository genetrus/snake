#pragma once

#include <SDL.h>

#include <string>
#include <vector>
#include <filesystem>

#include "core/Input.h"
#include "core/Time.h"
#include "audio/AudioSystem.h"
#include "game/Game.h"
#include "game/StateMachine.h"
#include "lua/LuaRuntime.h"
#include "io/Config.h"
#include "io/Highscores.h"
#include "render/Renderer.h"

namespace snake::core {

struct AppLuaContext {
    snake::game::Game* game = nullptr;
    snake::audio::AudioSystem* audio = nullptr;
};

class App {
public:
    App();
    ~App();
    int Run();

    Input& GetInput();
    const Input& GetInput() const;
    bool IsFocused() const;
    SDL_Point GetWindowSize() const;
    bool WasResizedThisFrame() const;
    bool RecreateRenderer(bool want_vsync);

private:
    void InitSDL();
    void CreateWindowAndRenderer(bool want_vsync);
    void ShutdownSDL();
    void RenderFrame();
    void ApplyConfig();
    void InitLua();
    void HandleMenus(bool& running);
    void HandleOptionsInput();
    void ApplyAudioSettings();
    void ApplyControlSettings();
    void NotifySettingChanged(const std::string& key);
    void BeginRebind(const std::string& action, int slot);
    void HandleRebind();
    void PushUiMessage(std::string msg);
    void ApplyImmediateSettings(const snake::io::ConfigData& cfg);
    void ApplyRoundSettingsOnRestart();
    void SyncActiveWithPendingPreserveRound();
    bool HasPendingRoundChanges() const;
    void RefreshPendingRoundRestartFlag();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    bool vsync_enabled_ = false;

    int window_w_ = 800;
    int window_h_ = 800;
    bool window_resized_ = false;
    bool is_focused_ = true;
    bool sdl_initialized_ = false;

    Input input_;
    Time time_;
    snake::game::Game game_;
    snake::audio::AudioSystem audio_;
    snake::lua::LuaRuntime lua_;
    snake::game::StateMachine sm_;
    snake::io::Config pending_config_;
    snake::io::Config active_config_;
    snake::io::Highscores highscores_;
    std::string ui_message_;
    std::string lua_reload_error_;
    bool pending_round_restart_ = false;
    bool rebinding_ = false;
    std::string rebind_action_;
    int rebind_slot_ = 0;
    int menu_index_ = 0;
    int options_index_ = 0;
    std::vector<std::string> menu_items_;
    std::filesystem::path config_path_;

    snake::render::Renderer renderer_impl_;
    AppLuaContext lua_ctx_{};
    double last_base_ticks_per_sec_ = 10.0;
    std::string renderer_error_text_;
};
}  // namespace snake::core
