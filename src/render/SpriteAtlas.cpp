#include "render/SpriteAtlas.h"

#include <SDL_image.h>

#include <algorithm>

namespace snake::render {

SpriteAtlas::~SpriteAtlas() {
    ResetTexture();
}

void SpriteAtlas::ResetTexture() {
    if (texture_ != nullptr) {
        SDL_DestroyTexture(texture_);
        texture_ = nullptr;
    }
    w_ = 0;
    h_ = 0;
}

bool SpriteAtlas::Load(SDL_Renderer* r, const std::filesystem::path& png_path) {
    ResetTexture();

    SDL_Surface* surface = IMG_Load(png_path.string().c_str());
    if (surface == nullptr) {
        return false;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(r, surface);
    if (tex == nullptr) {
        SDL_FreeSurface(surface);
        return false;
    }

    w_ = surface->w;
    h_ = surface->h;
    SDL_FreeSurface(surface);

    texture_ = tex;

    // Predefine placeholder sprite rects (32x32 grid positions).
    const int tile = 32;
    auto define_grid = [&](const std::string& name, int idx) {
        const int cols = (w_ > 0) ? std::max(1, w_ / tile) : 8;
        const int x = (idx % cols) * tile;
        const int y = (idx / cols) * tile;
        Define(name, SDL_Rect{x, y, tile, tile});
    };

    define_grid("tile_empty", 0);
    define_grid("tile_wall", 1);
    define_grid("food", 2);
    define_grid("bonus_score", 3);
    define_grid("bonus_slow", 4);
    define_grid("snake_head_up", 5);
    define_grid("snake_head_down", 6);
    define_grid("snake_head_left", 7);
    define_grid("snake_head_right", 8);
    define_grid("snake_body", 9);
    define_grid("snake_turn", 10);
    define_grid("snake_tail", 11);

    return true;
}

void SpriteAtlas::SetTexture(SDL_Texture* tex) {
    ResetTexture();
    texture_ = tex;
    if (texture_ != nullptr) {
        SDL_QueryTexture(texture_, nullptr, nullptr, &w_, &h_);
    }
}

SDL_Texture* SpriteAtlas::Texture() const {
    return texture_;
}

void SpriteAtlas::Define(std::string name, SDL_Rect rect) {
    rects_[std::move(name)] = rect;
}

const SDL_Rect* SpriteAtlas::Get(std::string_view name) const {
    auto it = rects_.find(std::string(name));
    if (it != rects_.end()) {
        return &it->second;
    }
    return nullptr;
}

int SpriteAtlas::Width() const {
    return w_;
}

int SpriteAtlas::Height() const {
    return h_;
}

}  // namespace snake::render
