@echo off
REM Build script for Phobos project (Windows)

echo Building Phobos...

REM Create build directory if it doesn't exist
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Navigate to build directory
cd build

REM Configure with CMake
echo Configuring CMake...
cmake ..

REM Build the project
echo Building project...
cmake --build .

REM Return to project root
cd ..

echo Build complete! Run with: build\phobos.exe
pause
