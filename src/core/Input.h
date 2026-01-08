#pragma once

#include <SDL.h>

#include <array>
#include <cstdint>
#include <vector>

namespace snake::core {
class Input {
public:
    void BeginFrame();
    void HandleEvent(const SDL_Event& e);
    void ProcessEvent(const SDL_Event& e) { HandleEvent(e); }

    void RequestQuit();
    bool QuitRequested() const;

    bool IsDown(SDL_Scancode scancode) const;
    bool WasPressed(SDL_Scancode scancode) const;
    bool WasReleased(SDL_Scancode scancode) const;

    bool KeyDown(SDL_Keycode key) const;
    bool KeyPressed(SDL_Keycode key) const;
    bool KeyReleased(SDL_Keycode key) const;
    const std::vector<SDL_Keycode>& KeyPresses() const;

    int MouseX() const;
    int MouseY() const;
    int MouseDeltaX() const;
    int MouseDeltaY() const;
    int MouseWheelY() const;

    bool MouseButtonDown(uint8_t button) const;
    bool MouseButtonPressed(uint8_t button) const;
    bool MouseButtonReleased(uint8_t button) const;

private:
    static constexpr std::size_t kMouseButtons = 8;

    bool ValidMouseButton(uint8_t button) const;

    bool quit_requested_ = false;

    std::array<bool, SDL_NUM_SCANCODES> keys_down_{};
    std::array<bool, SDL_NUM_SCANCODES> keys_pressed_{};
    std::array<bool, SDL_NUM_SCANCODES> keys_released_{};
    std::vector<SDL_Keycode> key_presses_;

    int mouse_x_ = 0;
    int mouse_y_ = 0;
    int mouse_dx_ = 0;
    int mouse_dy_ = 0;
    int mouse_wheel_y_ = 0;
    std::array<bool, kMouseButtons> mouse_buttons_down_{};
    std::array<bool, kMouseButtons> mouse_buttons_pressed_{};
    std::array<bool, kMouseButtons> mouse_buttons_released_{};
};
}  // namespace snake::core
