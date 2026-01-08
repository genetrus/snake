#pragma once

#include <SDL_mixer.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

#include "audio/AudioSystem.h"

namespace snake::audio {

enum class SfxId { Eat, GameOver, MenuClick, PauseOn, PauseOff };

class SFX {
public:
    bool LoadAll(const std::filesystem::path& sounds_dir); // loads WAVs
    void Reset();

    void SetAudioSystem(AudioSystem* sys); // not owning
    void Play(SfxId id, std::string_view event_name);
    void ApplyVolume();
    int ExpectedCount() const;
    int LoadedCount() const;
    int FallbackCount() const;
    const std::string& LastError() const;
    const std::string& LastPlay() const;

private:
    AudioSystem* sys_ = nullptr;
    Mix_Chunk* eat_ = nullptr;
    Mix_Chunk* gameover_ = nullptr;
    Mix_Chunk* menuclick_ = nullptr;
    Mix_Chunk* pauseon_ = nullptr;
    Mix_Chunk* pauseoff_ = nullptr;

    Mix_Chunk* Get(SfxId id) const;
    static Mix_Chunk* LoadWav(const std::filesystem::path& p);
    Mix_Chunk* LoadOrFallback(SfxId id, const std::filesystem::path& p);
    Mix_Chunk* MakeFallbackBeep(SfxId id) const;
    static std::string SfxName(SfxId id);

    std::unordered_map<SfxId, std::filesystem::path> paths_;
    std::unordered_map<SfxId, bool> using_fallback_;
    int expected_count_ = 0;
    int loaded_count_ = 0;
    int fallback_count_ = 0;
    std::string last_error_;
    std::string last_play_;
};
}  // namespace snake::audio
