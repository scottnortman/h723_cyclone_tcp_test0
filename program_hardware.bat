@echo off
echo === Programming STM32H723 with Updated UAVCAN Firmware ===
echo.

set PROGRAMMER="C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
set FIRMWARE="build\debug\build\h723_cyclone_tcp_test0.hex"

echo Checking if firmware file exists...
if not exist %FIRMWARE% (
    echo ERROR: Firmware file not found: %FIRMWARE%
    echo Please build the project first using build.bat
    pause
    exit /b 1
)

echo Firmware file found: %FIRMWARE%
echo.

echo Programming hardware...
%PROGRAMMER% -c port=SWD -w %FIRMWARE% -v -rst

if %ERRORLEVEL% EQU 0 (
    echo.
    echo SUCCESS: Hardware programmed successfully!
    echo The STM32H723 now has the updated firmware with real test implementations.
    echo.
    echo You can now run tests using:
    echo   .\test_simple_uavcan.ps1
    echo.
) else (
    echo.
    echo ERROR: Programming failed!
    echo Make sure:
    echo   1. STM32H723 is connected via ST-Link
    echo   2. ST-Link drivers are installed
    echo   3. Hardware is powered on
    echo.
)

pause