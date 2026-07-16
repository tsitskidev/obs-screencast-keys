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
// Port of draw_area_size() and the _draw_*_layer() family in ops.py, with
// substantial departures from the original based on testing feedback (see
// settings.h for the full rationale): the mouse icon, the shortcut
// (modifier) pill, and the text history are three fully independent
// elements, each with its own anchor -- none of them push or resize each
// other. The canvas is the union bounding box of whichever elements are
// shown, safely translated so negative offsets/spacing never clip content
// off-canvas. The "last operator" layer is dropped (no OBS analog).
LayoutResult build_display_list(const EventHistory& history, const InputState& input_state,
                                 const RenderSettings& settings, TargetOs target_os, GlyphAtlas& atlas);

} // namespace sk
