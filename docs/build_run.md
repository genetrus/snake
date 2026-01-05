# Build & Run (Windows, VS2022, MSVC x64, CMake + vcpkg)

This document is the **single source of truth** for building and running `snake` on **Windows** using:
- **Visual Studio 2022** (MSVC), **x64**
- **CMake** (generator: Visual Studio)
- **vcpkg** as a **git submodule** (`external/vcpkg`), **manifest mode**
- **dynamic** runtime dependencies (DLLs next to `snake.exe`)

---

## 0) Prerequisites

Install:
1) **Visual Studio 2022**
   - Workload: **Desktop development with C++**
   - Components: MSVC toolset + Windows 10/11 SDK
2) **Git for Windows** (so `git` works in PowerShell / cmd)
3) **CMake** (optional if you rely on VS CMake integration, but recommended to have CLI)

> Note: You do **not** need to install SDL/Lua manually. They are provided by **vcpkg**.

---

## 1) Get the source (with submodules)

Clone the repository **with submodules** (required because vcpkg is a submodule):

```powershell
git clone --recurse-submodules <REPO_URL>
cd snake
```

If you already cloned without submodules:

```powershell
git submodule update --init --recursive
```

Expected vcpkg location:
- `external\vcpkg\`

---

## 2) Bootstrap vcpkg (one-time per machine / after clean checkout)

From the repository root:

```powershell
.\external\vcpkg\bootstrap-vcpkg.bat
```

This produces:
- `external\vcpkg\vcpkg.exe`

---

## 3) Configure (CMake + vcpkg toolchain)

### Option A — Command line (recommended “works everywhere”)

From repository root:

```powershell
cmake -S . -B build/vs2022 `
  -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake
```

During configure, vcpkg will install the dependencies from `vcpkg.json` using the pinned baseline in `vcpkg-configuration.json`.

### Option B — Visual Studio “Open Folder”

1) Open VS2022
2) **File → Open → Folder…** and select the repo root `snake/`
3) VS will detect `CMakeLists.txt` and configure automatically (CMake Settings)

Make sure the configuration is **x64** and uses the vcpkg toolchain:
- `external/vcpkg/scripts/buildsystems/vcpkg.cmake`

---

## 4) Build

### Command line

```powershell
cmake --build build/vs2022 --config Debug
# or
cmake --build build/vs2022 --config Release
```

### Visual Studio

Select configuration **Debug** or **Release**, then build target `snake`.

---

## 5) Run

### What must be present next to `snake.exe`

Because dependencies are **dynamic**, the runtime directory (where you run `snake.exe`) must contain:

- `snake.exe`
- required DLLs (at minimum):
  - `SDL2.dll`
  - `SDL2_image.dll`
  - `SDL2_ttf.dll`
  - `SDL2_mixer.dll`
  - `lua54.dll`
  - plus any transitively required DLLs (e.g., codec/audio libs)
- the `assets/` folder:
  - `assets/scripts/`
  - `assets/sprites/`
  - `assets/sounds/`
  - `assets/fonts/`

### Where to run from

Run the executable from your chosen build output folder, for example:

- `build\vs2022\Debug\snake.exe`
- `build\vs2022\Release\snake.exe`

> If `assets/` and the DLLs are not automatically copied there yet, see **Troubleshooting** below.

### User data location (created on first launch)

On first launch the game uses:

- `%AppData%\snake\config.lua`
  - if missing → copied from `assets/scripts/config.lua`
- `%AppData%\snake\highscores.json`
  - if missing → created as empty JSON

---

## 6) Troubleshooting

### A) “The code execution cannot proceed because SDL2.dll was not found …”
This means required DLLs are not next to `snake.exe`.

**Fix (manual copy):**
1) Find vcpkg-installed binaries (one of these directories will exist depending on your setup):
   - `build\vs2022\vcpkg_installed\x64-windows\bin\`
   - `vcpkg_installed\x64-windows\bin\`
   - `external\vcpkg\installed\x64-windows\bin\` (less common for manifest builds; check if present)
2) Copy the needed `.dll` files into the folder that contains `snake.exe`.

Also ensure `lua54.dll` is next to `snake.exe` (it comes from the same vcpkg bin directory when `lua` is installed dynamically).

### B) Game starts but shows missing assets / blank sprites / missing font
Ensure `assets/` exists next to `snake.exe`:

- Copy repository `assets/` folder to the runtime folder where `snake.exe` is located.

### C) vcpkg installs different versions on different machines
This should **not** happen if:
- `vcpkg-configuration.json` contains a pinned `builtin-baseline` (commit)
- everyone uses the repo’s `external/vcpkg` submodule

If versions differ:
1) Verify submodule is on the correct commit:
   ```powershell
   git submodule status
   ```
2) Update submodules:
   ```powershell
   git submodule update --init --recursive
   ```

### D) Clean rebuild
To fully reconfigure:

```powershell
rmdir /s /q build/vs2022
cmake -S . -B build/vs2022 -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=external/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build/vs2022 --config Release
```

---

## 7) Developer notes (expected repository behavior)
As the project evolves, CMake should be set up so that:
- runtime DLLs are copied next to `snake.exe` automatically, and
- `assets/` is available in the runtime working directory.

If this is not yet implemented, use the manual copy steps in **Troubleshooting**.
