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

    LoadStringField(L, "player_name", &data_.player_name);

    lua_getfield(L, -1, "video");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "window_w", &data_.video.window_w);
        LoadIntField(L, "window_h", &data_.video.window_h);
        LoadIntField(L, "tile_px", &data_.video.tile_px);
        LoadBoolField(L, "fullscreen_desktop", &data_.video.fullscreen_desktop);
        LoadBoolField(L, "vsync", &data_.video.vsync);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "game");
    if (lua_istable(L, -1)) {
        LoadIntField(L, "board_w", &data_.game.board_w);
        LoadIntField(L, "board_h", &data_.game.board_h);
        std::string wall_mode;
        if (LoadStringField(L, "walls", &wall_mode)) {
            if (wall_mode == "wrap") {
                data_.game.walls = WallMode::Wrap;
            } else {
                data_.game.walls = WallMode::Death;
            }
        }
        LoadIntField(L, "food_score", &data_.game.food_score);
        LoadIntField(L, "bonus_score", &data_.game.bonus_score);
        LoadNumberField(L, "slow_multiplier", &data_.game.slow_multiplier);
        LoadNumberField(L, "slow_duration_sec", &data_.game.slow_duration_sec);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "grid");
    if (lua_istable(L, -1)) {
        bool wrap_mode = false;
        if (LoadBoolField(L, "wrap_mode", &wrap_mode)) {
            data_.game.walls = wrap_mode ? WallMode::Wrap : WallMode::Death;
        }
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "audio");
    if (lua_istable(L, -1)) {
        LoadBoolField(L, "enabled", &data_.audio.enabled);
        LoadIntField(L, "master_volume", &data_.audio.master_volume);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "keys");
    if (lua_istable(L, -1)) {
        std::string k;
        if (LoadStringField(L, "up", &k)) StringToKeycode(k, &data_.keys.up);
        if (LoadStringField(L, "down", &k)) StringToKeycode(k, &data_.keys.down);
        if (LoadStringField(L, "left", &k)) StringToKeycode(k, &data_.keys.left);
        if (LoadStringField(L, "right", &k)) StringToKeycode(k, &data_.keys.right);
        if (LoadStringField(L, "pause", &k)) StringToKeycode(k, &data_.keys.pause);
        if (LoadStringField(L, "restart", &k)) StringToKeycode(k, &data_.keys.restart);
        if (LoadStringField(L, "menu", &k)) StringToKeycode(k, &data_.keys.menu);
        if (LoadStringField(L, "confirm", &k)) StringToKeycode(k, &data_.keys.confirm);
    }
    lua_pop(L, 1);  // keys

    lua_pop(L, 1);  // config
    lua_close(L);

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

    ofs << "return {\n";
    ofs << "  player_name = \"" << EscapeLuaString(data_.player_name) << "\",\n";
    ofs << "  video = { window_w=" << data_.video.window_w << ", window_h=" << data_.video.window_h
        << ", tile_px=" << data_.video.tile_px
        << ", fullscreen_desktop=" << b(data_.video.fullscreen_desktop)
        << ", vsync=" << b(data_.video.vsync) << " },\n";
    ofs << "  game = { board_w=" << data_.game.board_w << ", board_h=" << data_.game.board_h
        << ", walls=\"" << (data_.game.walls == WallMode::Wrap ? "wrap" : "death")
        << "\", food_score=" << data_.game.food_score << ", bonus_score=" << data_.game.bonus_score
        << ", slow_multiplier=" << data_.game.slow_multiplier
        << ", slow_duration_sec=" << data_.game.slow_duration_sec << " },\n";
    ofs << "  grid = { wrap_mode=" << b(data_.game.walls == WallMode::Wrap) << " },\n";
    ofs << "  audio = { enabled=" << b(data_.audio.enabled)
        << ", master_volume=" << data_.audio.master_volume << " },\n";
    ofs << "  keys = { ";
    ofs << "up=\"" << KeycodeToString(data_.keys.up) << "\", ";
    ofs << "down=\"" << KeycodeToString(data_.keys.down) << "\", ";
    ofs << "left=\"" << KeycodeToString(data_.keys.left) << "\", ";
    ofs << "right=\"" << KeycodeToString(data_.keys.right) << "\", ";
    ofs << "pause=\"" << KeycodeToString(data_.keys.pause) << "\", ";
    ofs << "restart=\"" << KeycodeToString(data_.keys.restart) << "\", ";
    ofs << "menu=\"" << KeycodeToString(data_.keys.menu) << "\", ";
    ofs << "confirm=\"" << KeycodeToString(data_.keys.confirm) << "\"";
    ofs << " },\n";
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
    data_.game.board_w = clamp(data_.game.board_w, kMinBoardSize, kMaxBoardSize);
    data_.game.board_h = clamp(data_.game.board_h, kMinBoardSize, kMaxBoardSize);
    data_.video.tile_px = clamp(data_.video.tile_px, kMinTilePx, kMaxTilePx);
    data_.video.window_w = clamp(data_.video.window_w, kMinWindow, kMaxWindow);
    data_.video.window_h = clamp(data_.video.window_h, kMinWindow, kMaxWindow);
    data_.audio.master_volume = clamp(data_.audio.master_volume, 0, 128);
    data_.player_name = SanitizePlayerName(data_.player_name);

    auto ensure_allowed = [this](SDL_Keycode& key, SDL_Keycode fallback) {
        if (!IsAllowedKey(key)) {
            key = fallback;
        }
    };
    ensure_allowed(data_.keys.up, SDLK_UP);
    ensure_allowed(data_.keys.down, SDLK_DOWN);
    ensure_allowed(data_.keys.left, SDLK_LEFT);
    ensure_allowed(data_.keys.right, SDLK_RIGHT);
    ensure_allowed(data_.keys.pause, SDLK_p);
    ensure_allowed(data_.keys.restart, SDLK_r);
    ensure_allowed(data_.keys.menu, SDLK_ESCAPE);
    ensure_allowed(data_.keys.confirm, SDLK_RETURN);
}

