# Top-level project CMake for betaflight-host
#
# This project vendors ESP-IDF as a git submodule under ./esp-idf so builds are
# reproducible without a system-wide IDF install. See README.md for setup.

cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Sources live under src/main rather than the default ./main.
set(EXTRA_COMPONENT_DIRS "src")

project(betaflight-host)
