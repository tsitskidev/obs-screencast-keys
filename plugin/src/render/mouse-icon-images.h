#pragma once

#include <string>

#include <graphics/image-file.h>

#include "display-list.h"

namespace sk {

// Loads the 4 custom mouse images (base/left/right/middle) via libobs's
// gs_image_file2 helper. Port of common.py's ensure_custom_mouse_images() /
// reload_custom_mouse_image().
//
// Split into a CPU decode step (update_paths(), safe from video_tick -- it
// only reads/decodes image files, no gs_* calls) and a GPU upload step
// (flush_pending_uploads(), video_render only), the same pattern GlyphAtlas
// uses for glyph rasterization.
class CustomMouseImages {
public:
    ~CustomMouseImages();

    // Re-decodes any slot whose path changed since the last call. Call from
    // video_tick whenever settings change (cheap no-op otherwise).
    void update_paths(const std::string& base, const std::string& left, const std::string& right,
                       const std::string& middle);

    // Uploads any newly-decoded images to the GPU. Call from video_render
    // before drawing any Image command.
    void flush_pending_uploads();

    gs_texture_t* texture(MouseImageSlot slot) const;
    bool has_image(MouseImageSlot slot) const;

private:
    struct Slot {
        std::string path;
        gs_image_file2_t image{};
        bool loaded = false;
        bool needs_texture_init = false;
    };

    void reload_slot(Slot& slot, const std::string& new_path);

    Slot base_, left_, right_, middle_;
};

} // namespace sk
