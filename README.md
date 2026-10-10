# MoonandShadow

A single CubeModLoader API 7.3 mod combining moon lighting, a visible moon disk, dimmer night clouds, and real-time sun/moon/cloud shadows. Made by DieuDeGlace with the aid of AI (OpenAI ChatGPT/Codex). MIT licensed; see THIRD_PARTY_NOTICES.md for SDK attribution. Source migrated from SteamCubeWorldMods.

## Build

Requires Windows x64, CMake 3.20+, Ninja, Visual Studio 2022 C++ Build Tools, Windows SDK and clang-cl. In an x64 Native Tools command prompt with those tools on PATH:

```bat
git clone https://github.com/DieuDeGlace/MoonandShadow.git
cd MoonandShadow
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_BUILD_TYPE=RelWithDebInfo -DENABLE_GPU_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

## Install

Close Steam Cube World, remove MoonLight.dll and SteamShadows.dll from Mods, and install build/MoonandShadow.dll instead. Do not load the old standalone DLLs alongside it. Existing Mods/MoonLight.ini and Mods/SteamShadows.ini settings are retained; examples are in config. F7 toggles shadows. WorldMotion is optional for animated caster coordination. ReShade is untouched.

Supports the inspected Steam executable (PE timestamp 0x5d93ab0d, image size 0x5b7000); Alpha and Omega executables are unsupported. Moon and sun share one shadow map. GPU tests check rendering resources, filtering, cloud casting and moon rendering; they do not guarantee FPS or visual correctness in every location.
