cmake_minimum_required(VERSION 3.21)
project(palace CXX)

set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED)

set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext/stb")
include_directories(${STB_INCLUDE_DIR})
link_directories(${STB_INCLUDE_DIR})

add_subdirectory(ext)
add_subdirectory(source)
