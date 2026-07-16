#pragma once

#include "display-list.h"
#include "glyph-atlas.h"
#include "mouse-icon-images.h"

namespace sk {

// Issues the actual gs_* draw calls for a DisplayList produced by
// layout.cpp. This is the only file that touches gs_effect_t/gs_texture_t
// directly (besides GlyphAtlas's/CustomMouseImages's own upload steps) --
// must only be called from video_render.
//
// Port of ops.py's draw_rounded_box/draw_line/draw_rect/draw_text/
// draw_custom_mouse (via gpu_utils/imm.py), translated from Blender's
// y-up/bottom-left immediate mode to libobs's y-down/top-left
// gs_render_start/gs_vertex2f immediate mode. Thick lines are drawn as
// filled quads rather than via a custom polyline shader (matches the visual
// result without needing Blender's POLYLINE_UNIFORM_COLOR-equivalent
// shader, which libobs doesn't provide). `mouse_images` may be null if no
// Image commands are present (i.e. custom mouse images are disabled).
void draw_display_list(const DisplayList& commands, GlyphAtlas& atlas, CustomMouseImages* mouse_images);

} // namespace sk
