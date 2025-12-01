# Tests

# Find GTest (optional)
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(WARNING "Google Test not found - tests will not be built")
    message(WARNING "Install with: sudo apt install libgtest-dev")
    return()
endif()

# Test executable
add_executable(test_tracer
    test_tracer.cpp
)

target_link_libraries(test_tracer
    PRIVATE
        backend_core
        common
        GTest::gtest
        GTest::gtest_main
)

# Register tests
include(GoogleTest)
gtest_discover_tests(test_tracer)

message(STATUS "Tests configured")
