#pragma once

#include <SDL.h>

#include <cstdint>
#include <unordered_set>

namespace snake::core {
class Input {
public:
    void BeginFrame();
    void ProcessEvent(const SDL_Event& e);
    void RequestQuit();
    bool QuitRequested() const;

    bool KeyDown(SDL_Keycode key) const;
    bool KeyPressed(SDL_Keycode key) const;
    bool KeyReleased(SDL_Keycode key) const;

    int MouseX() const;
    int MouseY() const;
    bool MouseButtonDown(uint8_t button) const;

private:
    bool quit_requested_ = false;

    std::unordered_set<SDL_Keycode> keys_down_;
    std::unordered_set<SDL_Keycode> keys_pressed_;
    std::unordered_set<SDL_Keycode> keys_released_;

    int mouse_x_ = 0;
    int mouse_y_ = 0;
    std::unordered_set<uint8_t> mouse_buttons_down_;
};
}  // namespace snake::core
