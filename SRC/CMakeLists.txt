cmake_minimum_required(VERSION 3.16)
project(playdate_runtime C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_AARCH64 "Cross-compile for aarch64 (PortMaster)" OFF)

if(BUILD_AARCH64)
    set(CMAKE_SYSTEM_NAME Linux)
    set(CMAKE_SYSTEM_PROCESSOR aarch64)
    set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
    set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)
    set(CMAKE_FIND_ROOT_PATH /usr/aarch64-linux-gnu)
    set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
    set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
    set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2MIXER REQUIRED SDL2_mixer)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/vendor/lua
    ${SDL2_INCLUDE_DIRS}
    ${SDL2MIXER_INCLUDE_DIRS}
)

link_directories(${SDL2_LIBRARY_DIRS} ${SDL2MIXER_LIBRARY_DIRS})

file(GLOB LUA_SRC ${CMAKE_SOURCE_DIR}/vendor/lua/*.c)

file(GLOB RUNTIME_SRC
    ${CMAKE_SOURCE_DIR}/src/*.c
)

add_executable(playdate_runtime ${RUNTIME_SRC} ${LUA_SRC})

target_compile_definitions(playdate_runtime PRIVATE
    LUA_USE_LINUX
    LUA_32BITS
)

target_link_libraries(playdate_runtime PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2MIXER_LIBRARIES}
    m
    z
    dl
    pthread
)

install(TARGETS playdate_runtime DESTINATION bin)
