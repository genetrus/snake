cmake_minimum_required(VERSION 3.24)

project(snake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(snake
    src/main.cpp
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
target_compile_definitions(snake PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
target_link_libraries(snake
    PRIVATE
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
