#include "io/Config.h"

#include <lua.hpp>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>

namespace snake::io {

namespace {

constexpr int kMinBoardSize = 5;
constexpr int kMaxBoardSize = 60;
constexpr int kMinTilePx = 8;
constexpr int kMaxTilePx = 128;
constexpr int kMinWindow = 320;
constexpr int kMaxWindow = 3840;

const KeyBinds& DefaultKeybinds() {
    static const KeyBinds kDefaults{};
    return kDefaults;
}

std::string NormalizePanelMode(std::string m) {
    if (m == "top" || m == "right" || m == "auto") return m;
    return "auto";
}

using KeyMap = std::unordered_map<std::string_view, SDL_Keycode>;

const std::vector<std::pair<std::string_view, SDL_Keycode>>& AllowedKeyList() {
    static const std::vector<std::pair<std::string_view, SDL_Keycode>> list{
        {"UP", SDLK_UP},         {"DOWN", SDLK_DOWN}, {"LEFT", SDLK_LEFT}, {"RIGHT", SDLK_RIGHT},
        {"W", SDLK_w},           {"A", SDLK_a},       {"S", SDLK_s},       {"D", SDLK_d},
        {"ENTER", SDLK_RETURN},  {"RETURN", SDLK_RETURN},
        {"ESCAPE", SDLK_ESCAPE}, {"P", SDLK_p},       {"R", SDLK_r},
    };
    return list;
}

const KeyMap& AllowedKeyMap() {
    static const KeyMap map = [] {
        KeyMap m;
        for (const auto& kv : AllowedKeyList()) {
            m.emplace(kv.first, kv.second);
        }
        return m;
    }();
    return map;
}

std::string EscapeLuaString(std::string_view s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '\\':
            case '\"':
                out.push_back('\\');
                out.push_back(c);
                break;
            case '\n':
                out.append("\\n");
                break;
            case '\r':
                out.append("\\r");
                break;
            case '\t':
                out.append("\\t");
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

std::string KeycodeToString(SDL_Keycode key) {
    for (const auto& [name, code] : AllowedKeyList()) {
        if (code == key) {
            return std::string(name);
        }
    }
    return {};
}

bool StringToKeycode(std::string_view s, SDL_Keycode* out) {
    const auto& map = AllowedKeyMap();
    const auto it = map.find(s);
    if (it == map.end()) {
        return false;
    }
    if (out) {
        *out = it->second;
    }
    return true;
}

bool LoadBoolField(lua_State* L, const char* key, bool* out) {
    lua_getfield(L, -1, key);
    const bool is_bool = lua_isboolean(L, -1);
    if (is_bool && out) {
        *out = lua_toboolean(L, -1) != 0;
    }
    lua_pop(L, 1);
    return is_bool;
}

bool LoadIntField(lua_State* L, const char* key, int* out) {
    lua_getfield(L, -1, key);
    const bool is_int = lua_isinteger(L, -1);
    if (is_int && out) {
        *out = static_cast<int>(lua_tointeger(L, -1));
    }
    lua_pop(L, 1);
    return is_int;
}

bool LoadNumberField(lua_State* L, const char* key, double* out) {
    lua_getfield(L, -1, key);
    const bool is_num = lua_isnumber(L, -1);
    if (is_num && out) {
        *out = lua_tonumber(L, -1);
    }
    lua_pop(L, 1);
    return is_num;
}

bool LoadStringField(lua_State* L, const char* key, std::string* out) {
    lua_getfield(L, -1, key);
    const bool is_str = lua_isstring(L, -1);
    if (is_str && out) {
        out->assign(lua_tostring(L, -1));
    }
    lua_pop(L, 1);
    return is_str;
}

bool LoadKeyPair(lua_State* L, const char* key, KeyPair* out) {
    lua_getfield(L, -1, key);
    const bool is_table = lua_istable(L, -1);
    const bool is_string = lua_isstring(L, -1);
    bool loaded = false;
    SDL_Keycode primary = SDLK_UNKNOWN;
    SDL_Keycode secondary = SDLK_UNKNOWN;

    if (is_table) {
        lua_rawgeti(L, -1, 1);
        if (lua_isstring(L, -1)) {
            StringToKeycode(lua_tostring(L, -1), &primary);
        }
        lua_pop(L, 1);
        lua_rawgeti(L, -1, 2);
        if (lua_isstring(L, -1)) {
            StringToKeycode(lua_tostring(L, -1), &secondary);
        }
        lua_pop(L, 1);
        loaded = true;
    } else if (is_string) {
        StringToKeycode(lua_tostring(L, -1), &primary);
        secondary = primary;
        loaded = true;
    }

    lua_pop(L, 1);
    if (loaded && out) {
        out->primary = primary;
        out->secondary = secondary;
    }
    return loaded;
}

}  // namespace

std::string SanitizePlayerName(std::string name) {
    const auto is_allowed = [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '_' || c == '-';
    };

    name.erase(std::remove_if(name.begin(), name.end(), [&](char c) { return !is_allowed(c); }),
               name.end());

    const std::size_t max_len = 12;
    if (name.size() > max_len) {
        name.resize(max_len);
    }

    // Trim leading/trailing spaces
    while (!name.empty() && name.front() == ' ') name.erase(name.begin());
    while (!name.empty() && name.back() == ' ') name.pop_back();

    if (name.empty()) {
        name = "Player";
    }
    return name;
}

const ConfigData& Config::Data() const {
    return data_;
}

ConfigData& Config::Data() {
    return data_;
}

bool Config::LoadFromFile(const std::filesystem::path& path) {
    lua_State* L = luaL_newstate();
    if (!L) {
        return false;
    }
    luaL_openlibs(L);

    bool ok = true;
    if (luaL_loadfile(L, path.string().c_str()) != LUA_OK) {
        ok = false;
    } else if (lua_pcall(L, 0, LUA_MULTRET, 0) != LUA_OK) {
        ok = false;
    }

    ConfigData loaded = data_;

    if (ok) {
        if (lua_gettop(L) > 0 && lua_istable(L, -1)) {
            lua_setglobal(L, "config");
        } else {
            lua_settop(L, 0);
        }
    }

    lua_getglobal(L, "config");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_close(L);
        Sanitize();
        return false;
    }

