#pragma once

#include <string>

#include <obs-data.h>
#include <obs-properties.h>

#include "render/display-list.h"

namespace sk {

enum class Align { LEFT, CENTER, RIGHT };
enum class BackgroundMode { TEXT, DRAW_AREA };
enum class MouseEventsShowMode { EVENT_HISTORY, HOLD_STATUS, EVENT_HISTORY_AND_HOLD_STATUS };

// NORMAL: the displayed image is swapped for the held button's image (or
// the base image if none held). OVERLAY: the base image is always drawn,
// and each held button's image is drawn on top of it. Exact port of
// draw_custom_mouse's two branches in ops.py.
enum class CustomMouseImageDisplayMode { NORMAL, OVERLAY };

// Mirrors SK_Preferences (preferences.py), with the following deliberate
// departures based on real-world testing feedback:
//   - The canvas is always auto-sized to content (no Blender-style fixed
//     canvas option) -- it was cut for being redundant complexity once the
//     mouse block and text block became independently positioned (below).
//   - The mouse/modifier block is anchored at a fixed position (0,0) and
//     never moves based on the text history's size -- previously it sat at
//     the bottom of the whole content block, so it visibly shifted up/down
//     as history lines appeared/expired. text_initial_offset_x/y position
//     the (separate) text block instead, so the two never interact.
//   - margin/line_thickness/offset_x/offset_y are gone; text_spacing_x/y
//     (gap between/around history lines) replace margin's role for text,
//     and line stroke width for the default-drawn mouse icon is now a fixed
//     constant (kDefaultLineThickness in layout.cpp) rather than configurable.
//   - mouse_size split into independent X/Y so the icon isn't forced into a
//     fixed aspect ratio.
struct RenderSettings {
    Color color{1.0f, 1.0f, 1.0f, 1.0f};

    bool shadow = false;
    Color shadow_color{0.0f, 0.0f, 0.0f, 1.0f};

    bool background = false;
    BackgroundMode background_mode = BackgroundMode::DRAW_AREA;
    Color background_color{0.0f, 0.0f, 0.0f, 0.5f};
    int background_rounded_corner_radius = 4;

    int font_size = 24;

    int mouse_size_x = 48;
    int mouse_size_y = 62;

    bool use_custom_mouse_image = false;
    CustomMouseImageDisplayMode custom_mouse_image_display_mode = CustomMouseImageDisplayMode::OVERLAY;
    std::string custom_mouse_image_base;
    std::string custom_mouse_image_left;
    std::string custom_mouse_image_right;
    std::string custom_mouse_image_middle;

    Align align = Align::LEFT;

    // Where the text history block starts (independent of the mouse block,
    // which is always anchored at (0,0)).
    int text_initial_offset_x = 0;
    int text_initial_offset_y = 0;
    // Gap applied around/between history lines: text_spacing_y is the
    // vertical gap between consecutive lines; text_spacing_x is the
    // horizontal padding applied to each line's text/background.
    int text_spacing_x = 4;
    int text_spacing_y = 4;

    double display_time = 3.0;
    int max_event_history = 5;
    bool repeat_count = true;

    bool show_mouse_events = true;
    MouseEventsShowMode mouse_events_show_mode = MouseEventsShowMode::HOLD_STATUS;
};

// Predicates mirroring ops.py's module-level show_mouse_hold_status() /
// show_mouse_event_history() / show_text_background() /
// show_draw_area_background() helpers.
inline bool show_mouse_hold_status(const RenderSettings& s) {
    if (!s.show_mouse_events) return false;
    return s.mouse_events_show_mode == MouseEventsShowMode::HOLD_STATUS ||
           s.mouse_events_show_mode == MouseEventsShowMode::EVENT_HISTORY_AND_HOLD_STATUS;
}

inline bool show_mouse_event_history(const RenderSettings& s) {
    if (!s.show_mouse_events) return false;
    return s.mouse_events_show_mode == MouseEventsShowMode::EVENT_HISTORY ||
           s.mouse_events_show_mode == MouseEventsShowMode::EVENT_HISTORY_AND_HOLD_STATUS;
}

inline bool show_text_background(const RenderSettings& s) {
    return s.background && s.background_mode == BackgroundMode::TEXT;
}

inline bool show_draw_area_background(const RenderSettings& s) {
    return s.background && s.background_mode == BackgroundMode::DRAW_AREA;
}

// Builds the obs_properties_t UI mirroring SK_Preferences.draw(), with
// modified-callbacks to show/hide dependent fields (shadow color, background
// color/mode/radius) the same way the Blender panel only shows them when
// their parent toggle is on.
obs_properties_t* build_render_settings_properties();

// Registers RenderSettings' defaults into `data` (obs_source_info::get_defaults).
void set_render_settings_defaults(obs_data_t* data);

// Reads every RenderSettings field out of `data` (obs_source_info::update).
RenderSettings read_render_settings(obs_data_t* data);

} // namespace sk
