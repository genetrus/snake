#pragma once

#include <SDL_mixer.h>

namespace snake::audio {
class AudioSystem {
public:
    bool Init();            // Mix_OpenAudio, allocate channels
    void Shutdown();        // Mix_CloseAudio
    bool IsEnabled() const;

    // Volume: 0..128 (SDL_mixer scale); store as int
    void SetMasterVolume(int v);
    int MasterVolume() const;

    // Apply volume to all channels/chunks (simple global approach)
    void ApplyVolume();

private:
    bool enabled_ = false;
    int master_volume_ = 96; // default
};
}  // namespace snake::audio
