#include "AudioInputThread.h"
#include <QDebug>
#include <portaudio.h> // Обязательно еще раз здесь для реализации

AudioInputThread::AudioInputThread(QObject *parent)
    : QThread(parent), stream(nullptr), running(false)
{
    pitchDetector = new PitchDetector(SAMPLE_RATE, FRAMES_PER_BUFFER * 4, FRAMES_PER_BUFFER, this);
    // bufferSize для aubio (FRAMES_PER_BUFFER * 4) может быть больше hop_size для лучшего анализа
    // Попробуйте разные значения, например, 1024, 2048 для bufferSize, если FRAMES_PER_BUFFER=512
}

AudioInputThread::~AudioInputThread()
{
    stopRecording();
    wait(); // Ждем завершения потока
    delete pitchDetector;
}

void AudioInputThread::startRecording()
{
    if (running) return;

    running = true;
    start(); // Запускаем QThread
}

void AudioInputThread::stopRecording()
{
    if (!running) return;

    running = false; // Сигнал для остановки цикла в run()
    // Если поток запущен, он будет остановлен в run()
    // Pa_StopStream и Pa_CloseStream будут вызваны в run()
}

// Статическая функция-коллбэк PortAudio
int AudioInputThread::paCallback(const void *inputBuffer, void *outputBuffer,
                                 unsigned long framesPerBuffer,
                                 const PaStreamCallbackTimeInfo* timeInfo,
                                 PaStreamCallbackFlags statusFlags,
                                 void *userData)
{
    // Преобразуем userData обратно в указатель на AudioInputThread
    AudioInputThread *This = static_cast<AudioInputThread*>(userData);

    const float *in = (const float*)inputBuffer;
    float pitchHz = 0.0f;

    if( inputBuffer == nullptr ) {
        // Нет входных данных (например, тишина), aubio может вернуть 0
        pitchHz = 0.0f;
    } else {
        // Передаем данные в PitchDetector для обработки
        pitchHz = This->pitchDetector->processAudio(in);
    }

    emit This->pitchDetected(pitchHz);

    return paContinue; // Продолжаем запись
}

void AudioInputThread::run()
{
    PaError err;

    // Инициализация PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        emit errorOccurred(QString("PortAudio error: %1").arg(Pa_GetErrorText(err)));
        running = false;
    }

    if (running) {
        // Открытие аудиопотока
        err = Pa_OpenDefaultStream(
            &stream,
            1,                  // 1 входной канал
            0,                  // 0 выходных каналов
            paFloat32,          // 32-битный float формат
            SAMPLE_RATE,
            FRAMES_PER_BUFFER,  // Размер буфера PortAudio (hop_size для aubio)
            paCallback,         // Указатель на функцию-коллбэк
            this                // Передаем "this" в userData для доступа к члену класса
            );
        if (err != paNoError) {
            emit errorOccurred(QString("PortAudio error: %1").arg(Pa_GetErrorText(err)));
            running = false;
        }
    }

    if (running) {
        // Запуск потока PortAudio
        err = Pa_StartStream(stream);
        if (err != paNoError) {
            emit errorOccurred(QString("PortAudio error: %1").arg(Pa_GetErrorText(err)));
            running = false;
        }
    }

    // Главный цикл потока - ждем, пока running не станет false
    while (running) {
        QThread::msleep(50); // Небольшая задержка, чтобы не загружать CPU
    }

    // Остановка и закрытие потока PortAudio
    if (stream) {
        err = Pa_StopStream(stream);
        if (err != paNoError) {
            qWarning() << "PortAudio stop stream error:" << Pa_GetErrorText(err);
        }
        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            qWarning() << "PortAudio close stream error:" << Pa_GetErrorText(err);
        }
    }

    // Деинициализация PortAudio
    err = Pa_Terminate();
    if (err != paNoError) {
        qWarning() << "PortAudio terminate error:" << Pa_GetErrorText(err);
    }
}
