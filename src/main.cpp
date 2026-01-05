#include <exception>
#include <iostream>

#include "core/App.h"
#include "io/Bootstrap.h"

int main() {
    try {
        snake::io::BootstrapUserData();
        snake::core::App app;
        return app.Run();
    } catch (const std::exception& ex) {
        std::cerr << "Initialization failed: " << ex.what() << std::endl;
        return 1;
    }
}
