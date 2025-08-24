# STM32H723 CycloneTCP Project - Build Instructions

## Prerequisites

### Required Tools
1. **CMake** (version 3.20 or higher)
   - Download from: https://cmake.org/download/
   - Make sure it's added to your PATH

2. **Ninja Build System** (recommended)
   - Download from: https://ninja-build.org/
   - Or install via package manager
   - Add to your PATH

3. **ARM GCC Toolchain**
   - STM32CubeCLT (recommended): https://www.st.com/en/development-tools/stm32cubeclt.html
   - Or STM32CubeIDE (includes toolchain)
   - The project will auto-detect the toolchain location

### VS Code Extensions
Install the recommended extensions when prompted, or manually install:
- CMake Tools
- C/C++ Extension Pack
- STM32 VS Code Extension
- Cortex-Debug (for debugging)
- Hex Editor
- ARM Assembly

## Building the Project

### Method 1: Using VS Code (Recommended)
1. Open the project folder in VS Code
2. When prompted, select the CMake kit (should auto-detect ARM GCC)
3. Use Ctrl+Shift+P and run "CMake: Configure"
4. Use Ctrl+Shift+P and run "CMake: Build" or press F7
5. Or use the CMake extension's status bar buttons

### Method 2: Using Command Line
1. Open terminal in project root
2. Run the build script:
   ```cmd
   build.bat
   ```

### Method 3: Manual CMake Commands
```cmd
# Configure (Debug)
cmake --preset debug

# Build (Debug)
cmake --build --preset debug

# Configure and Build (Release)
cmake --preset release
cmake --build --preset release
```

## Build Outputs
- **ELF file**: `build/debug/build/h723_cyclone_tcp_test0.elf`
- **HEX file**: `build/debug/build/h723_cyclone_tcp_test0.hex`
- **BIN file**: `build/debug/build/h723_cyclone_tcp_test0.bin`

## Debugging
1. Connect your STM32H723 Nucleo board
2. Press F5 in VS Code to start debugging
3. The launch configuration will automatically build and flash the firmware

## Troubleshooting

### CMake Configuration Issues
- Ensure ARM GCC toolchain is installed and detected
- Check that STM32CubeCLT or STM32CubeIDE is properly installed
- Verify CMAKE_TOOLCHAIN_FILE path in CMakePresets.json

### Build Errors
- Clean build directory: `cmake --build --preset debug --target clean`
- Reconfigure: Delete `build` folder and run configure again
- Check that all source files exist and paths are correct

### IntelliSense Issues
- Ensure CMake Tools extension is active
- Run "C/C++: Reset IntelliSense Database" from command palette
- Check that c_cpp_properties.json uses CMake configuration provider

## Project Structure
- `Core/`: Application source code
- `Drivers/`: STM32 HAL drivers and BSP
- `Middlewares/`: FreeRTOS and CycloneTCP stack
- `cmake/`: CMake configuration files
- `.vscode/`: VS Code configuration
- `build/`: Build output directory (generated)