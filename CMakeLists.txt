cmake_minimum_required(VERSION 3.20)

project(snake LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)

pkg_check_modules(LUA lua)
if(NOT LUA_FOUND)
  pkg_check_modules(LUA lua5.4 REQUIRED)
endif()

add_executable(snake src/main.cpp)

target_include_directories(snake PRIVATE ${SDL2_INCLUDE_DIRS} ${LUA_INCLUDE_DIRS})
target_link_directories(snake PRIVATE ${SDL2_LIBRARY_DIRS} ${LUA_LIBRARY_DIRS})
target_link_libraries(snake PRIVATE ${SDL2_LIBRARIES} ${LUA_LIBRARIES})

target_compile_definitions(snake PRIVATE SDL_MAIN_HANDLED)
target_compile_options(snake PRIVATE ${SDL2_CFLAGS_OTHER} ${LUA_CFLAGS_OTHER})
