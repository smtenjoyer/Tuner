#include "NoteConverter.h"

const QString NoteConverter::noteNames[] = {
    "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
};

QString NoteConverter::frequencyToNoteName(float freqHz)
{
    if (freqHz <= 0.0f) return "---";

    // A4 = 440 Hz, MIDI note number 69
    // N = 12 * log2(F / 440 Hz) + 69
    float midiNoteNumF = 12.0f * std::log2(freqHz / 440.0f) + 69.0f;
    int midiNoteNum = static_cast<int>(std::round(midiNoteNumF));

    // Octave = (N / 12) - 1
    int octave = (midiNoteNum / 12) - 1;
    // Note index = N % 12
    int noteIndex = midiNoteNum % 12;

    return noteNames[noteIndex] + QString::number(octave);
}

float NoteConverter::frequencyToCents(float freqHz, QString& noteName, float& targetFreq)
{
    if (freqHz <= 0.0f) {
        noteName = "---";
        targetFreq = 0.0f;
        return 0.0f;
    }

    // A4 = 440 Hz, MIDI note number 69
    // N = 12 * log2(F / 440 Hz) + 69
    float midiNoteNumF = 12.0f * std::log2(freqHz / 440.0f) + 69.0f;
    int midiNoteNum = static_cast<int>(std::round(midiNoteNumF));

    // Target frequency for the closest MIDI note
    targetFreq = 440.0f * std::pow(2.0f, (midiNoteNum - 69) / 12.0f);

    // Calculate cents deviation: 1200 * log2(Factual / Ftarget)
    float cents = 1200.0f * std::log2(freqHz / targetFreq);

    // Set note name
    int octave = (midiNoteNum / 12) - 1;
    int noteIndex = midiNoteNum % 12;
    noteName = noteNames[noteIndex] + QString::number(octave);

    return cents;
}
