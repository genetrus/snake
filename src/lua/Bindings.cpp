#include "lua/Bindings.h"

#include <lua.hpp>

#include <iostream>
#include <string_view>

namespace snake::lua {

namespace {

int GetSnakeTable(lua_State* L) {
    lua_getglobal(L, "snake");
    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return 0;
    }
    return 1;
}

bool EvalString(lua_State* L, const char* expr) {
    return luaL_dostring(L, expr) == LUA_OK;
}

bool GetValue(lua_State* L, const char* expr, int type_expected) {
    if (!EvalString(L, expr)) {
        lua_pop(L, 1);  // error message
        return false;
    }
    const bool ok = lua_type(L, -1) == type_expected;
    if (!ok) {
        lua_pop(L, 1);
    }
    return ok;
}

}  // namespace

void Bindings::Register(lua_State* L) {
    lua_newtable(L);

    lua_pushcfunction(L, &Bindings::l_log);
    lua_setfield(L, -2, "log");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        lua_getglobal(L, "config");
        if (lua_isnil(L, -1)) {
            return 0;
        }
        return 1;
    });
    lua_setfield(L, -2, "get_config");

    lua_pushcfunction(L, [](lua_State* L) -> int {
        const char* msg = luaL_optstring(L, 1, "");
        lua_pushstring(L, msg);
        lua_setglobal(L, "snake_reload_error");
        return 0;
    });
    lua_setfield(L, -2, "set_reload_error");

    lua_setglobal(L, "snake");
}

bool Bindings::GetInt(lua_State* L, const char* expr, int* out) {
    if (!out) return false;
    const std::string wrapped = "return " + std::string(expr);
    if (!EvalString(L, wrapped.c_str())) {
        lua_pop(L, 1);
        return false;
    }
    if (!lua_isinteger(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    *out = static_cast<int>(lua_tointeger(L, -1));
    lua_pop(L, 1);
    return true;
}

bool Bindings::GetBool(lua_State* L, const char* expr, bool* out) {
    if (!out) return false;
    const std::string wrapped = "return " + std::string(expr);
    if (!EvalString(L, wrapped.c_str())) {
        lua_pop(L, 1);
        return false;
    }
    if (!lua_isboolean(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    *out = lua_toboolean(L, -1) != 0;
    lua_pop(L, 1);
    return true;
}

bool Bindings::GetString(lua_State* L, const char* expr, std::string* out) {
    if (!out) return false;
    const std::string wrapped = "return " + std::string(expr);
    if (!EvalString(L, wrapped.c_str())) {
        lua_pop(L, 1);
        return false;
    }
    if (!lua_isstring(L, -1)) {
        lua_pop(L, 1);
        return false;
    }
    out->assign(lua_tostring(L, -1));
    lua_pop(L, 1);
    return true;
}

int Bindings::l_log(lua_State* L) {
    const char* msg = luaL_optstring(L, 1, "");
    std::cout << "[lua] " << msg << std::endl;
    return 0;
}

}  // namespace snake::lua
