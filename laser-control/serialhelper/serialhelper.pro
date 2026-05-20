#-------------------------------------------------
#
# Project created by QtCreator 2025-08-14T09:31:48
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = serial
TEMPLATE = app

# 高DPI支持
DEFINES += QT_SCALE_FACTOR_ROUNDING_POLICY=PassThrough
DEFINES += QT_ENABLE_HIGHDPI_SCALING

# Windows高DPI支持
win32 {
    DEFINES += QT_USE_QSTRINGBUILDER
    # 只在MSVC编译器中使用 /permissive- 标志
    msvc {
        QMAKE_CXXFLAGS += /permissive-
    }
    DEFINES += QT_AUTO_SCREEN_SCALE_FACTOR=1
}

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        widget.cpp \
        laserchart.cpp \
        operatorwindow.cpp

HEADERS += \
        widget.h \
        laserchart.h \
        operatorwindow.h

FORMS += \
        widget.ui

RC_ICONS=serialhelp.ico

# Source files are UTF-8. Use the compiler-specific flag so Chinese UI text
# stays correct on both MSVC and MinGW/GCC builds.
msvc {
    QMAKE_CXXFLAGS += /utf-8
} else {
    QMAKE_CXXFLAGS += -finput-charset=UTF-8 -fexec-charset=UTF-8
}


