#include "event-history.h"

#include <algorithm>

namespace sk {

void EventHistory::record_press(EventType event_type, std::vector<EventType> modifiers, double now,
                                 bool repeat_count_enabled, double display_time_seconds) {
    if (!entries_.empty()) {
        HistoryEntry& last = entries_.back();
        const bool is_same = last.event_type == event_type && last.modifiers == modifiers;
        const double delta = now - last.timestamp;

        if (is_same && delta < kIgnoreIntervalSeconds) {
            // Hardware/OS duplicate (e.g. hidden double-fire) -- ignore.
            return;
        }
        if (repeat_count_enabled && is_same && delta < display_time_seconds) {
            last.timestamp = now;
            last.repeat_count += 1;
            return;
        }
    }

    entries_.push_back(HistoryEntry{now, event_type, std::move(modifiers), 1});
}

void EventHistory::prune(double now, double display_time_seconds, int max_entries) {
    std::vector<HistoryEntry> kept;
    kept.reserve(entries_.size());
    for (auto& entry : entries_) {
        if (now - entry.timestamp <= display_time_seconds) {
            kept.push_back(std::move(entry));
        }
    }

    if (max_entries >= 0 && static_cast<int>(kept.size()) > max_entries) {
        kept.erase(kept.begin(), kept.end() - max_entries);
    }

    entries_ = std::move(kept);
}

} // namespace sk
