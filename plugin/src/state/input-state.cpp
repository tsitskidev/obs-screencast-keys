#include "input-state.h"

namespace sk {

namespace {
// Index into modifier_side_down_ for a given ModifierSide (NONE excluded).
int side_index(ModifierSide side) {
    switch (side) {
        case ModifierSide::SHIFT_L: return 0;
        case ModifierSide::SHIFT_R: return 1;
        case ModifierSide::CTRL_L: return 2;
        case ModifierSide::CTRL_R: return 3;
        case ModifierSide::ALT_L: return 4;
        case ModifierSide::ALT_R: return 5;
        case ModifierSide::META_L: return 6;
        case ModifierSide::META_R: return 7;
        default: return -1;
    }
}
} // namespace

void InputState::on_key_event(uint16_t keycode, bool pressed) {
    const ModifierSide side = key_code_to_modifier_side(keycode);
    const int index = side_index(side);
    if (index < 0) {
        return; // Not a modifier key -- EventHistory handles history entries.
    }
    modifier_side_down_[index] = pressed;
}

void InputState::on_mouse_button_event(EventType button, bool pressed) {
    switch (button) {
        case EventType::MOUSE_LEFT: mouse_left_ = pressed; break;
        case EventType::MOUSE_RIGHT: mouse_right_ = pressed; break;
        case EventType::MOUSE_MIDDLE: mouse_middle_ = pressed; break;
        case EventType::MOUSE_BUTTON4: mouse_button4_ = pressed; break;
        case EventType::MOUSE_BUTTON5: mouse_button5_ = pressed; break;
        default: break; // Wheel events have no hold state.
    }
}

std::vector<EventType> InputState::held_modifiers() const {
    std::vector<EventType> result;

    if (modifier_side_down_[side_index(ModifierSide::SHIFT_L)] ||
        modifier_side_down_[side_index(ModifierSide::SHIFT_R)]) {
        result.push_back(EventType::MOD_SHIFT);
    }
    if (modifier_side_down_[side_index(ModifierSide::CTRL_L)] ||
        modifier_side_down_[side_index(ModifierSide::CTRL_R)]) {
        result.push_back(EventType::MOD_CTRL);
    }
    if (modifier_side_down_[side_index(ModifierSide::ALT_L)] ||
        modifier_side_down_[side_index(ModifierSide::ALT_R)]) {
        result.push_back(EventType::MOD_ALT);
    }
    if (modifier_side_down_[side_index(ModifierSide::META_L)] ||
        modifier_side_down_[side_index(ModifierSide::META_R)]) {
        result.push_back(EventType::MOD_META);
    }

    return result;
}

bool InputState::is_mouse_button_held(EventType button) const {
    switch (button) {
        case EventType::MOUSE_LEFT: return mouse_left_;
        case EventType::MOUSE_RIGHT: return mouse_right_;
        case EventType::MOUSE_MIDDLE: return mouse_middle_;
        case EventType::MOUSE_BUTTON4: return mouse_button4_;
        case EventType::MOUSE_BUTTON5: return mouse_button5_;
        default: return false;
    }
}

void InputState::clear() {
    modifier_side_down_.fill(false);
    mouse_left_ = mouse_right_ = mouse_middle_ = mouse_button4_ = mouse_button5_ = false;
}

} // namespace sk
