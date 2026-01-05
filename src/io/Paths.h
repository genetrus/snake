#pragma once

#include <filesystem>
#include <string_view>

namespace snake::io {

// Assets are read from ./assets next to the executable. User data lives in %AppData%/snake.
std::filesystem::path AssetsDir();                                         // returns "assets"
std::filesystem::path AssetsPath(std::string_view relative);               // returns AssetsDir()/relative
std::filesystem::path UserDir();                                           // returns GetAppDataDir()
std::filesystem::path UserPath(std::string_view filename);                 // returns UserDir()/filename

}  // namespace snake::io
