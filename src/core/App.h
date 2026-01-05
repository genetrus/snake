#pragma once

#include <SDL.h>

#include <string>

#include "audio/AudioSystem.h"
#include "audio/SFX.h"
#include "core/Input.h"
#include "core/Time.h"
#include "game/Game.h"
#include "render/Font.h"
#include "render/SpriteAtlas.h"
#include "render/UIRenderer.h"
#include "lua/LuaRuntime.h"
#include "lua/Bindings.h"

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

private:
    void InitSDL();
    void CreateWindowAndRenderer();
    void ShutdownSDL();

    void UpdateTick();
    void RenderFrame();

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    int window_w_ = 800;
    int window_h_ = 800;

    Time time_;
    Input input_;
    game::Game game_;
    audio::AudioSystem audio_;
    audio::SFX sfx_;
    AppLuaContext lua_ctx_;
    snake::lua::LuaRuntime lua_;
    std::string lua_error_;
    render::Font ui_font_;
    render::SpriteAtlas atlas_;
    render::UIRenderer ui_;

    bool sdl_initialized_ = false;
};
}  // namespace snake::core
