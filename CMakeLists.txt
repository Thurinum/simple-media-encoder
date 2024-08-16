cmake_minimum_required(VERSION 3.16)

project(MediaEncoder VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wextra -fconcepts-diagnostics-depth=5")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_SOURCE_DIR}/ui)
add_definitions(-DQT_DISABLE_DEPRECATED_UP_TO=0x060700)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

include_directories(${CMAKE_SOURCE_DIR}/include)

set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/encoder/encoder.cpp
    src/encoder/encoder_options_builder.cpp
    src/formats/metadata_loader.cpp
    src/formats/ffmpeg_format_support_loader.cpp
    src/notifier/message_box_notifier.cpp
    src/settings/settings_factory.cpp
    src/settings/ini_settings.cpp
    src/settings/serializer.cpp
    src/utils/platform_info.cpp
    src/utils/warnings.cpp
)

set(HEADERS
    include/encoder/encoder.hpp
    include/encoder/encoder_options.hpp
    include/encoder/encoder_options_builder.hpp
    include/encoder/encoder_strategy.hpp
    include/formats/codec.hpp
    include/formats/container.hpp
    include/formats/ffmpeg_format_support_loader.hpp
    include/formats/format_support.hpp
    include/formats/format_support_loader.hpp
    include/formats/metadata.hpp
    include/formats/metadata_loader.hpp
    include/notifier/message.hpp
    include/notifier/message_box_notifier.hpp
    include/notifier/notifier.hpp
    include/settings/ini_settings.hpp
    include/settings/serializer.hpp
    include/settings/settings.hpp
    include/settings/settings_factory.hpp
    include/utils/platform_info.hpp
    include/utils/warnings.hpp
    include/mainwindow.hpp
)

set(UI
    ui/mainwindow.ui
)

set(DISTFILES
    bin/appicon.ico
    bin/config_default.ini
)

qt_add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS} ${UI} ${DISTFILES})

target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Widgets)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
    WIN32_EXECUTABLE ON
    MACOSX_BUNDLE ON
)
