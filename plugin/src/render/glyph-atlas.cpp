#include "glyph-atlas.h"

#include <algorithm>
#include <cstring>

#include <ft2build.h>
#include FT_FREETYPE_H

namespace sk {

namespace {
FT_Library lib(void* p) { return static_cast<FT_Library>(p); }
FT_Face face(void* p) { return static_cast<FT_Face>(p); }
} // namespace

GlyphAtlas::GlyphAtlas() {
    FT_Library library = nullptr;
    if (FT_Init_FreeType(&library) == 0) {
        ft_library_ = library;
    }
}

GlyphAtlas::~GlyphAtlas() {
    if (ft_face_) {
        FT_Done_Face(face(ft_face_));
    }
    if (ft_library_) {
        FT_Done_FreeType(lib(ft_library_));
    }
    if (texture_) {
        gs_texture_destroy(texture_);
    }
}

bool GlyphAtlas::load_font(const std::string& font_path) {
    if (!ft_library_) {
        return false;
    }
    if (ft_face_) {
        FT_Done_Face(face(ft_face_));
        ft_face_ = nullptr;
    }

    FT_Face new_face = nullptr;
    if (FT_New_Face(lib(ft_library_), font_path.c_str(), 0, &new_face) != 0) {
        return false;
    }
    ft_face_ = new_face;
    reset_atlas();
    return true;
}

void GlyphAtlas::reset_atlas() {
    glyphs_.clear();
    atlas_cpu_.assign(static_cast<size_t>(kAtlasSize) * kAtlasSize, 0);
    shelf_x_ = shelf_y_ = shelf_height_ = 0;
    texture_dirty_ = true; // Forces a full re-upload of the (now-blank) atlas.
}

const GlyphInfo* GlyphAtlas::rasterize_and_pack(int font_size, uint32_t codepoint) {
    if (!ft_face_) {
        return nullptr;
    }

    FT_Face f = face(ft_face_);
    FT_Set_Pixel_Sizes(f, 0, static_cast<FT_UInt>(font_size));

    if (FT_Load_Char(f, codepoint, FT_LOAD_RENDER) != 0) {
        return nullptr;
    }

    FT_GlyphSlot slot = f->glyph;
    const int glyph_w = static_cast<int>(slot->bitmap.width);
    const int glyph_h = static_cast<int>(slot->bitmap.rows);

    // Shelf-pack: advance along the current row; start a new row when it
    // doesn't fit; wipe and restart from the top-left if we run out of
    // vertical space entirely (see the class docs -- rare in practice).
    if (shelf_x_ + glyph_w > kAtlasSize) {
        shelf_x_ = 0;
        shelf_y_ += shelf_height_;
        shelf_height_ = 0;
    }
    if (shelf_y_ + glyph_h > kAtlasSize) {
        reset_atlas();
        shelf_x_ = shelf_y_ = shelf_height_ = 0;
    }

    // Copy the (possibly zero-size, e.g. for ' ') coverage bitmap into the
    // CPU-side atlas buffer at (shelf_x_, shelf_y_).
    for (int row = 0; row < glyph_h; ++row) {
        uint8_t* dst = &atlas_cpu_[static_cast<size_t>(shelf_y_ + row) * kAtlasSize + shelf_x_];
        const uint8_t* src = slot->bitmap.buffer + static_cast<size_t>(row) * slot->bitmap.pitch;
        std::memcpy(dst, src, static_cast<size_t>(glyph_w));
    }

    GlyphInfo info;
    info.width = glyph_w;
    info.height = glyph_h;
    info.bearing_x = slot->bitmap_left;
    info.bearing_y = slot->bitmap_top;
    info.advance = static_cast<float>(slot->advance.x) / 64.0f; // 26.6 fixed point.
    info.u0 = static_cast<float>(shelf_x_) / kAtlasSize;
    info.v0 = static_cast<float>(shelf_y_) / kAtlasSize;
    info.u1 = static_cast<float>(shelf_x_ + glyph_w) / kAtlasSize;
    info.v1 = static_cast<float>(shelf_y_ + glyph_h) / kAtlasSize;

    shelf_x_ += glyph_w;
    shelf_height_ = std::max(shelf_height_, glyph_h);
    texture_dirty_ = true;

    GlyphKey key{font_size, codepoint};
    auto [it, inserted] = glyphs_.insert_or_assign(key, info);
    return &it->second;
}

const GlyphInfo* GlyphAtlas::find_glyph(int font_size, uint32_t codepoint) {
    GlyphKey key{font_size, codepoint};
    auto it = glyphs_.find(key);
    if (it != glyphs_.end()) {
        return &it->second;
    }
    return rasterize_and_pack(font_size, codepoint);
}

float GlyphAtlas::measure_text_width(const std::string& text, int font_size) {
    float width = 0.0f;
    // ASCII-only: every label this addon ever draws (key names, "Ctrl",
    // "Numpad Enter", repeat-count suffixes, ...) is plain ASCII, so
    // iterating raw bytes as codepoints is sufficient -- no UTF-8 decoding.
    for (unsigned char c : text) {
        if (const GlyphInfo* glyph = find_glyph(font_size, c)) {
            width += glyph->advance;
        }
    }
    return width;
}

float GlyphAtlas::line_height(int font_size) {
    if (!ft_face_) {
        return static_cast<float>(font_size);
    }
    FT_Face f = face(ft_face_);
    FT_Set_Pixel_Sizes(f, 0, static_cast<FT_UInt>(font_size));
    // face->size->metrics.height is already in 26.6 fixed point pixels.
    return static_cast<float>(f->size->metrics.height) / 64.0f;
}

void GlyphAtlas::flush_pending_uploads() {
    if (!texture_) {
        texture_ = gs_texture_create(kAtlasSize, kAtlasSize, GS_R8, 1, nullptr, GS_DYNAMIC);
        texture_dirty_ = true;
    }
    if (texture_dirty_ && texture_) {
        gs_texture_set_image(texture_, atlas_cpu_.data(), kAtlasSize, false);
        texture_dirty_ = false;
    }
}

} // namespace sk
