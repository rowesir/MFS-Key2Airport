QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    dialogenum.cpp \
    main.cpp \
    widget.cpp

HEADERS += \
    dialogenum.h \
    widget.h

FORMS += \
    dialogenum.ui \
    widget.ui

win32 {
    SIMCONNECT_SDK_DIR = $$PWD/SimConnect SDK

    INCLUDEPATH += $$quote($$SIMCONNECT_SDK_DIR/include)

    LIBS += -L$$quote($$SIMCONNECT_SDK_DIR/lib) -lSimConnect
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Icons.qrc
