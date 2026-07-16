#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include <graphics/graphics.h>

namespace sk {

struct GlyphInfo {
    // UV rect in the atlas texture, normalized [0,1].
    float u0 = 0, v0 = 0, u1 = 0, v1 = 0;
    // Glyph bitmap size in pixels.
    int width = 0, height = 0;
    // Offset from the pen position to the bitmap's top-left corner
    // (FreeType's bitmap_left / bitmap_top), in pixels, y-down.
    int bearing_x = 0, bearing_y = 0;
    // Horizontal distance to advance the pen after this glyph, in pixels.
    float advance = 0.0f;
};

// FreeType-backed glyph cache: rasterizes each (font_size, codepoint) pair
// once into a single shared atlas texture and remembers its UV rect and
// metrics, rather than caching whole rendered strings as textures.
//
// This matters here specifically because of how the Blender addon's content
// behaves: repeat-count text ("A x3" -> "A x4") and the sliding event
// history change on almost every keystroke, so a (string -> texture) cache
// would create/destroy a full GPU texture on nearly every event. Caching at
// the glyph level means a glyph is rasterized once ever; drawing a line of
// text becomes "look up each glyph's already-cached UV rect, emit one
// textured quad per glyph" -- the same approach OBS's own text-freetype2
// source uses. Color and drop-shadow are NOT baked into the glyph bitmap
// (it's an 8-bit coverage mask only); draw-utils.cpp applies those per draw
// call via the text effect's uniform color.
class GlyphAtlas {
public:
    GlyphAtlas();
    ~GlyphAtlas();

    GlyphAtlas(const GlyphAtlas&) = delete;
    GlyphAtlas& operator=(const GlyphAtlas&) = delete;

    // Loads a font file (TTF/OTF). Returns false if FreeType could not open
    // or parse it. See font-paths.h for platform-default candidates.
    bool load_font(const std::string& font_path);

    bool has_font() const { return ft_face_ != nullptr; }

    // Measures a string's rendered width/line-height in pixels at the given
    // pixel size, without drawing anything. Mirrors blf.dimensions. Also
    // stages (rasterizes, CPU-only) any glyphs not yet cached, since
    // measuring requires knowing each glyph's advance width anyway.
    float measure_text_width(const std::string& text, int font_size);
    float line_height(int font_size);

    // Uploads any glyphs rasterized since the last flush to the GPU atlas
    // texture. Must be called from video_render (this is the one gs_*-call
    // exception noted in the architecture: infrequent, dirty-flag-gated,
    // same pattern as the custom mouse image loader).
    void flush_pending_uploads();

    // Valid only after at least one flush_pending_uploads() call.
    gs_texture_t* texture() const { return texture_; }

    // nullptr if the glyph has never been staged; call measure_text_width()
    // or stage_text() on the containing string first (layout.cpp always
    // measures before it lays out, so this is naturally satisfied in
    // practice).
    const GlyphInfo* find_glyph(int font_size, uint32_t codepoint);

private:
    struct GlyphKey {
        int font_size;
        uint32_t codepoint;
        bool operator==(const GlyphKey& other) const {
            return font_size == other.font_size && codepoint == other.codepoint;
        }
    };
    struct GlyphKeyHash {
        size_t operator()(const GlyphKey& key) const {
            return (static_cast<size_t>(key.font_size) << 32) ^ static_cast<size_t>(key.codepoint);
        }
    };

    const GlyphInfo* rasterize_and_pack(int font_size, uint32_t codepoint);
    void reset_atlas();

    void* ft_library_ = nullptr; // FT_Library
    void* ft_face_ = nullptr;    // FT_Face

    static constexpr int kAtlasSize = 1024;
    std::vector<uint8_t> atlas_cpu_; // kAtlasSize * kAtlasSize, 8bpp coverage.
    gs_texture_t* texture_ = nullptr;
    bool texture_dirty_ = false;

    // Simple shelf (row) packer -- ample for the addon's ASCII-only,
    // few-font-sizes-at-once glyph set. If it overflows we just wipe the
    // atlas and start over (rare: only many distinct font sizes ever used
    // in one session would trigger it).
    int shelf_x_ = 0, shelf_y_ = 0, shelf_height_ = 0;

    std::unordered_map<GlyphKey, GlyphInfo, GlyphKeyHash> glyphs_;
};

} // namespace sk
