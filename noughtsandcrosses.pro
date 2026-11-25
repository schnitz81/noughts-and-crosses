QT += core widgets gui

CONFIG += c++17 console
CONFIG -= app_bundle

# Target Linux desktop exclusively

SOURCES += \
    main.cpp \
    board.cpp \
    gamewidget.cpp \
    mainwindow.cpp \
    computerworker.cpp \
    aboutdialog.cpp \
    statisticsdialog.cpp

HEADERS += \
    board.h \
    gamewidget.h \
    mainwindow.h \
    computerworker.h \
    aboutdialog.h \
    statisticsdialog.h

# Install target to /usr/local/bin on Linux desktop
target.path = /usr/local/bin
INSTALLS += target
