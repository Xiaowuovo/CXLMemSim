# Frontend Qt GUI

cmake_minimum_required(VERSION 3.16)

# Find Qt6
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui)

# Enable Qt MOC, UIC, RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Collect source files
set(FRONTEND_SOURCES
    main.cpp
    mainwindow.cpp
    widgets/topology_editor_widget.cpp
    widgets/config_tree_widget.cpp
    widgets/metrics_panel.cpp
    widgets/realtime_chart_widget.cpp
    widgets/experiment_panel_widget.cpp
    widgets/styled_dialogs.cpp
    widgets/workload_config_widget.cpp
    widgets/sidebar_widget.cpp
    widgets/benchmark_widget.cpp
    widgets/comparison_widget.cpp
    widgets/export_dialog.cpp
)

set(FRONTEND_HEADERS
    mainwindow.h
    widgets/topology_editor_widget.h
    widgets/config_tree_widget.h
    widgets/metrics_panel.h
    widgets/realtime_chart_widget.h
    widgets/experiment_panel_widget.h
    widgets/styled_dialogs.h
    widgets/workload_config_widget.h
    widgets/sidebar_widget.h
    widgets/benchmark_widget.h
    widgets/comparison_widget.h
    widgets/export_dialog.h
)

# Create executable
add_executable(cxlmemsim_gui
    ${FRONTEND_SOURCES}
    ${FRONTEND_HEADERS}
)

# Link libraries
target_link_libraries(cxlmemsim_gui
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Gui
        backend_core
        common
)

# Include directories
target_include_directories(cxlmemsim_gui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/widgets
)

message(STATUS "Qt6 GUI configured")
message(STATUS "  Qt6 version: ${Qt6_VERSION}")
