cmake_minimum_required(VERSION 3.20)
project(CXLMemSim VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")

# Detect PEBS support
include(CheckCXXSourceCompiles)

set(CMAKE_REQUIRED_INCLUDES "/usr/include")
set(PEBS_TEST_CODE "
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
int main() {
    struct perf_event_attr pe;
    pe.precise_ip = 2;
    syscall(__NR_perf_event_open, &pe, -1, 0, -1, 0);
    return 0;
}
")

check_cxx_source_compiles("${PEBS_TEST_CODE}" HAVE_PEBS_HEADERS)

if(HAVE_PEBS_HEADERS)
    message(STATUS "✓ PEBS headers found - will attempt runtime detection")
    add_compile_definitions(ENABLE_PEBS_TRACER)
else()
    message(WARNING "✗ PEBS headers not found - Mock tracer only")
    message(WARNING "  This is normal in VM environments")
endif()

# Find nlohmann_json
find_package(nlohmann_json 3.0 QUIET)
if(NOT nlohmann_json_FOUND)
    message(WARNING "nlohmann_json not found via CMake, will use header-only mode")
    # Assume it's installed in system include path
endif()

# Qt (optional, will be required when frontend is ready)
option(BUILD_GUI "Build Qt GUI frontend" ON)

if(BUILD_GUI)
    find_package(Qt6 QUIET COMPONENTS Core Widgets Gui)
    if(Qt6_FOUND)
        message(STATUS "✓ Qt6 found - GUI will be built")
        set(CMAKE_AUTOMOC ON)
        set(CMAKE_AUTORCC ON)
        set(CMAKE_AUTOUIC ON)
    else()
        message(WARNING "✗ Qt6 not found - GUI will not be built")
        message(WARNING "  Install with: sudo apt install qt6-base-dev qt6-tools-dev")
        set(BUILD_GUI OFF)
    endif()
endif()

# Add subdirectories
add_subdirectory(backend)
add_subdirectory(common)

if(BUILD_GUI AND Qt6_FOUND)
    add_subdirectory(frontend)
endif()

# Main CLI executable (for testing backend)
add_executable(cxlmemsim main.cpp)
target_link_libraries(cxlmemsim PRIVATE backend_core common)

# Demo executable
add_executable(simple_simulation demos/simple_simulation.cpp)
target_link_libraries(simple_simulation PRIVATE backend_core common)

# Testing
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Summary
message(STATUS "=================================")
message(STATUS "CXLMemSim Configuration Summary")
message(STATUS "=================================")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "PEBS support: ${HAVE_PEBS_HEADERS}")
message(STATUS "Build GUI: ${BUILD_GUI}")
message(STATUS "Build tests: ${BUILD_TESTS}")
message(STATUS "=================================")
