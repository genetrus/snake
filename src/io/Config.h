#pragma once

#include <SDL.h>

#include <filesystem>
#include <string>
#include <string_view>

namespace snake::io {

enum class WallMode { Death, Wrap };

struct KeyPair {
    SDL_Keycode primary = SDLK_UNKNOWN;
    SDL_Keycode secondary = SDLK_UNKNOWN;
};

struct KeyBinds {
    KeyPair up{SDLK_UP, SDLK_w};
    KeyPair down{SDLK_DOWN, SDLK_s};
    KeyPair left{SDLK_LEFT, SDLK_a};
    KeyPair right{SDLK_RIGHT, SDLK_d};
    KeyPair pause{SDLK_p, SDLK_p};
    KeyPair restart{SDLK_r, SDLK_r};
    KeyPair menu{SDLK_ESCAPE, SDLK_ESCAPE};
    KeyPair confirm{SDLK_RETURN, SDLK_RETURN};
};

struct WindowConfig {
    int width = 800;
    int height = 800;
    bool fullscreen_desktop = false;
    bool vsync = true;
};

struct GridConfig {
    int board_w = 20;
    int board_h = 20;
    int tile_size = 32;
    bool wrap_mode = false;
};

struct AudioConfig {
    bool enabled = true;
    int master_volume = 96;  // 0..128
    int sfx_volume = 96;     // 0..128
};

struct UIConfig {
    std::string panel_mode = "auto";
};

struct GameplayConfig {
    int food_score = 10;
    int bonus_score = 50;
    double slow_multiplier = 0.70;
    double slow_duration_sec = 6.0;
    int max_simultaneous_bonuses = 2;
    bool always_one_food = true;
    int bonus_score_score = 50;
};

struct ConfigData {
    std::string player_name = "Player";
    WindowConfig window;
    GridConfig grid;
    AudioConfig audio;
    UIConfig ui;
    GameplayConfig gameplay;
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
    void SetAudioEnabled(bool enabled);
    void SetSfxVolume(int v);

    // Key binding operations (limited allowed set)
    bool IsAllowedKey(SDL_Keycode k) const;
    bool SetBind(std::string_view action, SDL_Keycode k, int slot);  // slot: 0 or 1

    static std::string KeycodeToToken(SDL_Keycode key);
private:
    ConfigData data_{};
};

}  // namespace snake::io
