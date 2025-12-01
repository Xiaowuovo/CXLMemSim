# Common library (shared between frontend and backend)

# For now, this is header-only
add_library(common INTERFACE)

target_include_directories(common
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# nlohmann_json
if(nlohmann_json_FOUND)
    target_link_libraries(common INTERFACE nlohmann_json::nlohmann_json)
else()
    # Header-only mode - assumes installed in system path
    target_compile_definitions(common INTERFACE JSON_HEADER_ONLY)
endif()
