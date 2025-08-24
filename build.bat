@echo off
echo Building STM32H723 CycloneTCP Project...

REM Check if cmake is available
cmake --version >nul 2>&1
if %errorlevel% neq 0 (
    echo ERROR: CMake is not installed or not in PATH
    echo Please install CMake and add it to your PATH
    pause
    exit /b 1
)

REM Check if ninja is available
ninja --version >nul 2>&1
if %errorlevel% neq 0 (
    echo WARNING: Ninja is not installed. Using default generator.
    set GENERATOR=
) else (
    set GENERATOR=-G Ninja
)

REM Configure and build
echo Configuring project...
cmake --preset debug %GENERATOR%
if %errorlevel% neq 0 (
    echo ERROR: Configuration failed
    pause
    exit /b 1
)

echo Building project...
cmake --build --preset debug
if %errorlevel% neq 0 (
    echo ERROR: Build failed
    pause
    exit /b 1
)

echo Build completed successfully!
echo Output files are in: build\debug\build\
pause