#-------------------------------------------------
#
# Project created by QtCreator 2011-09-21T19:19:51
#
#-------------------------------------------------

QT       -= gui

TARGET = ../../parser2chan
TEMPLATE = lib

DEFINES += PARSER2CHAN_LIBRARY _LIB_VERSION="\\\"0.1\\\""

SOURCES += parser2chan.cpp

HEADERS += parser2chan.h\
        parser2chan_global.h

symbian {
    MMP_RULES += EXPORTUNFROZEN
    TARGET.UID3 = 0xE3114540
    TARGET.CAPABILITY = 
    TARGET.EPOCALLOWDLLDATA = 1
    addFiles.sources = parser2chan.dll
    addFiles.path = !:/sys/bin
    DEPLOYMENT += addFiles
}

unix:!symbian {
    maemo5 {
        target.path = /opt/usr/lib
    } else {
        target.path = /usr/lib
    }
    INSTALLS += target
}
