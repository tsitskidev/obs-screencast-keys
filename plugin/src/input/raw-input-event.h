#pragma once

#include <cstdint>

namespace sk {

enum class RawInputKind {
    KeyPressed,
    KeyReleased,
    MousePressed,
    MouseReleased,
    MouseWheel,
};

// A minimally-processed input event, already translated out of libuiohook's
// uiohook_event on the hook thread (see global-input-hook.cpp) so that
// nothing downstream of InputHookManager needs to know libuiohook exists.
struct RawInputEvent {
    RawInputKind kind = RawInputKind::KeyPressed;

    // Captured via std::chrono::steady_clock at dispatch time, in seconds.
    // Deliberately not derived from uiohook_event::time -- that field's
    // epoch/units are platform-specific and not something we want the
    // debounce/repeat-collapsing logic in EventHistory to depend on.
    double timestamp = 0.0;

    // For KeyPressed/KeyReleased: a raw libuiohook VC_* keycode.
    // For MousePressed/MouseReleased/MouseWheel: a MOUSE_BUTTON* id (1-5),
    // or for MouseWheel, an arbitrary button id of 0 (see wheel_rotation).
    uint16_t code = 0;

    // Only meaningful for MouseWheel: positive = scroll up/away, negative =
    // scroll down/toward (libuiohook's rotation sign convention).
    int16_t wheel_rotation = 0;
};

} // namespace sk
