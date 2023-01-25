QT = core gui widgets network

CONFIG += c++20
QMAKE_CXXFLAGS += -Werror -Wextra
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000


INCLUDEPATH += $$PWD/include

SOURCES += \
	$$files("$$PWD/src/*.cpp", true)

HEADERS += \
	$$files("$$PWD/include/*.hpp", true)

FORMS += \
	$$files("$$PWD/ui/*.ui", true)

DISTFILES += \
	$$files("$$PWD/bin/*	", true)

DESTDIR = $$PWD/bin
