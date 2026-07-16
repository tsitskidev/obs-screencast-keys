# Standalone test binary for the pure-logic core (state/event-history,
# state/input-state, input/event-types). Deliberately has zero dependency on
# libobs or libuiohook -- it builds and runs on its own, independent of the
# full plugin build, so the highest port-fidelity-risk logic (the fading /
# repeat-collapsing history state machine) gets verified even if the OBS SDK
# build has issues.

cmake_minimum_required(VERSION 3.28)
project(screencast-keys-tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
  doctest
  GIT_REPOSITORY https://github.com/doctest/doctest.git
  GIT_TAG v2.4.11
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(doctest)

add_executable(screencast-keys-tests
  test-main.cpp
  event-history-test.cpp
  input-state-test.cpp
  event-types-test.cpp
  ${CMAKE_SOURCE_DIR}/../src/input/event-types.cpp
  ${CMAKE_SOURCE_DIR}/../src/state/event-history.cpp
  ${CMAKE_SOURCE_DIR}/../src/state/input-state.cpp
)

target_include_directories(screencast-keys-tests PRIVATE ${doctest_SOURCE_DIR}/doctest)

enable_testing()
add_test(NAME screencast-keys-tests COMMAND screencast-keys-tests)
