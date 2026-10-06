cmake_minimum_required(VERSION 3.16)

project(s-shot
    VERSION 0.9.0
    DESCRIPTION "Lightweight Screenshot & Annotation Tool for Linux"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Svg Network Test)
find_package(X11 REQUIRED)

set(SOURCES
    src/main.cpp
    src/core/SettingsManager.cpp
    src/core/HotkeyManager.cpp
    src/tray/TrayManager.cpp
    src/capture/CaptureManager.cpp
    src/capture/RegionSnippingOverlay.cpp
    src/capture/ScrollingCaptureDialog.cpp
    src/capture/ColorPickerOverlay.cpp
    src/editor/MainWindow.cpp
    src/editor/CanvasView.cpp
    src/editor/CanvasScene.cpp
    src/editor/items/BaseAnnotationItem.cpp
    src/editor/items/PenItem.cpp
    src/editor/items/ArrowItem.cpp
    src/editor/items/ShapeItem.cpp
    src/editor/items/BadgeItem.cpp
    src/editor/items/TextItem.cpp
    src/editor/items/BlurItem.cpp
    src/dialogs/SettingsDialog.cpp
    src/dialogs/AboutDialog.cpp
    resources/resources.qrc
)

add_executable(s-shot ${SOURCES})

target_include_directories(s-shot PRIVATE
    src
    src/core
    src/tray
    src/capture
    src/editor
    src/editor/items
    src/dialogs
    ${X11_INCLUDE_DIR}
)

target_link_libraries(s-shot PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Svg Qt6::Network
    ${X11_LIBRARIES}
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_options(s-shot PRIVATE -O3 -ffunction-sections -fdata-sections)
    target_link_options(s-shot PRIVATE -Wl,--gc-sections -Wl,--as-needed)
endif()

enable_testing()
add_executable(test_editor_tools
    tests/test_editor_tools.cpp
    src/editor/CanvasScene.cpp
    src/editor/items/BaseAnnotationItem.cpp
    src/editor/items/PenItem.cpp
    src/editor/items/ArrowItem.cpp
    src/editor/items/ShapeItem.cpp
    src/editor/items/BadgeItem.cpp
    src/editor/items/TextItem.cpp
    src/editor/items/BlurItem.cpp
)
target_include_directories(test_editor_tools PRIVATE
    src
    src/editor
    src/editor/items
)
target_link_libraries(test_editor_tools PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Test
)
add_test(NAME test_editor_tools COMMAND test_editor_tools)

# Installation rules
include(GNUInstallDirs)

install(TARGETS s-shot
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES resources/s-shot.desktop
    DESTINATION ${CMAKE_INSTALL_DATADIR}/applications
)

install(FILES resources/icons/s-shot.svg
    DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
)

# CPack configuration
set(CPACK_PACKAGE_NAME "s-shot")
set(CPACK_PACKAGE_VENDOR "S-Shot")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Lightweight Screenshot & Annotation Tool for Linux")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/s-shot/s-shot")
set(CPACK_PACKAGE_CONTACT "s-shot-dev@example.com")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")

# Debian packaging
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "S-Shot Developer <support@example.com>")
set(CPACK_DEBIAN_PACKAGE_SECTION "graphics")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

# RPM packaging
set(CPACK_RPM_PACKAGE_LICENSE "GPLv3+")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Graphics")
set(CPACK_RPM_PACKAGE_AUTOREQPROV ON)

include(CPack)