    LoadStringField(L, "player_name", &loaded.player_name);

    // New layout: window
    lua_getfield(L, -1, "window");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "width", &loaded.window.width);
        LoadIntField(L, "height", &loaded.window.height);
        LoadBoolField(L, "fullscreen_desktop", &loaded.window.fullscreen_desktop);
        LoadBoolField(L, "vsync", &loaded.window.vsync);
    }
    lua_pop(L, 1);

    // Legacy layout: video
    lua_getfield(L, -1, "video");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "window_w", &loaded.window.width);
        LoadIntField(L, "window_h", &loaded.window.height);
        LoadIntField(L, "tile_px", &loaded.grid.tile_size);
        LoadBoolField(L, "fullscreen_desktop", &loaded.window.fullscreen_desktop);
        LoadBoolField(L, "vsync", &loaded.window.vsync);
    }
    lua_pop(L, 1);

    // Grid layout
    lua_getfield(L, -1, "grid");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "board_w", &loaded.grid.board_w);
        LoadIntField(L, "board_h", &loaded.grid.board_h);
        LoadIntField(L, "tile_size", &loaded.grid.tile_size);
        bool wrap_mode = loaded.grid.wrap_mode;
        if (LoadBoolField(L, "wrap_mode", &wrap_mode)) {
            loaded.grid.wrap_mode = wrap_mode;
        }
    }
    lua_pop(L, 1);

    // Legacy game layout
    lua_getfield(L, -1, "game");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "board_w", &loaded.grid.board_w);
        LoadIntField(L, "board_h", &loaded.grid.board_h);
        std::string wall_mode;
        if (LoadStringField(L, "walls", &wall_mode)) {
            loaded.grid.wrap_mode = (wall_mode == "wrap");
        }
        LoadIntField(L, "food_score", &loaded.gameplay.food_score);
        LoadIntField(L, "bonus_score", &loaded.gameplay.bonus_score);
        LoadNumberField(L, "slow_multiplier", &loaded.gameplay.slow_multiplier);
        LoadNumberField(L, "slow_duration_sec", &loaded.gameplay.slow_duration_sec);
    }
    lua_pop(L, 1);

    // Gameplay layout
    lua_getfield(L, -1, "gameplay");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "food_score", &loaded.gameplay.food_score);
        LoadIntField(L, "bonus_score_score", &loaded.gameplay.bonus_score_score);
        LoadIntField(L, "bonus_score", &loaded.gameplay.bonus_score);
        LoadNumberField(L, "slow_multiplier", &loaded.gameplay.slow_multiplier);
        LoadNumberField(L, "slow_duration_sec", &loaded.gameplay.slow_duration_sec);
        LoadIntField(L, "max_simultaneous_bonuses", &loaded.gameplay.max_simultaneous_bonuses);
        LoadBoolField(L, "always_one_food", &loaded.gameplay.always_one_food);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "audio");
    if (lua_istable(L, -1)) {
        LoadBoolField(L, "enabled", &loaded.audio.enabled);
        LoadIntField(L, "master_volume", &loaded.audio.master_volume);
        LoadIntField(L, "sfx_volume", &loaded.audio.sfx_volume);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "ui");
    if (lua_istable(L, -1)) {
        std::string mode;
        if (LoadStringField(L, "panel_mode", &mode)) {
            loaded.ui.panel_mode = mode;
        }
    }
    lua_pop(L, 1);

    // Keybinds (new layout)
    lua_getfield(L, -1, "keybinds");
    if (lua_istable(L, -1)) {
        LoadKeyPair(L, "up", &loaded.keys.up);
        LoadKeyPair(L, "down", &loaded.keys.down);
        LoadKeyPair(L, "left", &loaded.keys.left);
        LoadKeyPair(L, "right", &loaded.keys.right);
        LoadKeyPair(L, "pause", &loaded.keys.pause);
        LoadKeyPair(L, "restart", &loaded.keys.restart);
        LoadKeyPair(L, "menu", &loaded.keys.menu);
        LoadKeyPair(L, "confirm", &loaded.keys.confirm);
    }
    lua_pop(L, 1);

    // Legacy: single-key table
    lua_getfield(L, -1, "keys");
    if (lua_istable(L, -1)) {
        std::string k;
        if (LoadStringField(L, "up", &k)) StringToKeycode(k, &loaded.keys.up.primary);
        if (LoadStringField(L, "down", &k)) StringToKeycode(k, &loaded.keys.down.primary);
        if (LoadStringField(L, "left", &k)) StringToKeycode(k, &loaded.keys.left.primary);
        if (LoadStringField(L, "right", &k)) StringToKeycode(k, &loaded.keys.right.primary);
        if (LoadStringField(L, "pause", &k)) StringToKeycode(k, &loaded.keys.pause.primary);
        if (LoadStringField(L, "restart", &k)) StringToKeycode(k, &loaded.keys.restart.primary);
        if (LoadStringField(L, "menu", &k)) StringToKeycode(k, &loaded.keys.menu.primary);
        if (LoadStringField(L, "confirm", &k)) StringToKeycode(k, &loaded.keys.confirm.primary);
        loaded.keys.up.secondary = loaded.keys.up.primary;
        loaded.keys.down.secondary = loaded.keys.down.primary;
        loaded.keys.left.secondary = loaded.keys.left.primary;
        loaded.keys.right.secondary = loaded.keys.right.primary;
        loaded.keys.pause.secondary = loaded.keys.pause.primary;
        loaded.keys.restart.secondary = loaded.keys.restart.primary;
        loaded.keys.menu.secondary = loaded.keys.menu.primary;
        loaded.keys.confirm.secondary = loaded.keys.confirm.primary;
    }
    lua_pop(L, 1);  // keys

    lua_pop(L, 1);  // config
    lua_close(L);

    data_ = loaded;
    Sanitize();
    return ok;
}

