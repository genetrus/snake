#pragma once

#include <lua.hpp>

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace snake::lua {

struct LuaError {
    std::string message;
    std::string where;
};

class LuaRuntime {
public:
    LuaRuntime();
    ~LuaRuntime();

    bool Init();
    void Shutdown();

    bool IsReady() const;
    const std::optional<LuaError>& LastError() const;
    void ClearLastError();

    bool LoadRules(const std::filesystem::path& rules_path);
    bool LoadConfig(const std::filesystem::path& config_path);

    bool CallVoid(std::string_view fn);
    bool CallWithCtx(std::string_view fn, void* ctx_ptr);
    bool CallWithCtxIfExists(std::string_view fn, void* ctx_ptr);
    bool CallWithCtxIfExists(std::string_view fn, void* ctx_ptr, std::string_view arg1);

    bool HotReload(const std::filesystem::path& rules_path, const std::filesystem::path& config_path);

    lua_State* L() const;

    // speed_ticks_per_sec(score, config) -> number (>0)
    bool GetBaseTicksPerSec(int score, double* out_ticks_per_sec);
    bool GetSpeedTicksPerSec(int score, double* out_ticks_per_sec);

private:
    lua_State* L_ = nullptr;
    std::optional<LuaError> last_error_;
    std::string last_logged_error_;

    bool PCall(int nargs, int nrets, std::string_view where);
    bool LoadFile(const std::filesystem::path& p, std::string_view where);

    static int Traceback(lua_State* L);
    void SetError(std::string_view where, std::string_view msg);
};

}  // namespace snake::lua
