#include "mouse-icon-images.h"

#include <util/platform.h>

namespace sk {

CustomMouseImages::~CustomMouseImages() {
    for (Slot* slot : {&base_, &left_, &right_, &middle_}) {
        if (slot->loaded) {
            gs_image_file2_free(&slot->image);
        }
    }
}

void CustomMouseImages::reload_slot(Slot& slot, const std::string& new_path) {
    if (new_path == slot.path) {
        return;
    }
    slot.path = new_path;

    if (slot.loaded) {
        gs_image_file2_free(&slot.image);
        slot.loaded = false;
    }

    if (new_path.empty() || !os_file_exists(new_path.c_str())) {
        return;
    }

    gs_image_file2_init(&slot.image, new_path.c_str());
    slot.loaded = slot.image.image.loaded;
    slot.needs_texture_init = slot.loaded;
}

void CustomMouseImages::update_paths(const std::string& base, const std::string& left, const std::string& right,
                                     const std::string& middle) {
    reload_slot(base_, base);
    reload_slot(left_, left);
    reload_slot(right_, right);
    reload_slot(middle_, middle);
}

void CustomMouseImages::flush_pending_uploads() {
    for (Slot* slot : {&base_, &left_, &right_, &middle_}) {
        if (slot->loaded && slot->needs_texture_init) {
            gs_image_file2_init_texture(&slot->image);
            slot->needs_texture_init = false;
        }
    }
}

gs_texture_t* CustomMouseImages::texture(MouseImageSlot slot_id) const {
    const Slot* slot = nullptr;
    switch (slot_id) {
        case MouseImageSlot::Base: slot = &base_; break;
        case MouseImageSlot::Left: slot = &left_; break;
        case MouseImageSlot::Right: slot = &right_; break;
        case MouseImageSlot::Middle: slot = &middle_; break;
    }
    return (slot && slot->loaded) ? slot->image.image.texture : nullptr;
}

bool CustomMouseImages::has_image(MouseImageSlot slot_id) const {
    return texture(slot_id) != nullptr;
}

} // namespace sk
