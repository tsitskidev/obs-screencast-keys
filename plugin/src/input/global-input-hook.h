#pragma once

#include <functional>

#include "raw-input-event.h"

// Thin wrapper around libuiohook's process-wide global hook.
//
// libuiohook only supports a single dispatch_proc per process (it's a plain
// C function pointer, not per-object state), so this is a namespace of free
// functions backed by static state in the .cpp, not a class you instantiate
// per source -- InputHookManager (input-hook-manager.h) is the layer that
// fans a single hook out to multiple source instances.
namespace sk::global_input_hook {

using Callback = std::function<void(const RawInputEvent&)>;

// Starts libuiohook on a dedicated thread and begins delivering translated
// events to `callback`. The callback is invoked from libuiohook's hook
// thread -- keep it fast (push to a queue, don't block), since on Windows a
// slow low-level keyboard/mouse hook callback can cause the OS to silently
// uninstall the hook.
//
// Returns false if a hook is already installed or libuiohook failed to
// start (see the OS-specific UIOHOOK_ERROR_* codes logged via hook_set_logger_proc).
bool install(Callback callback);

// Stops the hook and joins its thread. Safe to call even if install() was
// never called or already failed; a no-op if not currently installed.
void uninstall();

bool is_installed();

} // namespace sk::global_input_hook
