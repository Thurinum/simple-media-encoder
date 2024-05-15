QT = core gui widgets

CONFIG += c++20
QMAKE_CXXFLAGS += -Werror -Wextra -fconcepts-diagnostics-depth=5
DEFINES += QT_DISABLE_DEPRECATED_UP_TO=0x060700

INCLUDEPATH += $$PWD/include

SOURCES += \
    $$files("$$PWD/src/*.cpp", true)

HEADERS += \
    $$files("$$PWD/include/*.hpp", true)

FORMS += \
    $$files("$$PWD/ui/*.ui", true)

DISTFILES += \
    $$files("$$PWD/bin/*", true)

DESTDIR = $$PWD/bin
