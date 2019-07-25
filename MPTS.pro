QT -= gui

CONFIG += c++11 console
CONFIG -= app_bundle
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += \
        /usr/local/include/tensorflow \
        /usr/local/include/tensorflow/bazel-genfiles \
        /usr/local/include/tensorflow/bazel-tensorflow/external/com_google_absl \
        /usr/include/eigen3 \
        /usr/include/x86_64-linux-gnu/
#    /home/vok/tensorflow \
#    /usr/include/eigen3


HEADERS += \
        dataparser.h    \
        detector.h \
        ff.h \
        tcpclient.h \
        tcpserver.h

SOURCES += \
        dataparser.c    \
        detector.cpp \
        ff.c \
        main.cpp        \
        tcpclient.c \
        tcpserver.c
LIBS += \
        -L/usr/local/lib \
        -lopencv_core -lopencv_videoio -lopencv_highgui -lopencv_imgcodecs \
        -lavformat -lavcodec -lavutil -lswscale  -ldl -lswresample -lm -lz  \
        #-L/usr/local/lib/tensorflow_cc \
        #-ltensorflow_cc
        #-L/home/vok/sysroot/xavier/usr/lib/aarch64-linux-gnu/tegra \
        #-lnvll -lnvrm -lnvrm_graphics -lnvdc -lnvos -lnvimp
# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /home/nvidia/projects/$${TARGET}/bin
#!isEmpty(target.path): INSTALLS += target
