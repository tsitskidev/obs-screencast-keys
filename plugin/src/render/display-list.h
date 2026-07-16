#pragma once

#include <string>
#include <vector>

namespace sk {

struct Color {
    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
};

// round_corners order matches ops.py's draw_rounded_box: [bottom-right,
// bottom-left, top-right, top-left] in Blender's y-up space. layout.cpp
// documents the y-flip applied when translating to OBS's y-down space.
struct RoundCorners {
    bool bottom_right = true;
    bool bottom_left = true;
    bool top_right = true;
    bool top_left = true;
};

enum class DrawCommandType {
    Rect,
    RoundedRect,
    Line,
    Text,
};

// Plain-data draw instruction. layout.cpp produces a DisplayList (a vector
// of these) from the current EventHistory/InputState during video_tick;
// draw-utils.cpp is the only code that turns a DisplayList into actual gs_*
// calls, and does so only from video_render. This split is what makes the
// "gs_* only in video_render" rule structurally enforceable: layout.cpp
// never has a gs_texture_t/gs_effect_t in scope at all.
struct DrawCommand {
    DrawCommandType type = DrawCommandType::Rect;

    // Rect/RoundedRect: x,y,w,h is the bounding box. Line: (x,y)-(x2,y2).
    // Text: (x,y) is the baseline-left origin (matching blf.position).
    float x = 0, y = 0, w = 0, h = 0;
    float x2 = 0, y2 = 0;

    float radius = 0.0f; // RoundedRect corner radius.
    RoundCorners round_corners{};

    bool filled = true;
    float line_thickness = 1.0f;

    Color color{};

    // Text only. font_size is baked in at layout time (not re-measured in
    // draw-utils), since layout.cpp already measured with this exact size.
    std::string text;
    int font_size = 16;
};

using DisplayList = std::vector<DrawCommand>;

} // namespace sk
