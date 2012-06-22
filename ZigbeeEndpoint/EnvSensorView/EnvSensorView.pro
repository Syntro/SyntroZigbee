TEMPLATE = app
TARGET = EnvSensorView

win32* {
	DESTDIR = Release
}
else {
	DESTDIR = Output
}

QT += core gui network

CONFIG += release

unix {
	CONFIG += link_pkgconfig
	macx:CONFIG -= app_bundle
	PKGCONFIG += syntro
}

DEFINES += QT_NETWORK_LIB

INCLUDEPATH += GeneratedFiles \
    GeneratedFiles/Release \
    ../../Common

win32-g++:LIBS += -L"$(SYNTRODIR)/bin"

win32-msvc*:LIBS += -L"$(SYNTRODIR)/lib"

win32 {
	DEFINES += _CRT_SECURE_NO_WARNINGS
	INCLUDEPATH += $(SYNTRODIR)/include
	LIBS += -lSyntroLib -lSyntroGUI
}

MOC_DIR += GeneratedFiles/Release

OBJECTS_DIR += Release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(EnvSensorView.pri)
