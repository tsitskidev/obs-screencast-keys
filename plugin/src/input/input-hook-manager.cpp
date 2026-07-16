#include "input-hook-manager.h"

#include "global-input-hook.h"

namespace sk {

InputHookManager& InputHookManager::instance() {
    static InputHookManager singleton;
    return singleton;
}

int InputHookManager::register_instance() {
    std::lock_guard<std::mutex> lock(mutex_);

    const int id = next_id_++;
    mailboxes_.emplace(id, std::vector<RawInputEvent>{});

    if (mailboxes_.size() == 1) {
        global_input_hook::install([this](const RawInputEvent& event) { on_raw_event(event); });
    }

    return id;
}

void InputHookManager::unregister_instance(int mailbox_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    mailboxes_.erase(mailbox_id);

    if (mailboxes_.empty()) {
        global_input_hook::uninstall();
    }
}

std::vector<RawInputEvent> InputHookManager::drain(int mailbox_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = mailboxes_.find(mailbox_id);
    if (it == mailboxes_.end()) {
        return {};
    }

    std::vector<RawInputEvent> drained = std::move(it->second);
    it->second.clear();
    return drained;
}

void InputHookManager::on_raw_event(const RawInputEvent& event) {
    // Called from libuiohook's hook thread (via global_input_hook's
    // callback) -- keep this fast, it's fanning out to every registered
    // instance under one short-lived lock.
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto& [id, mailbox] : mailboxes_) {
        mailbox.push_back(event);
    }
}

} // namespace sk
