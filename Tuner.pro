QT       += core gui multimedia multimediawidgets # ДОБАВЛЕНО multimedia и multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

SOURCES += \
    qtaudiorecorder.cpp \
    main.cpp \
    mainwindow.cpp \
    noteconverter.cpp \
    pitchdetector.cpp

HEADERS += \
    qtaudiorecorder.h \
    mainwindow.h \
    noteconverter.h \
    pitchdetector.h

FORMS += \
    mainwindow.ui

MSYS2_PATH = C:/msys64/mingw64

win32: {
    INCLUDEPATH += $$MSYS2_PATH/include

    LIBS += -L$$MSYS2_PATH/lib \
            -laubio \
            -lfftw3 \
            -lsamplerate \
            -lmpg123 \
            -lsndfile

}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    icon.qrc
