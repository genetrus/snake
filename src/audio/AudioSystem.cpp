#include "audio/AudioSystem.h"

#include <algorithm>
#include <filesystem>
#include <SDL.h>

namespace snake::audio {

bool AudioSystem::Init() {
    initialized_ = false;
    diagnostics_ = {};
    spec_ = {};

    diagnostics_.working_dir = std::filesystem::current_path().string();
    SDL_Log("Audio: working dir: %s", diagnostics_.working_dir.c_str());

    diagnostics_.sdl_audio_init_result = SDL_InitSubSystem(SDL_INIT_AUDIO);
    if (diagnostics_.sdl_audio_init_result != 0) {
        diagnostics_.sdl_audio_init_error = SDL_GetError();
        diagnostics_.last_error = diagnostics_.sdl_audio_init_error;
        SDL_Log("Audio: SDL_InitSubSystem(SDL_INIT_AUDIO) failed: %s", diagnostics_.sdl_audio_init_error.c_str());
    } else {
        SDL_Log("Audio: SDL_InitSubSystem(SDL_INIT_AUDIO) ok");
    }

    diagnostics_.mix_init_flags_requested = 0;
    diagnostics_.mix_init_result = Mix_Init(diagnostics_.mix_init_flags_requested);
    if (diagnostics_.mix_init_result < 0) {
        diagnostics_.mix_init_error = Mix_GetError();
        diagnostics_.last_error = diagnostics_.mix_init_error;
        SDL_Log("Audio: Mix_Init(0) failed: %s", diagnostics_.mix_init_error.c_str());
        return false;
    }
    SDL_Log("Audio: Mix_Init flags requested=%d result=%d", diagnostics_.mix_init_flags_requested, diagnostics_.mix_init_result);

    diagnostics_.open_freq = 44100;
    diagnostics_.open_format = MIX_DEFAULT_FORMAT;
    diagnostics_.open_channels = 2;
    diagnostics_.open_chunk_size = 2048;
    SDL_Log("Audio: Mix_OpenAudio freq=%d format=0x%x channels=%d chunk=%d",
            diagnostics_.open_freq,
            diagnostics_.open_format,
            diagnostics_.open_channels,
            diagnostics_.open_chunk_size);
    if (Mix_OpenAudio(diagnostics_.open_freq,
                      diagnostics_.open_format,
                      diagnostics_.open_channels,
                      diagnostics_.open_chunk_size) != 0) {
        diagnostics_.last_error = Mix_GetError();
        SDL_Log("Audio: Mix_OpenAudio failed: %s", diagnostics_.last_error.c_str());
        Mix_Quit();
        return false;
    }

    diagnostics_.device_opened = true;
    const int query_result = Mix_QuerySpec(&diagnostics_.actual_freq, &diagnostics_.actual_format, &diagnostics_.actual_channels);
    if (query_result == 0) {
        diagnostics_.last_error = Mix_GetError();
        SDL_Log("Audio: Mix_QuerySpec failed: %s", diagnostics_.last_error.c_str());
    } else {
        SDL_Log("Audio: Mix_QuerySpec freq=%d format=0x%x channels=%d",
                diagnostics_.actual_freq,
                diagnostics_.actual_format,
                diagnostics_.actual_channels);
        spec_.freq = diagnostics_.actual_freq;
        spec_.format = diagnostics_.actual_format;
        spec_.channels = diagnostics_.actual_channels;
    }

    diagnostics_.allocated_channels = Mix_AllocateChannels(16);
    SDL_Log("Audio: Mix_AllocateChannels=%d", diagnostics_.allocated_channels);
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

const AudioDiagnostics& AudioSystem::Diagnostics() const {
    return diagnostics_;
}

AudioSpec AudioSystem::Spec() const {
    return spec_;
}

void AudioSystem::SetLastError(std::string error) {
    diagnostics_.last_error = std::move(error);
}

}  // namespace snake::audio
