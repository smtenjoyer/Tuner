#include "MainWindow.h"
#include "./ui_mainwindow.h"
#include <QMessageBox>
#include <QPushButton>
#include <QLabel>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , recordingActive(false)
    , currentTargetString("")
    , currentTargetFrequency(0.0f)
    , manualStringSelection(false)
{
    ui->setupUi(this);

    audioRecorder = new QtAudioRecorder(this);

    // Подключаем сигнал о обнаруженной высоте тона
    connect(audioRecorder, &QtAudioRecorder::pitchDetected,
            this, &MainWindow::updateTunerDisplay, Qt::QueuedConnection);

    connect(audioRecorder, &QtAudioRecorder::errorOccurred,
            this, &MainWindow::handleAudioError);

    // Подключаем кнопки струн
    connect(ui->e2Button, &QPushButton::clicked, this, &MainWindow::onE2ButtonClicked);
    connect(ui->aButton, &QPushButton::clicked, this, &MainWindow::onAButtonClicked);
    connect(ui->dButton, &QPushButton::clicked, this, &MainWindow::onDButtonClicked);
    connect(ui->gButton, &QPushButton::clicked, this, &MainWindow::onGButtonClicked);
    connect(ui->bButton, &QPushButton::clicked, this, &MainWindow::onBButtonClicked);
    connect(ui->e4Button, &QPushButton::clicked, this, &MainWindow::onE4ButtonClicked);

    connect(ui->helpButton, &QPushButton::clicked, this, &MainWindow::showHelpDialog);

    connect(ui->autoButton, &QPushButton::clicked, this, [this]() {
        manualStringSelection = false;
        currentTargetString = "";
        currentTargetFrequency = 0.0f;
        updateTargetIndicator();
        resetStringHighlights();
        ui->statusbar->showMessage("Auto detection enabled", 2000);
    });

    // Начальные настройки UI
    ui->startStopButton->setText("🎤 Старт");
    resetDisplay();

    // Добавляем индикатор целевой ноты в статусбар
    QLabel *targetIndicator = new QLabel("Режим: Автоматический", this);
    targetIndicator->setObjectName("targetIndicator");
    targetIndicator->setStyleSheet("color: #4facfe; font-weight: bold;");
    ui->statusbar->addPermanentWidget(targetIndicator);
}

MainWindow::~MainWindow()
{
    if (recordingActive) {
        audioRecorder->stopRecording();
    }
    delete audioRecorder;
    delete ui;
}


void MainWindow::setTargetString(const QString& stringName, float frequency)
{
    currentTargetString = stringName;
    currentTargetFrequency = frequency;
    manualStringSelection = true;

    // Обновляем UI
    updateTargetIndicator();

    // Сбрасываем выделение всех кнопок
    resetStringHighlights();

    // Выделяем выбранную струну
    if (stringName == "E2") {
        ui->e2Button->setChecked(true);
        ui->e2Button->setStyleSheet(ui->e2Button->styleSheet() +
                                    "QPushButton:checked { background-color: rgba(255, 107, 107, 0.8); }");
    } else if (stringName == "A") {
        ui->aButton->setChecked(true);
        ui->aButton->setStyleSheet(ui->aButton->styleSheet() +
                                   "QPushButton:checked { background-color: rgba(77, 171, 247, 0.8); }");
    } else if (stringName == "D") {
        ui->dButton->setChecked(true);
        ui->dButton->setStyleSheet(ui->dButton->styleSheet() +
                                   "QPushButton:checked { background-color: rgba(106, 255, 182, 0.8); }");
    } else if (stringName == "G") {
        ui->gButton->setChecked(true);
        ui->gButton->setStyleSheet(ui->gButton->styleSheet() +
                                   "QPushButton:checked { background-color: rgba(255, 218, 121, 0.8); }");
    } else if (stringName == "B") {
        ui->bButton->setChecked(true);
        ui->bButton->setStyleSheet(ui->bButton->styleSheet() +
                                   "QPushButton:checked { background-color: rgba(200, 121, 255, 0.8); }");
    } else if (stringName == "E4") {
        ui->e4Button->setChecked(true);
        ui->e4Button->setStyleSheet(ui->e4Button->styleSheet() +
                                    "QPushButton:checked { background-color: rgba(255, 121, 177, 0.8); }");
    }

    // Показываем сообщение
    ui->statusbar->showMessage(QString("Режим: %1 (%2 Гц)").arg(stringName).arg(frequency), 3000);
}