bool Config::SaveToFile(const std::filesystem::path& path) const {
    const std::filesystem::path tmp = path.string() + ".tmp";

    std::ofstream ofs(tmp, std::ios::trunc);
    if (!ofs) {
        return false;
    }

    auto b = [](bool v) { return v ? "true" : "false"; };
    auto write_keypair = [&](const char* name, const KeyPair& kp) {
        ofs << "    " << name << " = { \""
            << KeycodeToString(kp.primary) << "\", \""
            << KeycodeToString(kp.secondary) << "\" },\n";
    };

    ofs << "return {\n";
    ofs << "  player_name = \"" << EscapeLuaString(data_.player_name) << "\",\n";
    ofs << "  window = { width = " << data_.window.width << ", height = " << data_.window.height
        << ", fullscreen_desktop = " << b(data_.window.fullscreen_desktop)
        << ", vsync = " << b(data_.window.vsync) << " },\n";
    ofs << "  grid = { board_w = " << data_.grid.board_w << ", board_h = " << data_.grid.board_h
        << ", tile_size = " << data_.grid.tile_size
        << ", wrap_mode = " << b(data_.grid.wrap_mode) << " },\n";
    ofs << "  audio = { enabled = " << b(data_.audio.enabled)
        << ", master_volume = " << data_.audio.master_volume
        << ", sfx_volume = " << data_.audio.sfx_volume << " },\n";
    ofs << "  ui = { panel_mode = \"" << EscapeLuaString(data_.ui.panel_mode) << "\" },\n";
    ofs << "  keybinds = {\n";
    write_keypair("up", data_.keys.up);
    write_keypair("down", data_.keys.down);
    write_keypair("left", data_.keys.left);
    write_keypair("right", data_.keys.right);
    write_keypair("pause", data_.keys.pause);
    write_keypair("restart", data_.keys.restart);
    write_keypair("menu", data_.keys.menu);
    write_keypair("confirm", data_.keys.confirm);
    ofs << "  },\n";
    ofs << "  gameplay = { food_score = " << data_.gameplay.food_score
        << ", bonus_score_score = " << data_.gameplay.bonus_score_score
        << ", bonus_score = " << data_.gameplay.bonus_score
        << ", slow_multiplier = " << data_.gameplay.slow_multiplier
        << ", slow_duration_sec = " << data_.gameplay.slow_duration_sec
        << ", max_simultaneous_bonuses = " << data_.gameplay.max_simultaneous_bonuses
        << ", always_one_food = " << b(data_.gameplay.always_one_food) << " },\n";
    ofs << "}\n";

    ofs.close();
    if (!ofs) {
        std::filesystem::remove(tmp);
        return false;
    }

    std::error_code ec;
    std::filesystem::remove(path, ec);
    ec.clear();
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(tmp);
        return false;
    }
    return true;
}

