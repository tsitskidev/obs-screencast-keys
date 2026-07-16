#pragma once

#include <mutex>
#include <unordered_map>
#include <vector>

#include "raw-input-event.h"

namespace sk {

// Process-wide, refcounted fan-out on top of global_input_hook: there is
// only one keyboard/mouse on the machine, but the user may place multiple
// styled copies of the Screencast Keys source in different scenes, each
// needing its own independent history/style. The global hook installs when
// the first source instance registers and uninstalls when the last one
// unregisters; every registered instance gets a copy of every event via its
// own mailbox, so instances never steal events from each other.
class InputHookManager {
public:
    static InputHookManager& instance();

    InputHookManager(const InputHookManager&) = delete;
    InputHookManager& operator=(const InputHookManager&) = delete;

    // Call from the source's create(). Installs the global hook if this is
    // the first registered instance. Returns a mailbox id to pass to
    // drain()/unregister_instance().
    int register_instance();

    // Call from the source's destroy(). Uninstalls the global hook if this
    // was the last registered instance.
    void unregister_instance(int mailbox_id);

    // Moves out (and clears) all events queued for `mailbox_id` since the
    // last call. Call once per video_tick.
    std::vector<RawInputEvent> drain(int mailbox_id);

private:
    InputHookManager() = default;

    void on_raw_event(const RawInputEvent& event);

    std::mutex mutex_;
    std::unordered_map<int, std::vector<RawInputEvent>> mailboxes_;
    int next_id_ = 1;
};

} // namespace sk
