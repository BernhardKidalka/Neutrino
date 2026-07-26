# Neutrino

![Alt text](doc/logo/Neutrino_splash_screen.png)

**Neutrino** is a Real-Time **Rendering and Compute Engine** implemented with **Vulkan** and **C++**.
Neutrino is based on the engine example implementation from the Khronos Vulkan Tutorial (https://docs.vulkan.org/tutorial/latest/00_Introduction.html).

## Prerequisites

- **CMake** >= 3.20
- **C++20** compatible compiler (MSVC, GCC, Clang)
- **Vulkan SDK** installed and available on system
- **Git** (for submodule support)

## Clone Repository with Submodules

```bash
git clone https://github.com/BernhardKidalka/Neutrino.git
cd Neutrino
git submodule update --init --recursive
```

## Building Neutrino

### Windows (example for Visual Studio 2022)

```
cmake -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

### Windows (example for Visual Studio 2026)

```
cmake -B build -G "Visual Studio 18 2026"
cmake --build build --config Release
```

### Linux (Unix makefiles)

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Project Structure

```
Neutrino/
├── CMakeLists.txt              # Main CMake configuration
├── third_party/
│   └── glfw/                   # GLFW (window abstraction) - Git submodule
├── engine/
│   ├── CMakeLists.txt          # Engine library configuration
│   ├── include/
│   │    └── engine.h
│   └── core/
│        ├── engine.cpp
│        ├── logger.h
│        └── logger.cpp
└── src/
    ├── CMakeLists.txt          # Executable configuration
    └── main.cpp
```

## Dependencies

- **Vulkan SDK** - Graphics API
- **GLFW** (submodule) - Window and input abstraction