void Config::Sanitize() {
    auto clamp = [](int v, int lo, int hi) { return std::clamp(v, lo, hi); };
    data_.grid.board_w = clamp(data_.grid.board_w, kMinBoardSize, kMaxBoardSize);
    data_.grid.board_h = clamp(data_.grid.board_h, kMinBoardSize, kMaxBoardSize);
    data_.grid.tile_size = clamp(data_.grid.tile_size, kMinTilePx, kMaxTilePx);
    data_.window.width = clamp(data_.window.width, kMinWindow, kMaxWindow);
    data_.window.height = clamp(data_.window.height, kMinWindow, kMaxWindow);
    data_.audio.master_volume = clamp(data_.audio.master_volume, 0, 128);
    data_.audio.sfx_volume = clamp(data_.audio.sfx_volume, 0, 128);
    data_.gameplay.bonus_score = std::max(0, data_.gameplay.bonus_score);
    data_.gameplay.bonus_score_score = std::max(0, data_.gameplay.bonus_score_score);
    data_.gameplay.food_score = std::max(1, data_.gameplay.food_score);
    data_.gameplay.max_simultaneous_bonuses = std::max(0, data_.gameplay.max_simultaneous_bonuses);
    data_.gameplay.slow_multiplier = std::max(0.0, data_.gameplay.slow_multiplier);
    data_.gameplay.slow_duration_sec = std::max(0.0, data_.gameplay.slow_duration_sec);
    data_.player_name = SanitizePlayerName(data_.player_name);
    data_.ui.panel_mode = NormalizePanelMode(data_.ui.panel_mode);

    const KeyBinds defaults = DefaultKeybinds();
    auto ensure_pair = [this](KeyPair& kp, const KeyPair& def) {
        auto ensure_allowed = [this](SDL_Keycode& key, SDL_Keycode fallback) {
            if (!IsAllowedKey(key)) {
                key = fallback;
            }
        };
        ensure_allowed(kp.primary, def.primary);
        ensure_allowed(kp.secondary, def.secondary);
    };
    ensure_pair(data_.keys.up, defaults.up);
    ensure_pair(data_.keys.down, defaults.down);
    ensure_pair(data_.keys.left, defaults.left);
    ensure_pair(data_.keys.right, defaults.right);
    ensure_pair(data_.keys.pause, defaults.pause);
    ensure_pair(data_.keys.restart, defaults.restart);
    ensure_pair(data_.keys.menu, defaults.menu);
    ensure_pair(data_.keys.confirm, defaults.confirm);
}

