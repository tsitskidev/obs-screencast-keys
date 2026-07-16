#pragma once

#include <string>

namespace sk {

struct ResolvedFont {
    std::string path; // Empty if nothing could be resolved.
    // True if `path` already IS the requested style (a dedicated bold/italic
    // font file was found) -- false means the caller should synthesize that
    // style (FreeType FT_GlyphSlot_Embolden/Oblique) on top of the regular file.
    bool exact_bold = false;
    bool exact_italic = false;
};

// Best-effort mapping from a font family name (as returned by OBS's native
// font-picker property, e.g. "Arial") plus desired bold/italic to an actual
// font file FreeType can load. Windows-only for now (registry-based lookup
// under HKLM/HKCU ...\CurrentVersion\Fonts); other platforms always return
// an empty path, and callers should fall back to a bundled/system default.
ResolvedFont resolve_font(const std::string& face, bool want_bold, bool want_italic);

} // namespace sk
