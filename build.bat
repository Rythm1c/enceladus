@echo off
REM Build script for Enceladus project (Windows)

echo Building Enceladus...

REM Create build directory if it doesn't exist
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

REM Navigate to build directory
cd build

REM Configure with CMake

echo Configuring CMake...

REM choose a default generator: prefer Unix Makefiles only when in an
REM MSYS2/MinGW shell, otherwise use Visual Studio.  Users can override by
REM setting the environment variable GENERATOR before invoking this script.
if not defined GENERATOR (
    if defined MSYSTEM (
        rem MSYS2 sets MSYSTEM to values like MINGW64, UCRT64, MSYS, etc.
        echo Detected MSYSTEM=%MSYSTEM%
        echo | findstr /I "MINGW\|UCRT" >nul
        if %ERRORLEVEL%==0 (
            set "GENERATOR=Unix Makefiles"
        )
    )
    if not defined GENERATOR (
        rem Fall back to Visual Studio; adjust version as necessary
        rem use the 2022 string (version 17) which is the installed
        rem generator; if you have a different VS version change this.
        set "GENERATOR=Visual Studio 17 2022"
    )
)

echo Using generator '%GENERATOR%'
cmake -G "%GENERATOR%" ..

REM Build the project
echo Building project...
cmake --build .

REM Compile shaders if glslangValidator is available
if exist "C:\VulkanSDK\1.4.321.1\Bin\glsl.exe" (
    echo Compiling shaders...
    if not exist "build\shaders" (
        mkdir build\shaders
    )
    "C:\VulkanSDK\1.4.321.1\Bin\glslc.exe" shaders\shader.vert -o build\shaders\shader.vert.spv
    "C:\VulkanSDK\1.4.321.1\Bin\glslc.exe" shaders\shader.frag -o build\shaders\shader.frag.spv
    echo Shaders compiled to build\shaders\
) else (
    echo glslangValidator not found at C:\VulkanSDK\1.4.321.1\Bin\glslangValidator.exe. Install Vulkan SDK or update the path.
)

REM Return to project root
cd ..

echo Build complete! Run with: build\enceladus.exe
pause
