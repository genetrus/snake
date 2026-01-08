#pragma once

#include <SDL_mixer.h>

#include <string>

namespace snake::audio {
struct AudioDiagnostics {
    int sdl_audio_init_result = 0;
    std::string sdl_audio_init_error;
    int mix_init_flags_requested = 0;
    int mix_init_result = 0;
    std::string mix_init_error;
    bool device_opened = false;
    int open_freq = 0;
    Uint16 open_format = 0;
    int open_channels = 0;
    int open_chunk_size = 0;
    int actual_freq = 0;
    Uint16 actual_format = 0;
    int actual_channels = 0;
    int allocated_channels = 0;
    std::string last_error;
    std::string working_dir;
};

struct AudioSpec {
    int freq = 0;
    Uint16 format = 0;
    int channels = 0;
};

class AudioSystem {
public:
    bool Init();            // Mix_OpenAudio, allocate channels
    void Shutdown();        // Mix_CloseAudio
    bool IsEnabled() const;
    void SetEnabled(bool enabled);  // user toggle

    // Volume: 0..128 (SDL_mixer scale); store as int
    void SetMasterVolume(int v);
    int MasterVolume() const;
    void SetSfxVolume(int v);
    int SfxVolume() const;

    // Apply volume to all channels/chunks (simple global approach)
    void ApplyVolume();
    const AudioDiagnostics& Diagnostics() const;
    AudioSpec Spec() const;
    void SetLastError(std::string error);

private:
    bool initialized_ = false;
    bool audio_enabled_ = true;
    int master_volume_ = 96; // default
    int sfx_volume_ = 96;    // default
    AudioDiagnostics diagnostics_{};
    AudioSpec spec_{};
};
}  // namespace snake::audio
