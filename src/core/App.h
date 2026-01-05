#pragma once

#include <SDL.h>

#include "core/Input.h"
#include "core/Time.h"

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
    Time time_;
    Input input_;
    bool sdl_initialized_ = false;
};
}  // namespace snake::core
