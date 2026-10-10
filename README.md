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

## Native palette and sunlight

World rendering is gently desaturated (86% saturation) with a 3% cool tint, based on supplied Alpha/Omega references. This is a stylistic approximation, not an exact recreation of their renderer. A native D3D11 pass runs once before the observed orthographic HUD draw; menus are skipped when no native world scene is observed. If a scene boundary cannot be identified, grading is skipped. ReShade files and effects are untouched; existing ReShade effects may still influence the final image.

`Mods/NativePalette.ini`: Enabled=1, SaturationPercent=86, CoolTintPercent=3. Set Enabled=0 to disable grading after restarting. The palette is included in MoonandShadow.dll; no additional DLL is required. The world color copy and fullscreen triangle add some GPU bandwidth/work, with no CPU GPU-readback stall. Resources follow the actual scene dimensions rather than assuming 1080p. Ordinary 2D targets and MSAA resolve are supported; other target layouts are skipped.

`Mods/SteamShadows.ini`: SunWarmthPercent=3 adds a small warm brightness contribution only to sun-facing, unshadowed surfaces in the shadow receiver range, fading toward its edge and sunset. It reuses the existing shadow receiver pass and adds no caster pass or shadow map. Nighttime illumination is unchanged. Native local-light protection and lantern fill are retained.

Graphics tests check palette colors at 720p, 1080p and 360p, render-state restoration and released target bindings, and warm sunlight/shadow lighting. Live gameplay appearance, UI boundary detection and performance still require verification on the supported game.
