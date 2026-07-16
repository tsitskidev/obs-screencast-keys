#pragma once

#include "../input/event-types.h"
#include "../settings.h"
#include "../state/event-history.h"
#include "../state/input-state.h"
#include "display-list.h"
#include "glyph-atlas.h"

namespace sk {

struct LayoutResult {
    DisplayList commands;
    int width = 1;
    int height = 1;
};

// Builds the DisplayList (and measures the resulting content size) for the
// current input state + event history, per the given style settings. Pure
// CPU work -- the only gs_*-adjacent thing it touches is GlyphAtlas's
// measurement functions, which stage glyph rasterization (FreeType, CPU
// only) for later GPU upload but never touch a gs_texture_t themselves.
//
// Port of draw_area_size() and the _draw_*_layer() family in ops.py. The
// "last operator" layer is dropped (no OBS analog); origin/region-relative
// positioning is replaced by alignment within either the auto-sized
// bounding box or a fixed canvas (see RenderSettings::canvas_size_mode).
// Some Blender-specific layout constants (separator width tied to mouse
// width, exact centering ratios) are simplified rather than reproduced
// pixel-for-pixel, since they were tuned for a floating viewport overlay,
// not a fixed-canvas video source.
LayoutResult build_display_list(const EventHistory& history, const InputState& input_state,
                                 const RenderSettings& settings, TargetOs target_os, GlyphAtlas& atlas);

} // namespace sk