void Config::SetWallMode(WallMode m) {
    data_.grid.wrap_mode = (m == WallMode::Wrap);
    Sanitize();
}

void Config::SetBoardSize(int w, int h) {
    data_.grid.board_w = w;
    data_.grid.board_h = h;
    Sanitize();
}

void Config::SetWindowSize(int w, int h) {
    data_.window.width = w;
    data_.window.height = h;
    Sanitize();
}

void Config::SetTilePx(int px) {
    data_.grid.tile_size = px;
    Sanitize();
}

void Config::SetVsync(bool on) {
    data_.window.vsync = on;
    Sanitize();
}

void Config::SetFullscreenDesktop(bool on) {
    data_.window.fullscreen_desktop = on;
    Sanitize();
}

void Config::SetMasterVolume(int v) {
    data_.audio.master_volume = v;
    Sanitize();
}

void Config::SetPanelMode(std::string m) {
    data_.ui.panel_mode = std::move(m);
    Sanitize();
}

void Config::SetPlayerName(std::string s) {
    data_.player_name = std::move(s);
    Sanitize();
}

void Config::SetAudioEnabled(bool enabled) {
    data_.audio.enabled = enabled;
    Sanitize();
}

void Config::SetSfxVolume(int v) {
    data_.audio.sfx_volume = v;
    Sanitize();
}

bool Config::IsAllowedKey(SDL_Keycode k) const {
    for (const auto& kv : AllowedKeyMap()) {
        if (kv.second == k) {
            return true;
        }
    }
    return false;
}

bool Config::SetBind(std::string_view action, SDL_Keycode k, int slot) {
    if (slot != 0 && slot != 1) {
        return false;
    }
    if (!IsAllowedKey(k)) {
        return false;
    }

    KeyPair* target = nullptr;
    if (action == "up") target = &data_.keys.up;
    else if (action == "down") target = &data_.keys.down;
    else if (action == "left") target = &data_.keys.left;
    else if (action == "right") target = &data_.keys.right;
    else if (action == "pause") target = &data_.keys.pause;
    else if (action == "restart") target = &data_.keys.restart;
    else if (action == "menu") target = &data_.keys.menu;
    else if (action == "confirm") target = &data_.keys.confirm;

    if (target == nullptr) {
        return false;
    }

    if (slot == 0) {
        target->primary = k;
    } else {
        target->secondary = k;
    }

    Sanitize();
    return true;
}

std::string Config::KeycodeToToken(SDL_Keycode key) {
    return KeycodeToString(key);
}

}  // namespace snake::io
