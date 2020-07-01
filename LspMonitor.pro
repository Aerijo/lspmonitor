TEMPLATE = app
TARGET = lspmonitor

QT = core gui widgets

CONFIG += c++20

SOURCES += \
    asciiparsing.cpp \
    connectionstream.cpp \
    framebuilder.cpp \
    lspmessagebuilder.cpp \
    lspschemavalidator.cpp \
    main.cpp \
    messagebuilder.cpp \
    msglogmodel.cpp \
    stdiomitm.cpp

HEADERS += \
    asciiparsing.h \
    connectionstream.h \
    framebuilder.h \
    lspmessagebuilder.h \
    lspschemavalidator.h \
    messagebuilder.h \
    msglogmodel.h \
    option.h \
    stdiomitm.h

FORMS +=



