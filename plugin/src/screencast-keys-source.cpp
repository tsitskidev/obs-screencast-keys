#include "screencast-keys-source.h"

#include <vector>

#include <obs-source.h>
#include <util/platform.h>

#include "input/event-types.h"
#include "input/input-hook-manager.h"
#include "render/draw-utils.h"
#include "render/font-resolver.h"
#include "render/glyph-atlas.h"
#include "render/layout.h"
#include "render/mouse-icon-images.h"
#include "settings.h"
#include "state/event-history.h"
#include "state/input-state.h"

namespace {

using namespace sk;

// Candidate system font files, tried in order at source creation. Windows
// is the priority platform (see the project plan); macOS/Linux candidates
// are included since they cost nothing, but are not exercised/verified here.
const char* kFontCandidates[] = {
    "C:\\Windows\\Fonts\\segoeui.ttf",
    "C:\\Windows\\Fonts\\arial.ttf",
    "/System/Library/Fonts/Helvetica.ttc",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
};

#if defined(_WIN32)
constexpr TargetOs kTargetOs = TargetOs::WINDOWS;
#elif defined(__APPLE__)
constexpr TargetOs kTargetOs = TargetOs::MACOS;
#else
constexpr TargetOs kTargetOs = TargetOs::LINUX;
#endif

struct ScreencastKeysSource {
    obs_source_t* source = nullptr;
    int mailbox_id = 0;
    obs_hotkey_pair_id hotkey_pair = OBS_INVALID_HOTKEY_PAIR_ID;

    InputState input_state;
    EventHistory history;
    RenderSettings settings;
    GlyphAtlas atlas;
    CustomMouseImages mouse_images;

    // Tracks which (face, bold, italic) combination is currently loaded into
    // `atlas`, so source_update() only reloads (and resets the glyph cache)
    // when the font actually changed, not on every unrelated settings edit.
    bool font_loaded = false;
    std::string last_font_face;
    bool last_font_bold = false;
    bool last_font_italic = false;

    // Direct analog of the Blender addon's start/stop modal operator toggle:
    // pauses this instance's capture/display without touching the shared
    // global hook (other source instances keep working) or this source's
    // OBS-level enabled/visible state.
    bool enabled = true;

    DisplayList pending_commands;
    int cached_width = 1;
    int cached_height = 1;
};

const char* source_get_name(void*) {
    return "Screencast Keys";
}

void source_update(void* data, obs_data_t* settings);

// Resolves the current settings' (face, bold, italic) to an actual font
// file and (re)loads it into the atlas, falling back to a bundled system
// font if resolution fails outright.
void apply_font(ScreencastKeysSource* ctx) {
    const ResolvedFont resolved =
        resolve_font(ctx->settings.font_face, ctx->settings.font_bold, ctx->settings.font_italic);

    if (!resolved.path.empty() &&
        ctx->atlas.load_font(resolved.path, ctx->settings.font_bold && !resolved.exact_bold,
                              ctx->settings.font_italic && !resolved.exact_italic)) {
        return;
    }

    for (const char* path : kFontCandidates) {
        if (ctx->atlas.load_font(path, ctx->settings.font_bold, ctx->settings.font_italic)) {
            return;
        }
    }
    blog(LOG_WARNING, "[screencast-keys] no system font could be loaded; text will not render");
}

bool hotkey_enable(void* data, obs_hotkey_pair_id, obs_hotkey_t*, bool pressed) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);
    if (pressed) {
        ctx->enabled = true;
    }
    return pressed;
}

bool hotkey_disable(void* data, obs_hotkey_pair_id, obs_hotkey_t*, bool pressed) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);
    if (pressed) {
        ctx->enabled = false;
        ctx->history.clear();
        ctx->input_state.clear();
    }
    return pressed;
}

void* source_create(obs_data_t* settings, obs_source_t* source) {
    auto* ctx = new ScreencastKeysSource();
    ctx->source = source;
    ctx->mailbox_id = InputHookManager::instance().register_instance();

    ctx->hotkey_pair = obs_hotkey_pair_register_source(source, "ScreencastKeys.Enable", "Enable Screencast Keys",
                                                        "ScreencastKeys.Disable", "Disable Screencast Keys",
                                                        hotkey_enable, hotkey_disable, ctx, ctx);

    source_update(ctx, settings);
    return ctx;
}

void source_destroy(void* data) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);
    obs_hotkey_pair_unregister(ctx->hotkey_pair);
    InputHookManager::instance().unregister_instance(ctx->mailbox_id);
    delete ctx;
}

