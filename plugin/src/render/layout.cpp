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

    const float margin = static_cast<float>(settings.margin);
    const float line_h = atlas.line_height(settings.font_size);
    const float row_h = std::max(line_h, static_cast<float>(settings.mouse_size)) + margin * 2.0f;

    // --- Measure phase --------------------------------------------------
    std::vector<HistoryRow> history_rows;
    for (const HistoryEntry& entry : history.entries()) {
        HistoryRow row;
        row.text = format_history_line(entry, target_os);
        row.width = atlas.measure_text_width(row.text, settings.font_size) + margin * 2.0f;
        history_rows.push_back(std::move(row));
    }

    const std::vector<EventType> held_modifiers = input_state.held_modifiers();
    const std::string modifier_text = held_modifiers.empty() ? "" : join_modifiers(held_modifiers, target_os);
    const bool draw_mouse_row = show_mouse_hold_status(settings) || !modifier_text.empty();

    float mouse_row_width = 0.0f;
    if (draw_mouse_row) {
        const float mouse_w = show_mouse_hold_status(settings) ? static_cast<float>(settings.mouse_size) : 0.0f;
        const float mod_w =
            modifier_text.empty() ? 0.0f : atlas.measure_text_width(modifier_text, settings.font_size) + margin * 2.0f;
        const float separator = (mouse_w > 0.0f && mod_w > 0.0f) ? margin * 2.0f : 0.0f;
        mouse_row_width = mouse_w + separator + mod_w + margin * 2.0f;
    }

    float content_width = mouse_row_width;
    for (const HistoryRow& row : history_rows) {
        content_width = std::max(content_width, row.width);
    }
    content_width = std::max(content_width, 1.0f);

    const float history_height = static_cast<float>(history_rows.size()) * row_h;
    const float mouse_row_height = draw_mouse_row ? row_h : 0.0f;
    const float content_height = std::max(history_height + mouse_row_height, 1.0f);

    const bool use_fixed_canvas = settings.canvas_size_mode == CanvasSizeMode::FIXED;
    const float canvas_width = use_fixed_canvas ? static_cast<float>(settings.canvas_width) : content_width;
    const float canvas_height = use_fixed_canvas ? static_cast<float>(settings.canvas_height) : content_height;

    result.width = std::max(1, static_cast<int>(std::ceil(canvas_width)));
    result.height = std::max(1, static_cast<int>(std::ceil(canvas_height)));

    // offset_x/offset_y only mean anything in FIXED mode -- in AUTO mode the
    // canvas exactly matches the content, so there is no room to offset
    // within it; OBS's own scene-item transform is the positioning
    // mechanism there instead.
    const float offset_x = use_fixed_canvas ? static_cast<float>(settings.offset_x) : 0.0f;
    const float offset_y = use_fixed_canvas ? static_cast<float>(settings.offset_y) : 0.0f;

    auto align_x = [&](float row_width) -> float {
        switch (settings.align) {
            case Align::LEFT:
                return offset_x;
            case Align::CENTER:
                return (canvas_width - row_width) * 0.5f;
            case Align::RIGHT:
                return canvas_width - row_width - offset_x;
        }
        return 0.0f;
    };

    // Bottom-anchored, matching the Blender addon's stacking: mouse/modifier
    // row nearest the bottom, history growing upward above it.
    const float base_y = canvas_height - content_height - offset_y;

    if (show_draw_area_background(settings)) {
        DrawCommand bg;
        bg.type = DrawCommandType::RoundedRect;
        bg.x = 0.0f;
        bg.y = base_y;
        bg.w = content_width;
        bg.h = content_height;
        bg.filled = true;
        bg.radius = clamp_radius(static_cast<float>(settings.background_rounded_corner_radius), bg.w, bg.h);
        bg.color = settings.background_color;
        result.commands.push_back(bg);
    }

    float y = base_y;

    // History, oldest first (top of the stack), newest last (bottom row,
    // immediately above the mouse/modifier row) -- matches
    // _draw_event_history_layer's event_history[::-1] iteration order.
    for (const HistoryRow& row : history_rows) {
        const float row_x = align_x(row.width);

        if (show_text_background(settings)) {
            DrawCommand bg;
            bg.type = DrawCommandType::RoundedRect;
            bg.x = row_x;
            bg.y = y;
            bg.w = row.width;
            bg.h = row_h;
            bg.filled = true;
            bg.radius = clamp_radius(static_cast<float>(settings.background_rounded_corner_radius), bg.w, bg.h);
            bg.color = settings.background_color;
            result.commands.push_back(bg);
        }

        const float baseline_y = y + margin + line_h * 0.8f;

        if (settings.shadow) {
            DrawCommand shadow_cmd;
            shadow_cmd.type = DrawCommandType::Text;
            shadow_cmd.text = row.text;
            shadow_cmd.font_size = settings.font_size;
            shadow_cmd.x = row_x + margin + 2.0f;
            shadow_cmd.y = baseline_y + 2.0f;
            shadow_cmd.color = settings.shadow_color;
            result.commands.push_back(shadow_cmd);
        }

        DrawCommand text_cmd;
        text_cmd.type = DrawCommandType::Text;
        text_cmd.text = row.text;
        text_cmd.font_size = settings.font_size;
        text_cmd.x = row_x + margin;
        text_cmd.y = baseline_y;
        text_cmd.color = settings.color;
        result.commands.push_back(text_cmd);

        y += row_h;
    }

    // Mouse hold status + modifier pill, bottom row.
    if (draw_mouse_row) {
        const float row_x = align_x(mouse_row_width);
        float x = row_x + margin;

        if (show_mouse_hold_status(settings)) {
            const float mouse_w = static_cast<float>(settings.mouse_size);

            if (settings.use_custom_mouse_image) {
                // Simplified vs. the Blender addon's separate custom_mouse_size
                // width/height: custom images use a square mouse_w x mouse_w
                // footprint here rather than an independently configurable size.
                const float mouse_h = mouse_w;
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

                x += mouse_w + margin * 2.0f;
            } else {
                const float mouse_h = mouse_w * 1.3f;
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
                body.line_thickness = settings.line_thickness;
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

                x += mouse_w + margin * 2.0f;
            }
        }

        if (!modifier_text.empty()) {
            const float mod_w = atlas.measure_text_width(modifier_text, settings.font_size) + margin * 2.0f;
            const bool pill_filled = show_text_background(settings) || show_draw_area_background(settings);

            DrawCommand pill;
            pill.type = DrawCommandType::RoundedRect;
            pill.x = x;
            pill.y = y;
            pill.w = mod_w;
            pill.h = row_h;
            pill.filled = pill_filled;
            pill.radius = clamp_radius(row_h * 0.2f, pill.w, pill.h);
            pill.color = pill_filled ? settings.background_color : settings.color;
            pill.line_thickness = settings.line_thickness;
            result.commands.push_back(pill);

            DrawCommand text_cmd;
            text_cmd.type = DrawCommandType::Text;
            text_cmd.text = modifier_text;
            text_cmd.font_size = settings.font_size;
            text_cmd.x = x + margin;
            text_cmd.y = y + margin + line_h * 0.8f;
            text_cmd.color = settings.color;
            result.commands.push_back(text_cmd);
        }
    }

    return result;
}

} // namespace sk
