#include "lua/LuaRuntime.h"

#include <lua.hpp>

#include <utility>

#include "lua/Bindings.h"

namespace snake::lua {

namespace {
constexpr const char* kRulesChunk = "rules";
constexpr const char* kConfigChunk = "config";
}  // namespace

LuaRuntime::LuaRuntime() = default;

LuaRuntime::~LuaRuntime() {
    Shutdown();
}

bool LuaRuntime::Init() {
    Shutdown();
    L_ = luaL_newstate();
    if (!L_) {
        SetError("init", "failed to create lua state");
        return false;
    }
    luaL_openlibs(L_);
    last_error_.reset();
    return true;
}

void LuaRuntime::Shutdown() {
    if (L_) {
        lua_close(L_);
        L_ = nullptr;
    }
}

bool LuaRuntime::IsReady() const {
    return L_ != nullptr;
}

const std::optional<LuaError>& LuaRuntime::LastError() const {
    return last_error_;
}

void LuaRuntime::ClearLastError() {
    last_error_.reset();
}

bool LuaRuntime::LoadRules(const std::filesystem::path& rules_path) {
    if (!IsReady()) return false;
    return LoadFile(rules_path, "loadfile:rules.lua");
}

bool LuaRuntime::LoadConfig(const std::filesystem::path& config_path) {
    if (!IsReady()) return false;

    const int top_before = lua_gettop(L_);
    if (!LoadFile(config_path, "loadfile:config.lua")) {
        return false;
    }

    // If chunk returned a table, set it as global config
    if (lua_gettop(L_) > top_before && lua_istable(L_, -1)) {
        lua_setglobal(L_, "config");
    }

    lua_settop(L_, top_before);

    lua_getglobal(L_, "config");
    if (lua_isnil(L_, -1)) {
        lua_pop(L_, 1);
        SetError("config", "config.lua did not define global 'config'");
        return false;
    }
    lua_pop(L_, 1);

    return true;
}

bool LuaRuntime::CallVoid(std::string_view fn) {
    if (!IsReady()) return false;
    lua_getglobal(L_, std::string(fn).c_str());
    if (lua_isnil(L_, -1)) {
        lua_pop(L_, 1);
        return true;
    }
    return PCall(0, 0, std::string("pcall:").append(fn));
}

bool LuaRuntime::CallWithCtx(std::string_view fn, void* ctx_ptr) {
    if (!IsReady()) return false;
    lua_getglobal(L_, std::string(fn).c_str());
    if (lua_isnil(L_, -1)) {
        lua_pop(L_, 1);
        return true;
    }
    lua_pushlightuserdata(L_, ctx_ptr);
    return PCall(1, 0, std::string("pcall:").append(fn));
}

bool LuaRuntime::HotReload(const std::filesystem::path& rules_path,
                           const std::filesystem::path& config_path) {
    LuaRuntime tmp;
    if (!tmp.Init()) {
        SetError("reload:init", "failed to init temp lua state");
        return false;
    }

    Bindings::Register(tmp.L());

    if (!tmp.LoadRules(rules_path)) {
        last_error_ = tmp.last_error_;
        return false;
    }
    if (!tmp.LoadConfig(config_path)) {
        last_error_ = tmp.last_error_;
        return false;
    }

    std::swap(L_, tmp.L_);
    last_error_.reset();
    return true;
}

lua_State* LuaRuntime::L() const {
    return L_;
}

bool LuaRuntime::PCall(int nargs, int nrets, std::string_view where) {
    const int base = lua_gettop(L_) - nargs;
    lua_pushcfunction(L_, &Traceback);
    lua_insert(L_, base);
    const int status = lua_pcall(L_, nargs, nrets, base);
    lua_remove(L_, base);
    if (status != LUA_OK) {
        const char* msg = lua_tostring(L_, -1);
        SetError(where, msg ? msg : "unknown lua error");
        lua_pop(L_, 1);
        return false;
    }
    return true;
}

bool LuaRuntime::LoadFile(const std::filesystem::path& p, std::string_view where) {
    if (luaL_loadfile(L_, p.string().c_str()) != LUA_OK) {
        const char* msg = lua_tostring(L_, -1);
        SetError(where, msg ? msg : "loadfile failed");
        lua_pop(L_, 1);
        return false;
    }
    if (!PCall(0, LUA_MULTRET, where)) {
        return false;
    }
    return true;
}

int LuaRuntime::Traceback(lua_State* L) {
    const char* msg = lua_tostring(L, 1);
    if (msg) {
        luaL_traceback(L, L, msg, 1);
    } else if (!lua_isnoneornil(L, 1)) {
        if (luaL_callmeta(L, 1, "__tostring") && lua_type(L, -1) == LUA_TSTRING) {
            return 1;
        }
        switch (lua_type(L, 1)) {
            case LUA_TSTRING:
                lua_pushvalue(L, 1);
                break;
            default:
                lua_pushfstring(L, "error object is a %s value", luaL_typename(L, 1));
                break;
        }
    } else {
        lua_pushliteral(L, "unknown error");
    }
    return 1;
}

void LuaRuntime::SetError(std::string_view where, std::string_view msg) {
    last_error_ = LuaError{std::string(msg), std::string(where)};
}

}  // namespace snake::lua
