# ---------------------------------------------------------------------------
# external/CMakeLists.txt
# Each third-party library lives in its own subdirectory and exposes a
# CMake target that src/CMakeLists.txt can link against by name.
# ---------------------------------------------------------------------------
add_subdirectory(math)
add_subdirectory(imgui)