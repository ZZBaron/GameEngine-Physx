# GameEngine PhysX
A 3D physics engine using OpenGL and PhysX.

# Prerequisites

- C++20 compatible compiler
- CMake 3.15 or higher
- Git
- Internet connection (for vcpkg dependencies)

---

## Quick Start

# 1. Clone the repository

```bash
git clone https://github.com/ZZBaron/GameEngine-PhysX
cd GameEngine-PhysX
```

# 2. Install and configure vcpkg

```batch
git clone https://github.com/Microsoft/vcpkg.git
```


### Windows
```batch
.\vcpkg\bootstrap-vcpkg.bat
set VCPKG_ROOT=%CD%\vcpkg
```


### Linux/Macos
```batch
./vcpkg/bootstrap-vcpkg.sh
export VCPKG_ROOT=$(pwd)/vcpkg
```

# 3. Build with CMake (platform-specific commands below)

### Windows
```batch
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release
```

### Linux/Macos
```batch
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

# Installing Prerequisites

### Windows
1. Install Visual Studio with "Desktop development with C++" workload
   - Or you can try installing MinGW-w64/MSYS2, but MinGW is not officially supported by PhysX
2. Install CMake from https://cmake.org/download/

### macOS
1. Install Xcode Command Line Tools:
```bash
xcode-select --install
```
2. Install CMake:
```bash
brew install cmake
```

### Linux (Ubuntu/Debian)
1. Install required packages:
```bash
sudo apt update
sudo apt install build-essential cmake git
```

## Running the Application

### Windows
```bash
.\build\Release\GameEnginePhysx.exe
```

### Linux/macOS
```bash
./build/GameEnginePhysx
```

---

## Features
- OpenGL rendering
- PhysX physics simulation
- Real-time 3D graphics
- Rigid body dynamics
- Shadow mapping
- Interactive camera controls

## Controls
- **Space**: Toggle camera movement
- **WASD**: Move camera when enabled
- **Mouse**: Look around when camera movement enabled
- **P**: Toggle menu
- **G**: Generate physics objects
- **L**: Play/pause simulation

## License
This project is licensed under the MIT License - see the LICENSE.txt file for details.

## Third-Party Licenses  
This project uses the following third-party libraries:  

1. **PhysX**: Licensed under the [NVIDIA Source Code License Agreement](https://github.com/NVIDIA/PhysX/blob/main/LICENSE.md).  

2. **OpenGL and GLFW**: GLFW is licensed under the [zlib/libpng license](https://github.com/glfw/glfw/blob/main/LICENSE.md).  

3. **vcpkg**: Licensed under the [MIT License](https://github.com/microsoft/vcpkg/blob/master/LICENSE.txt).  

4. **Dear ImGui**: Licensed under the [MIT License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt).  

These licenses apply to their respective components only.

