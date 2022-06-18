QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Sanitizers
#CONFIG += sanitizer
#CONFIG += sanitize_address
#CONFIG += sanitize_memory
#CONFIG += sanitize_thread
#CONFIG += sanitize_undefined

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# use this due to "undefined reference to __atomic_store" and "undefined reference to __atomic_load"
# only for windows qt
LIBS += -latomic

# AVX2 AND FMA SUPPORT
DEFINES += AVX=1
QMAKE_CXXFLAGS += -mavx -mfma

# Another performance flags
QMAKE_CXXFLAGS_RELEASE -= -O0
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

INCLUDEPATH += \
    include \
    include/widgets \
    include/windows

SOURCES += \
    src/main.cpp \
    src/renderer.cpp \
    src/windows/mainwindow.cpp \
    src/windows/parametersdialog.cpp \
    src/widgets/statusbar.cpp \
    src/widgets/viewport.cpp

HEADERS += \
    include/mandelbrot.h \
    include/renderer.h \
    include/windows/mainwindow.h \
    include/windows/parametersdialog.h \
    include/widgets/statusbar.h \
    include/widgets/viewport.h

FORMS += \
    forms/mainwindow.ui \
    forms/parametersdialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
