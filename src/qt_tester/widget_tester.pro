QT       += core gui widgets

TARGET = widget_tester

!include(../common.pri) : error("Failed to load common settings")

TEMPLATE = app

SOURCES += main.cpp\
    loggerwidget.cpp \
    loggingmodel.cpp

HEADERS  += \
    $$rootdir/include/logger.h \
    loggerwidget.h \
    loggingmodel.h