void MainWindow::updateTargetIndicator()
{
    QLabel *indicator = ui->statusbar->findChild<QLabel*>("targetIndicator");
    if (indicator) {
        if (manualStringSelection && !currentTargetString.isEmpty()) {
            indicator->setText(QString("Режим: %1").arg(currentTargetString));
            indicator->setStyleSheet("color: #ff9a9e; font-weight: bold;");
        } else {
            indicator->setText("Режим: Автоматический");
            indicator->setStyleSheet("color: #4facfe; font-weight: bold;");
        }
    }
}

void MainWindow::highlightCorrectString(float pitchHz)
{
    // Если ручной выбор струны включен, не меняем автоматически
    if (manualStringSelection) {
        // Показываем отклонение от выбранной струны
        if (currentTargetFrequency > 0) {
            float cents = 1200.0f * std::log2(pitchHz / currentTargetFrequency);

            // Обновляем прогресс-бар относительно целевой частоты
            int barValue = qBound(-50, qRound(cents), 50);
            ui->tuningBar->setValue(barValue);

            // Показываем целевую частоту в интерфейсе
            ui->noteLabel->setText(QString("%1 ➔ %2 Гц").arg(currentTargetString).arg(currentTargetFrequency, 0, 'f', 1));
        }
        return;
    }

    // Автоматический поиск ближайшей струны
    QString closestString;
    float minDiff = 9999;

    for (auto it = stringFrequencies.begin(); it != stringFrequencies.end(); ++it) {
        float diff = qAbs(pitchHz - it.value());
        if (diff < minDiff) {
            minDiff = diff;
            closestString = it.key();
        }
    }

    // Сброс всех струн
    resetStringHighlights();

    // Подсветка правильной струны, если достаточно близко
    if (minDiff < 10) {
        currentTargetString = closestString;
        currentTargetFrequency = stringFrequencies[closestString];

        if (closestString == "E2") {
            ui->e2Button->setChecked(true);
        } else if (closestString == "A") {
            ui->aButton->setChecked(true);
        } else if (closestString == "D") {
            ui->dButton->setChecked(true);
        } else if (closestString == "G") {
            ui->gButton->setChecked(true);
        } else if (closestString == "B") {
            ui->bButton->setChecked(true);
        } else if (closestString == "E4") {
            ui->e4Button->setChecked(true);
        }

        updateTargetIndicator();
    }
}

void MainWindow::resetStringHighlights()
{
    ui->e2Button->setChecked(false);
    ui->aButton->setChecked(false);
    ui->dButton->setChecked(false);
    ui->gButton->setChecked(false);
    ui->bButton->setChecked(false);
    ui->e4Button->setChecked(false);
}

void MainWindow::resetDisplay()
{
    ui->frequencyLabel->setText("--- Гц");
    ui->frequencyLabel->setStyleSheet("color: #00dbde; background: transparent;");
    ui->noteLabel->setText("---");
    ui->noteLabel->setStyleSheet("color: #ff9a9e; background: transparent;");
    ui->centsLabel->setText("Центы: ---");
    ui->centsLabel->setStyleSheet("color: #a0aec0; background: transparent;");
    ui->tuningBar->setValue(0);

    // Сброс целевой струны
    currentTargetString = "";
    currentTargetFrequency = 0.0f;
    manualStringSelection = false;
    updateTargetIndicator();
    resetStringHighlights();
}


void MainWindow::onE2ButtonClicked()
{
    setTargetString("E2", 82.41f);
}

void MainWindow::onAButtonClicked()
{
    setTargetString("A", 110.00f);
}

void MainWindow::onDButtonClicked()
{
    setTargetString("D", 146.83f);
}

void MainWindow::onGButtonClicked()
{
    setTargetString("G", 196.00f);
}

void MainWindow::onBButtonClicked()
{
    setTargetString("B", 246.94f);
}

void MainWindow::onE4ButtonClicked()
{
    setTargetString("E4", 329.63f);
}


