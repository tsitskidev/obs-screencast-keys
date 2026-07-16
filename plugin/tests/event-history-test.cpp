#include "doctest.h"

#include "../src/state/event-history.h"

using sk::EventHistory;
using sk::EventType;

TEST_CASE("first press is appended as a new entry") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 3.0);

    REQUIRE(history.entries().size() == 1);
    CHECK(history.entries()[0].event_type == EventType::A);
    CHECK(history.entries()[0].repeat_count == 1);
}

TEST_CASE("duplicate press within the debounce window is ignored") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 3.0);
    history.record_press(EventType::A, {}, 0.03, true, 3.0); // < 0.05s later

    REQUIRE(history.entries().size() == 1);
    CHECK(history.entries()[0].repeat_count == 1);
    CHECK(history.entries()[0].timestamp == 0.0); // Not refreshed either.
}

TEST_CASE("repeated press outside debounce but within display_time bumps repeat_count") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 3.0);
    history.record_press(EventType::A, {}, 0.5, true, 3.0);
    history.record_press(EventType::A, {}, 1.0, true, 3.0);

    REQUIRE(history.entries().size() == 1);
    CHECK(history.entries()[0].repeat_count == 3);
    CHECK(history.entries()[0].timestamp == 1.0); // Refreshed to latest press.
}

TEST_CASE("repeat_count disabled always appends a new entry instead of collapsing") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, false, 3.0);
    history.record_press(EventType::A, {}, 0.5, false, 3.0);

    REQUIRE(history.entries().size() == 2);
    CHECK(history.entries()[0].repeat_count == 1);
    CHECK(history.entries()[1].repeat_count == 1);
}

TEST_CASE("repeated press after display_time has elapsed starts a fresh entry") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 1.0);
    history.record_press(EventType::A, {}, 2.0, true, 1.0); // 2s > display_time.

    REQUIRE(history.entries().size() == 2);
    CHECK(history.entries()[0].repeat_count == 1);
    CHECK(history.entries()[1].repeat_count == 1);
}

TEST_CASE("different modifiers on the same key are treated as a different entry") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 3.0);
    history.record_press(EventType::A, {EventType::MOD_CTRL}, 0.5, true, 3.0);

    REQUIRE(history.entries().size() == 2);
    CHECK(history.entries()[1].modifiers == std::vector<EventType>{EventType::MOD_CTRL});
}

TEST_CASE("prune drops entries older than display_time") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 1.0);
    history.record_press(EventType::B, {}, 5.0, true, 1.0);

    history.prune(5.5, 1.0, 100);

    REQUIRE(history.entries().size() == 1);
    CHECK(history.entries()[0].event_type == EventType::B);
}

TEST_CASE("prune caps to max_entries keeping the most recent") {
    EventHistory history;
    for (int i = 0; i < 5; ++i) {
        history.record_press(EventType::A, {}, static_cast<double>(i) * 10.0, false, 100.0);
    }

    history.prune(40.0, 100.0, 2);

    REQUIRE(history.entries().size() == 2);
    CHECK(history.entries()[0].timestamp == 30.0);
    CHECK(history.entries()[1].timestamp == 40.0);
}

TEST_CASE("clear empties the history") {
    EventHistory history;
    history.record_press(EventType::A, {}, 0.0, true, 3.0);
    history.clear();

    CHECK(history.entries().empty());
}
