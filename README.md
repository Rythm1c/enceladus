# Phobos - Vulkan/SDL2 Project

A template project for developing with Vulkan and SDL2 using CMake.

## Prerequisites

Install the required development libraries:

### Ubuntu/Debian:
```bash
sudo apt-get install cmake build-essential
sudo apt-get install libvulkan-dev vulkan-tools
sudo apt-get install libsdl2-dev
```

### Fedora/RHEL:
```bash
sudo dnf install cmake gcc-c++ make
sudo dnf install vulkan-loader-devel vulkan-tools
sudo dnf install SDL2-devel
```

### macOS:
```bash
brew install cmake vulkan-headers sdl2
```

## Building the Project

1. Create a build directory:
```bash
mkdir build && cd build
```

2. Generate build files with CMake:
```bash
cmake ..
```

3. Build the project:
```bash
cmake --build .
```

## Running

```bash
./phobos
```

## Project Structure

```
phobos/
├── CMakeLists.txt           # Root CMake configuration
├── src/
│   ├── CMakeLists.txt       # Source directory build config
│   └── main.cpp             # Main entry point
└── README.md                # This file
```

## Next Steps

- Implement Vulkan instance creation and device selection
- Add rendering pipeline and frame rendering
- Implement input handling and window management improvements
- Add resource management (textures, models, shaders)
