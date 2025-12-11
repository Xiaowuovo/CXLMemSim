# Common library (shared between frontend and backend)

set(COMMON_SOURCES
    src/config_parser.cpp
)

add_library(common STATIC ${COMMON_SOURCES})

target_include_directories(common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# nlohmann_json
if(nlohmann_json_FOUND)
    target_link_libraries(common PUBLIC nlohmann_json::nlohmann_json)
else()
    # Header-only mode - assumes installed in system path
    target_compile_definitions(common PUBLIC JSON_HEADER_ONLY)
endif()
