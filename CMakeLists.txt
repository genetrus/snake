cmake_minimum_required(VERSION 3.20)

project(snake VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(snake
    src/main.cpp
)

target_include_directories(snake PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

if(EXISTS "${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDeps.cmake")
    include(${CMAKE_SOURCE_DIR}/cmake/CopyRuntimeDeps.cmake)
endif()
