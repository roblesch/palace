cmake_minimum_required(VERSION 3.21)
project(palace CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/stb")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ext/imgui")
include_directories("src/")

add_subdirectory(ext)
add_subdirectory(src)