void Config::SetWallMode(WallMode m) {
    data_.game.walls = m;
    Sanitize();
}

void Config::SetBoardSize(int w, int h) {
    data_.game.board_w = w;
    data_.game.board_h = h;
    Sanitize();
}

void Config::SetWindowSize(int w, int h) {
    data_.video.window_w = w;
    data_.video.window_h = h;
    Sanitize();
}

void Config::SetTilePx(int px) {
    data_.video.tile_px = px;
    Sanitize();
}

void Config::SetVsync(bool on) {
    data_.video.vsync = on;
    Sanitize();
}

void Config::SetFullscreenDesktop(bool on) {
    data_.video.fullscreen_desktop = on;
    Sanitize();
}

void Config::SetMasterVolume(int v) {
    data_.audio.master_volume = v;
    Sanitize();
}

void Config::SetPlayerName(std::string s) {
    data_.player_name = std::move(s);
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

bool Config::SetBind(std::string_view action, SDL_Keycode k) {
    if (!IsAllowedKey(k)) {
        return false;
    }

    SDL_Keycode* target = nullptr;
    if (action == "up") target = &data_.keys.up;
    else if (action == "down") target = &data_.keys.down;
    else if (action == "left") target = &data_.keys.left;
    else if (action == "right") target = &data_.keys.right;
    else if (action == "pause") target = &data_.keys.pause;
    else if (action == "restart") target = &data_.keys.restart;
    else if (action == "menu") target = &data_.keys.menu;
    else if (action == "confirm") target = &data_.keys.confirm;

    if (!target) {
        return false;
    }
    *target = k;
    Sanitize();
    return true;
}

}  // namespace snake::io