void MainWindow::updateTunerDisplay(float pitchHz)
{
    if (!recordingActive) return;

    float smoothedHz = applyAdaptiveSmoothing(pitchHz);

    if (smoothedHz > 0.0f) {
        // Отображаем частоту
        ui->frequencyLabel->setText(QString("%1 Гц").arg(smoothedHz, 0, 'f', 1));

        if (manualStringSelection && currentTargetFrequency > 0) {
            // Режим ручной настройки на конкретную струну
            float cents = 1200.0f * std::log2(smoothedHz / currentTargetFrequency);

            // Обновляем прогресс-бар
            int barValue = qBound(-50, qRound(cents), 50);
            ui->tuningBar->setValue(barValue);

            // Цвет индикации в зависимости от точности
            QString barColor;
            if (qAbs(cents) < 5) barColor = "#00ff88";
            else if (qAbs(cents) < 10) barColor = "#ffaa00";
            else if (qAbs(cents) < 20) barColor = "#ff5500";
            else barColor = "#ff0000";

            ui->tuningBar->setStyleSheet(
                QString("QProgressBar::chunk { background-color: %1; border-radius: 8px; }").arg(barColor)
                );

            // Отображаем отклонение
            ui->centsLabel->setText(QString("Cents: %1").arg(qRound(cents)));
            ui->centsLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(barColor));

            // Показываем целевую ноту
            ui->noteLabel->setText(QString("%1 ➔ %2 Гц").arg(currentTargetString).arg(currentTargetFrequency, 0, 'f', 1));

        } else {
            // Автоматический режим
            QString noteName;
            float targetFreq;
            float cents = NoteConverter::frequencyToCents(smoothedHz, noteName, targetFreq);

            ui->noteLabel->setText(noteName);
            ui->centsLabel->setText(QString("Центы: %1").arg(qRound(cents)));

            // Обновляем прогресс-бар
            int barValue = qBound(-50, qRound(cents), 50);
            ui->tuningBar->setValue(barValue);

            // Цвет индикации
            QString barColor;
            if (qAbs(cents) < 5) barColor = "#00ff88";
            else if (qAbs(cents) < 10) barColor = "#ffaa00";
            else if (qAbs(cents) < 20) barColor = "#ff5500";
            else barColor = "#ff0000";

            ui->tuningBar->setStyleSheet(
                QString("QProgressBar::chunk { background-color: %1; border-radius: 8px; }").arg(barColor)
                );

            ui->centsLabel->setStyleSheet(QString("color: %1; background: transparent;").arg(barColor));

            // Автоматическое определение струны
            highlightCorrectString(smoothedHz);
        }

    } else {
        // Нет сигнала
        ui->frequencyLabel->setText("--- Гц");
        ui->noteLabel->setText("---");
        ui->centsLabel->setText("Центы: ---");
        ui->tuningBar->setValue(0);
        ui->tuningBar->setStyleSheet("QProgressBar::chunk { background-color: #555555; }");
        ui->centsLabel->setStyleSheet("color: #a0aec0; background: transparent;");

        if (!manualStringSelection) {
            resetStringHighlights();
        }
    }
}

void MainWindow::on_startStopButton_clicked()
{
    if (!recordingActive) {
        audioRecorder->startRecording();
        ui->startStopButton->setText("⏹ Стоп");
        recordingActive = true;
        ui->statusbar->showMessage("Прослушивание...", 2000);
    } else {
        audioRecorder->stopRecording();
        ui->startStopButton->setText("🎤 Старт");
        recordingActive = false;
        resetDisplay();
        ui->statusbar->showMessage("Прослушивание остановлено", 2000);
    }
}

void MainWindow::handleAudioError(const QString& message)
{
    QMessageBox::critical(this, "Ошибка Аудио", message);
    if (recordingActive) {
        audioRecorder->stopRecording();
        ui->startStopButton->setText("🎤 Старт");
        recordingActive = false;
        resetDisplay();
    }
}

float MainWindow::applyAdaptiveSmoothing(float newFrequency)
{
    if (newFrequency <= 0.0f) {
        frequencyHistory.clear();
        return 0.0f;
    }

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    FrequencySample sample{newFrequency, currentTime};
    frequencyHistory.append(sample);

    // Удаляем старые samples (старше 500 мс)
    while (!frequencyHistory.isEmpty() &&
           currentTime - frequencyHistory.first().timestamp > 500) {
        frequencyHistory.removeFirst();
    }

    // Если слишком много samples, ограничиваем
    if (frequencyHistory.size() > MAX_HISTORY_SIZE) {
        frequencyHistory.removeFirst();
    }

    // Если мало данных, не сглаживаем
    if (frequencyHistory.size() < 3) {
        return newFrequency;
    }

    // Взвешенное среднее: более свежие значения имеют больший вес
    float weightedSum = 0.0f;
    float totalWeight = 0.0f;

    for (int i = 0; i < frequencyHistory.size(); ++i) {
        float age = (currentTime - frequencyHistory[i].timestamp) / 1000.0f; // В секундах
        float weight = exp(-age * 2.0f); // Экспоненциальное затухание веса
        weightedSum += frequencyHistory[i].value * weight;
        totalWeight += weight;
    }

    return weightedSum / totalWeight;
}

