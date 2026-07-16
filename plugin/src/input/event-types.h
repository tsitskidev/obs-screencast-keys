#pragma once

#include <cstdint>
#include <string>

namespace sk {

// Logical input event identifiers. This is our own enum, independent of any
// particular hooking library's keycodes -- input/event-types.cpp is the only
// place that knows how to translate libuiohook's VC_*/MOUSE_BUTTON* numeric
// codes into these. Ports the *meaningful subset* of the Blender addon's
// EventType (derived from bpy.types.Event) that makes sense for a global,
// OS-wide hook: no NDOF/tablet/Blender-internal event identifiers.
enum class EventType : uint16_t {
    UNKNOWN = 0,

    // Alphanumeric zone.
    A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    KEY_0, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9,

    // Function keys.
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
    F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24,

    // Punctuation / symbols.
    MINUS, EQUALS, BACKSPACE, TAB, CAPS_LOCK, BACKQUOTE,
    OPEN_BRACKET, CLOSE_BRACKET, BACK_SLASH, SEMICOLON, QUOTE, ENTER,
    COMMA, PERIOD, SLASH, SPACE,

    ESCAPE, PRINT_SCREEN, SCROLL_LOCK, PAUSE, CONTEXT_MENU,

    // Edit / navigation zone.
    INSERT, DEL, HOME, END, PAGE_UP, PAGE_DOWN,
    UP_ARROW, DOWN_ARROW, LEFT_ARROW, RIGHT_ARROW,

    // Numeric zone.
    NUM_LOCK, KP_DIVIDE, KP_MULTIPLY, KP_SUBTRACT, KP_ADD, KP_ENTER, KP_DECIMAL,
    KP_0, KP_1, KP_2, KP_3, KP_4, KP_5, KP_6, KP_7, KP_8, KP_9,

    // Modifier keys. Left/Right are tracked separately in input-state (so
    // releasing one Shift doesn't clear a still-held other Shift) but always
    // collapse to one of these four canonical values for display, mirroring
    // the Blender addon's fix_modifier_display_text.
    MOD_SHIFT, MOD_CTRL, MOD_ALT, MOD_META,

    // Media keys.
    MEDIA_PLAY, MEDIA_STOP, MEDIA_PREVIOUS, MEDIA_NEXT,
    VOLUME_MUTE, VOLUME_UP, VOLUME_DOWN,

    // Mouse.
    MOUSE_LEFT, MOUSE_RIGHT, MOUSE_MIDDLE, MOUSE_BUTTON4, MOUSE_BUTTON5,
    MOUSE_WHEEL_UP, MOUSE_WHEEL_DOWN,
};

// True for MOD_SHIFT/MOD_CTRL/MOD_ALT/MOD_META.
bool is_modifier(EventType type);

// True for any MOUSE_* value (buttons or wheel).
bool is_mouse_event(EventType type);

// Maps a libuiohook keyboard_event_data.keycode (a VC_* constant) to our
// EventType. Deliberately does not #include uiohook.h -- the VC_* values are
// a stable, documented part of libuiohook's public API, and keeping this
// translation dependency-free makes it unit-testable without pulling in the
// hooking library. Returns EventType::UNKNOWN for anything not mapped
// (matches how the Blender addon silently ignores '' event types).
EventType key_code_to_event_type(uint16_t keycode);

// Maps a libuiohook mouse_event_data.button (a MOUSE_BUTTON* constant, 1-5)
// to our EventType.
EventType mouse_button_to_event_type(uint16_t button);

// Which physical modifier key (if any) a raw keycode identifies. This is the
// one place that knows about Left/Right-specific VC_* codes; input-state.cpp
// tracks hold state per ModifierSide (so releasing one Shift doesn't clear a
// still-held other Shift) without needing to know any raw keycode itself.
enum class ModifierSide {
    NONE,
    SHIFT_L, SHIFT_R,
    CTRL_L, CTRL_R,
    ALT_L, ALT_R,
    META_L, META_R,
};

ModifierSide key_code_to_modifier_side(uint16_t keycode);

// Collapses a physical side to its canonical (display/history) EventType,
// e.g. SHIFT_L and SHIFT_R both -> EventType::MOD_SHIFT.
EventType canonical_modifier(ModifierSide side);

enum class TargetOs { WINDOWS, MACOS, LINUX };

// Human-readable display name. Modifier keys always collapse Left/Right and
// remap per target OS, exactly mirroring the Blender addon's
// fix_modifier_display_text:
//   Windows: Shift / Ctrl / Alt / Windows Key
//   macOS:   Shift / Control / Option / Command
//   Linux:   Shift / Ctrl / Alt / OS Key
std::string display_name(EventType type, TargetOs os);

} // namespace sk
