#pragma once

#include <SDL_mixer.h>

#include <filesystem>

#include "audio/AudioSystem.h"

namespace snake::audio {

enum class SfxId { Eat, GameOver, MenuClick, PauseOn, PauseOff };

class SFX {
public:
    bool LoadAll(const std::filesystem::path& sounds_dir); // loads WAVs
    void Reset();

    void SetAudioSystem(AudioSystem* sys); // not owning
    void Play(SfxId id);
    void ApplyVolume();

private:
    AudioSystem* sys_ = nullptr;
    Mix_Chunk* eat_ = nullptr;
    Mix_Chunk* gameover_ = nullptr;
    Mix_Chunk* menuclick_ = nullptr;
    Mix_Chunk* pauseon_ = nullptr;
    Mix_Chunk* pauseoff_ = nullptr;

    Mix_Chunk* Get(SfxId id) const;
    static Mix_Chunk* LoadWav(const std::filesystem::path& p);
};
}  // namespace snake::audio
