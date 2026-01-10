
#include "qtaudiorecorder.h"
#include <QDebug>
#include <QMessageBox>

static int getSampleSizeInBytes(QAudioFormat::SampleFormat format) {
    switch (format) {
    case QAudioFormat::UInt8:
        return 1;
    case QAudioFormat::Int16:
        return 2;
    case QAudioFormat::Int32:
    case QAudioFormat::Float:
        return 4;
    default:
        qWarning() << "Unsupported QAudioFormat::SampleFormat:" << format;
        return 0;
    }
}

QtAudioRecorder::QtAudioRecorder(QObject *parent)
    : QObject(parent),
    audioSource(nullptr),
    audioInputDevice(nullptr),
    pitchDetector(nullptr),
    processingThread(nullptr),
    running(false)
{
    QAudioFormat format;
    format.setSampleRate(QT_SAMPLE_RATE);
    format.setChannelCount(QT_CHANNEL_COUNT);
    format.setSampleFormat(QAudioFormat::Float);


    QAudioDevice info = QMediaDevices::defaultAudioInput();

    if (info.isNull() || info.description().isEmpty()) {
        qCritical() << "No default audio input device found or it is invalid.";
        emit errorOccurred("No default audio input device found. Please check your microphone.");
        return;
    }

    if (!info.isFormatSupported(format)) {
        qCritical() << "Default input device does not support requested format (Float 44.1kHz mono).";
        emit errorOccurred("Microphone does not support required audio format (Float 44.1kHz mono).");
        return;
    }

    audioSource = new QAudioSource(info, format, this);

    pitchDetector = new PitchDetector(format.sampleRate(), QT_BUFFER_SIZE_FRAMES * 4, QT_BUFFER_SIZE_FRAMES);
    processingThread = new QThread(this);
    pitchDetector->moveToThread(processingThread);

    connect(pitchDetector, &PitchDetector::pitchDetected, this, &QtAudioRecorder::handlePitchDetection, Qt::QueuedConnection);
    connect(processingThread, &QThread::started, this, &QtAudioRecorder::handleProcessingThreadStarted);
    connect(processingThread, &QThread::finished, pitchDetector, &QObject::deleteLater);
}

QtAudioRecorder::~QtAudioRecorder()
{
    stopRecording();
    cleanupPitchDetector();

    if (processingThread) {
        delete processingThread;
    }

    if (audioSource) {
        delete audioSource;
    }
}

void QtAudioRecorder::startRecording()
{
    if (running) return;

    cleanupPitchDetector();

    pitchDetector = new PitchDetector(audioSource->format().sampleRate(),
                                      QT_BUFFER_SIZE_FRAMES * 4,
                                      QT_BUFFER_SIZE_FRAMES);
    pitchDetector->moveToThread(processingThread);

    connect(pitchDetector, &PitchDetector::pitchDetected,
            this, &QtAudioRecorder::handlePitchDetection,
            Qt::QueuedConnection);

    processingThread->start();


    audioDataBuffer.clear();
    audioInputDevice = audioSource->start();

    if (audioInputDevice) {
        connect(audioInputDevice, &QIODevice::readyRead,
                this, &QtAudioRecorder::readMoreAudioData);
        running = true;
        qDebug() << "Audio recording started.";
    } else {
        emit errorOccurred("Failed to start audio input.");
        cleanupPitchDetector();
    }
}

void QtAudioRecorder::stopRecording()
{
    if (!running) return;

    running = false;

    if (audioInputDevice) {
        disconnect(audioInputDevice, &QIODevice::readyRead,
                   this, &QtAudioRecorder::readMoreAudioData);
        audioInputDevice = nullptr;
    }

    audioSource->stop();

    // Очищаем буфер
    audioDataBuffer.clear();

    // Останавливаем и очищаем pitchDetector
    cleanupPitchDetector();

    qDebug() << "Audio recording stopped.";
}

void QtAudioRecorder::cleanupPitchDetector()
{
    // Отключаем все соединения
    if (pitchDetector) {
        disconnect(pitchDetector, nullptr, this, nullptr);
    }

    // Останавливаем поток
    if (processingThread && processingThread->isRunning()) {
        processingThread->quit();
        processingThread->wait(500); // Ждем 500ms

        // Если поток не завершился, принудительно
        if (processingThread->isRunning()) {
            processingThread->terminate();
            processingThread->wait();
        }
    }

    // Удаляем pitchDetector
    if (pitchDetector) {
        delete pitchDetector;
        pitchDetector = nullptr;
    }
}


void QtAudioRecorder::readMoreAudioData()
{
    if (!running || !audioInputDevice) return;

    QByteArray newAudioData = audioInputDevice->readAll();
    audioDataBuffer.append(newAudioData);

    int sampleSize = getSampleSizeInBytes(audioSource->format().sampleFormat());
    if (sampleSize == 0) {
        qCritical() << "Unsupported sample format detected during read. Stopping audio.";
        emit errorOccurred("Unsupported audio sample format. Recording stopped.");
        stopRecording();
        return;
    }
    int frameSize = sampleSize * audioSource->format().channelCount();

    while (audioDataBuffer.size() >= QT_BUFFER_SIZE_FRAMES * frameSize) {
        QByteArray chunk = audioDataBuffer.mid(0, QT_BUFFER_SIZE_FRAMES * frameSize);
        audioDataBuffer.remove(0, QT_BUFFER_SIZE_FRAMES * frameSize);

        const float* audioFloats = reinterpret_cast<const float*>(chunk.constData());

        QMetaObject::invokeMethod(pitchDetector, "processAudio", Qt::QueuedConnection,
                                  Q_ARG(const float*, audioFloats));
    }
}

void QtAudioRecorder::handlePitchDetection(float pitchHz)
{
    emit pitchDetected(pitchHz);
}

void QtAudioRecorder::handleProcessingThreadStarted()
{
    qDebug() << "PitchDetector processing thread started.";
}
