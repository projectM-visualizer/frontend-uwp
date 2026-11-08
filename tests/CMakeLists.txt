# Unit tests for projectM-UWP.Core. This is a separate desktop project, because the
# library has no UWP dependencies and UWP test binaries can't run in CI.
#
#   cmake -S tests -B build/tests
#   cmake --build build/tests --config Release
#   ctest --test-dir build/tests -C Release --output-on-failure
cmake_minimum_required(VERSION 3.21)
project(projectM-UWP-CoreTests LANGUAGES CXX)

set(CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../src/projectM-UWP/projectM-UWP.Core")

add_executable(core_tests
    core_tests.cpp
    "${CORE_DIR}/AudioRingBuffer.cpp"
    "${CORE_DIR}/FrameStats.cpp"
    "${CORE_DIR}/Settings.cpp"
)
target_include_directories(core_tests PRIVATE "${CORE_DIR}")
target_compile_features(core_tests PRIVATE cxx_std_14)

enable_testing()
add_test(NAME core_tests COMMAND core_tests)
