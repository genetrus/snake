#pragma once

#include <SDL.h>

#include "core/Input.h"

namespace snake::core {

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

    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;

    int window_w_ = 800;
    int window_h_ = 800;
    bool window_resized_ = false;
    bool is_focused_ = true;
    bool sdl_initialized_ = false;

    Input input_;
};
}  // namespace snake::core
