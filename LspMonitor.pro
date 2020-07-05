TEMPLATE = app
TARGET = lspmonitor

QT = core gui widgets

CONFIG += c++20

SOURCES += \
    asciiparsing.cpp \
    communicationmodel.cpp \
    connectionstream.cpp \
    framebuilder.cpp \
    lspschemavalidator.cpp \
    main.cpp \
    messagebuilder.cpp \
    stdiomitm.cpp

HEADERS += \
    asciiparsing.h \
    communicationmodel.h \
    connectionstream.h \
    framebuilder.h \
    lspschemavalidator.h \
    messagebuilder.h \
    option.h \
    stdiomitm.h

FORMS +=

RESOURCES += \
    resources.qrc



