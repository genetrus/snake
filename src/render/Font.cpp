#include "render/Font.h"

#include <string>

namespace snake::render {

Font::~Font() {
    Reset();
}

bool Font::Load(const std::filesystem::path& ttf_path, int pt_size) {
    Reset();
    font_path_ = ttf_path;
    const std::string path = ttf_path.string();
    font_ = TTF_OpenFont(path.c_str(), pt_size);
    if (font_ == nullptr) {
        last_error_ = TTF_GetError();
        SDL_Log("TTF_OpenFont failed for '%s': %s", path.c_str(), last_error_.c_str());
        return false;
    }
    last_error_.clear();
    SDL_Log("Loaded font: %s", path.c_str());
    return true;
}

void Font::Reset() {
    if (font_ != nullptr) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    last_error_.clear();
    font_path_.clear();
}

bool Font::IsLoaded() const {
    return font_ != nullptr;
}

bool Font::MeasureText(std::string_view text, int* out_w, int* out_h) const {
    if (font_ == nullptr) {
        last_error_ = "TTF font not loaded";
        return false;
    }

    int w = 0;
    int h = 0;
    if (TTF_SizeUTF8(font_, std::string(text).c_str(), &w, &h) != 0) {
        last_error_ = TTF_GetError();
        SDL_Log("TTF_SizeUTF8 failed for '%s' (font: %s): %s",
                std::string(text).c_str(),
                font_path_.string().c_str(),
                last_error_.c_str());
        return false;
    }

    if (out_w) {
        *out_w = w;
    }
    if (out_h) {
        *out_h = h;
    }
    return true;
}

const std::string& Font::LastError() const {
    return last_error_;
}

const std::filesystem::path& Font::FontPath() const {
    return font_path_;
}

SDL_Texture* Font::RenderText(SDL_Renderer* r, std::string_view text, SDL_Color color, int* out_w, int* out_h) const {
    if (font_ == nullptr) {
        last_error_ = "TTF font not loaded";
        return nullptr;
    }

    SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, std::string(text).c_str(), color);
    if (surface == nullptr) {
        last_error_ = TTF_GetError();
        SDL_Log("TTF_RenderUTF8_Blended failed for '%s' (font: %s): %s",
                std::string(text).c_str(),
                font_path_.string().c_str(),
                last_error_.c_str());
        return nullptr;
    }

    if (out_w) {
        *out_w = surface->w;
    }
    if (out_h) {
        *out_h = surface->h;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surface);
    if (tex == nullptr) {
        last_error_ = SDL_GetError();
        SDL_Log("SDL_CreateTextureFromSurface failed for '%s' (font: %s): %s",
                std::string(text).c_str(),
                font_path_.string().c_str(),
                last_error_.c_str());
    }
    SDL_FreeSurface(surface);
    return tex;
}

}  // namespace snake::render
