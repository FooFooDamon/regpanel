MAKEFILE = QtMakefile

CONFIG += c++11 qt warn_on release
exists(regpanel.debug.pri) {
    include(regpanel.debug.pri) # Should contain: CONFIG += debug
}
TEMPLATE = app
TARGET = regpanel

FORMS += *.ui
HEADERS += regpanel.hpp
SOURCES += *.cpp
QT += widgets

