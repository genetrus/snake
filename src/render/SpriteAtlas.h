#pragma once

#include <SDL.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

namespace snake::render {

class SpriteAtlas {
public:
    ~SpriteAtlas();

    bool Load(SDL_Renderer* r, const std::filesystem::path& png_path);
    void SetTexture(SDL_Texture* tex);  // optional internal
    SDL_Texture* Texture() const;

    void Define(std::string name, SDL_Rect rect);
    const SDL_Rect* Get(std::string_view name) const;

    int Width() const;
    int Height() const;

private:
    void ResetTexture();

    SDL_Texture* texture_ = nullptr;  // owned
    int w_ = 0;
    int h_ = 0;
    std::unordered_map<std::string, SDL_Rect> rects_;
};

}  // namespace snake::render
