#pragma once

#include <filesystem>

namespace snake::io {

std::filesystem::path GetAppDataDir();

void EnsureAppDataDirExists();

}  // namespace snake::io
