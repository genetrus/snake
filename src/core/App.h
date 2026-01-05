#pragma once

#include <SDL.h>

#include "audio/AudioSystem.h"
#include "audio/SFX.h"
#include "core/Input.h"
#include "core/Time.h"
#include "game/Game.h"
#include "render/Font.h"
#include "render/SpriteAtlas.h"
#include "render/UIRenderer.h"

namespace snake::core {
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
    render::Font ui_font_;
    render::SpriteAtlas atlas_;
    render::UIRenderer ui_;

    bool sdl_initialized_ = false;
};
}  // namespace snake::core
