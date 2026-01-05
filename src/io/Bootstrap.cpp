#include "io/Bootstrap.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "io/AppData.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace snake::io {

namespace {
constexpr auto kDefaultConfigPath = "assets/scripts/config.lua";
}  // namespace

void BootstrapUserData() {
    EnsureAppDataDirExists();

    const fs::path appdata_dir = GetAppDataDir();
    const fs::path user_config = appdata_dir / "config.lua";
    const fs::path highscores = appdata_dir / "highscores.json";
    const fs::path default_config = fs::path(kDefaultConfigPath);

    if (!fs::exists(user_config)) {
        if (!fs::exists(default_config)) {
            throw std::runtime_error(
                "Default config missing at runtime: assets/scripts/config.lua");
        }

        std::error_code ec;
        fs::copy_file(default_config, user_config, fs::copy_options::none, ec);
        if (ec) {
            throw std::runtime_error("Failed to copy default config to AppData: " + ec.message());
        }
    }

    if (!fs::exists(highscores)) {
        nlohmann::json json{{"version", 1}, {"entries", nlohmann::json::array()}};

        std::ofstream ofs(highscores);
        if (!ofs) {
            throw std::runtime_error("Failed to create highscores file at " + highscores.string());
        }

        ofs << json.dump(2);
    }

    std::cout << "AppData directory prepared at: " << appdata_dir << std::endl;
}

}  // namespace snake::io
