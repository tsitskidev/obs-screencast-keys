cmake_minimum_required(VERSION 3.28)
project(screencast-keys CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- libobs -----------------------------------------------------------------
# Rather than depending on obs-studio's own install/export CMake package
# (which requires a full `cmake --install`, not just building the `libobs`
# target), this points directly at a locally-built obs-studio checkout's
# build tree. See BUILDING.md for how that tree is produced (a from-source
# build of just the `libobs` target, UI/browser/plugins/scripting disabled,
# matching the installed OBS Studio version).
set(OBS_STUDIO_SOURCE_DIR "${CMAKE_SOURCE_DIR}/../build-deps/obs-studio" CACHE PATH
    "Path to an obs-studio checkout with a built libobs (build_x64/libobs/<config>/obs.{lib,dll})")
set(OBS_BUILD_CONFIG "RelWithDebInfo" CACHE STRING "Config subdirectory used when libobs was built")

add_library(obs-libobs SHARED IMPORTED)
set_target_properties(obs-libobs PROPERTIES
    IMPORTED_IMPLIB "${OBS_STUDIO_SOURCE_DIR}/build_x64/libobs/${OBS_BUILD_CONFIG}/obs.lib"
    IMPORTED_LOCATION "${OBS_STUDIO_SOURCE_DIR}/build_x64/libobs/${OBS_BUILD_CONFIG}/obs.dll"
    INTERFACE_INCLUDE_DIRECTORIES "${OBS_STUDIO_SOURCE_DIR}/libobs;${OBS_STUDIO_SOURCE_DIR}/build_x64/config"
)

# --- libuiohook ---------------------------------------------------------------
# Pinned to the exact commit tag 1.2.2 points to (not the floating `master`
# branch) for reproducible builds. Built as a static lib and linked privately
# so its symbols don't leak into the plugin's public surface.
include(FetchContent)
FetchContent_Declare(
    uiohook
    GIT_REPOSITORY https://github.com/kwhat/libuiohook.git
    GIT_TAG 23acecfe207f8a8b5161bec97a8a6fd6ad0aea88 # v1.2.2
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(uiohook)

# --- plugin -------------------------------------------------------------------
add_library(screencast-keys MODULE
    src/plugin-main.cpp
    src/screencast-keys-source.cpp
    src/input/event-types.cpp
    src/input/global-input-hook.cpp
    src/input/input-hook-manager.cpp
    src/state/event-history.cpp
    src/state/input-state.cpp
)

target_link_libraries(screencast-keys PRIVATE obs-libobs uiohook)

set_target_properties(screencast-keys PROPERTIES
    PREFIX ""
    OUTPUT_NAME "screencast-keys"
)

if(MSVC)
    target_compile_options(screencast-keys PRIVATE /W4)
endif()
