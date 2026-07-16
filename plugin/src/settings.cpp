#include "settings.h"

namespace sk {

namespace {

uint32_t pack_color(const Color& c) {
    return (static_cast<uint32_t>(c.a * 255.0f) << 24) | (static_cast<uint32_t>(c.b * 255.0f) << 16) |
           (static_cast<uint32_t>(c.g * 255.0f) << 8) | static_cast<uint32_t>(c.r * 255.0f);
}

Color unpack_color(uint32_t packed) {
    Color c;
    c.r = static_cast<float>(packed & 0xFF) / 255.0f;
    c.g = static_cast<float>((packed >> 8) & 0xFF) / 255.0f;
    c.b = static_cast<float>((packed >> 16) & 0xFF) / 255.0f;
    c.a = static_cast<float>((packed >> 24) & 0xFF) / 255.0f;
    return c;
}

bool shadow_modified(obs_properties_t* props, obs_property_t*, obs_data_t* settings) {
    obs_property_set_visible(obs_properties_get(props, "shadow_color"), obs_data_get_bool(settings, "shadow"));
    return true;
}

bool background_modified(obs_properties_t* props, obs_property_t*, obs_data_t* settings) {
    const bool on = obs_data_get_bool(settings, "background");
    obs_property_set_visible(obs_properties_get(props, "background_mode"), on);
    obs_property_set_visible(obs_properties_get(props, "background_color"), on);
    obs_property_set_visible(obs_properties_get(props, "background_rounded_corner_radius"), on);
    obs_property_set_visible(obs_properties_get(props, "background_margin"), on);
    return true;
}

bool show_mouse_events_modified(obs_properties_t* props, obs_property_t*, obs_data_t* settings) {
    obs_property_set_visible(obs_properties_get(props, "mouse_events_show_mode"),
                              obs_data_get_bool(settings, "show_mouse_events"));
    return true;
}

bool use_custom_mouse_image_modified(obs_properties_t* props, obs_property_t*, obs_data_t* settings) {
    const bool on = obs_data_get_bool(settings, "use_custom_mouse_image");
    for (const char* name : {"custom_mouse_image_display_mode", "custom_mouse_image_base", "custom_mouse_image_left",
                              "custom_mouse_image_right", "custom_mouse_image_middle"}) {
        obs_property_set_visible(obs_properties_get(props, name), on);
    }
    return true;
}

} // namespace

obs_properties_t* build_render_settings_properties() {
    obs_properties_t* props = obs_properties_create();

    obs_properties_add_color_alpha(props, "color", "Color");

    obs_property_t* shadow = obs_properties_add_bool(props, "shadow", "Shadow");
    obs_properties_add_color_alpha(props, "shadow_color", "Shadow Color");
    obs_property_set_modified_callback(shadow, shadow_modified);

    obs_property_t* background = obs_properties_add_bool(props, "background", "Background");
    obs_property_t* background_mode =
        obs_properties_add_list(props, "background_mode", "Background Mode", OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(background_mode, "Text", static_cast<int>(BackgroundMode::TEXT));
    obs_property_list_add_int(background_mode, "Draw Area", static_cast<int>(BackgroundMode::DRAW_AREA));
    obs_properties_add_color_alpha(props, "background_color", "Background Color");
    obs_properties_add_int(props, "background_rounded_corner_radius", "Background Corner Radius", 0, 100, 1);
    obs_properties_add_int(props, "background_margin", "Background Margin", 0, 200, 1);
    obs_property_set_modified_callback(background, background_modified);

    obs_properties_add_int(props, "font_size", "Font Size", 6, 200, 1);

    obs_properties_add_int(props, "mouse_size_x", "Mouse Size X", 8, 500, 1);
    obs_properties_add_int(props, "mouse_size_y", "Mouse Size Y", 8, 500, 1);

    obs_property_t* use_custom_mouse_image =
        obs_properties_add_bool(props, "use_custom_mouse_image", "Use Custom Mouse Image");
    obs_property_t* custom_mouse_mode =
        obs_properties_add_list(props, "custom_mouse_image_display_mode", "Custom Mouse Image Display Mode",
                                 OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(custom_mouse_mode, "Normal (swap)", static_cast<int>(CustomMouseImageDisplayMode::NORMAL));
    obs_property_list_add_int(custom_mouse_mode, "Overlay (stack)",
                               static_cast<int>(CustomMouseImageDisplayMode::OVERLAY));
    static const char* kImageFilter = "Image Files (*.png *.jpg *.jpeg *.bmp *.gif);;All Files (*.*)";
    obs_properties_add_path(props, "custom_mouse_image_base", "Base Image", OBS_PATH_FILE, kImageFilter, nullptr);
    obs_properties_add_path(props, "custom_mouse_image_left", "Left Mouse Image", OBS_PATH_FILE, kImageFilter, nullptr);
    obs_properties_add_path(props, "custom_mouse_image_right", "Right Mouse Image", OBS_PATH_FILE, kImageFilter,
                             nullptr);
    obs_properties_add_path(props, "custom_mouse_image_middle", "Middle Mouse Image", OBS_PATH_FILE, kImageFilter,
                             nullptr);
    obs_property_set_modified_callback(use_custom_mouse_image, use_custom_mouse_image_modified);

    obs_properties_add_int(props, "shortcut_offset_x", "Shortcut (Modifier Keys) Offset X", -4000, 4000, 1);
    obs_properties_add_int(props, "shortcut_offset_y", "Shortcut (Modifier Keys) Offset Y", -4000, 4000, 1);

    obs_properties_add_int(props, "text_initial_offset_x", "Text Initial Offset X", -4000, 4000, 1);
    obs_properties_add_int(props, "text_initial_offset_y", "Text Initial Offset Y", -4000, 4000, 1);
    obs_properties_add_int(props, "text_spacing_x", "Text Spacing X (older entries cascade by this much)", -500, 500,
                            1);
    obs_properties_add_int(props, "text_spacing_y", "Text Spacing Y (older entries cascade by this much)", -500, 500,
                            1);

    obs_properties_add_float(props, "display_time", "Display Time (seconds)", 0.5, 10.0, 0.1);
    obs_properties_add_int(props, "max_event_history", "Max Event History", 1, 50, 1);
    obs_properties_add_bool(props, "repeat_count", "Repeat Count");

    obs_property_t* show_mouse_events = obs_properties_add_bool(props, "show_mouse_events", "Show Mouse Events");
    obs_property_t* mouse_mode = obs_properties_add_list(props, "mouse_events_show_mode", "Mouse Events Mode",
                                                          OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(mouse_mode, "Event History", static_cast<int>(MouseEventsShowMode::EVENT_HISTORY));
    obs_property_list_add_int(mouse_mode, "Hold Status", static_cast<int>(MouseEventsShowMode::HOLD_STATUS));
    obs_property_list_add_int(mouse_mode, "Event History + Hold Status",
                               static_cast<int>(MouseEventsShowMode::EVENT_HISTORY_AND_HOLD_STATUS));
    obs_property_set_modified_callback(show_mouse_events, show_mouse_events_modified);

    return props;
}

void set_render_settings_defaults(obs_data_t* data) {
    const RenderSettings d;

    obs_data_set_default_int(data, "color", static_cast<long long>(pack_color(d.color)));
    obs_data_set_default_bool(data, "shadow", d.shadow);
    obs_data_set_default_int(data, "shadow_color", static_cast<long long>(pack_color(d.shadow_color)));
    obs_data_set_default_bool(data, "background", d.background);
    obs_data_set_default_int(data, "background_mode", static_cast<int>(d.background_mode));
    obs_data_set_default_int(data, "background_color", static_cast<long long>(pack_color(d.background_color)));
    obs_data_set_default_int(data, "background_rounded_corner_radius", d.background_rounded_corner_radius);
    obs_data_set_default_int(data, "background_margin", d.background_margin);
    obs_data_set_default_int(data, "font_size", d.font_size);
    obs_data_set_default_int(data, "mouse_size_x", d.mouse_size_x);
    obs_data_set_default_int(data, "mouse_size_y", d.mouse_size_y);
    obs_data_set_default_bool(data, "use_custom_mouse_image", d.use_custom_mouse_image);
    obs_data_set_default_int(data, "custom_mouse_image_display_mode",
                              static_cast<int>(d.custom_mouse_image_display_mode));
    obs_data_set_default_int(data, "shortcut_offset_x", d.shortcut_offset_x);
    obs_data_set_default_int(data, "shortcut_offset_y", d.shortcut_offset_y);
    obs_data_set_default_int(data, "text_initial_offset_x", d.text_initial_offset_x);
    obs_data_set_default_int(data, "text_initial_offset_y", d.text_initial_offset_y);
    obs_data_set_default_int(data, "text_spacing_x", d.text_spacing_x);
    obs_data_set_default_int(data, "text_spacing_y", d.text_spacing_y);
    obs_data_set_default_double(data, "display_time", d.display_time);
    obs_data_set_default_int(data, "max_event_history", d.max_event_history);
    obs_data_set_default_bool(data, "repeat_count", d.repeat_count);
    obs_data_set_default_bool(data, "show_mouse_events", d.show_mouse_events);
    obs_data_set_default_int(data, "mouse_events_show_mode", static_cast<int>(d.mouse_events_show_mode));
}

RenderSettings read_render_settings(obs_data_t* data) {
    RenderSettings s;

    s.color = unpack_color(static_cast<uint32_t>(obs_data_get_int(data, "color")));
    s.shadow = obs_data_get_bool(data, "shadow");
    s.shadow_color = unpack_color(static_cast<uint32_t>(obs_data_get_int(data, "shadow_color")));
    s.background = obs_data_get_bool(data, "background");
    s.background_mode = static_cast<BackgroundMode>(obs_data_get_int(data, "background_mode"));
    s.background_color = unpack_color(static_cast<uint32_t>(obs_data_get_int(data, "background_color")));
    s.background_rounded_corner_radius =
        static_cast<int>(obs_data_get_int(data, "background_rounded_corner_radius"));
    s.background_margin = static_cast<int>(obs_data_get_int(data, "background_margin"));
    s.font_size = static_cast<int>(obs_data_get_int(data, "font_size"));
    s.mouse_size_x = static_cast<int>(obs_data_get_int(data, "mouse_size_x"));
    s.mouse_size_y = static_cast<int>(obs_data_get_int(data, "mouse_size_y"));
    s.use_custom_mouse_image = obs_data_get_bool(data, "use_custom_mouse_image");
    s.custom_mouse_image_display_mode =
        static_cast<CustomMouseImageDisplayMode>(obs_data_get_int(data, "custom_mouse_image_display_mode"));
    s.custom_mouse_image_base = obs_data_get_string(data, "custom_mouse_image_base");
    s.custom_mouse_image_left = obs_data_get_string(data, "custom_mouse_image_left");
    s.custom_mouse_image_right = obs_data_get_string(data, "custom_mouse_image_right");
    s.custom_mouse_image_middle = obs_data_get_string(data, "custom_mouse_image_middle");
    s.shortcut_offset_x = static_cast<int>(obs_data_get_int(data, "shortcut_offset_x"));
    s.shortcut_offset_y = static_cast<int>(obs_data_get_int(data, "shortcut_offset_y"));
    s.text_initial_offset_x = static_cast<int>(obs_data_get_int(data, "text_initial_offset_x"));
    s.text_initial_offset_y = static_cast<int>(obs_data_get_int(data, "text_initial_offset_y"));
    s.text_spacing_x = static_cast<int>(obs_data_get_int(data, "text_spacing_x"));
    s.text_spacing_y = static_cast<int>(obs_data_get_int(data, "text_spacing_y"));
    s.display_time = obs_data_get_double(data, "display_time");
    s.max_event_history = static_cast<int>(obs_data_get_int(data, "max_event_history"));
    s.repeat_count = obs_data_get_bool(data, "repeat_count");
    s.show_mouse_events = obs_data_get_bool(data, "show_mouse_events");
    s.mouse_events_show_mode = static_cast<MouseEventsShowMode>(obs_data_get_int(data, "mouse_events_show_mode"));

    return s;
}

} // namespace sk
