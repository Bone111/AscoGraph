SOURCES += $$files(../../server/*.cpp) $$files(../../server/*.c) $$files(*.cpp) $$files(../../server/json/*.cpp)
HEADERS += $$files(../../server/*.h) $$files(*.h) $$files(../../server/json/*.h)

DEFINES += "JSON_ONLY"

TEMPLATE = app
win32 {
	TEMPLATE = vcapp
}
QT += widgets printsupport
CONFIG += console
macx:CONFIG -= app_bundle
#DESTDIR = ../bin

# GuidoQt library link for each platform
win32:LIBS += ../GuidoQt.lib
unix:LIBS += -L.. -lGuidoQt -L/usr/local/lib
unix:LIBS += -lmicrohttpd -lcrypto -lcurl
INCLUDEPATH += ../GuidoQt/include
INCLUDEPATH += ../../server
INCLUDEPATH += /usr/local/include

include( ../GUIDOEngineLink.pri )
