#include "screencast-keys-source.h"

#include <graphics/graphics.h>
#include <obs-source.h>
#include <util/platform.h>

#include "input/event-types.h"
#include "input/input-hook-manager.h"
#include "state/event-history.h"
#include "state/input-state.h"

// Phase 1/2 checkpoint: real global input capture + the ported state engine
// (InputState/EventHistory) wired into a real obs_source_info, but with
// placeholder (non-text) rendering. This exists to prove the CMake -> MSVC
// -> libobs link -> libuiohook capture -> video_tick/video_render pipeline
// end-to-end before Phase 3 replaces the placeholder with the real
// FreeType/glyph-atlas/layout-based rendering that matches the Blender
// addon's actual visual output.

namespace {

constexpr double kPlaceholderDisplayTimeSeconds = 3.0;
constexpr int kPlaceholderMaxHistory = 5;
constexpr bool kPlaceholderRepeatCountEnabled = true;

constexpr uint32_t kMinWidth = 220;
constexpr uint32_t kMinHeight = 60;
constexpr uint32_t kHistoryRowHeight = 22;
constexpr uint32_t kPadding = 8;

struct ScreencastKeysSource {
    obs_source_t* source = nullptr;
    int mailbox_id = 0;

    sk::InputState input_state;
    sk::EventHistory history;

    uint32_t cached_width = kMinWidth;
    uint32_t cached_height = kMinHeight;
};

const char* source_get_name(void*) {
    return "Screencast Keys";
}

void* source_create(obs_data_t* /*settings*/, obs_source_t* source) {
    auto* ctx = new ScreencastKeysSource();
    ctx->source = source;
    ctx->mailbox_id = sk::InputHookManager::instance().register_instance();
    return ctx;
}

void source_destroy(void* data) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);
    sk::InputHookManager::instance().unregister_instance(ctx->mailbox_id);
    delete ctx;
}

uint32_t source_get_width(void* data) {
    return static_cast<ScreencastKeysSource*>(data)->cached_width;
}

uint32_t source_get_height(void* data) {
    return static_cast<ScreencastKeysSource*>(data)->cached_height;
}

void apply_raw_event(ScreencastKeysSource* ctx, const sk::RawInputEvent& raw, double now) {
    using sk::EventType;
    using sk::RawInputKind;

    switch (raw.kind) {
        case RawInputKind::KeyPressed:
        case RawInputKind::KeyReleased: {
            const bool pressed = raw.kind == RawInputKind::KeyPressed;
            ctx->input_state.on_key_event(raw.code, pressed);

            if (pressed) {
                const EventType type = sk::key_code_to_event_type(raw.code);
                if (type != EventType::UNKNOWN && !sk::is_modifier(type)) {
                    ctx->history.record_press(type, ctx->input_state.held_modifiers(), now,
                                               kPlaceholderRepeatCountEnabled,
                                               kPlaceholderDisplayTimeSeconds);
                }
            }
            break;
        }
        case RawInputKind::MousePressed:
        case RawInputKind::MouseReleased: {
            const bool pressed = raw.kind == RawInputKind::MousePressed;
            const EventType button = sk::mouse_button_to_event_type(raw.code);
            ctx->input_state.on_mouse_button_event(button, pressed);

            if (pressed && button != EventType::UNKNOWN) {
                ctx->history.record_press(button, ctx->input_state.held_modifiers(), now,
                                           kPlaceholderRepeatCountEnabled,
                                           kPlaceholderDisplayTimeSeconds);
            }
            break;
        }
        case RawInputKind::MouseWheel: {
            const EventType type =
                raw.wheel_rotation < 0 ? EventType::MOUSE_WHEEL_DOWN : EventType::MOUSE_WHEEL_UP;
            ctx->history.record_press(type, ctx->input_state.held_modifiers(), now,
                                       kPlaceholderRepeatCountEnabled, kPlaceholderDisplayTimeSeconds);
            break;
        }
    }
}

