#include "layout.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace sk {

namespace {

// Stroke width for the default-drawn mouse icon's outline. Was a user
// setting (line_thickness) before; simplified to a fixed constant based on
// testing feedback (removed to cut down the settings surface).
constexpr float kDefaultLineThickness = 2.0f;

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

// Tracks the union bounding box of every element actually drawn, in
// "raw" (pre-translation) coordinates -- elements are positioned
// independently and may use negative offsets/spacing, so the canvas is
// sized to whatever their union turns out to be, then everything is
// shifted so the box's top-left lands on (0,0) (see translate_x/y below).
// This is what makes negative text_spacing_x/y or shortcut/text offsets
// work without clipping anything off-canvas.
struct BoundingBox {
    float min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    bool any = false;

    void expand(float x, float y, float w, float h) {
        if (w <= 0.0f || h <= 0.0f) {
            return;
        }
        if (!any) {
            min_x = x;
            min_y = y;
            max_x = x + w;
            max_y = y + h;
            any = true;
            return;
        }
        min_x = std::min(min_x, x);
        min_y = std::min(min_y, y);
        max_x = std::max(max_x, x + w);
        max_y = std::max(max_y, y + h);
    }
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
    // Pass 1: compute each element's raw (pre-translation) position and
    // the union bounding box. The mouse icon, the shortcut (modifier) pill,
    // and every text history line are fully independent -- none of them
    // affect each other's position or size.
    // ===================================================================
    BoundingBox bounds;

    const float mouse_w = static_cast<float>(settings.mouse_size_x);
    const float mouse_h = static_cast<float>(settings.mouse_size_y);
    if (draw_mouse) {
        bounds.expand(0.0f, 0.0f, mouse_w, mouse_h);
    }

    const float shortcut_x = static_cast<float>(settings.shortcut_offset_x);
    const float shortcut_y = static_cast<float>(settings.shortcut_offset_y);
    float pill_w = 0.0f;
    if (draw_shortcut) {
        pill_w = atlas.measure_text_width(modifier_text, settings.font_size) + bg_margin * 2.0f;
        bounds.expand(shortcut_x, shortcut_y, pill_w, row_h);
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
        row_x[i] = text_x0 + age * spacing_x;
        row_y[i] = text_y0 + age * spacing_y;
        bounds.expand(row_x[i], row_y[i], history_rows[i].width, row_h);
    }

    if (!bounds.any) {
        bounds.min_x = bounds.min_y = 0.0f;
        bounds.max_x = bounds.max_y = 1.0f;
    }
    const float canvas_width = bounds.max_x - bounds.min_x;
    const float canvas_height = bounds.max_y - bounds.min_y;
    const float translate_x = -bounds.min_x;
    const float translate_y = -bounds.min_y;

    result.width = std::max(1, static_cast<int>(std::ceil(canvas_width)));
    result.height = std::max(1, static_cast<int>(std::ceil(canvas_height)));

    // ===================================================================
    // Pass 2: emit draw commands at each element's translated position.
    // ===================================================================
    if (show_draw_area_background(settings)) {
        DrawCommand bg;
        bg.type = DrawCommandType::RoundedRect;
        bg.x = 0.0f;
        bg.y = 0.0f;
        bg.w = canvas_width;
        bg.h = canvas_height;
        bg.filled = true;
        bg.radius = clamp_radius(static_cast<float>(settings.background_rounded_corner_radius), bg.w, bg.h);
        bg.color = settings.background_color;
        result.commands.push_back(bg);
    }

    for (size_t i = 0; i < n; ++i) {
        const HistoryRow& row = history_rows[i];
        const float x = row_x[i] + translate_x;
        const float y = row_y[i] + translate_y;

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
        const float x = translate_x;
        const float y = translate_y;

        if (settings.use_custom_mouse_image) {
            auto emit_image = [&](MouseImageSlot slot) {
                DrawCommand img;
                img.type = DrawCommandType::Image;
                img.x = x;
                img.y = y;
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
            const float body_w = mouse_w / 3.0f;

            DrawCommand body;
            body.type = DrawCommandType::RoundedRect;
            body.x = x;
            body.y = y;
            body.w = mouse_w;
            body.h = mouse_h;
            body.filled = false;
            body.radius = clamp_radius(mouse_w * 0.15f, mouse_w, mouse_h);
            body.color = settings.color;
            body.line_thickness = kDefaultLineThickness;
            result.commands.push_back(body);

            const EventType buttons[3] = {EventType::MOUSE_LEFT, EventType::MOUSE_MIDDLE, EventType::MOUSE_RIGHT};
            for (int i = 0; i < 3; ++i) {
                DrawCommand btn;
                btn.type = DrawCommandType::RoundedRect;
                btn.x = x + body_w * static_cast<float>(i);
                btn.y = y;
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
        const float x = shortcut_x + translate_x;
        const float y = shortcut_y + translate_y;
        const bool pill_filled = show_text_background(settings) || show_draw_area_background(settings);

        DrawCommand pill;
        pill.type = DrawCommandType::RoundedRect;
        pill.x = x;
        pill.y = y;
        pill.w = pill_w;
        pill.h = row_h;
        pill.filled = pill_filled;
        pill.radius = clamp_radius(row_h * 0.2f, pill.w, pill.h);
        pill.color = pill_filled ? settings.background_color : settings.color;
        pill.line_thickness = kDefaultLineThickness;
        result.commands.push_back(pill);

        DrawCommand text_cmd;
        text_cmd.type = DrawCommandType::Text;
        text_cmd.text = modifier_text;
        text_cmd.font_size = settings.font_size;
        text_cmd.x = x + bg_margin;
        text_cmd.y = y + bg_margin + line_h * 0.8f;
        text_cmd.color = settings.color;
        result.commands.push_back(text_cmd);
    }

    return result;
}

} // namespace sk
