#include "doctest.h"

#include "../src/input/event-types.h"

using namespace sk;

TEST_CASE("letter and digit keycodes map correctly") {
    CHECK(key_code_to_event_type(0x001E) == EventType::A); // VC_A
    CHECK(key_code_to_event_type(0x002C) == EventType::Z); // VC_Z
    CHECK(key_code_to_event_type(0x000B) == EventType::KEY_0); // VC_0
}

TEST_CASE("unmapped keycode returns UNKNOWN") {
    CHECK(key_code_to_event_type(0xFFFE) == EventType::UNKNOWN);
}

TEST_CASE("modifier keycodes collapse Left/Right to a canonical EventType") {
    CHECK(key_code_to_event_type(0x002A) == EventType::MOD_SHIFT); // VC_SHIFT_L
    CHECK(key_code_to_event_type(0x0036) == EventType::MOD_SHIFT); // VC_SHIFT_R
    CHECK(key_code_to_event_type(0x001D) == EventType::MOD_CTRL);  // VC_CONTROL_L
    CHECK(key_code_to_event_type(0x0E1D) == EventType::MOD_CTRL);  // VC_CONTROL_R
}

TEST_CASE("key_code_to_modifier_side distinguishes physical sides") {
    CHECK(key_code_to_modifier_side(0x002A) == ModifierSide::SHIFT_L);
    CHECK(key_code_to_modifier_side(0x0036) == ModifierSide::SHIFT_R);
    CHECK(key_code_to_modifier_side(0x001E) == ModifierSide::NONE); // VC_A, not a modifier.
}

TEST_CASE("mouse_button_to_event_type maps 1-5") {
    CHECK(mouse_button_to_event_type(1) == EventType::MOUSE_LEFT);
    CHECK(mouse_button_to_event_type(2) == EventType::MOUSE_RIGHT);
    CHECK(mouse_button_to_event_type(3) == EventType::MOUSE_MIDDLE);
    CHECK(mouse_button_to_event_type(6) == EventType::UNKNOWN);
}

TEST_CASE("is_modifier and is_mouse_event classify correctly") {
    CHECK(is_modifier(EventType::MOD_SHIFT));
    CHECK_FALSE(is_modifier(EventType::A));
    CHECK(is_mouse_event(EventType::MOUSE_LEFT));
    CHECK(is_mouse_event(EventType::MOUSE_WHEEL_UP));
    CHECK_FALSE(is_mouse_event(EventType::A));
}

TEST_CASE("display_name remaps modifiers per target OS, matching fix_modifier_display_text") {
    CHECK(display_name(EventType::MOD_CTRL, TargetOs::WINDOWS) == "Ctrl");
    CHECK(display_name(EventType::MOD_CTRL, TargetOs::MACOS) == "Control");
    CHECK(display_name(EventType::MOD_ALT, TargetOs::MACOS) == "Option");
    CHECK(display_name(EventType::MOD_META, TargetOs::WINDOWS) == "Windows Key");
    CHECK(display_name(EventType::MOD_META, TargetOs::MACOS) == "Command");
    CHECK(display_name(EventType::MOD_META, TargetOs::LINUX) == "OS Key");
    CHECK(display_name(EventType::MOD_SHIFT, TargetOs::LINUX) == "Shift");
}

TEST_CASE("display_name covers plain keys") {
    CHECK(display_name(EventType::A, TargetOs::WINDOWS) == "A");
    CHECK(display_name(EventType::SPACE, TargetOs::WINDOWS) == "Space");
    CHECK(display_name(EventType::MOUSE_LEFT, TargetOs::WINDOWS) == "Left Mouse");
}
