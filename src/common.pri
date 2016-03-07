rootdir=$$PWD/..

message($$rootdir)

CONFIG(debug, debug|release) {
    cfg=debug
    DEFINES += _DEBUG
} else {
    cfg=release
}

builddir=$$rootdir/tmp/$$cfg/$$TARGET

INCLUDEPATH += $$rootdir/src

RCC_DIR=$$builddir
OBJECTS_DIR=$$builddir
MOC_DIR=$$builddir
DESTDIR=$$rootdir/bin/$$cfg
UI_DIR = $$builddir/ui

DEFINES+=UNICODE _CRT_SECURE_NO_WARNINGS
CONFIG += c++11

*-g++*{
    QMAKE_CXXFLAGS += -Wextra
    QMAKE_CXXFLAGS += -Werror
}

equals(cfg,debug) {
    DEFINES+=_DEBUG
}

