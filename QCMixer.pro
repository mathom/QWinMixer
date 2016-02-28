#-------------------------------------------------
#
# Project created by QtCreator 2016-02-27T00:29:37
#
#-------------------------------------------------

QT       += core gui winextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QWinMixer
TEMPLATE = app


SOURCES += main.cpp\
        mixerwindow.cpp \
    slider.cpp

HEADERS  += mixerwindow.h \
    slider.h \
    macros.h

FORMS    += mixerwindow.ui \
    slider.ui

DISTFILES +=

win32: LIBS += -lole32 -luser32 -lshlwapi

RESOURCES += \
    icons.qrc
