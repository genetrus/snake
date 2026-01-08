#include "audio/SFX.h"

#include <SDL.h>

#include <cmath>
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

    expected_count_ = static_cast<int>(files.size());
    loaded_count_ = 0;
    fallback_count_ = 0;
    paths_ = files;
    using_fallback_.clear();
    SDL_Log("SFX: resolving sounds under %s", sounds_dir.string().c_str());

    for (const auto& [id, path] : files) {
        SDL_Log("SFX: resolved %s -> %s", SfxName(id).c_str(), path.string().c_str());
        Mix_Chunk* chunk = LoadOrFallback(id, path);
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
    }

    ApplyVolume();
    return loaded_count_ > 0 || fallback_count_ > 0;
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
    paths_.clear();
    using_fallback_.clear();
    expected_count_ = 0;
    loaded_count_ = 0;
    fallback_count_ = 0;
    last_error_.clear();
    last_play_.clear();
}

void SFX::SetAudioSystem(AudioSystem* sys) {
    sys_ = sys;
    ApplyVolume();
}

void SFX::Play(SfxId id, std::string_view event_name) {
    const std::string name = SfxName(id);
    const auto path_it = paths_.find(id);
    const std::string path = (path_it != paths_.end()) ? path_it->second.string() : std::string("unknown");
    SDL_Log("SFX: event '%.*s' -> %s (%s)", static_cast<int>(event_name.size()), event_name.data(), name.c_str(), path.c_str());
    if (sys_ == nullptr || !sys_->IsEnabled()) {
        last_play_ = name + " -> muted/disabled";
        return;
    }

    Mix_Chunk* chunk = Get(id);
    if (chunk == nullptr) {
        last_play_ = name + " -> no chunk";
        return;
    }

    Mix_VolumeChunk(chunk, sys_->SfxVolume());
    const int channel = Mix_PlayChannel(-1, chunk, 0);
    if (channel == -1) {
        const std::string error = Mix_GetError();
        SDL_Log("SFX: Mix_PlayChannel failed for %s: %s", name.c_str(), error.c_str());
        last_error_ = error;
        if (sys_ != nullptr) {
            sys_->SetLastError(error);
        }
        last_play_ = name + " -> play failed";
    } else {
        SDL_Log("SFX: Mix_PlayChannel ok for %s: channel=%d", name.c_str(), channel);
        last_play_ = name + " -> channel " + std::to_string(channel);
    }
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
        SDL_Log("SFX missing file: %s", p.string().c_str());
        return nullptr;
    }

    Mix_Chunk* chunk = Mix_LoadWAV(p.string().c_str());
    if (chunk == nullptr) {
        SDL_Log("SFX: failed to load WAV %s: %s", p.string().c_str(), Mix_GetError());
    }
    return chunk;
}

Mix_Chunk* SFX::LoadOrFallback(SfxId id, const std::filesystem::path& p) {
    Mix_Chunk* chunk = LoadWav(p);
    if (chunk != nullptr) {
        using_fallback_[id] = false;
        ++loaded_count_;
        return chunk;
    }

    if (!std::filesystem::exists(p)) {
        last_error_ = "missing file: " + p.string();
        if (sys_ != nullptr) {
            sys_->SetLastError(last_error_);
        }
    } else {
        const std::string error = Mix_GetError();
        if (!error.empty()) {
            last_error_ = error;
            if (sys_ != nullptr) {
                sys_->SetLastError(error);
            }
        }
    }

    chunk = MakeFallbackBeep(id);
    if (chunk != nullptr) {
        using_fallback_[id] = true;
        ++fallback_count_;
        SDL_Log("SFX: using fallback beep for %s", SfxName(id).c_str());
    }
    return chunk;
}

Mix_Chunk* SFX::MakeFallbackBeep(SfxId id) const {
    if (sys_ == nullptr) {
        SDL_Log("SFX: no audio system for fallback beep");
        return nullptr;
    }
    const AudioSpec spec = sys_->Spec();
    if (spec.freq <= 0 || spec.channels <= 0) {
        SDL_Log("SFX: no audio spec available for fallback beep");
        return nullptr;
    }
    if ((spec.format != AUDIO_S16SYS) && (spec.format != AUDIO_S16LSB) && (spec.format != AUDIO_S16MSB)) {
        SDL_Log("SFX: unsupported audio format for fallback beep (0x%x)", spec.format);
        return nullptr;
    }

    int frequency = 440;
    switch (id) {
        case SfxId::MenuClick:
            frequency = 880;
            break;
        case SfxId::Eat:
            frequency = 660;
            break;
        case SfxId::GameOver:
            frequency = 220;
            break;
        case SfxId::PauseOn:
            frequency = 520;
            break;
        case SfxId::PauseOff:
            frequency = 420;
            break;
        default:
            break;
    }

    const double duration_sec = 0.18;
    const int frames = static_cast<int>(spec.freq * duration_sec);
    const int samples = frames * spec.channels;
    const int bytes = samples * static_cast<int>(sizeof(int16_t));
    auto* buffer = static_cast<Uint8*>(SDL_malloc(bytes));
    if (buffer == nullptr) {
        SDL_Log("SFX: failed to allocate fallback beep buffer");
        return nullptr;
    }

    const double amplitude = 0.4 * 32767.0;
    constexpr double kPi = 3.14159265358979323846;
    auto* pcm = reinterpret_cast<int16_t*>(buffer);
    for (int i = 0; i < frames; ++i) {
        const double t = static_cast<double>(i) / static_cast<double>(spec.freq);
        const double sample = std::sin(2.0 * kPi * frequency * t) * amplitude;
        const auto value = static_cast<int16_t>(sample);
        for (int ch = 0; ch < spec.channels; ++ch) {
            pcm[i * spec.channels + ch] = value;
        }
    }

    auto* chunk = static_cast<Mix_Chunk*>(SDL_malloc(sizeof(Mix_Chunk)));
    if (chunk == nullptr) {
        SDL_free(buffer);
        SDL_Log("SFX: failed to allocate fallback Mix_Chunk");
        return nullptr;
    }
    chunk->allocated = 1;
    chunk->abuf = buffer;
    chunk->alen = static_cast<Uint32>(bytes);
    chunk->volume = MIX_MAX_VOLUME;
    return chunk;
}

std::string SFX::SfxName(SfxId id) {
    switch (id) {
        case SfxId::Eat:
            return "eat";
        case SfxId::GameOver:
            return "game_over";
        case SfxId::MenuClick:
            return "menu_click";
        case SfxId::PauseOn:
            return "pause_on";
        case SfxId::PauseOff:
            return "pause_off";
        default:
            break;
    }
    return "unknown";
}

int SFX::ExpectedCount() const {
    return expected_count_;
}

int SFX::LoadedCount() const {
    return loaded_count_;
}

int SFX::FallbackCount() const {
    return fallback_count_;
}

const std::string& SFX::LastError() const {
    return last_error_;
}

const std::string& SFX::LastPlay() const {
    return last_play_;
}

}  // namespace snake::audio
