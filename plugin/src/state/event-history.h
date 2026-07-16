#pragma once

#include <vector>

#include "../input/event-types.h"

namespace sk {

// One line of the fading event-history display, e.g. "Ctrl + Shift + A x3".
// Direct port of the Blender addon's [time, event_type, modifiers,
// repeat_count] event_history tuples (ops.py).
struct HistoryEntry {
    double timestamp = 0.0; // seconds, monotonic clock (not wall-clock)
    EventType event_type = EventType::UNKNOWN;
    std::vector<EventType> modifiers; // canonical, in InputState::held_modifiers() order
    int repeat_count = 1;
};

inline bool operator==(const HistoryEntry& a, const HistoryEntry& b) {
    return a.timestamp == b.timestamp && a.event_type == b.event_type &&
           a.modifiers == b.modifiers && a.repeat_count == b.repeat_count;
}

// Pure logic, no OBS/libuiohook dependency: a fading, repeat-collapsing list
// of recent key/mouse presses. Port of the relevant slice of ops.py's
// modal() (the event_history bookkeeping) and removed_old_event_history().
class EventHistory {
public:
    // Debounce window: a duplicate (type, modifiers) press arriving within
    // this many seconds of the last one is ignored outright rather than
    // counted, suppressing hardware/OS double-fire artifacts. Mirrors the
    // Blender addon's INTERVAL_FOR_IGNORE_EVENT.
    static constexpr double kIgnoreIntervalSeconds = 0.05;

    // Records a key/mouse press at time `now` (seconds). `modifiers` must
    // already be the canonical, order-stable list of currently-held
    // modifiers (from InputState::held_modifiers()), excluding event_type
    // itself if event_type is itself a modifier.
    //
    // Behavior (mirrors modal()'s event_history update exactly):
    //   - If this is the same (event_type, modifiers) as the most recent
    //     entry and less than kIgnoreIntervalSeconds has passed: ignored.
    //   - Else if repeat_count_enabled and it's the same entry and less than
    //     display_time_seconds has passed: bump repeat_count and refresh the
    //     entry's timestamp (keeps it from expiring).
    //   - Else: appended as a new entry.
    void record_press(EventType event_type, std::vector<EventType> modifiers, double now,
                       bool repeat_count_enabled, double display_time_seconds);

    // Drops entries older than display_time_seconds, then caps the remainder
    // to at most max_entries (keeping the most recent). Call once per tick,
    // before reading entries() for layout/rendering. Port of
    // removed_old_event_history().
    void prune(double now, double display_time_seconds, int max_entries);

    const std::vector<HistoryEntry>& entries() const { return entries_; }
    void clear() { entries_.clear(); }

private:
    std::vector<HistoryEntry> entries_;
};

} // namespace sk
