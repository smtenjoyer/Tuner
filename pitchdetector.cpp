#include "PitchDetector.h"
#include <QDebug>

PitchDetector::PitchDetector(float sampleRate, int bufferSize, int hopSize, QObject *parent)
    : QObject(parent),
    sampleRate(sampleRate),
    bufferSize(bufferSize),
    hopSize(hopSize)
{
    pitch = new_aubio_pitch("schmitt", bufferSize, hopSize, sampleRate);
    if (!pitch) {
        qCritical() << "Failed to create aubio pitch object.";

    }
    inputBuffer = new_fvec(hopSize);
    outputBuffer = new_fvec(1);
}

PitchDetector::~PitchDetector()
{
    del_aubio_pitch(pitch);
    del_fvec(inputBuffer);
    del_fvec(outputBuffer);
}

void PitchDetector::processAudio(const float* audioData)
{
    if (!pitch) {
        emit pitchDetected(0.0f);
        return;
    }

    for (int i = 0; i < hopSize; ++i) {

        inputBuffer->data[i] = audioData[i];
    }

    aubio_pitch_do(pitch, inputBuffer, outputBuffer);

    emit pitchDetected(outputBuffer->data[0]);
}
