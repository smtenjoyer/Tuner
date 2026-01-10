#ifndef NOTECONVERTER_H
#define NOTECONVERTER_H

#include <QString>
#include <cmath>

class NoteConverter
{
public:

    static QString frequencyToNoteName(float freqHz);


    static float frequencyToCents(float freqHz, QString& noteName, float& targetFreq);

private:
    NoteConverter() = delete;
    static const QString noteNames[];
};

#endif // NOTECONVERTER_H
