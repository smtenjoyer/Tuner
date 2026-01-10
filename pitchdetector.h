#ifndef PITCHDETECTOR_H
#define PITCHDETECTOR_H

#include <QObject>
#include <aubio/aubio.h>

class PitchDetector : public QObject
{
    Q_OBJECT
public:
    explicit PitchDetector(float sampleRate, int bufferSize, int hopSize, QObject *parent = nullptr);
    ~PitchDetector();

public slots:
    void processAudio(const float* audioData);

signals:
    void pitchDetected(float pitchHz);

private:
    aubio_pitch_t* pitch;
    fvec_t* inputBuffer;
    fvec_t* outputBuffer;
    float sampleRate;
    int bufferSize;
    int hopSize;

};

#endif // PITCHDETECTOR_H
