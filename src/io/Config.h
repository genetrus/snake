#pragma once

#include <SDL.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace snake::io {

enum class WallMode { Death, Wrap };

struct KeyBinds {
    SDL_Keycode up = SDLK_UP;
    SDL_Keycode down = SDLK_DOWN;
    SDL_Keycode left = SDLK_LEFT;
    SDL_Keycode right = SDLK_RIGHT;
    SDL_Keycode pause = SDLK_p;
    SDL_Keycode restart = SDLK_r;
    SDL_Keycode menu = SDLK_ESCAPE;
    SDL_Keycode confirm = SDLK_RETURN;
};

struct VideoConfig {
    int window_w = 800;
    int window_h = 800;
    int tile_px = 32;
    bool fullscreen_desktop = false;
    bool vsync = true;
};

struct GameConfig {
    int board_w = 20;
    int board_h = 20;
    WallMode walls = WallMode::Death;
    int food_score = 10;
    int bonus_score = 50;
    double slow_multiplier = 0.70;
    double slow_duration_sec = 6.0;
};

struct AudioConfig {
    bool enabled = true;
    int master_volume = 96;  // 0..128
};

struct UIConfig {
    std::string panel_mode = "auto";
};

struct ConfigData {
    std::string player_name = "Player";
    VideoConfig video;
    GameConfig game;
    AudioConfig audio;
    UIConfig ui;
    KeyBinds keys;
};

// Sanitizes a player name according to the allowed charset/length rules.
std::string SanitizePlayerName(std::string name);

class Config {
public:
    bool LoadFromFile(const std::filesystem::path& path);
    bool SaveToFile(const std::filesystem::path& path) const;  // atomic write

    const ConfigData& Data() const;
    ConfigData& Data();

    // Validation + normalization
    void Sanitize();

    // Helpers for UI: set values and auto-sanitize
    void SetWallMode(WallMode m);
    void SetBoardSize(int w, int h);
    void SetWindowSize(int w, int h);
    void SetTilePx(int px);
    void SetVsync(bool on);
    void SetFullscreenDesktop(bool on);
    void SetMasterVolume(int v);
    void SetPanelMode(std::string m);
    void SetPlayerName(std::string s);

    // Key binding operations (limited allowed set)
    bool IsAllowedKey(SDL_Keycode k) const;
    bool SetBind(std::string_view action, SDL_Keycode k);  // "up","down","left","right","pause","restart","menu","confirm"

private:
    ConfigData data_{};
};

}  // namespace snake::io
