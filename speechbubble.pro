######################################################################
# Automatically generated by qmake (2.01a) la lokakuuta 26 12:59:07 2013
######################################################################

TEMPLATE     = app
TARGET       = speechbubble
DEPENDPATH  += . src
INCLUDEPATH += . src
CONFIG      += debug
QT          += network

# Input
HEADERS     += ./src/*.h
SOURCES     += ./src/*.cc
FORMS       += ./ui/*.ui

# Build directory
OBJECTS_DIR += ./build/
UI_DIR      += ./build/
MOC_DIR     += ./build/

QMAKE_CXXFLAGS += -std=c++0x