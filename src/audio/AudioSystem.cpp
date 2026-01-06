#include "audio/AudioSystem.h"

#include <algorithm>
#include <SDL.h>

namespace snake::audio {

bool AudioSystem::Init() {
    initialized_ = false;

    if (Mix_Init(0) < 0) {
        SDL_Log("Mix_Init failed: %s", Mix_GetError());
        return false;
    }

    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
        SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
        Mix_Quit();
        return false;
    }

    Mix_AllocateChannels(16);
    initialized_ = true;
    audio_enabled_ = true;
    ApplyVolume();
    return true;
}

void AudioSystem::Shutdown() {
    Mix_CloseAudio();
    Mix_Quit();
    initialized_ = false;
}

bool AudioSystem::IsEnabled() const {
    return initialized_ && audio_enabled_;
}

void AudioSystem::SetEnabled(bool enabled) {
    audio_enabled_ = enabled;
    ApplyVolume();
}

void AudioSystem::SetMasterVolume(int v) {
    const int clamped = std::clamp(v, 0, 128);
    master_volume_ = clamped;
    ApplyVolume();
}

int AudioSystem::MasterVolume() const {
    return master_volume_;
}

void AudioSystem::SetSfxVolume(int v) {
    const int clamped = std::clamp(v, 0, 128);
    sfx_volume_ = clamped;
    ApplyVolume();
}

int AudioSystem::SfxVolume() const {
    return sfx_volume_;
}

void AudioSystem::ApplyVolume() {
    if (!initialized_) {
        return;
    }
    const int channel_volume = audio_enabled_ ? master_volume_ : 0;
    Mix_Volume(-1, channel_volume);
}

}  // namespace snake::audio
