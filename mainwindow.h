#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QMap>
#include "qtaudiorecorder.h"
#include "NoteConverter.h"
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startStopButton_clicked();
    void updateTunerDisplay(float pitchHz);
    void handleAudioError(const QString& message);

    // Слоты для кнопок струн
    void onE2ButtonClicked();
    void onAButtonClicked();
    void onDButtonClicked();
    void onGButtonClicked();
    void onBButtonClicked();
    void onE4ButtonClicked();
    void showHelpDialog();

private:
    Ui::MainWindow *ui;
    QtAudioRecorder *audioRecorder;
    bool recordingActive;

    QString currentTargetString;
    float currentTargetFrequency;
    bool manualStringSelection;

    // Методы для работы с целевыми струнами
    void setTargetString(const QString& stringName, float frequency);
    void highlightCorrectString(float pitchHz);
    void resetStringHighlights();
    void resetDisplay();
    void updateTargetIndicator();

    // Частоты стандартных гитарных струн
    const QMap<QString, float> stringFrequencies = {
        {"E2", 82.41f},
        {"A", 110.00f},
        {"D", 146.83f},
        {"G", 196.00f},
        {"B", 246.94f},
        {"E4", 329.63f}
    };

    struct FrequencySample {
        float value;
        qint64 timestamp; // Время получения
    };
    QVector<FrequencySample> frequencyHistory;
    static const int MAX_HISTORY_SIZE = 15;

    float applyAdaptiveSmoothing(float newFrequency);
};

#endif // MAINWINDOW_H
