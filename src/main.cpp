#include <exception>
#include <filesystem>
#include <iostream>

#include "io/AppData.h"
#include "io/Bootstrap.h"

int main() {
    try {
        snake::io::BootstrapUserData();
        const auto app_data_dir = snake::io::GetAppDataDir();
        std::cout << "AppData directory ready at: " << app_data_dir << std::endl;
        std::cout << "Snake game skeleton initialized." << std::endl;
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Initialization failed: " << ex.what() << std::endl;
        return 1;
    }
}
