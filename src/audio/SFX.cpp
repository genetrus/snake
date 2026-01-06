#include "audio/SFX.h"

#include <SDL.h>

#include <filesystem>
#include <unordered_map>

namespace snake::audio {

bool SFX::LoadAll(const std::filesystem::path& sounds_dir) {
    Reset();

    const std::unordered_map<SfxId, std::filesystem::path> files{
        {SfxId::Eat, sounds_dir / "eat.wav"},
        {SfxId::GameOver, sounds_dir / "gameover.wav"},
        {SfxId::MenuClick, sounds_dir / "menu_click.wav"},
        {SfxId::PauseOn, sounds_dir / "pause_on.wav"},
        {SfxId::PauseOff, sounds_dir / "pause_off.wav"},
    };

    bool any_loaded = false;
    for (const auto& [id, path] : files) {
        Mix_Chunk* chunk = LoadWav(path);
        switch (id) {
            case SfxId::Eat:
                eat_ = chunk;
                break;
            case SfxId::GameOver:
                gameover_ = chunk;
                break;
            case SfxId::MenuClick:
                menuclick_ = chunk;
                break;
            case SfxId::PauseOn:
                pauseon_ = chunk;
                break;
            case SfxId::PauseOff:
                pauseoff_ = chunk;
                break;
            default:
                break;
        }
        if (chunk != nullptr) {
            any_loaded = true;
        }
    }

    ApplyVolume();
    return any_loaded;
}

void SFX::Reset() {
    auto free_chunk = [](Mix_Chunk*& chunk) {
        if (chunk != nullptr) {
            Mix_FreeChunk(chunk);
            chunk = nullptr;
        }
    };

    free_chunk(eat_);
    free_chunk(gameover_);
    free_chunk(menuclick_);
    free_chunk(pauseon_);
    free_chunk(pauseoff_);
}

void SFX::SetAudioSystem(AudioSystem* sys) {
    sys_ = sys;
    ApplyVolume();
}

void SFX::Play(SfxId id) {
    if (sys_ == nullptr || !sys_->IsEnabled()) {
        return;
    }

    Mix_Chunk* chunk = Get(id);
    if (chunk == nullptr) {
        return;
    }

    Mix_VolumeChunk(chunk, sys_->SfxVolume());
    Mix_PlayChannel(-1, chunk, 0);
}

void SFX::ApplyVolume() {
    if (sys_ == nullptr || !sys_->IsEnabled()) {
        return;
    }

    const int volume = sys_->SfxVolume();
    auto apply = [volume](Mix_Chunk* chunk) {
        if (chunk != nullptr) {
            Mix_VolumeChunk(chunk, volume);
        }
    };

    apply(eat_);
    apply(gameover_);
    apply(menuclick_);
    apply(pauseon_);
    apply(pauseoff_);
}

Mix_Chunk* SFX::Get(SfxId id) const {
    switch (id) {
        case SfxId::Eat:
            return eat_;
        case SfxId::GameOver:
            return gameover_;
        case SfxId::MenuClick:
            return menuclick_;
        case SfxId::PauseOn:
            return pauseon_;
        case SfxId::PauseOff:
            return pauseoff_;
        default:
            break;
    }
    return nullptr;
}

Mix_Chunk* SFX::LoadWav(const std::filesystem::path& p) {
    if (!std::filesystem::exists(p)) {
        SDL_Log("SFX missing: %s", p.string().c_str());
        return nullptr;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(p.string().c_str());
    if (chunk == nullptr) {
        SDL_Log("Failed to load WAV %s: %s", p.string().c_str(), Mix_GetError());
    }
    return chunk;
}

}  // namespace snake::audio
