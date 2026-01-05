#include "audio/AudioSystem.h"

#include <algorithm>
#include <SDL.h>

namespace snake::audio {

bool AudioSystem::Init() {
    enabled_ = false;

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
    enabled_ = true;
    ApplyVolume();
    return true;
}

void AudioSystem::Shutdown() {
    Mix_CloseAudio();
    Mix_Quit();
    enabled_ = false;
}

bool AudioSystem::IsEnabled() const {
    return enabled_;
}

void AudioSystem::SetMasterVolume(int v) {
    const int clamped = std::clamp(v, 0, 128);
    master_volume_ = clamped;
    ApplyVolume();
}

int AudioSystem::MasterVolume() const {
    return master_volume_;
}

void AudioSystem::ApplyVolume() {
    if (!enabled_) {
        return;
    }
    Mix_Volume(-1, master_volume_);
}

}  // namespace snake::audio
