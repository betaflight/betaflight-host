# Top-level project CMake for betaflight-host
#
# This project vendors ESP-IDF as a git submodule under ./esp-idf so builds are
# reproducible without a system-wide IDF install. See README.md for setup.

cmake_minimum_required(VERSION 3.16)

# Target board: a subdirectory of boards/. Selects the flash size, partition
# table, PSRAM and board identity. Override with: idf.py -DBOARD=<name> ...
# (delete ./sdkconfig and reconfigure when switching boards).
set(BOARD "esp32s3-zero" CACHE STRING "Target board (a subdir of boards/)")

file(GLOB _board_dirs RELATIVE "${CMAKE_CURRENT_LIST_DIR}/boards" "${CMAKE_CURRENT_LIST_DIR}/boards/*")
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}/sdkconfig.defaults")
    message(FATAL_ERROR "Unknown BOARD '${BOARD}'. Available boards: ${_board_dirs}")
endif()

# Common defaults first, then the board's overrides (later files win).
set(SDKCONFIG_DEFAULTS
    "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.defaults"
    "${CMAKE_CURRENT_LIST_DIR}/boards/${BOARD}/sdkconfig.defaults")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Sources live under src/main rather than the default ./main.
set(EXTRA_COMPONENT_DIRS "src")

project(betaflight-host)
