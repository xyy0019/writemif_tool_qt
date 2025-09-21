#-------------------------------------------------
#
# Project created by QtCreator 2024-12-14T00:52:40
#
#-------------------------------------------------

QT       += core gui
QT += multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = writemif
TEMPLATE = app

SOURCES += main.cpp\
        mainwindow.cpp \
    writemif.cpp \
    hdmi_parse.cpp \
    pktinfo.cpp \
    bmp.cpp \
    div.cpp

HEADERS  += mainwindow.h \
    parse_hdmi.h \
    pktinfo.h \
    bmp.h \
    div.h

FORMS    +=

RESOURCES += \
    res.qrc
