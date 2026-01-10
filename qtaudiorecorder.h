#ifndef QTAUDIORECORDER_H
#define QTAUDIORECORDER_H

#include <QObject>
#include <QAudioFormat>
#include <QIODevice>
#include <QThread>
#include <QByteArray>

#include <QAudioSource>
#include <QMediaDevices>

#include "pitchdetector.h"

const int QT_SAMPLE_RATE = 48000;
const int QT_CHANNEL_COUNT = 1;

const int QT_BUFFER_SIZE_FRAMES = 512;

class QtAudioRecorder : public QObject
{
    Q_OBJECT
public:
    explicit QtAudioRecorder(QObject *parent = nullptr);
    ~QtAudioRecorder();

public slots:
    void startRecording();
    void stopRecording();

signals:
    void pitchDetected(float pitchHz);
    void errorOccurred(const QString& message);

private slots:
    void readMoreAudioData(); // Слот для чтения данных из QAudioSource
    void handlePitchDetection(float pitchHz); // Слот для получения питча от PitchDetector
    void handleProcessingThreadStarted(); // Слот, вызываемый при старте потока обработки

private:
    QAudioSource *audioSource;
    QIODevice *audioInputDevice;

    PitchDetector *pitchDetector;
    QThread *processingThread;
    bool running;

    QByteArray audioDataBuffer;
    void cleanupPitchDetector();

};

#endif // QTAUDIORECORDER_H
