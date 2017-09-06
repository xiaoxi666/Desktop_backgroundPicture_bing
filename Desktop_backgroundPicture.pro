TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.cpp \
    tinyxml2.cpp \
    jpeg2bmp.c

QMAKE_CXXFLAGS += -std=c++11

HEADERS += \
    tinyxml2.h \
    jpeg.h

LIBS += D:\Desktop\Go\C++learnProgram\build-Desktop_backgroundPicture-Qt-Release\release\URLMON.DLL

#LIBS += D:\Desktop\Go\C++learnProgram\build-Desktop_backgroundPicture-Qt-Release\release\User32.dll
