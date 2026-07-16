#pragma once

#include <obs-module.h>

// Registers the "Screencast Keys" obs_source_info with libobs. Call once
// from obs_module_load().
void screencast_keys_source_register();
