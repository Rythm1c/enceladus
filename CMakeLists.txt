cmake_minimum_required(VERSION 3.16)
project(Enceladus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# Add subdirectories
add_subdirectory(src)
