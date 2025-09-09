epiCBattle
===========

CMake-based raylib prototype: menu, character select (Asgore/Metrocop), and a placeholder arena.

Prerequisites
-------------
- CMake 3.20+
- Git (FetchContent pulls raylib)
- One of the following build toolchains:
  - Visual Studio 2022 (Desktop development with C++)
  - Ninja + LLVM/Clang or MinGW
  - NMake Makefiles via Visual Studio Developer Prompt

Recommended build (Ninja)
-------------------------
1) Install Ninja and a compiler (e.g., LLVM from MSYS/Choco) and ensure they are in PATH.
2) Configure and build:

```powershell
cmake -S . -B build-ninja -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build-ninja --config Release
./build-ninja/epiCBattle.exe
```

Alternative: Visual Studio generator
------------------------------------
Open a "x64 Native Tools Command Prompt for VS 2022" and run:

```bat
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Debug
build-vs\Debug\epiCBattle.exe
```

Notes
-----
- The `models` folder is copied next to the executable on build.
- Controls:
  - Menu: Enter
  - Character Select: Left/Right (or mouse wheel), Enter confirm, Esc back
  - Arena: Mouse orbital camera, Esc back
- GLTFs load via raylib's tinygltf; textures are assigned from known filenames in each model's `textures` folder.

