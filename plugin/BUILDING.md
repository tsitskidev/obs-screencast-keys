# Building (Windows)

These are the exact steps used to build and deploy this plugin during
development against a locally-installed **OBS Studio 32.0.4**. Adjust the
version-specific bits (git tag, SDK version) if your installed OBS differs.

## Prerequisites

- Visual Studio 2022 (Community is fine) with the "Desktop development with
  C++" workload.
- **Windows SDK 10.0.20348.0 or newer.** OBS's own CMake enforces this
  minimum. If `cmake --build` fails with `compilerconfig.cmake: OBS requires
  Windows SDK version 10.0.20348.0 or more recent`, install a newer one:
  ```
  winget install --id Microsoft.WindowsSDK.10.0.22621
  ```
  (a standalone SDK install, not a full Visual Studio component change --
  no admin-elevated VS modify required beyond winget's own install prompt).
- CMake 3.28+, Git, and a working internet connection (this project fetches
  libuiohook and FreeType via `FetchContent`, and obs-studio's own CMake
  fetches ~1-1.5GB of prebuilt dependencies -- see below).

## 1. Build a local `libobs` matching your installed OBS version

Third-party OBS plugins normally link against a prebuilt OBS SDK. That SDK
isn't distributed separately for Windows, so this builds `libobs` from the
matching obs-studio source tag instead -- UI, browser (CEF), plugins, and
scripting all disabled, so it's just the core library, no Qt required.

```sh
git clone --branch 32.0.4 --depth 1 https://github.com/obsproject/obs-studio.git build-deps/obs-studio
cd build-deps/obs-studio

cmake -S . -B build_x64 -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_SYSTEM_VERSION=10.0.22621.0 \
  -DENABLE_FRONTEND=OFF \
  -DENABLE_BROWSER=OFF \
  -DENABLE_PLUGINS=OFF \
  -DENABLE_SCRIPTING=OFF

cmake --build build_x64 --target libobs --config RelWithDebInfo --parallel
```

Notes:
- Replace `32.0.4` with your installed OBS Studio's version (`obs64.exe`'s
  file version, or Help > About in OBS).
- The configure step downloads obs-studio's prebuilt Windows dependency
  package (FFmpeg, zlib, jansson, etc. -- needed even for `libobs` alone)
  **and**, due to how `obs-studio`'s Windows dependency-fetch script is
  keyed purely off architecture (not `ENABLE_FRONTEND`/`ENABLE_BROWSER`),
  it *also* unconditionally fetches the Qt6 prebuilt package for x64/arm64
  builds even though this plugin never uses it. Budget ~1-1.5GB of download
  for this step; CEF itself (the largest one) is correctly skipped via
  `ENABLE_BROWSER=OFF`.
- Only the `libobs` target is built (not `--target ALL`), so this doesn't
  build the OBS application itself.

## 2. Configure and build the plugin

```sh
cd plugin

cmake -S . -B build -G "Visual Studio 17 2022" -A x64 \
  -DCMAKE_SYSTEM_VERSION=10.0.22621.0 \
  -DADVAPI32="C:/Program Files (x86)/Windows Kits/10/Lib/10.0.22621.0/um/x64/Advapi32.lib"

cmake --build build --config RelWithDebInfo --parallel
```

The `-DADVAPI32=...` cache variable is a workaround: libuiohook's Windows
CMake does a bare `find_library(ADVAPI32 Advapi32)` with no search hints,
which fails to resolve in some environments. Pre-seeding the cache variable
with the SDK's actual `Advapi32.lib` path short-circuits that failing
lookup (CMake won't re-run `find_library` if the variable is already set).

This produces `build/RelWithDebInfo/screencast-keys.dll`, statically linked
against libuiohook and FreeType (its only runtime DLL dependency besides
`obs.dll` and the standard CRT is confirmed via `dumpbin /dependents`).

CMake variables you can override (see the top of `CMakeLists.txt`):
- `OBS_STUDIO_SOURCE_DIR` -- defaults to `../build-deps/obs-studio`.
- `OBS_BUILD_CONFIG` -- defaults to `RelWithDebInfo`, must match whatever
  config you built `libobs` with in step 1.

## 3. Deploy without admin rights

OBS Studio (28+) automatically scans a per-machine plugin directory under
`%ProgramData%` in addition to its own `Program Files\obs-studio\obs-plugins`
tree -- writable without elevation, so this is the easiest way to iterate:

```
C:\ProgramData\obs-studio\plugins\screencast-keys\bin\64bit\screencast-keys.dll
C:\ProgramData\obs-studio\plugins\screencast-keys\data\locale\en-US.ini
```

Copy the built DLL and this repo's `data/` folder into that layout, then
(re)start OBS Studio. `Screencast Keys` should appear in Sources > Add.

If OBS is already running with the plugin loaded, Windows will refuse to
overwrite the DLL (`Device or resource busy` / access denied) -- close OBS
first.

## 4. Verifying it loaded

Check `%APPDATA%\obs-studio\logs\` (the most recent log) for
`[screencast-keys] plugin loaded`, or any load errors. `dumpbin /exports`
on the built DLL should list `obs_module_load`, `obs_module_ver`,
`obs_module_set_pointer`, etc.

## Running the unit tests (no OBS/libuiohook dependency)

```sh
cd tests
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_POLICY_VERSION_MINIMUM=3.5
cmake --build build --config Debug
./build/Debug/screencast-keys-tests.exe
```

These cover `EventHistory`'s debounce/repeat-collapsing/expiry state machine,
`InputState`'s Left/Right-aware modifier tracking, and `event-types`'
keycode-to-display-name mapping -- the highest port-fidelity-risk logic --
independent of whether the full OBS build works.
