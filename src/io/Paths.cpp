#include "io/Paths.h"

#include <SDL.h>

#include <filesystem>
#include <string>

#include "io/AppData.h"

namespace snake::io {

std::filesystem::path AssetsDir() {
    static std::filesystem::path resolved = []() {
        const std::filesystem::path cwd_assets = std::filesystem::current_path() / "assets";
        if (std::filesystem::exists(cwd_assets)) {
            return cwd_assets;
        }

        std::filesystem::path base_assets;
        if (char* base = SDL_GetBasePath()) {
            std::filesystem::path base_path(base);
            SDL_free(base);
            base_assets = base_path / "assets";
            if (std::filesystem::exists(base_assets)) {
                return base_assets;
            }

            const std::filesystem::path parent_assets = base_path.parent_path() / "assets";
            if (std::filesystem::exists(parent_assets)) {
                return parent_assets;
            }
        }

        return std::filesystem::path("assets");
    }();

    return resolved;
}

std::filesystem::path AssetsPath(std::string_view relative) {
    return AssetsDir() / std::filesystem::path(std::string(relative));
}

std::filesystem::path UserDir() {
    return GetAppDataDir();
}

std::filesystem::path UserPath(std::string_view filename) {
    return UserDir() / std::filesystem::path(std::string(filename));
}

}  // namespace snake::io
