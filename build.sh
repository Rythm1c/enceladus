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

# Configure with CMake
echo "Configuring CMake..."
cmake ..

# Build the project
echo "Building project..."
cmake --build .

# Return to project root
cd ..

echo "Build complete! Run with: ./build/src/enceladus"
