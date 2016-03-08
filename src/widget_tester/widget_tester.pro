QT       += core gui widgets

TARGET = widget_tester

!include(../common.pri) : error("Failed to load common settings")

TEMPLATE = app

SOURCES += main.cpp\
    ../logging/logger.cpp \
    ../logging/loggerwidget.cpp \
    ../logging/loggingmodel.cpp

HEADERS  += \
    ../logging/logger.h \
    ../logging/loggerwidget.h \
    ../logging/loggingmodel.h

FORMS    +=
