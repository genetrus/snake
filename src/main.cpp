#include <exception>
#include <filesystem>
#include <iostream>

#include "io/AppData.h"

int main() {
    try {
        const auto app_data_dir = snake::io::GetAppDataDir();
        snake::io::EnsureAppDataDirExists();
        std::cout << "AppData directory: " << app_data_dir << std::endl;
        std::cout << "Snake game skeleton initialized." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Initialization failed: " << ex.what() << std::endl;
        return 1;
    }
}
