#include <obs-module.h>

#include "screencast-keys-source.h"

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("screencast-keys", "en-US")

MODULE_EXPORT const char* obs_module_description(void) {
    return "Displays pressed keys and mouse buttons, ported from the Blender Screencast Keys addon";
}

bool obs_module_load(void) {
    screencast_keys_source_register();
    blog(LOG_INFO, "[screencast-keys] plugin loaded");
    return true;
}

void obs_module_unload(void) {
    blog(LOG_INFO, "[screencast-keys] plugin unloaded");
}
