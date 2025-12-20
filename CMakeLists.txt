cmake_minimum_required(VERSION 3.16)
project(qml-niri VERSION 0.1.2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_library(Niri SHARED)

set_target_properties(Niri PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Niri
)

qt_add_qml_module(Niri
        URI Niri
    VERSION 0.1.2
    SOURCES
        src/icon.h src/icon.cpp
        src/ipcclient.h src/ipcclient.cpp
        src/logging.cpp
        src/niri.h src/niri.cpp
        src/windowmodel.h src/windowmodel.cpp
        src/workspacemodel.h src/workspacemodel.cpp
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Niri
)

target_include_directories(Niri PUBLIC src)

target_link_libraries(Niri PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
)
