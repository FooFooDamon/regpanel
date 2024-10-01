MAKEFILE = QtMakefile

TEMPLATE = app
TARGET = regpanel
CONFIG += c++11 qt warn_on release
exists($${TARGET}.debug.pri) {
    include($${TARGET}.debug.pri) # Should contain: CONFIG += debug
}

FORMS += *.ui
HEADERS += $${TARGET}.hpp private_widgets.hpp
SOURCES += *.cpp
QT += widgets

