#pragma once

#include <string>

#include <obs-data.h>
#include <obs-properties.h>

#include "render/display-list.h"

namespace sk {

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
//     canvas option).
//   - The mouse icon, the held-modifier ("shortcut") pill, and the text
//     history are three fully independent elements, each with its own
//     anchor: the mouse icon is fixed at (0,0); the shortcut pill is at
//     (shortcut_offset_x, shortcut_offset_y); the text history starts at
//     (text_initial_offset_x, text_initial_offset_y). None of them push or
//     resize each other. The overall canvas is just the bounding box of
//     whichever elements are actually shown (and safely includes negative
//     spacing/offsets -- see layout.cpp's translation pass).
//   - Each history entry's position is the newest entry at the text anchor,
//     with the i-th older entry offset by i * (text_spacing_x,
//     text_spacing_y) -- i.e. spacing directly controls how far older text
//     is from the newest, in whichever direction the sign implies (negative
//     spacing is valid and reverses the direction).
//   - Text alignment doesn't apply anymore now that each line's x position
//     is derived from the spacing cascade rather than measured against a
//     shared block width.
//   - margin/line_thickness/offset_x/offset_y (old, block-relative) are
//     gone; background_margin controls the padding around text within its
//     background box (both TEXT and DRAW_AREA modes). Line stroke width for
//     the default-drawn mouse icon is now a fixed constant
//     (kDefaultLineThickness in layout.cpp) rather than configurable.
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
    int background_margin = 8;

    int font_size = 24;

    int mouse_size_x = 48;
    int mouse_size_y = 62;

    bool use_custom_mouse_image = false;
    CustomMouseImageDisplayMode custom_mouse_image_display_mode = CustomMouseImageDisplayMode::OVERLAY;
    std::string custom_mouse_image_base;
    std::string custom_mouse_image_left;
    std::string custom_mouse_image_right;
    std::string custom_mouse_image_middle;

    // Where the held-modifier ("Ctrl"/"Shift"/...) pill is drawn, independent
    // of both the mouse icon and the text history.
    int shortcut_offset_x = 60;
    int shortcut_offset_y = 0;

    // Where the newest history entry is drawn; older entries cascade from
    // here by (i * text_spacing_x, i * text_spacing_y). Any of the four may
    // be negative.
    int text_initial_offset_x = 0;
    int text_initial_offset_y = 70;
    int text_spacing_x = 0;
    int text_spacing_y = 30;

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
// color/mode/radius/margin) the same way the Blender panel only shows them
// when their parent toggle is on.
obs_properties_t* build_render_settings_properties();

// Registers RenderSettings' defaults into `data` (obs_source_info::get_defaults).
void set_render_settings_defaults(obs_data_t* data);

// Reads every RenderSettings field out of `data` (obs_source_info::update).
RenderSettings read_render_settings(obs_data_t* data);

} // namespace sk
