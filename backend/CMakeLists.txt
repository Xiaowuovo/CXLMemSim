# Backend library

# Collect source files
set(BACKEND_SOURCES
    src/tracer/tracer_factory.cpp
    src/tracer/mock_tracer.cpp
    src/topology/topology_graph.cpp
    src/analyzer/latency_model.cpp
    src/analyzer/timing_analyzer.cpp
)

# Add PEBS tracer only if supported
if(HAVE_PEBS_HEADERS)
    list(APPEND BACKEND_SOURCES src/tracer/pebs_tracer.cpp)
endif()

# Create backend library
add_library(backend_core STATIC ${BACKEND_SOURCES})

target_include_directories(backend_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link libraries
target_link_libraries(backend_core
    PUBLIC
        common
)

# Platform-specific libraries
target_link_libraries(backend_core
    PRIVATE
        pthread
)

message(STATUS "Backend library configured with ${BACKEND_SOURCES}")
