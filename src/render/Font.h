#pragma once

#include <SDL.h>
#include <SDL_ttf.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace snake::render {

class Font {
public:
    ~Font();

    bool Load(const std::filesystem::path& ttf_path, int pt_size);
    void Reset();
    bool IsLoaded() const;
    bool MeasureText(std::string_view text, int* out_w, int* out_h) const;
    const std::string& LastError() const;
    const std::filesystem::path& FontPath() const;

    // Renders text to a texture (caller owns returned texture; provide helper for RAII usage)
    SDL_Texture* RenderText(SDL_Renderer* r, std::string_view text, SDL_Color color, int* out_w, int* out_h) const;

private:
    TTF_Font* font_ = nullptr;
    std::filesystem::path font_path_;
    mutable std::string last_error_;
};

}  // namespace snake::render
