#include "event-types.h"

namespace sk {

// Raw libuiohook VC_* keycode values, copied from uiohook.h. Kept as local
// literals (rather than #include <uiohook.h>) so this translation unit has
// no dependency on the hooking library and can be unit-tested in isolation.
namespace vc {
constexpr uint16_t ESCAPE = 0x0001;

constexpr uint16_t F1 = 0x003B, F2 = 0x003C, F3 = 0x003D, F4 = 0x003E, F5 = 0x003F;
constexpr uint16_t F6 = 0x0040, F7 = 0x0041, F8 = 0x0042, F9 = 0x0043, F10 = 0x0044;
constexpr uint16_t F11 = 0x0057, F12 = 0x0058;
constexpr uint16_t F13 = 0x005B, F14 = 0x005C, F15 = 0x005D, F16 = 0x0063, F17 = 0x0064;
constexpr uint16_t F18 = 0x0065, F19 = 0x0066, F20 = 0x0067, F21 = 0x0068, F22 = 0x0069;
constexpr uint16_t F23 = 0x006A, F24 = 0x006B;

constexpr uint16_t BACKQUOTE = 0x0029;
constexpr uint16_t K1 = 0x0002, K2 = 0x0003, K3 = 0x0004, K4 = 0x0005, K5 = 0x0006;
constexpr uint16_t K6 = 0x0007, K7 = 0x0008, K8 = 0x0009, K9 = 0x000A, K0 = 0x000B;
constexpr uint16_t MINUS = 0x000C, EQUALS = 0x000D, BACKSPACE = 0x000E;
constexpr uint16_t TAB = 0x000F, CAPS_LOCK = 0x003A;

constexpr uint16_t A = 0x001E, B = 0x0030, C = 0x002E, D = 0x0020, E = 0x0012;
constexpr uint16_t F = 0x0021, G = 0x0022, H = 0x0023, I = 0x0017, J = 0x0024;
constexpr uint16_t K = 0x0025, L = 0x0026, M = 0x0032, N = 0x0031, O = 0x0018;
constexpr uint16_t P = 0x0019, Q = 0x0010, R = 0x0013, S = 0x001F, T = 0x0014;
constexpr uint16_t U = 0x0016, V = 0x002F, W = 0x0011, X = 0x002D, Y = 0x0015;
constexpr uint16_t Z = 0x002C;

constexpr uint16_t OPEN_BRACKET = 0x001A, CLOSE_BRACKET = 0x001B, BACK_SLASH = 0x002B;
constexpr uint16_t SEMICOLON = 0x0027, QUOTE = 0x0028, ENTER = 0x001C;
constexpr uint16_t COMMA = 0x0033, PERIOD = 0x0034, SLASH = 0x0035;
constexpr uint16_t SPACE = 0x0039;

constexpr uint16_t PRINTSCREEN = 0x0E37, SCROLL_LOCK = 0x0046, PAUSE = 0x0E45;

constexpr uint16_t INSERT = 0x0E52, DELETE_ = 0x0E53, HOME = 0x0E47, END = 0x0E4F;
constexpr uint16_t PAGE_UP = 0x0E49, PAGE_DOWN = 0x0E51;

constexpr uint16_t UP = 0xE048, LEFT = 0xE04B, RIGHT = 0xE04D, DOWN = 0xE050;

constexpr uint16_t NUM_LOCK = 0x0045, KP_DIVIDE = 0x0E35, KP_MULTIPLY = 0x0037;
constexpr uint16_t KP_SUBTRACT = 0x004A, KP_ADD = 0x004E, KP_ENTER = 0x0E1C;
constexpr uint16_t KP_SEPARATOR = 0x0053;
constexpr uint16_t KP_1 = 0x004F, KP_2 = 0x0050, KP_3 = 0x0051, KP_4 = 0x004B;
constexpr uint16_t KP_5 = 0x004C, KP_6 = 0x004D, KP_7 = 0x0047, KP_8 = 0x0048;
constexpr uint16_t KP_9 = 0x0049, KP_0 = 0x0052;

constexpr uint16_t SHIFT_L = 0x002A, SHIFT_R = 0x0036;
constexpr uint16_t CONTROL_L = 0x001D, CONTROL_R = 0x0E1D;
constexpr uint16_t ALT_L = 0x0038, ALT_R = 0x0E38;
constexpr uint16_t META_L = 0x0E5B, META_R = 0x0E5C;
constexpr uint16_t CONTEXT_MENU = 0x0E5D;

constexpr uint16_t MEDIA_PLAY = 0xE022, MEDIA_STOP = 0xE024;
constexpr uint16_t MEDIA_PREVIOUS = 0xE010, MEDIA_NEXT = 0xE019;
constexpr uint16_t VOLUME_MUTE = 0xE020, VOLUME_UP = 0xE030, VOLUME_DOWN = 0xE02E;
} // namespace vc

bool is_modifier(EventType type) {
    switch (type) {
        case EventType::MOD_SHIFT:
        case EventType::MOD_CTRL:
        case EventType::MOD_ALT:
        case EventType::MOD_META:
            return true;
        default:
            return false;
    }
}

bool is_mouse_event(EventType type) {
    switch (type) {
        case EventType::MOUSE_LEFT:
        case EventType::MOUSE_RIGHT:
        case EventType::MOUSE_MIDDLE:
        case EventType::MOUSE_BUTTON4:
        case EventType::MOUSE_BUTTON5:
        case EventType::MOUSE_WHEEL_UP:
        case EventType::MOUSE_WHEEL_DOWN:
            return true;
        default:
            return false;
    }
}

ModifierSide key_code_to_modifier_side(uint16_t keycode) {
    switch (keycode) {
        case vc::SHIFT_L: return ModifierSide::SHIFT_L;
        case vc::SHIFT_R: return ModifierSide::SHIFT_R;
        case vc::CONTROL_L: return ModifierSide::CTRL_L;
        case vc::CONTROL_R: return ModifierSide::CTRL_R;
        case vc::ALT_L: return ModifierSide::ALT_L;
        case vc::ALT_R: return ModifierSide::ALT_R;
        case vc::META_L: return ModifierSide::META_L;
        case vc::META_R: return ModifierSide::META_R;
        default: return ModifierSide::NONE;
    }
}

EventType canonical_modifier(ModifierSide side) {
    switch (side) {
        case ModifierSide::SHIFT_L:
        case ModifierSide::SHIFT_R:
            return EventType::MOD_SHIFT;
        case ModifierSide::CTRL_L:
        case ModifierSide::CTRL_R:
            return EventType::MOD_CTRL;
        case ModifierSide::ALT_L:
        case ModifierSide::ALT_R:
            return EventType::MOD_ALT;
        case ModifierSide::META_L:
        case ModifierSide::META_R:
            return EventType::MOD_META;
        default:
            return EventType::UNKNOWN;
    }
}

EventType key_code_to_event_type(uint16_t keycode) {
    // Modifier keys collapse Left/Right at this layer -- callers that need
    // per-side hold tracking use key_code_to_modifier_side() instead.
    if (ModifierSide side = key_code_to_modifier_side(keycode); side != ModifierSide::NONE) {
        return canonical_modifier(side);
    }

    switch (keycode) {
        case vc::A: return EventType::A;
        case vc::B: return EventType::B;
        case vc::C: return EventType::C;
        case vc::D: return EventType::D;
        case vc::E: return EventType::E;
        case vc::F: return EventType::F;
        case vc::G: return EventType::G;
        case vc::H: return EventType::H;
        case vc::I: return EventType::I;
        case vc::J: return EventType::J;
        case vc::K: return EventType::K;
        case vc::L: return EventType::L;
        case vc::M: return EventType::M;
        case vc::N: return EventType::N;
        case vc::O: return EventType::O;
        case vc::P: return EventType::P;
        case vc::Q: return EventType::Q;
        case vc::R: return EventType::R;
        case vc::S: return EventType::S;
        case vc::T: return EventType::T;
        case vc::U: return EventType::U;
        case vc::V: return EventType::V;
        case vc::W: return EventType::W;
        case vc::X: return EventType::X;
        case vc::Y: return EventType::Y;
        case vc::Z: return EventType::Z;

        case vc::K0: return EventType::KEY_0;
        case vc::K1: return EventType::KEY_1;
        case vc::K2: return EventType::KEY_2;
        case vc::K3: return EventType::KEY_3;
        case vc::K4: return EventType::KEY_4;
        case vc::K5: return EventType::KEY_5;
        case vc::K6: return EventType::KEY_6;
        case vc::K7: return EventType::KEY_7;
        case vc::K8: return EventType::KEY_8;
        case vc::K9: return EventType::KEY_9;

        case vc::F1: return EventType::F1;
        case vc::F2: return EventType::F2;
        case vc::F3: return EventType::F3;
        case vc::F4: return EventType::F4;
        case vc::F5: return EventType::F5;
        case vc::F6: return EventType::F6;
        case vc::F7: return EventType::F7;
        case vc::F8: return EventType::F8;
        case vc::F9: return EventType::F9;
        case vc::F10: return EventType::F10;
        case vc::F11: return EventType::F11;
        case vc::F12: return EventType::F12;
        case vc::F13: return EventType::F13;
        case vc::F14: return EventType::F14;
        case vc::F15: return EventType::F15;
        case vc::F16: return EventType::F16;
        case vc::F17: return EventType::F17;
        case vc::F18: return EventType::F18;
        case vc::F19: return EventType::F19;
        case vc::F20: return EventType::F20;
        case vc::F21: return EventType::F21;
        case vc::F22: return EventType::F22;
        case vc::F23: return EventType::F23;
        case vc::F24: return EventType::F24;

        case vc::MINUS: return EventType::MINUS;
        case vc::EQUALS: return EventType::EQUALS;
        case vc::BACKSPACE: return EventType::BACKSPACE;
        case vc::TAB: return EventType::TAB;
        case vc::CAPS_LOCK: return EventType::CAPS_LOCK;
        case vc::BACKQUOTE: return EventType::BACKQUOTE;
        case vc::OPEN_BRACKET: return EventType::OPEN_BRACKET;
        case vc::CLOSE_BRACKET: return EventType::CLOSE_BRACKET;
        case vc::BACK_SLASH: return EventType::BACK_SLASH;
        case vc::SEMICOLON: return EventType::SEMICOLON;
        case vc::QUOTE: return EventType::QUOTE;
        case vc::ENTER: return EventType::ENTER;
        case vc::COMMA: return EventType::COMMA;
        case vc::PERIOD: return EventType::PERIOD;
        case vc::SLASH: return EventType::SLASH;
        case vc::SPACE: return EventType::SPACE;

        case vc::ESCAPE: return EventType::ESCAPE;
        case vc::PRINTSCREEN: return EventType::PRINT_SCREEN;
        case vc::SCROLL_LOCK: return EventType::SCROLL_LOCK;
        case vc::PAUSE: return EventType::PAUSE;
        case vc::CONTEXT_MENU: return EventType::CONTEXT_MENU;

        case vc::INSERT: return EventType::INSERT;
        case vc::DELETE_: return EventType::DEL;
        case vc::HOME: return EventType::HOME;
        case vc::END: return EventType::END;
        case vc::PAGE_UP: return EventType::PAGE_UP;
        case vc::PAGE_DOWN: return EventType::PAGE_DOWN;

        case vc::UP: return EventType::UP_ARROW;
        case vc::DOWN: return EventType::DOWN_ARROW;
        case vc::LEFT: return EventType::LEFT_ARROW;
        case vc::RIGHT: return EventType::RIGHT_ARROW;

        case vc::NUM_LOCK: return EventType::NUM_LOCK;
        case vc::KP_DIVIDE: return EventType::KP_DIVIDE;
        case vc::KP_MULTIPLY: return EventType::KP_MULTIPLY;
        case vc::KP_SUBTRACT: return EventType::KP_SUBTRACT;
        case vc::KP_ADD: return EventType::KP_ADD;
        case vc::KP_ENTER: return EventType::KP_ENTER;
        case vc::KP_SEPARATOR: return EventType::KP_DECIMAL;
        case vc::KP_0: return EventType::KP_0;
        case vc::KP_1: return EventType::KP_1;
        case vc::KP_2: return EventType::KP_2;
        case vc::KP_3: return EventType::KP_3;
        case vc::KP_4: return EventType::KP_4;
        case vc::KP_5: return EventType::KP_5;
        case vc::KP_6: return EventType::KP_6;
        case vc::KP_7: return EventType::KP_7;
        case vc::KP_8: return EventType::KP_8;
        case vc::KP_9: return EventType::KP_9;

        case vc::MEDIA_PLAY: return EventType::MEDIA_PLAY;
        case vc::MEDIA_STOP: return EventType::MEDIA_STOP;
        case vc::MEDIA_PREVIOUS: return EventType::MEDIA_PREVIOUS;
        case vc::MEDIA_NEXT: return EventType::MEDIA_NEXT;
        case vc::VOLUME_MUTE: return EventType::VOLUME_MUTE;
        case vc::VOLUME_UP: return EventType::VOLUME_UP;
        case vc::VOLUME_DOWN: return EventType::VOLUME_DOWN;

        default: return EventType::UNKNOWN;
    }
}

EventType mouse_button_to_event_type(uint16_t button) {
    switch (button) {
        case 1: return EventType::MOUSE_LEFT;
        case 2: return EventType::MOUSE_RIGHT;
        case 3: return EventType::MOUSE_MIDDLE;
        case 4: return EventType::MOUSE_BUTTON4;
        case 5: return EventType::MOUSE_BUTTON5;
        default: return EventType::UNKNOWN;
    }
}

std::string display_name(EventType type, TargetOs os) {
    switch (type) {
        case EventType::A: return "A"; case EventType::B: return "B";
        case EventType::C: return "C"; case EventType::D: return "D";
        case EventType::E: return "E"; case EventType::F: return "F";
        case EventType::G: return "G"; case EventType::H: return "H";
        case EventType::I: return "I"; case EventType::J: return "J";
        case EventType::K: return "K"; case EventType::L: return "L";
        case EventType::M: return "M"; case EventType::N: return "N";
        case EventType::O: return "O"; case EventType::P: return "P";
        case EventType::Q: return "Q"; case EventType::R: return "R";
        case EventType::S: return "S"; case EventType::T: return "T";
        case EventType::U: return "U"; case EventType::V: return "V";
        case EventType::W: return "W"; case EventType::X: return "X";
        case EventType::Y: return "Y"; case EventType::Z: return "Z";

        case EventType::KEY_0: return "0"; case EventType::KEY_1: return "1";
        case EventType::KEY_2: return "2"; case EventType::KEY_3: return "3";
        case EventType::KEY_4: return "4"; case EventType::KEY_5: return "5";
        case EventType::KEY_6: return "6"; case EventType::KEY_7: return "7";
        case EventType::KEY_8: return "8"; case EventType::KEY_9: return "9";

        case EventType::F1: return "F1"; case EventType::F2: return "F2";
        case EventType::F3: return "F3"; case EventType::F4: return "F4";
        case EventType::F5: return "F5"; case EventType::F6: return "F6";
        case EventType::F7: return "F7"; case EventType::F8: return "F8";
        case EventType::F9: return "F9"; case EventType::F10: return "F10";
        case EventType::F11: return "F11"; case EventType::F12: return "F12";
        case EventType::F13: return "F13"; case EventType::F14: return "F14";
        case EventType::F15: return "F15"; case EventType::F16: return "F16";
        case EventType::F17: return "F17"; case EventType::F18: return "F18";
        case EventType::F19: return "F19"; case EventType::F20: return "F20";
        case EventType::F21: return "F21"; case EventType::F22: return "F22";
        case EventType::F23: return "F23"; case EventType::F24: return "F24";

        case EventType::MINUS: return "-"; case EventType::EQUALS: return "=";
        case EventType::BACKSPACE: return "Backspace"; case EventType::TAB: return "Tab";
        case EventType::CAPS_LOCK: return "Caps Lock"; case EventType::BACKQUOTE: return "`";
        case EventType::OPEN_BRACKET: return "["; case EventType::CLOSE_BRACKET: return "]";
        case EventType::BACK_SLASH: return "\\"; case EventType::SEMICOLON: return ";";
        case EventType::QUOTE: return "'"; case EventType::ENTER: return "Enter";
        case EventType::COMMA: return ","; case EventType::PERIOD: return ".";
        case EventType::SLASH: return "/"; case EventType::SPACE: return "Space";

        case EventType::ESCAPE: return "Esc";
        case EventType::PRINT_SCREEN: return "Print Screen";
        case EventType::SCROLL_LOCK: return "Scroll Lock";
        case EventType::PAUSE: return "Pause";
        case EventType::CONTEXT_MENU: return "Menu";

        case EventType::INSERT: return "Insert"; case EventType::DEL: return "Delete";
        case EventType::HOME: return "Home"; case EventType::END: return "End";
        case EventType::PAGE_UP: return "Page Up"; case EventType::PAGE_DOWN: return "Page Down";
        case EventType::UP_ARROW: return "Up"; case EventType::DOWN_ARROW: return "Down";
        case EventType::LEFT_ARROW: return "Left"; case EventType::RIGHT_ARROW: return "Right";

        case EventType::NUM_LOCK: return "Num Lock";
        case EventType::KP_DIVIDE: return "Numpad /"; case EventType::KP_MULTIPLY: return "Numpad *";
        case EventType::KP_SUBTRACT: return "Numpad -"; case EventType::KP_ADD: return "Numpad +";
        case EventType::KP_ENTER: return "Numpad Enter"; case EventType::KP_DECIMAL: return "Numpad .";
        case EventType::KP_0: return "Numpad 0"; case EventType::KP_1: return "Numpad 1";
        case EventType::KP_2: return "Numpad 2"; case EventType::KP_3: return "Numpad 3";
        case EventType::KP_4: return "Numpad 4"; case EventType::KP_5: return "Numpad 5";
        case EventType::KP_6: return "Numpad 6"; case EventType::KP_7: return "Numpad 7";
        case EventType::KP_8: return "Numpad 8"; case EventType::KP_9: return "Numpad 9";

        case EventType::MOD_SHIFT: return "Shift";
        case EventType::MOD_CTRL:
            return os == TargetOs::MACOS ? "Control" : "Ctrl";
        case EventType::MOD_ALT:
            return os == TargetOs::MACOS ? "Option" : "Alt";
        case EventType::MOD_META:
            if (os == TargetOs::MACOS) return "Command";
            if (os == TargetOs::WINDOWS) return "Windows Key";
            return "OS Key";

        case EventType::MEDIA_PLAY: return "Media Play";
        case EventType::MEDIA_STOP: return "Media Stop";
        case EventType::MEDIA_PREVIOUS: return "Media Previous";
        case EventType::MEDIA_NEXT: return "Media Next";
        case EventType::VOLUME_MUTE: return "Volume Mute";
        case EventType::VOLUME_UP: return "Volume Up";
        case EventType::VOLUME_DOWN: return "Volume Down";

        case EventType::MOUSE_LEFT: return "Left Mouse";
        case EventType::MOUSE_RIGHT: return "Right Mouse";
        case EventType::MOUSE_MIDDLE: return "Middle Mouse";
        case EventType::MOUSE_BUTTON4: return "Mouse 4";
        case EventType::MOUSE_BUTTON5: return "Mouse 5";
        case EventType::MOUSE_WHEEL_UP: return "Wheel Up";
        case EventType::MOUSE_WHEEL_DOWN: return "Wheel Down";

        default: return "Unknown";
    }
}

} // namespace sk
