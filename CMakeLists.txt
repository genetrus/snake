cmake_minimum_required(VERSION 3.24)

project(snake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(snake
    src/main.cpp
    src/core/App.cpp
    src/core/Input.cpp
    src/core/Time.cpp
    src/audio/AudioSystem.cpp
    src/audio/SFX.cpp
    src/game/Board.cpp
    src/game/Effects.cpp
    src/game/Game.cpp
    src/game/ScoreSystem.cpp
    src/game/Snake.cpp
    src/game/Spawner.cpp
    src/game/StateMachine.cpp
    src/io/Config.cpp
    src/io/AppData.cpp
    src/io/Bootstrap.cpp
    src/io/Highscores.cpp
    src/io/Paths.cpp
    src/lua/LuaRuntime.cpp
    src/lua/Bindings.cpp
    src/render/Animation.cpp
    src/render/Font.cpp
    src/render/SpriteAtlas.cpp
    src/render/UIRenderer.cpp
    src/render/Renderer.cpp
)

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(SDL2_mixer CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(LUA_TARGET "")
find_package(Lua CONFIG QUIET)
if(TARGET Lua::Lua)
    set(LUA_TARGET Lua::Lua)
endif()

if(NOT LUA_TARGET)
    find_package(unofficial-lua CONFIG QUIET)
    if(TARGET unofficial::lua::lua)
        set(LUA_TARGET unofficial::lua::lua)
    endif()
endif()

if(NOT LUA_TARGET)
    find_package(Lua REQUIRED)
    if(NOT TARGET Lua::Lua AND LUA_LIBRARIES)
        add_library(Lua::Lua INTERFACE IMPORTED)
        target_include_directories(Lua::Lua INTERFACE "${LUA_INCLUDE_DIR}")
        target_link_libraries(Lua::Lua INTERFACE ${LUA_LIBRARIES})
    endif()
    set(LUA_TARGET Lua::Lua)
endif()

target_include_directories(snake PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(snake PRIVATE SDL_MAIN_HANDLED NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(snake
    PRIVATE
        shell32
        Ole32
        SDL2::SDL2
        SDL2::SDL2main
        SDL2_image::SDL2_image
        SDL2_ttf::SDL2_ttf
        SDL2_mixer::SDL2_mixer
        nlohmann_json::nlohmann_json
        ${LUA_TARGET}
)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDeps.cmake")
    include(${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDeps.cmake)
    snake_copy_runtime_deps(snake)
endif()
