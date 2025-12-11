# Tests

# Find GTest (optional)
find_package(GTest QUIET)

if(NOT GTest_FOUND)
    message(WARNING "Google Test not found - tests will not be built")
    message(WARNING "Install with: sudo apt install libgtest-dev")
    return()
endif()

# Test executables
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

add_executable(test_config_parser
    test_config_parser.cpp
)

target_link_libraries(test_config_parser
    PRIVATE
        common
        GTest::gtest
        GTest::gtest_main
)

add_executable(test_integration
    test_integration.cpp
)

target_link_libraries(test_integration
    PRIVATE
        backend_core
        common
        GTest::gtest
        GTest::gtest_main
)

# Register tests
include(GoogleTest)
gtest_discover_tests(test_tracer)
gtest_discover_tests(test_config_parser)
gtest_discover_tests(test_integration)

message(STATUS "Tests configured")
