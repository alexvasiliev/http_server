#-------------------------------------------------
#
# Project created by QtCreator 2014-09-23T17:20:38
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = testHttpServer
CONFIG   += console
CONFIG   -= app_bundle
QMAKE_CXXFLAGS +=-std=c++11
LIBS += -levent -levent_pthreads -lpthread -lboost_system
TEMPLATE = app


SOURCES += main.cpp \
    server.cpp

HEADERS += \
    server.h
