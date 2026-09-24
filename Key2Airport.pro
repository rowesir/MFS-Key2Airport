QT       += core gui multimedia
RC_ICONS = icon.ico

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    configexecutor.cpp \
    configmanager.cpp \
    dialogenum.cpp \
    dialogtest.cpp \
    directinputlistener.cpp \
    inputlistener.cpp \
    main.cpp \
    simconnectclient.cpp \
    widget.cpp

HEADERS += \
    configexecutor.h \
    configmanager.h \
    dialogenum.h \
    dialogtest.h \
    directinputlistener.h \
    inputlistener.h \
    simconnectclient.h \
    widget.h

FORMS += \
    dialogenum.ui \
    dialogtest.ui \
    widget.ui

win32 {
    SIMCONNECT_SDK_DIR = $$PWD/SimConnect SDK

    INCLUDEPATH += $$quote($$SIMCONNECT_SDK_DIR/include)

    LIBS += -L$$quote($$SIMCONNECT_SDK_DIR/lib) -lSimConnect

    LIBS += -ldinput8 -ldxguid

    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_quote($$shell_path($$SIMCONNECT_SDK_DIR/lib/SimConnect.dll)) $(DESTDIR)

    # Deploy the Qt runtime and required plugins next to the executable.
    # windeployqt must come from the same Qt kit used for this build.
    QT_DEPLOY_TOOL = $$[QT_INSTALL_BINS]/windeployqt.exe
    CONFIG(release, debug|release) {
        QT_DEPLOY_MODE = --release
    } else {
        QT_DEPLOY_MODE = --debug
    }

    exists($$QT_DEPLOY_TOOL) {
        QMAKE_POST_LINK += && $$shell_quote($$shell_path($$QT_DEPLOY_TOOL)) $$QT_DEPLOY_MODE --compiler-runtime --no-translations $(DESTDIR_TARGET)
    } else {
        warning("windeployqt.exe was not found; Qt runtime deployment was skipped")
    }
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    Icons.qrc
