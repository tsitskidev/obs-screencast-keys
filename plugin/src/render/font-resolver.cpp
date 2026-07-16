#include "font-resolver.h"

#include <algorithm>
#include <cctype>
#include <vector>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace sk {

#if defined(_WIN32)

namespace {

std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

bool contains_word(const std::string& lower_haystack, const std::string& lower_needle) {
    return lower_haystack.find(lower_needle) != std::string::npos;
}

std::string strip_suffix_parens(std::string name) {
    // Registry value names look like "Arial Bold (TrueType)" -- drop the
    // trailing "(...)" annotation.
    const size_t paren = name.find_last_of('(');
    if (paren != std::string::npos) {
        name.erase(paren);
    }
    while (!name.empty() && name.back() == ' ') {
        name.pop_back();
    }
    return name;
}

std::string windows_fonts_dir() {
    char win_dir[MAX_PATH] = {0};
    GetWindowsDirectoryA(win_dir, MAX_PATH);
    return std::string(win_dir) + "\\Fonts\\";
}

struct Candidate {
    std::string style_name; // e.g. "Arial Bold", parens already stripped
    std::string file_path;
    bool has_bold = false;
    bool has_italic = false;
};

void enumerate_font_registry_key(HKEY root, const char* subkey, std::vector<Candidate>& out) {
    HKEY key;
    if (RegOpenKeyExA(root, subkey, 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return;
    }

    char value_name[512];
    unsigned char value_data[MAX_PATH];
    DWORD index = 0;
    while (true) {
        DWORD name_len = sizeof(value_name);
        DWORD data_len = sizeof(value_data);
        DWORD type = 0;
        const LONG result =
            RegEnumValueA(key, index, value_name, &name_len, nullptr, &type, value_data, &data_len);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        ++index;
        if (result != ERROR_SUCCESS || type != REG_SZ) {
            continue;
        }

        Candidate c;
        c.style_name = strip_suffix_parens(value_name);
        std::string file(reinterpret_cast<char*>(value_data), data_len > 0 ? data_len - 1 : 0);
        // Bare filenames (no path separator) live in the standard Fonts dir;
        // user-installed fonts sometimes register an absolute path instead.
        c.file_path = (file.find('\\') == std::string::npos) ? (windows_fonts_dir() + file) : file;

        const std::string lower_style = to_lower(c.style_name);
        c.has_bold = contains_word(lower_style, "bold");
        c.has_italic = contains_word(lower_style, "italic") || contains_word(lower_style, "oblique");

        out.push_back(std::move(c));
    }

    RegCloseKey(key);
}

std::string base_family_name(const std::string& lower_style_name) {
    static const char* kStyleWords[] = {"bold", "italic", "oblique", "regular"};
    std::string base = lower_style_name;
    for (const char* word : kStyleWords) {
        size_t pos;
        while ((pos = base.find(word)) != std::string::npos) {
            base.erase(pos, std::string(word).size());
        }
    }
    while (!base.empty() && base.back() == ' ') {
        base.pop_back();
    }
    return base;
}

} // namespace

ResolvedFont resolve_font(const std::string& face, bool want_bold, bool want_italic) {
    std::vector<Candidate> candidates;
    enumerate_font_registry_key(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts",
                                 candidates);
    enumerate_font_registry_key(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Fonts", candidates);

    const std::string lower_face = to_lower(face);

    const Candidate* best = nullptr;
    int best_score = -1;
    for (const Candidate& c : candidates) {
        const std::string lower_style = to_lower(c.style_name);
        const std::string family = base_family_name(lower_style);
        const bool family_matches = family == lower_face || lower_style.find(lower_face) != std::string::npos;
        if (!family_matches) {
            continue;
        }

        int score = 1; // Base: family matched at all.
        if (c.has_bold == want_bold) score += 2;
        if (c.has_italic == want_italic) score += 2;
        // Prefer the plainest matching style as a tiebreak (avoids e.g.
        // picking "Arial Black" over "Arial" for a non-bold request).
        score -= static_cast<int>(lower_style.size()) / 32;

        if (score > best_score) {
            best_score = score;
            best = &c;
        }
    }

    if (!best) {
        return {};
    }

    ResolvedFont result;
    result.path = best->file_path;
    result.exact_bold = (best->has_bold == want_bold) && (want_bold || !best->has_bold);
    result.exact_italic = (best->has_italic == want_italic) && (want_italic || !best->has_italic);
    return result;
}

#else // !_WIN32

ResolvedFont resolve_font(const std::string&, bool, bool) {
    return {}; // Not implemented on this platform yet -- caller falls back to a bundled/system default.
}

#endif

} // namespace sk
