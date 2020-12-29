#-------------------------------------------------
#
# Project created by QtCreator 2020-05-14T13:48:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets  multimedia serialport

TARGET = RSPduoEME
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# For development testing with FFTW
# INCLUDEPATH += D:\Qt\RSPapi\x86  D:\Qt\FFTW-3-3-5-32bit
# DEPENDPATH += D:\Qt\RSPapi\x86

# Where the library files are located in the development directory
# LIBS += -L D:\Qt\RSPapi\x86 -l_sdrplay_api -l fftw3-3

# Where the library files are located in the project directory
LIBS += -L "$$shell_path($$_PRO_FILE_PWD_)" -l_sdrplay_api


SOURCES += \
        main.cpp \
        mainwindow.cpp \
        rspduointerface.cpp \
        processthread.cpp \
        dspthread.cpp

HEADERS += \
        mainwindow.h \
        rspduointerface.h \
        processthread.h \
        sdrplay_api.h \
        filters.h \
        dspthread.h

FORMS += \
        mainwindow.ui
