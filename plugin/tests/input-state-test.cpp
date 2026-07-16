#include "doctest.h"

#include "../src/state/input-state.h"

using sk::EventType;
using sk::InputState;

namespace {
// Raw VC_* values duplicated from event-types.cpp for test readability
// (this file intentionally exercises InputState only via keycodes/EventType,
// never by reaching into event-types internals).
constexpr uint16_t VC_SHIFT_L = 0x002A;
constexpr uint16_t VC_SHIFT_R = 0x0036;
constexpr uint16_t VC_CONTROL_L = 0x001D;
constexpr uint16_t VC_ALT_L = 0x0038;
constexpr uint16_t VC_A = 0x001E; // Non-modifier key, should be a no-op.
} // namespace

TEST_CASE("no modifiers held initially") {
    InputState state;
    CHECK(state.held_modifiers().empty());
}

TEST_CASE("pressing a modifier key adds it to held_modifiers") {
    InputState state;
    state.on_key_event(VC_SHIFT_L, true);

    CHECK(state.held_modifiers() == std::vector<EventType>{EventType::MOD_SHIFT});
}

TEST_CASE("releasing the same physical modifier clears it") {
    InputState state;
    state.on_key_event(VC_SHIFT_L, true);
    state.on_key_event(VC_SHIFT_L, false);

    CHECK(state.held_modifiers().empty());
}

TEST_CASE("releasing one side of a modifier does not clear the other side") {
    InputState state;
    state.on_key_event(VC_SHIFT_L, true);
    state.on_key_event(VC_SHIFT_R, true);
    state.on_key_event(VC_SHIFT_L, false);

    // Left Shift released, but Right Shift is still down -> Shift still held.
    CHECK(state.held_modifiers() == std::vector<EventType>{EventType::MOD_SHIFT});
}

TEST_CASE("held_modifiers returns Shift, Ctrl, Alt, Meta in that canonical order") {
    InputState state;
    state.on_key_event(VC_ALT_L, true);
    state.on_key_event(VC_SHIFT_L, true);
    state.on_key_event(VC_CONTROL_L, true);

    std::vector<EventType> expected{EventType::MOD_SHIFT, EventType::MOD_CTRL, EventType::MOD_ALT};
    CHECK(state.held_modifiers() == expected);
}

TEST_CASE("non-modifier keycodes are a no-op") {
    InputState state;
    state.on_key_event(VC_A, true);

    CHECK(state.held_modifiers().empty());
}

TEST_CASE("mouse button hold tracks press/release directly, no MOUSEMOVE workaround needed") {
    InputState state;
    CHECK_FALSE(state.is_mouse_button_held(EventType::MOUSE_LEFT));

    state.on_mouse_button_event(EventType::MOUSE_LEFT, true);
    CHECK(state.is_mouse_button_held(EventType::MOUSE_LEFT));

    state.on_mouse_button_event(EventType::MOUSE_LEFT, false);
    CHECK_FALSE(state.is_mouse_button_held(EventType::MOUSE_LEFT));
}

TEST_CASE("mouse buttons are tracked independently") {
    InputState state;
    state.on_mouse_button_event(EventType::MOUSE_LEFT, true);
    state.on_mouse_button_event(EventType::MOUSE_RIGHT, true);
    state.on_mouse_button_event(EventType::MOUSE_LEFT, false);

    CHECK_FALSE(state.is_mouse_button_held(EventType::MOUSE_LEFT));
    CHECK(state.is_mouse_button_held(EventType::MOUSE_RIGHT));
}

TEST_CASE("wheel events are not mouse-button hold state") {
    InputState state;
    state.on_mouse_button_event(EventType::MOUSE_WHEEL_UP, true);
    CHECK_FALSE(state.is_mouse_button_held(EventType::MOUSE_WHEEL_UP));
}

TEST_CASE("clear resets all modifier and mouse button state") {
    InputState state;
    state.on_key_event(VC_SHIFT_L, true);
    state.on_mouse_button_event(EventType::MOUSE_LEFT, true);

    state.clear();

    CHECK(state.held_modifiers().empty());
    CHECK_FALSE(state.is_mouse_button_held(EventType::MOUSE_LEFT));
}
