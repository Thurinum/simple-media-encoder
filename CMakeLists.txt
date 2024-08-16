cmake_minimum_required(VERSION 3.16)

project(MediaEncoder VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wextra -fconcepts-diagnostics-depth=5")
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)
set(BOOST_DI_CFG_DIAGNOSTICS_LEVEL 2)
add_definitions(-DQT_DISABLE_DEPRECATED_UP_TO=0x060700)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

include_directories(${CMAKE_SOURCE_DIR})

set(SOURCES
        core/main.cpp
        core/mainwindow.hpp
        core/mainwindow.cpp
        core/encoder/encoder.hpp
        core/encoder/encoder.cpp
        core/encoder/encoder_options.hpp
        core/encoder/encoder_options_builder.cpp
        core/encoder/encoder_options_builder.hpp
        core/encoder/encoder_strategy.hpp
        core/formats/codec.hpp
        core/formats/container.hpp
        core/formats/ffmpeg_format_support_loader.hpp
        core/formats/ffmpeg_format_support_loader.cpp
        core/formats/format_support.hpp
        core/formats/format_support_loader.hpp
        core/formats/metadata.hpp
        core/formats/metadata_loader.hpp
        core/formats/metadata_loader.cpp
        core/notifier/message.hpp
        core/notifier/message_box_notifier.hpp
        core/notifier/message_box_notifier.cpp
        core/notifier/notifier.hpp
        core/settings/ini_settings.hpp
        core/settings/ini_settings.cpp
        core/settings/serializer.hpp
        core/settings/serializer.cpp
        core/settings/settings.hpp
        core/settings/settings_factory.hpp
        core/settings/settings_factory.cpp
        core/utils/platform_info.hpp
        core/utils/platform_info.cpp
        core/utils/warnings.hpp
        core/utils/warnings.cpp
)

set(RESOURCES
        ui/mainwindow.ui
        bin/appicon.ico
        bin/config_default.ini
)

add_library(boost-di INTERFACE)
target_include_directories(boost-di INTERFACE ${CMAKE_SOURCE_DIR}/thirdparty/boost-di)

qt_add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets boost-di)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
