#include "render/Font.h"

#include <string>

namespace snake::render {

Font::~Font() {
    Reset();
}

bool Font::Load(const std::filesystem::path& ttf_path, int pt_size) {
    Reset();
    font_ = TTF_OpenFont(ttf_path.string().c_str(), pt_size);
    return font_ != nullptr;
}

void Font::Reset() {
    if (font_ != nullptr) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
}

bool Font::IsLoaded() const {
    return font_ != nullptr;
}

SDL_Texture* Font::RenderText(SDL_Renderer* r, std::string_view text, SDL_Color color, int* out_w, int* out_h) const {
    if (font_ == nullptr) {
        return nullptr;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, std::string(text).c_str(), color);
    if (surface == nullptr) {
        return nullptr;
    }

    if (out_w) {
        *out_w = surface->w;
    }
    if (out_h) {
        *out_h = surface->h;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surface);
    SDL_FreeSurface(surface);
    return tex;
}

}  // namespace snake::render
