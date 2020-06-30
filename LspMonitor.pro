TEMPLATE = app
TARGET = lspmonitor

QT = core gui widgets

CONFIG += c++20

SOURCES += \
    connectionstream.cpp \
    framebuilder.cpp \
    lspmessagebuilder.cpp \
    lspschemavalidator.cpp \
    main.cpp \
    msglogmodel.cpp \
    stdiomitm.cpp

HEADERS += \
    connectionstream.h \
    framebuilder.h \
    lspmessagebuilder.h \
    lspschemavalidator.h \
    msglogmodel.h \
    stdiomitm.h

FORMS +=



