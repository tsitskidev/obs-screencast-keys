#include "global-input-hook.h"

#include <mutex>
#include <thread>

#include <uiohook.h>

namespace sk::global_input_hook {

namespace {

std::mutex g_state_mutex;
Callback g_callback;
std::thread g_hook_thread;
bool g_installed = false;

double now_seconds() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

// This is the actual libuiohook dispatch_proc: a plain C function pointer,
// called on libuiohook's own hook thread. Must do the absolute minimum --
// translate the event into our own plain-data type and hand it off. On
// Windows, a slow low-level keyboard/mouse hook callback risks the OS
// silently uninstalling the hook (see the class-level docs in the header).
void dispatch_proc(uiohook_event* const event) {
    RawInputEvent raw;
    raw.timestamp = now_seconds();

    switch (event->type) {
        case EVENT_KEY_PRESSED:
            raw.kind = RawInputKind::KeyPressed;
            raw.code = event->data.keyboard.keycode;
            break;
        case EVENT_KEY_RELEASED:
            raw.kind = RawInputKind::KeyReleased;
            raw.code = event->data.keyboard.keycode;
            break;
        case EVENT_MOUSE_PRESSED:
            raw.kind = RawInputKind::MousePressed;
            raw.code = event->data.mouse.button;
            break;
        case EVENT_MOUSE_RELEASED:
            raw.kind = RawInputKind::MouseReleased;
            raw.code = event->data.mouse.button;
            break;
        case EVENT_MOUSE_WHEEL:
            raw.kind = RawInputKind::MouseWheel;
            raw.wheel_rotation = event->data.wheel.rotation;
            break;
        default:
            return; // EVENT_KEY_TYPED, EVENT_MOUSE_MOVED/DRAGGED/CLICKED, hook enable/disable -- not needed.
    }

    // Copy the callback under lock, then invoke it unlocked: `callback`
    // itself (InputHookManager::on_raw_event) takes its own lock over the
    // per-instance mailboxes, and we don't want to hold two locks or risk
    // g_callback being torn down mid-call from uninstall() on another thread.
    Callback callback_copy;
    {
        std::lock_guard<std::mutex> lock(g_state_mutex);
        callback_copy = g_callback;
    }
    if (callback_copy) {
        callback_copy(raw);
    }
}

} // namespace

bool install(Callback callback) {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    if (g_installed) {
        return false;
    }

    g_callback = std::move(callback);
    hook_set_dispatch_proc(&dispatch_proc);

    // hook_run() blocks pumping events until hook_stop() is called from
    // another thread, so it needs its own dedicated thread. libuiohook has
    // no asynchronous "hook is ready" notification -- a failed install (e.g.
    // missing Accessibility permission on macOS) just means hook_run()
    // returns quickly on its own and no events ever arrive; callers should
    // treat "no events after use" as the diagnostic signal on those
    // platforms rather than a return value from here.
    g_hook_thread = std::thread([]() { hook_run(); });

    g_installed = true;
    return true;
}

void uninstall() {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    if (!g_installed) {
        return;
    }

    hook_stop();
    if (g_hook_thread.joinable()) {
        g_hook_thread.join();
    }
    g_callback = nullptr;
    g_installed = false;
}

bool is_installed() {
    std::lock_guard<std::mutex> lock(g_state_mutex);
    return g_installed;
}

} // namespace sk::global_input_hook
