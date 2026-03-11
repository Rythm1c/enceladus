cmake_minimum_required(VERSION 3.16)
project(Enceladus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages (installed via package manager)
# When using GCC (MSYS/MinGW/UCRT) avoid the SDL2 config shipped with the
# Vulkan SDK; those headers conflict with MSVC and aren't the ones we
# actually installed via pacman.  Prefer the SDL2 from the MinGW or UCRT prefix.
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if(DEFINED ENV{MSYSTEM})
        if($ENV{MSYSTEM} MATCHES "UCRT")
            set(SDL2_DIR "/ucrt64/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config")
        else()
            set(SDL2_DIR "/mingw64/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config")
        endif()
    else()
        # default to mingw64 if MSYSTEM is not set
        set(SDL2_DIR "/mingw64/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config")
    endif()
endif()

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# Add subdirectories
add_subdirectory(src)
