#ifndef AUDIOINPUTTHREAD_H
#define AUDIOINPUTTHREAD_H

#include <QThread>
#include <portaudio.h> // Включаем PortAudio
#include "PitchDetector.h" // Включаем наш PitchDetector

// Параметры аудио
const int SAMPLE_RATE = 44100;
const int FRAMES_PER_BUFFER = 512; // Также будет hop_size для aubio

class AudioInputThread : public QThread
{
    Q_OBJECT
public:
    explicit AudioInputThread(QObject *parent = nullptr);
    ~AudioInputThread();

    void run() override; // Метод, выполняемый в потоке

    void startRecording();
    void stopRecording();

signals:
    void pitchDetected(float pitchHz);
    void errorOccurred(const QString& message);

private:
    PaStream *stream;
    PitchDetector *pitchDetector;
    volatile bool running; // Флаг для контроля цикла записи

    // Статическая функция-коллбэк PortAudio
    static int paCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo* timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData);
};

#endif // AUDIOINPUTTHREAD_H
