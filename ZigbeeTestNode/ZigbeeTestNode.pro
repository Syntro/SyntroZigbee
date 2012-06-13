TEMPLATE = app
TARGET = ZigbeeTestNode

win32* {
	DESTDIR = Release
}
else {
	DESTDIR = Output
}

QT += core gui

CONFIG += debug

unix {
	CONFIG += link_pkgconfig
	macx:CONFIG -= app_bundle
}

INCLUDEPATH += GeneratedFiles \
    GeneratedFiles/Release \


MOC_DIR += GeneratedFiles/Release

OBJECTS_DIR += Release

UI_DIR += GeneratedFiles

RCC_DIR += GeneratedFiles

include(ZigbeeTestNode.pri)

include(../Common/ZigbeeCommon.pri)

include(../3rdparty/qextserialport/src/qextserialport.pri)
