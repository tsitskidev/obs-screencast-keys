#pragma once

#include <array>
#include <vector>

#include "../input/event-types.h"

namespace sk {

// Tracks currently-held modifier keys (Left/Right-aware, so releasing one
// Shift doesn't clear a still-held other Shift) and currently-held mouse
// buttons.
//
// Port of ops.py's update_hold_modifier_keys()/update_mouse_buttons_status(),
// with one deliberate change: the Blender addon force-releases all mouse
// buttons on every MOUSEMOVE event because Blender's own event delivery
// misses RELEASE events in some cases (context menus, drags). A global input
// hook sees OS-level press/release directly and doesn't have that problem,
// so that workaround is intentionally not ported -- press/release events are
// tracked directly here.
class InputState {
public:
    // `pressed` is true for a key-down, false for a key-up. `keycode` is a
    // raw libuiohook keycode; only modifier keys affect state (all other
    // keycodes are no-ops here -- they go through EventHistory instead).
    void on_key_event(uint16_t keycode, bool pressed);

    // `button` must be one of the MOUSE_LEFT/RIGHT/MIDDLE/BUTTON4/BUTTON5
    // EventType values; anything else (e.g. wheel) is a no-op.
    void on_mouse_button_event(EventType button, bool pressed);

    // Canonical, order-stable (Shift, Ctrl, Alt, Meta) list of the modifiers
    // currently held -- mirrors sorted_modifier_keys()'s ordering, which is
    // why display always collapses Left/Right regardless of which physical
    // key is down.
    std::vector<EventType> held_modifiers() const;

    bool is_mouse_button_held(EventType button) const;

    void clear();

private:
    // Indexed by (ModifierSide enum value - 1); NONE has no slot.
    std::array<bool, 8> modifier_side_down_{};

    bool mouse_left_ = false;
    bool mouse_right_ = false;
    bool mouse_middle_ = false;
    bool mouse_button4_ = false;
    bool mouse_button5_ = false;
};

} // namespace sk
