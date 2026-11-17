cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake CACHE STRING "")

project(tts_server VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(fmt CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(gtest CONFIG REQUIRED)

find_path(MINIAUDIO_INCLUDE_DIRS "miniaudio.h")

add_subdirectory(app)
add_subdirectory(src)
add_subdirectory(audio)