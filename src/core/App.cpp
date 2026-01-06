#include "core/App.h"

#include <SDL.h>

#include <stdexcept>

namespace snake::core {

namespace {
constexpr int kDefaultWindowW = 800;
constexpr int kDefaultWindowH = 800;
}  // namespace

App::App() = default;

App::~App() {
    ShutdownSDL();
}

int App::Run() {
    try {
        InitSDL();
        CreateWindowAndRenderer();

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

            SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
            SDL_RenderClear(renderer_);
            SDL_RenderPresent(renderer_);
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
}

void App::CreateWindowAndRenderer() {
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

    renderer_ = SDL_CreateRenderer(window_, -1, SDL_RENDERER_ACCELERATED);
    if (renderer_ == nullptr) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
        throw std::runtime_error(SDL_GetError());
    }

    SDL_GetWindowSize(window_, &window_w_, &window_h_);
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

}  // namespace snake::core
