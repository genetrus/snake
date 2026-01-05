#pragma once

#include <lua.hpp>

#include <string>

namespace snake::lua {

class Bindings {
public:
    static void Register(lua_State* L);

    static bool GetInt(lua_State* L, const char* expr, int* out);
    static bool GetBool(lua_State* L, const char* expr, bool* out);
    static bool GetString(lua_State* L, const char* expr, std::string* out);

private:
    static int l_log(lua_State* L);
};

}  // namespace snake::lua
