#include "io/Bootstrap.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#include "io/AppData.h"
#include "io/Paths.h"
#include "nlohmann/json.hpp"

namespace fs = std::filesystem;

namespace snake::io {

void BootstrapUserData() {
    EnsureAppDataDirExists();

    const fs::path appdata_dir = UserDir();
    const fs::path user_config = UserPath("config.lua");
    const fs::path highscores = UserPath("highscores.json");
    const fs::path default_config = AssetsPath("scripts/config.lua");

    if (!fs::exists(user_config)) {
        if (!fs::exists(default_config)) {
            throw std::runtime_error("Default config missing at runtime: " +
                                     default_config.string());
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