void source_video_tick(void* data, float /*seconds*/) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);

    const double now = os_gettime_ns() / 1000000000.0;

    for (const sk::RawInputEvent& raw : sk::InputHookManager::instance().drain(ctx->mailbox_id)) {
        apply_raw_event(ctx, raw, now);
    }

    ctx->history.prune(now, kPlaceholderDisplayTimeSeconds, kPlaceholderMaxHistory);

    // Placeholder sizing (Phase 3 replaces this with real text-measurement
    // driven layout, matching draw_area_size()'s dynamic width/height).
    const auto& entries = ctx->history.entries();
    ctx->cached_height =
        kMinHeight + static_cast<uint32_t>(entries.size()) * kHistoryRowHeight;
    ctx->cached_width = kMinWidth;
}

void draw_filled_rect(float x, float y, float w, float h, const vec4& color) {
    gs_effect_t* solid = obs_get_base_effect(OBS_EFFECT_SOLID);
    gs_eparam_t* color_param = gs_effect_get_param_by_name(solid, "color");
    gs_technique_t* tech = gs_effect_get_technique(solid, "Solid");

    gs_effect_set_vec4(color_param, &color);

    gs_technique_begin(tech);
    gs_technique_begin_pass(tech, 0);

    gs_render_start(true);
    gs_vertex2f(x, y);
    gs_vertex2f(x + w, y);
    gs_vertex2f(x, y + h);
    gs_vertex2f(x + w, y + h);
    gs_vertbuffer_t* vb = gs_render_save();
    gs_load_vertexbuffer(vb);
    gs_draw(GS_TRISTRIP, 0, 4);
    gs_vertexbuffer_destroy(vb);

    gs_technique_end_pass(tech);
    gs_technique_end(tech);
}

void source_video_render(void* data, gs_effect_t* /*effect*/) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);

    const float width = static_cast<float>(ctx->cached_width);
    const float height = static_cast<float>(ctx->cached_height);

    // Background.
    vec4 bg{};
    vec4_set(&bg, 0.08f, 0.08f, 0.08f, 0.85f);
    draw_filled_rect(0, 0, width, height, bg);

    // One bar per currently-held mouse button/modifier -- a crude stand-in
    // for the real mouse-hold-status + modifier-pill row (Phase 3).
    using sk::EventType;
    float x = kPadding;
    const float bar_w = 28.0f, bar_h = 28.0f;
    const EventType mouse_buttons[] = {EventType::MOUSE_LEFT, EventType::MOUSE_MIDDLE,
                                        EventType::MOUSE_RIGHT};
    for (EventType button : mouse_buttons) {
        vec4 color{};
        if (ctx->input_state.is_mouse_button_held(button)) {
            vec4_set(&color, 0.2f, 0.8f, 0.3f, 1.0f);
        } else {
            vec4_set(&color, 0.4f, 0.4f, 0.4f, 1.0f);
        }
        draw_filled_rect(x, kPadding, bar_w, bar_h, color);
        x += bar_w + 4;
    }

    for (EventType modifier : ctx->input_state.held_modifiers()) {
        (void)modifier;
        vec4 color{};
        vec4_set(&color, 0.3f, 0.5f, 0.9f, 1.0f);
        draw_filled_rect(x, kPadding, bar_w, bar_h, color);
        x += bar_w + 4;
    }

    // One bar per event-history entry -- stand-in for the real fading text
    // history (Phase 3).
    float y = kPadding * 2 + bar_h;
    for (const auto& entry : ctx->history.entries()) {
        const float entry_w = 40.0f + static_cast<float>(entry.repeat_count) * 10.0f;
        vec4 color{};
        vec4_set(&color, 0.7f, 0.7f, 0.2f, 1.0f);
        draw_filled_rect(kPadding, y, entry_w, kHistoryRowHeight - 4, color);
        y += kHistoryRowHeight;
    }
}

obs_properties_t* source_get_properties(void* /*data*/) {
    obs_properties_t* props = obs_properties_create();
    obs_properties_add_color(props, "color", "Text Color (placeholder)");
    return props;
}

} // namespace

void screencast_keys_source_register() {
    obs_source_info info{};
    info.id = "screencast_keys_source";
    info.type = OBS_SOURCE_TYPE_INPUT;
    info.output_flags = OBS_SOURCE_VIDEO;
    info.get_name = source_get_name;
    info.create = source_create;
    info.destroy = source_destroy;
    info.get_width = source_get_width;
    info.get_height = source_get_height;
    info.video_tick = source_video_tick;
    info.video_render = source_video_render;
    info.get_properties = source_get_properties;

    obs_register_source(&info);
}
