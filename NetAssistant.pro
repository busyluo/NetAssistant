#-------------------------------------------------
#
# Project created by QtCreator 2014-04-10T19:21:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = NetAssistant
target.path=/usr/local/bin
INSTALLS=target

TEMPLATE = app


SOURCES += main.cpp\
    mainwindow.cpp \
    TcpServer.cpp

HEADERS  += \
    define.h \
    mainwindow.h \
    TcpServer.h

FORMS    += \
    mainwindow.ui

RESOURCES += \
    qrc.qrc

QT  +=network

RC_FILE += icon.rc

DISTFILES += \
    android/AndroidManifest.xml

TRANSLATIONS += language/English.ts \
                language/Chinese.ts

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
