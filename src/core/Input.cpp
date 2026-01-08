#include "core/Input.h"

namespace snake::core {

void Input::BeginFrame() {
    keys_pressed_.fill(false);
    keys_released_.fill(false);
    mouse_buttons_pressed_.fill(false);
    mouse_buttons_released_.fill(false);
    key_presses_.clear();
    mouse_dx_ = 0;
    mouse_dy_ = 0;
    mouse_wheel_y_ = 0;
}

void Input::HandleEvent(const SDL_Event& e) {
    switch (e.type) {
        case SDL_QUIT:
            quit_requested_ = true;
            break;
        case SDL_KEYDOWN: {
            const SDL_Scancode scancode = e.key.keysym.scancode;
            if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
                keys_down_[scancode] = true;
                if (e.key.repeat == 0) {
                    keys_pressed_[scancode] = true;
                    key_presses_.push_back(e.key.keysym.sym);
                }
            }
            break;
        }
        case SDL_KEYUP: {
            const SDL_Scancode scancode = e.key.keysym.scancode;
            if (scancode >= 0 && scancode < SDL_NUM_SCANCODES) {
                keys_down_[scancode] = false;
                keys_released_[scancode] = true;
            }
            break;
        }
        case SDL_MOUSEMOTION:
            mouse_x_ = e.motion.x;
            mouse_y_ = e.motion.y;
            mouse_dx_ += e.motion.xrel;
            mouse_dy_ += e.motion.yrel;
            break;
        case SDL_MOUSEBUTTONDOWN: {
            const uint8_t button = e.button.button;
            if (ValidMouseButton(button)) {
                const std::size_t idx = static_cast<std::size_t>(button - 1);
                mouse_buttons_down_[idx] = true;
                mouse_buttons_pressed_[idx] = true;
            }
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            const uint8_t button = e.button.button;
            if (ValidMouseButton(button)) {
                const std::size_t idx = static_cast<std::size_t>(button - 1);
                mouse_buttons_down_[idx] = false;
                mouse_buttons_released_[idx] = true;
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            mouse_wheel_y_ += e.wheel.y;
            break;
        }
        default:
            break;
    }
}

void Input::RequestQuit() {
    quit_requested_ = true;
}

bool Input::QuitRequested() const {
    return quit_requested_;
}

bool Input::IsDown(SDL_Scancode scancode) const {
    return scancode >= 0 && scancode < SDL_NUM_SCANCODES && keys_down_[scancode];
}

bool Input::WasPressed(SDL_Scancode scancode) const {
    return scancode >= 0 && scancode < SDL_NUM_SCANCODES && keys_pressed_[scancode];
}

bool Input::WasReleased(SDL_Scancode scancode) const {
    return scancode >= 0 && scancode < SDL_NUM_SCANCODES && keys_released_[scancode];
}

bool Input::KeyDown(SDL_Keycode key) const {
    return IsDown(SDL_GetScancodeFromKey(key));
}

bool Input::KeyPressed(SDL_Keycode key) const {
    return WasPressed(SDL_GetScancodeFromKey(key));
}

bool Input::KeyReleased(SDL_Keycode key) const {
    return WasReleased(SDL_GetScancodeFromKey(key));
}

const std::vector<SDL_Keycode>& Input::KeyPresses() const {
    return key_presses_;
}

int Input::MouseX() const {
    return mouse_x_;
}

int Input::MouseY() const {
    return mouse_y_;
}

int Input::MouseDeltaX() const {
    return mouse_dx_;
}

int Input::MouseDeltaY() const {
    return mouse_dy_;
}

int Input::MouseWheelY() const {
    return mouse_wheel_y_;
}

bool Input::MouseButtonDown(uint8_t button) const {
    if (!ValidMouseButton(button)) {
        return false;
    }
    return mouse_buttons_down_[static_cast<std::size_t>(button - 1)];
}

bool Input::MouseButtonPressed(uint8_t button) const {
    if (!ValidMouseButton(button)) {
        return false;
    }
    return mouse_buttons_pressed_[static_cast<std::size_t>(button - 1)];
}

bool Input::MouseButtonReleased(uint8_t button) const {
    if (!ValidMouseButton(button)) {
        return false;
    }
    return mouse_buttons_released_[static_cast<std::size_t>(button - 1)];
}

bool Input::ValidMouseButton(uint8_t button) const {
    return button > 0 && button <= kMouseButtons;
}

}  // namespace snake::core
