#include "core/Input.h"

namespace snake::core {

void Input::BeginFrame() {
    keys_pressed_.clear();
    keys_released_.clear();
}

void Input::ProcessEvent(const SDL_Event& e) {
    switch (e.type) {
        case SDL_QUIT:
            quit_requested_ = true;
            break;
        case SDL_KEYDOWN: {
            const SDL_Keycode key = e.key.keysym.sym;
            if (key == SDLK_ESCAPE) {
                quit_requested_ = true;
            }

            if (e.key.repeat == 0) {
                keys_down_.insert(key);
                keys_pressed_.insert(key);
            }
            break;
        }
        case SDL_KEYUP: {
            const SDL_Keycode key = e.key.keysym.sym;
            keys_down_.erase(key);
            keys_released_.insert(key);
            break;
        }
        case SDL_MOUSEMOTION:
            mouse_x_ = e.motion.x;
            mouse_y_ = e.motion.y;
            break;
        case SDL_MOUSEBUTTONDOWN: {
            const uint8_t button = e.button.button;
            mouse_buttons_down_.insert(button);
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            const uint8_t button = e.button.button;
            mouse_buttons_down_.erase(button);
            break;
        }
        default:
            break;
    }
}

bool Input::QuitRequested() const {
    return quit_requested_;
}

bool Input::KeyDown(SDL_Keycode key) const {
    return keys_down_.find(key) != keys_down_.end();
}

bool Input::KeyPressed(SDL_Keycode key) const {
    return keys_pressed_.find(key) != keys_pressed_.end();
}

bool Input::KeyReleased(SDL_Keycode key) const {
    return keys_released_.find(key) != keys_released_.end();
}

int Input::MouseX() const {
    return mouse_x_;
}

int Input::MouseY() const {
    return mouse_y_;
}

bool Input::MouseButtonDown(uint8_t button) const {
    return mouse_buttons_down_.find(button) != mouse_buttons_down_.end();
}

}  // namespace snake::core
