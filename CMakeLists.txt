cmake_minimum_required(VERSION 3.16)
project(qml-niri VERSION 0.2.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wno-unknown-argument)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml)

qt_standard_project_setup(REQUIRES 6.5)

set(CMAKE_INSTALL_RPATH "$ORIGIN")
set(QML_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/qt6/qml"
    CACHE PATH "QML module install directory")

qt_add_library(Niri SHARED)

set_target_properties(Niri PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Niri
)

qt_add_qml_module(Niri
        URI Niri
    VERSION 0.2.1
    SOURCES
        src/icon.h src/icon.cpp
        src/ipcclient.h src/ipcclient.cpp
        src/logging.cpp
        src/niri.h src/niri.cpp
        src/windowmodel.h src/windowmodel.cpp
        src/workspacemodel.h src/workspacemodel.cpp
        src/keyboardlayouts.h src/keyboardlayouts.cpp
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Niri
)

target_include_directories(Niri PUBLIC src)

target_link_libraries(Niri PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
)

target_compile_definitions(Niri PRIVATE QT_USE_QSTRINGBUILDER)

install(TARGETS Niri Niriplugin
    LIBRARY DESTINATION "${QML_INSTALL_DIR}/Niri"
)

install(FILES
    "${CMAKE_BINARY_DIR}/Niri/qmldir"
    "${CMAKE_BINARY_DIR}/Niri/Niri.qmltypes"
    DESTINATION "${QML_INSTALL_DIR}/Niri"
)
