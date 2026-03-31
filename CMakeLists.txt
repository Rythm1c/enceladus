cmake_minimum_required(VERSION 3.16)
project(Enceladus)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# SDL2 — prefer the MinGW/UCRT installation over the Vulkan SDK copy
# ---------------------------------------------------------------------------
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    if(DEFINED ENV{MSYSTEM})
        if($ENV{MSYSTEM} MATCHES "UCRT")
            set(SDL2_DIR "/ucrt64/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config")
        else()
            set(SDL2_DIR "/mingw64/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config")
        endif()
    else()
        set(SDL2_DIR "/mingw64/lib/cmake/SDL2" CACHE PATH "SDL2 CMake config")
    endif()
endif()

find_package(Vulkan REQUIRED)
find_package(SDL2 REQUIRED)

# ---------------------------------------------------------------------------
# Subdirectories — order matters: external libs before src so their targets
# exist when src/CMakeLists.txt calls target_link_libraries.
# ---------------------------------------------------------------------------
add_subdirectory(external)
add_subdirectory(math)
add_subdirectory(renderer)
add_subdirectory(physics)
add_subdirectory(scene)
add_subdirectory(src)