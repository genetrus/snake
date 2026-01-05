#include "core/App.h"

#include <SDL.h>

#include <stdexcept>

namespace snake::core {

App::App() {
    try {
        InitSDL();
        sdl_initialized_ = true;
        CreateWindowAndRenderer();
        time_.Init();
        game_.ResetAll();
    } catch (...) {
        ShutdownSDL();
        throw;
    }
}

App::~App() {
    if (sdl_initialized_) {
        ShutdownSDL();
    }
}

int App::Run() {
    try {
        bool running = true;
        while (running) {
            input_.BeginFrame();

            SDL_Event event;
            while (SDL_PollEvent(&event)) {
                input_.ProcessEvent(event);
            }

            game_.HandleInput(input_);

            if (game_.State() == snake::game::State::Menu && input_.KeyPressed(SDLK_ESCAPE)) {
                input_.RequestQuit();
            }

            time_.UpdateFrame();
            while (time_.ConsumeTick()) {
                UpdateTick();
                if (input_.QuitRequested()) {
                    running = false;
                    break;
                }
            }

            if (!running) {
                break;
            }

            RenderFrame();
            SDL_RenderPresent(renderer_);

            if (input_.QuitRequested()) {
                break;
            }
        }
    } catch (const std::exception&) {
        return 1;
    }

    return 0;
}

void App::InitSDL() {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        throw std::runtime_error(SDL_GetError());
    }
}

void App::CreateWindowAndRenderer() {
    window_ = SDL_CreateWindow(
        "Snake",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        800,
        800,
        SDL_WINDOW_SHOWN);

    if (window_ == nullptr) {
        throw std::runtime_error(SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (renderer_ == nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        throw std::runtime_error(SDL_GetError());
    }
}

void App::ShutdownSDL() {
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

void App::UpdateTick() {
    game_.Tick(time_.TickDt());
}

void App::RenderFrame() {
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
}

}  // namespace snake::core
