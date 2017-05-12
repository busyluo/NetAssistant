#-------------------------------------------------
#
# Project created by QtCreator 2014-04-10T19:21:07
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = mNetAssist
target.path=/usr/local/bin
INSTALLS=target

TEMPLATE = app


SOURCES += main.cpp\
    mTcpServer.cpp \
    mNetAssistWidget.cpp

HEADERS  += \
    mdefine.h \
    mTcpServer.h \
    mNetAssistWidget.h

FORMS    += \
    mNetAssistWidget.ui

RESOURCES += \
    mqrc.qrc

QT  +=network

RC_FILE += icon.rc

DISTFILES += \
    android/AndroidManifest.xml

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android
