#include "io/AppData.h"

#include <ShlObj.h>

#include <filesystem>
#include <stdexcept>
#include <string>

namespace snake::io {

std::filesystem::path GetAppDataDir() {
    PWSTR raw_path = nullptr;
    const HRESULT result =
        SHGetKnownFolderPath(FOLDERID_RoamingAppData, KF_FLAG_DEFAULT, nullptr, &raw_path);

    if (FAILED(result) || raw_path == nullptr) {
        throw std::runtime_error("Failed to resolve the AppData directory.");
    }

    std::wstring appdata_wstr(raw_path);
    CoTaskMemFree(raw_path);

    return std::filesystem::path(appdata_wstr) / L"snake";
}

void EnsureAppDataDirExists() {
    const std::filesystem::path appdata_dir = GetAppDataDir();

    std::error_code ec;
    const bool created = std::filesystem::create_directories(appdata_dir, ec);

    if (ec) {
        throw std::runtime_error("Failed to create AppData directory at \"" +
                                 appdata_dir.string() + "\": " + ec.message());
    }

    if (!created && !std::filesystem::exists(appdata_dir)) {
        throw std::runtime_error("Failed to ensure AppData directory exists at \"" +
                                 appdata_dir.string() + "\".");
    }
}

}  // namespace snake::io
