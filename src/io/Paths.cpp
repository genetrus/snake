#include "io/Paths.h"

#include <filesystem>
#include <string>

#include "io/AppData.h"

namespace snake::io {

std::filesystem::path AssetsDir() {
    return std::filesystem::path("assets");
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
