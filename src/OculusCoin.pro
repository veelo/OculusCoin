QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OculusCoin
TEMPLATE = app

CONFIG += console

SOURCES      += main.cpp

HEADERS      +=

# Support for the Coin3D Open Inventor implementation.
LIBS         += -L$(COINDIR)/lib
INCLUDEPATH  += $(COINDIR)/include
DEFINES      += COIN_DLL QUARTER_DLL
CONFIG(debug, debug|release) {
    LIBS     += -lcoin4d -lquarter1d
} else {
    LIBS     += -lcoin4 -lquarter1
}

# Support for Oculus Rift from SDK.
##OVRDIR = C:/SARC/Oculus/ovr_sdk_win_0.3.1_preview/OculusSDK/LibOVR
##INCLUDEPATH  += $$OVRDIR/Include
##INCLUDEPATH  += $$OVRDIR/../3rdParty/glext
##win32: LIBS  += -L$$OVRDIR/Lib/Win32/VS2012
##CONFIG(debug, debug|release) {
##    LIBS     += -llibovrd
##} else {
##    LIBS     += -llibovr
##}
##win32: LIBS  += Winmm.lib Shell32.lib

# Support for Oculus Rift from Git.
OVRDIR = C:/SARC/Oculus/git/OculusSDK/LibOVR
INCLUDEPATH  += $$OVRDIR/Include
INCLUDEPATH  += $$OVRDIR/../3rdParty/glext
LIBS     += OculusVR.lib
CONFIG(debug, debug|release) {
    LIBS  += -L$$OVRDIR/Debug
} else {
    LIBS  += -L$$OVRDIR/Release
}
win32: LIBS  += Winmm.lib Shell32.lib Setupapi.lib
