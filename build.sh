#!/bin/bash

# Build script for Phobos project (Unix/Linux/macOS)

set -e  # Exit on error

echo "Building Enceladus..."

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
    echo "Creating build directory..."
    mkdir build
fi

# Navigate to build directory
cd build

# Ensure cmake is available
if ! command -v cmake >/dev/null 2>&1; then
    echo "Error: cmake not found in PATH."
    echo "On MSYS2 install it with: pacman -S mingw-w64-x86_64-cmake"
    echo "and restart your MinGW64 or UCRT64 shell."
    exit 1
fi

# If we're going to use the Unix Makefiles generator, make sure make (or
# an equivalent) is on PATH. MSYS2 sometimes installs the program as
# mingw32-make or mingw64-make instead of plain "make".
MAKE_CMD=""
if [ -n "$MSYSTEM" ]; then
    # only check for make when we're in an MSYS2-derived environment
    if echo "$MSYSTEM" | grep -Ei "mingw|ucrt" >/dev/null 2>&1; then
        if command -v make >/dev/null 2>&1; then
            MAKE_CMD=make
        elif command -v mingw32-make >/dev/null 2>&1; then
            MAKE_CMD=mingw32-make
        elif command -v mingw64-make >/dev/null 2>&1; then
            MAKE_CMD=mingw64-make
        else
            echo "Error: make not found in PATH (searched for make, mingw32-make, mingw64-make)."
            echo "Install it with: pacman -S mingw-w64-x86_64-make"
            echo "and restart your shell."
            exit 1
        fi
    fi
fi


# Configure with CMake
# in MSYS2 we explicitly pick the Unix Makefiles generator to avoid
# accidentally using MSVC when cl.exe is on PATH
GEN_ARGS=()
if [ -n "$MSYSTEM" ]; then
    case "$MSYSTEM" in
        MINGW*|UCRT*|msys)
            echo "Detected MSYS2 environment ($MSYSTEM), forcing Unix Makefiles generator"
            GEN_ARGS+=("-G" "Unix Makefiles")
            GEN_ARGS+=("-DCMAKE_C_COMPILER=gcc" "-DCMAKE_CXX_COMPILER=g++")
            ;;
    esac
fi

echo "Configuring CMake..."
# If MAKE_CMD was chosen above, pass it explicitly so CMake can find the
# make program even if it's named mingw32-make.
if [ -n "$MAKE_CMD" ]; then
    cmake "${GEN_ARGS[@]}" -DCMAKE_MAKE_PROGRAM=$MAKE_CMD ..
else
    cmake "${GEN_ARGS[@]}" ..
fi

# Build the project
echo "Building project..."
cmake --build .

# Compile shaders if glslc is available
if [ -f "/c/VulkanSDK/1.4.321.1/Bin/glslc.exe" ]; then
    echo "Compiling shaders..."
    mkdir -p shaders
    ls ..
    "/c/VulkanSDK/1.4.321.1/Bin/glslc.exe" ../shaders/shader.vert -o shaders/shader.vert.spv
    "/c/VulkanSDK/1.4.321.1/Bin/glslc.exe" ../shaders/shader.frag -o shaders/shader.frag.spv
    echo "Shaders compiled to build/shaders/"
else
    echo "glslc not found at /c/VulkanSDK/1.4.321.1/Bin/glslc.exe. Install Vulkan SDK or update the path."
fi

# Return to project root
cd ..

echo "Build complete! Run with: ./build/src/enceladus"
