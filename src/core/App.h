#pragma once

#include <SDL.h>

#include <string>

#include "core/Input.h"
#include "core/Time.h"
#include "audio/AudioSystem.h"
#include "game/Game.h"
#include "lua/LuaRuntime.h"
#include "io/Config.h"
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
    snake::io::Config config_;
    snake::render::Renderer renderer_impl_;
    AppLuaContext lua_ctx_{};
    double last_ticks_per_sec_ = 10.0;
    std::string renderer_error_text_;
};
}  // namespace snake::core