void MainWindow::showHelpDialog(){
        QDialog *helpDialog = new QDialog(this);
    helpDialog->setWindowTitle("Tuner Help & Accuracy");
    helpDialog->resize(450, 500);

    helpDialog->setModal(false);
    helpDialog->setAttribute(Qt::WA_DeleteOnClose);

    QVBoxLayout *mainLayout = new QVBoxLayout(helpDialog);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    QScrollArea *scrollArea = new QScrollArea(helpDialog);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget *contentWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(contentWidget);
    layout->setSpacing(15);
    layout->setContentsMargins(20, 20, 20, 20);

    // --- Секция "Точность" ---
    QLabel *accuracyTitle = new QLabel("Руководство по точности", contentWidget);
    accuracyTitle->setObjectName("section");
    accuracyTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    layout->addWidget(accuracyTitle);

    QLabel *accuracyText = new QLabel(
        "<table cellspacing='5'>"
        "<tr><td><div style='background-color: #00ff00; width: 20px; height: 20px; border-radius: 3px;'></div></td>"
        "<td><b>Идеально</b>: В пределах ±5 центов</td></tr>"
        "<tr><td><div style='background-color: #ffaa00; width: 20px; height: 20px; border-radius: 3px;'></div></td>"
        "<td><b>Хорошо</b>: В пределах ±10 центов</td></tr>"
        "<tr><td><div style='background-color: #ff5500; width: 20px; height: 20px; border-radius: 3px;'></div></td>"
        "<td><b>Средне</b>: В пределах ±20 центов</td></tr>"
        "<tr><td><div style='background-color: #ff0000; width: 20px; height: 20px; border-radius: 3px;'></div></td>"
        "<td><b>Плохо</b>: Более ±20 центов</td></tr>"
        "</table><br>"
        "<b>Примечание</b>: 100 центов = 1 полутон",
        contentWidget
        );
    accuracyText->setWordWrap(true);
    layout->addWidget(accuracyText);

    // --- Секция "Советы" ---
    QLabel *tipsTitle = new QLabel("Советы по эксплуатации", contentWidget);
    tipsTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    layout->addWidget(tipsTitle);

    QLabel *tipsText = new QLabel(
        "•  Рекомендуется настраивать инструмент в тихом помещении<br>"
        "•  Держать ноту 2-3 секунды для более точных результатов<br>"
        "•  Не задействовать более одной ноты<br>"
        "•  Интерфейс тюнера удобен для настройки гитары, но программу можно использовать для настройки других музыкальных инструментов если сравнивать ожидаемую ноту с выводимой<br>"
        ""
        "•  Стандартный строй шестиструнной гитары:<br>"
        "       E2: 82.41 Гц | A: 110.00 Гц | D: 146.83 Гц<br>"
        "       G: 196.00 Гц | B: 246.94 Гц | E4: 329.63 Гц<br>",
        contentWidget
        );
    tipsText->setWordWrap(true);
    layout->addWidget(tipsText);

    // --- Секция "Ноты" ---
    QLabel *notesTitle = new QLabel("Буквенное обозначение нот и тональностей", contentWidget);
    notesTitle->setStyleSheet("font-weight: bold; font-size: 16px;");
    layout->addWidget(notesTitle);

    QLabel *notesText = new QLabel(
        "•  Буквенным обозначениям, выводимым программой, соответствуют следующие слоговые:<br>"
        "   C - До<br>"
        "   D - Ре<br>"
        "   E - Ми<br>"
        "   F - Фа<br>"
        "   G - Соль<br>"
        "   A - Ля<br>"
        "   B - Си<br>"
        ""
        "•  Цифра после ноты обозначает октаву (Например: E2 - Ми второй октавы)"
        "•  Диез (♯) — это знак, который повышает натуральную ноту на полтона (100 центов) вверх",
        contentWidget
        );
    notesText->setWordWrap(true);
    layout->addWidget(notesText);

    // Кнопка закрытия
    QPushButton *closeButton = new QPushButton("Ясно", contentWidget);
    closeButton->setMinimumHeight(35);
    connect(closeButton, &QPushButton::clicked, helpDialog, &QDialog::close);
    layout->addWidget(closeButton, 0, Qt::AlignCenter);

    layout->addStretch();

    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    helpDialog->show();
}
