#include "layout.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace sk {

namespace {

std::string join_modifiers(const std::vector<EventType>& mods, TargetOs os) {
    std::string result;
    for (size_t i = 0; i < mods.size(); ++i) {
        if (i > 0) {
            result += " + ";
        }
        result += display_name(mods[i], os);
    }
    return result;
}

std::string format_history_line(const HistoryEntry& entry, TargetOs os) {
    std::string text;
    if (!entry.modifiers.empty()) {
        text = join_modifiers(entry.modifiers, os) + " + ";
    }
    text += display_name(entry.event_type, os);
    if (entry.repeat_count > 1) {
        text += " x" + std::to_string(entry.repeat_count);
    }
    return text;
}

// Clamped so a tiny row never gets a corner radius larger than half its own
// width/height (which would otherwise make the arcs overlap/self-intersect).
float clamp_radius(float radius, float w, float h) {
    return std::min({radius, w * 0.5f, h * 0.5f});
}

Color faded(const Color& c, float alpha_scale) {
    return Color{c.r, c.g, c.b, c.a * alpha_scale};
}

struct HistoryRow {
    float width = 0;
    std::string text;
};

} // namespace

LayoutResult build_display_list(const EventHistory& history, const InputState& input_state,
                                 const RenderSettings& settings, TargetOs target_os, GlyphAtlas& atlas) {
    LayoutResult result;

    const float line_h = atlas.line_height(settings.font_size);
    const float bg_margin = static_cast<float>(settings.background_margin);
    const float row_h = line_h + bg_margin * 2.0f;

    // History rows, oldest-first (as EventHistory::entries() returns them).
    std::vector<HistoryRow> history_rows;
    for (const HistoryEntry& entry : history.entries()) {
        HistoryRow row;
        row.text = format_history_line(entry, target_os);
        row.width = atlas.measure_text_width(row.text, settings.font_size) + bg_margin * 2.0f;
        history_rows.push_back(std::move(row));
    }
    const size_t n = history_rows.size();

    const std::vector<EventType> held_modifiers = input_state.held_modifiers();
    const std::string modifier_text = held_modifiers.empty() ? "" : join_modifiers(held_modifiers, target_os);
    const bool draw_mouse = show_mouse_hold_status(settings);
    const bool draw_shortcut = !modifier_text.empty();

    // ===================================================================
    // The mouse icon is always drawn at literal (0,0) and is never shifted
    // -- no global translation is applied to the canvas based on other
    // elements' positions. (An earlier version sized the canvas to the
    // union bounding box of every element and then translated everything
    // so nothing clipped; that meant a negative shortcut/text offset --
    // placing something to the left of or above the mouse -- shifted the
    // *mouse* to make room, since the origin those elements share is the
    // mouse's own top-left corner.) The trade-off: an element positioned
    // with a large enough negative offset can now clip off-canvas instead
    // of growing the canvas -- accepted so the mouse truly never moves.
    // ===================================================================
    const float mouse_w = static_cast<float>(settings.mouse_size_x);
    const float mouse_h = static_cast<float>(settings.mouse_size_y);

    // Shortcut (modifier) pill position -- align-aware like text rows:
    // Left treats shortcut_offset_x as the pill's left edge (grows
    // rightward); Right treats it as the right edge (grows leftward).
    float pill_w = 0.0f, pill_x = 0.0f, pill_y = 0.0f;
    if (draw_shortcut) {
        const float shortcut_x = static_cast<float>(settings.shortcut_offset_x);
        pill_y = static_cast<float>(settings.shortcut_offset_y);
        pill_w = atlas.measure_text_width(modifier_text, settings.font_size) + bg_margin * 2.0f;
        pill_x = settings.align == Align::RIGHT ? shortcut_x - pill_w : shortcut_x;
    }

    // Newest entry anchored at (text_initial_offset_x/y); the i-th-older
    // entry cascades by i * (text_spacing_x, text_spacing_y). history_rows
    // is oldest-first, so "age" (0 = newest) counts down from the end.
    const float text_x0 = static_cast<float>(settings.text_initial_offset_x);
    const float text_y0 = static_cast<float>(settings.text_initial_offset_y);
    const float spacing_x = static_cast<float>(settings.text_spacing_x);
    const float spacing_y = static_cast<float>(settings.text_spacing_y);

    std::vector<float> row_x(n), row_y(n);
    for (size_t i = 0; i < n; ++i) {
        const float age = static_cast<float>(n - 1 - i);
        const float anchor_x = text_x0 + age * spacing_x;
        // Left: anchor_x is each line's left edge (text grows rightward).
        // Right: anchor_x is each line's right edge (text grows leftward),
        // so lines of different widths stay flush against a shared edge.
        row_x[i] = settings.align == Align::RIGHT ? anchor_x - history_rows[i].width : anchor_x;
        row_y[i] = text_y0 + age * spacing_y;
    }

    // Canvas size: at least big enough for the (fixed-at-origin) mouse, and
    // grown to include any element's positive extent. Elements sitting
    // partly or fully in negative space are simply clipped, not folded into
    // the canvas size (see the note above).
    float canvas_w = draw_mouse ? mouse_w : 1.0f;
    float canvas_h = draw_mouse ? mouse_h : 1.0f;
    auto grow = [&](float x, float y, float w, float h) {
        canvas_w = std::max(canvas_w, x + w);
        canvas_h = std::max(canvas_h, y + h);
    };
    if (draw_shortcut) {
        grow(pill_x, pill_y, pill_w, row_h);
    }
    for (size_t i = 0; i < n; ++i) {
        grow(row_x[i], row_y[i], history_rows[i].width, row_h);
    }
    canvas_w = std::max(canvas_w, 1.0f);
    canvas_h = std::max(canvas_h, 1.0f);

    result.width = std::max(1, static_cast<int>(std::ceil(canvas_w)));
    result.height = std::max(1, static_cast<int>(std::ceil(canvas_h)));

    // ===================================================================
    // Emit draw commands.
    // ===================================================================
    if (show_draw_area_background(settings)) {
        DrawCommand bg;
        bg.type = DrawCommandType::RoundedRect;
        bg.x = 0.0f;
        bg.y = 0.0f;
        bg.w = canvas_w;
        bg.h = canvas_h;
        bg.filled = true;
        bg.radius = clamp_radius(static_cast<float>(settings.background_rounded_corner_radius), bg.w, bg.h);
        bg.color = settings.background_color;
        result.commands.push_back(bg);
    }

    for (size_t i = 0; i < n; ++i) {
        const HistoryRow& row = history_rows[i];
        const float x = row_x[i];
        const float y = row_y[i];

        if (show_text_background(settings)) {
            DrawCommand bg;
            bg.type = DrawCommandType::RoundedRect;
            bg.x = x;
            bg.y = y;
            bg.w = row.width;
            bg.h = row_h;
            bg.filled = true;
            bg.radius = clamp_radius(static_cast<float>(settings.background_rounded_corner_radius), bg.w, bg.h);
            bg.color = settings.background_color;
            result.commands.push_back(bg);
        }

        const float baseline_y = y + bg_margin + line_h * 0.8f;

        if (settings.shadow) {
            DrawCommand shadow_cmd;
            shadow_cmd.type = DrawCommandType::Text;
            shadow_cmd.text = row.text;
            shadow_cmd.font_size = settings.font_size;
            shadow_cmd.x = x + bg_margin + 2.0f;
            shadow_cmd.y = baseline_y + 2.0f;
            shadow_cmd.color = settings.shadow_color;
            result.commands.push_back(shadow_cmd);
        }

        DrawCommand text_cmd;
        text_cmd.type = DrawCommandType::Text;
        text_cmd.text = row.text;
        text_cmd.font_size = settings.font_size;
        text_cmd.x = x + bg_margin;
        text_cmd.y = baseline_y;
        text_cmd.color = settings.color;
        result.commands.push_back(text_cmd);
    }

    if (draw_mouse) {
        if (settings.use_custom_mouse_image) {
            auto emit_image = [&](MouseImageSlot slot) {
                DrawCommand img;
                img.type = DrawCommandType::Image;
                img.x = 0.0f;
                img.y = 0.0f;
                img.w = mouse_w;
                img.h = mouse_h;
                img.image_slot = slot;
                result.commands.push_back(img);
            };

            const bool left_held = input_state.is_mouse_button_held(EventType::MOUSE_LEFT);
            const bool right_held = input_state.is_mouse_button_held(EventType::MOUSE_RIGHT);
            const bool middle_held = input_state.is_mouse_button_held(EventType::MOUSE_MIDDLE);

            if (settings.custom_mouse_image_display_mode == CustomMouseImageDisplayMode::OVERLAY) {
                emit_image(MouseImageSlot::Base);
                if (left_held) emit_image(MouseImageSlot::Left);
                if (right_held) emit_image(MouseImageSlot::Right);
                if (middle_held) emit_image(MouseImageSlot::Middle);
            } else if (left_held) {
                emit_image(MouseImageSlot::Left);
            } else if (right_held) {
                emit_image(MouseImageSlot::Right);
            } else if (middle_held) {
                emit_image(MouseImageSlot::Middle);
            } else {
                emit_image(MouseImageSlot::Base);
            }
        } else {
            // Three filled button rectangles only -- no outer body outline
            // (a prior version drew one; it read as a stray thin border
            // around the icon and has been removed).
            const float body_w = mouse_w / 3.0f;
            const EventType buttons[3] = {EventType::MOUSE_LEFT, EventType::MOUSE_MIDDLE, EventType::MOUSE_RIGHT};
            for (int i = 0; i < 3; ++i) {
                DrawCommand btn;
                btn.type = DrawCommandType::RoundedRect;
                btn.x = body_w * static_cast<float>(i);
                btn.y = 0.0f;
                btn.w = body_w;
                btn.h = mouse_h * 0.5f;
                btn.filled = true;
                btn.radius = clamp_radius(mouse_w * 0.1f, btn.w, btn.h);
                btn.color =
                    input_state.is_mouse_button_held(buttons[i]) ? settings.color : faded(settings.color, 0.35f);
                result.commands.push_back(btn);
            }
        }
    }

    if (draw_shortcut) {
        // Only fills its own background in TEXT mode -- in DRAW_AREA mode
        // the whole-canvas background already covers this, so filling here
        // too would draw a visibly different rect on top of it.
        const bool pill_filled = show_text_background(settings);

        DrawCommand pill;
        pill.type = DrawCommandType::RoundedRect;
        pill.x = pill_x;
        pill.y = pill_y;
        pill.w = pill_w;
        pill.h = row_h;
        pill.filled = pill_filled;
        pill.radius = clamp_radius(row_h * 0.2f, pill.w, pill.h);
        pill.color = pill_filled ? settings.background_color : settings.color;
        result.commands.push_back(pill);

        DrawCommand text_cmd;
        text_cmd.type = DrawCommandType::Text;
        text_cmd.text = modifier_text;
        text_cmd.font_size = settings.font_size;
        text_cmd.x = pill_x + bg_margin;
        text_cmd.y = pill_y + bg_margin + line_h * 0.8f;
        text_cmd.color = settings.color;
        result.commands.push_back(text_cmd);
    }

    return result;
}

} // namespace sk