uint32_t source_get_width(void* data) {
    return static_cast<uint32_t>(static_cast<ScreencastKeysSource*>(data)->cached_width);
}

uint32_t source_get_height(void* data) {
    return static_cast<uint32_t>(static_cast<ScreencastKeysSource*>(data)->cached_height);
}

void apply_raw_event(ScreencastKeysSource* ctx, const RawInputEvent& raw, double now) {
    switch (raw.kind) {
        case RawInputKind::KeyPressed:
        case RawInputKind::KeyReleased: {
            const bool pressed = raw.kind == RawInputKind::KeyPressed;
            ctx->input_state.on_key_event(raw.code, pressed);

            if (pressed) {
                const EventType type = key_code_to_event_type(raw.code);
                if (type != EventType::UNKNOWN && !is_modifier(type)) {
                    ctx->history.record_press(type, ctx->input_state.held_modifiers(), now,
                                               ctx->settings.repeat_count, ctx->settings.display_time);
                }
            }
            break;
        }
        case RawInputKind::MousePressed:
        case RawInputKind::MouseReleased: {
            const bool pressed = raw.kind == RawInputKind::MousePressed;
            const EventType button = mouse_button_to_event_type(raw.code);
            ctx->input_state.on_mouse_button_event(button, pressed);

            if (pressed && button != EventType::UNKNOWN && show_mouse_event_history(ctx->settings)) {
                ctx->history.record_press(button, ctx->input_state.held_modifiers(), now,
                                           ctx->settings.repeat_count, ctx->settings.display_time);
            }
            break;
        }
        case RawInputKind::MouseWheel: {
            if (show_mouse_event_history(ctx->settings)) {
                const EventType type =
                    raw.wheel_rotation < 0 ? EventType::MOUSE_WHEEL_DOWN : EventType::MOUSE_WHEEL_UP;
                ctx->history.record_press(type, ctx->input_state.held_modifiers(), now, ctx->settings.repeat_count,
                                           ctx->settings.display_time);
            }
            break;
        }
    }
}

void source_video_tick(void* data, float /*seconds*/) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);

    const double now = os_gettime_ns() / 1000000000.0;
    std::vector<RawInputEvent> events = InputHookManager::instance().drain(ctx->mailbox_id);

    if (!ctx->enabled) {
        ctx->pending_commands.clear();
        ctx->cached_width = 1;
        ctx->cached_height = 1;
        return;
    }

    for (const RawInputEvent& raw : events) {
        apply_raw_event(ctx, raw, now);
    }

    ctx->history.prune(now, ctx->settings.display_time, ctx->settings.max_event_history);

    if (ctx->settings.use_custom_mouse_image) {
        ctx->mouse_images.update_paths(ctx->settings.custom_mouse_image_base, ctx->settings.custom_mouse_image_left,
                                       ctx->settings.custom_mouse_image_right,
                                       ctx->settings.custom_mouse_image_middle);
    }

    LayoutResult layout =
        build_display_list(ctx->history, ctx->input_state, ctx->settings, kTargetOs, ctx->atlas);
    ctx->pending_commands = std::move(layout.commands);
    ctx->cached_width = layout.width;
    ctx->cached_height = layout.height;
}

void source_video_render(void* data, gs_effect_t* /*effect*/) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);
    draw_display_list(ctx->pending_commands, ctx->atlas,
                       ctx->settings.use_custom_mouse_image ? &ctx->mouse_images : nullptr);
}

obs_properties_t* source_get_properties(void* /*data*/) {
    return build_render_settings_properties();
}

void source_get_defaults(obs_data_t* settings) {
    set_render_settings_defaults(settings);
}

void source_update(void* data, obs_data_t* settings) {
    auto* ctx = static_cast<ScreencastKeysSource*>(data);
    ctx->settings = read_render_settings(settings);

    if (!ctx->font_loaded || ctx->settings.font_face != ctx->last_font_face ||
        ctx->settings.font_bold != ctx->last_font_bold || ctx->settings.font_italic != ctx->last_font_italic) {
        apply_font(ctx);
        ctx->last_font_face = ctx->settings.font_face;
        ctx->last_font_bold = ctx->settings.font_bold;
        ctx->last_font_italic = ctx->settings.font_italic;
        ctx->font_loaded = true;
    }
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
    info.get_defaults = source_get_defaults;
    info.update = source_update;

    obs_register_source(&info);
}
