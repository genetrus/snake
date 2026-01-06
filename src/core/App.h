#pragma once

#include <SDL.h>

#include "core/Input.h"
#include "core/Time.h"
#include "audio/AudioSystem.h"
#include "game/Game.h"
#include "lua/LuaRuntime.h"
#include "io/Config.h"

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

private:
    void InitSDL();
    void CreateWindowAndRenderer();
    void ShutdownSDL();
    void RenderFrame();
    void ApplyConfig();
    void InitLua();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

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
    snake::io::Config config_;
    AppLuaContext lua_ctx_{};
    double last_ticks_per_sec_ = 10.0;
};
}  // namespace snake::core
