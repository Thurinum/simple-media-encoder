QT = core gui widgets

CONFIG += c++20
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    compressor.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    compressor.hpp \
    mainwindow.hpp

FORMS += \
    mainwindow.ui \
    mainwindow.ui.old

DISTFILES += \
	config.ini
