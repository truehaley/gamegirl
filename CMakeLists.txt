cmake_minimum_required(VERSION 3.24)
cmake_policy(SET CMP0168 NEW) # The FetchContent module implements steps directly instead of through a sub-build
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_STANDARD 17)

########
## Define the main Colorweave project!
########
# Note: this is also what triggers inclusion of the toolchain file, so it must be done early!
project(gamegirl
        VERSION 0.1
        DESCRIPTION "GameGirl"
        LANGUAGES C CXX
)
add_executable(gamegirl)

########
## Common configuration
########
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/external)
include(FetchContent)

# always generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

########
## Dependencies
########
set(RAYLIB_VERSION 5.5)
set(RAYGUI_VERSION 4.0)

########
## RayLib / RayGui
########
## Raylib
FetchContent_Declare(
    raylib
    URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/raylib-${RAYLIB_VERSION}
)
## Raygui
FetchContent_Declare(
    raygui
    URL https://github.com/raysan5/raygui/archive/refs/tags/${RAYGUI_VERSION}.tar.gz
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/raygui-${RAYGUI_VERSION}
    SOURCE_SUBDIR projects/CMake
)
FetchContent_MakeAvailable(raylib raygui)

## Mooneye Test Suite
FetchContent_Declare(
    mooneye
    URL https://gekkio.fi/files/mooneye-test-suite/mts-20260714-0944-31510e1/mts-20260714-0944-31510e1.tar.gz
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/external/mooneye
    DOWNLOAD_EXTRACT_TIMESTAMP OFF
)
FetchContent_MakeAvailable(mooneye)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

########
## argp-standalone
########
# Must be installed separately, for example from 'brew install argp-standalone'
add_library(argp_standalone INTERFACE)
target_link_libraries(argp_standalone INTERFACE argp)
target_link_directories(argp_standalone INTERFACE /opt/homebrew/Cellar/argp-standalone/1.5.0/lib)
target_include_directories(argp_standalone INTERFACE /opt/homebrew/Cellar/argp-standalone/1.5.0/include)


########
# common config
########
target_include_directories(gamegirl PRIVATE
    include
    src
)

target_sources(gamegirl PRIVATE
    src/audio.cpp
    src/cartridge.cpp
    src/controls.c
    src/cpu_dis2.c
    src/cpu.cpp
    src/display.cpp
    src/gb.c
    src/gui.c
    src/main.c
    src/mem.c
    src/raygui_impl.c
    src/serial.c
    src/timer.c
)

target_compile_options(gamegirl PRIVATE
    -Wno-c99-designator
)

target_link_libraries(gamegirl
    argp_standalone
    raylib
    raygui
)
