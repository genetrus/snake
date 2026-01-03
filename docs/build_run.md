# Build and run

## Windows (MSYS2 MinGW64 + Ninja)

1. Install dependencies:
   ```bash
   pacman -Syu --noconfirm
   pacman -S --noconfirm base-devel mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja mingw-w64-x86_64-pkgconf mingw-w64-x86_64-SDL2 mingw-w64-x86_64-lua
   ```
2. Configure and build:
   ```bash
   cmake -S . -B build -G Ninja
   cmake --build build
   ```
3. Run:
   ```bash
   ./build/snake
   ```

> Примечание: при запуске вне MSYS2 убедитесь, что `SDL2.dll` и Lua DLL находятся рядом с `snake.exe` или прописаны в `PATH`.

## Linux (Ubuntu/Debian)

1. Install dependencies:
   ```bash
   sudo apt update
   sudo apt install -y cmake ninja-build pkg-config libsdl2-dev liblua5.4-dev
   ```
2. Configure and build:
   ```bash
   cmake -S . -B build -G Ninja
   cmake --build build
   ```
3. Run:
   ```bash
   ./build/snake
   ```

## macOS (Homebrew)

1. Install dependencies:
   ```bash
   brew install cmake ninja pkg-config sdl2 lua
   ```
2. Configure and build:
   ```bash
   cmake -S . -B build -G Ninja
   cmake --build build
   ```
3. Run:
   ```bash
   ./build/snake
   ```
