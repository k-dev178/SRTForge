#include "MainWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QtGlobal>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();
    wireEvents();
    refreshModelStatus();
}

void MainWindow::buildUi() {
    setWindowTitle("SRTForge");
    resize(980, 760);
    setMinimumSize(860, 640);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(18, 18, 18, 18);
    root->setSpacing(12);

    auto *title = new QLabel("SRTForge", central);
    QFont titleFont = title->font();
    titleFont.setPointSize(24);
    titleFont.setBold(true);
    title->setFont(titleFont);
    root->addWidget(title);

    auto *subtitle = new QLabel("음성/영상 파일을 로컬 whisper.cpp로 전사해서 SRT 자막 파일을 만듭니다.", central);
    root->addWidget(subtitle);

    auto *fileGroup = new QGroupBox("파일", central);
    auto *fileForm = new QFormLayout(fileGroup);
    fileForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    fileForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *inputRow = new QWidget(fileGroup);
    auto *inputLayout = new QHBoxLayout(inputRow);
    inputLayout->setContentsMargins(0, 0, 0, 0);
    inputEdit_ = new QLineEdit(inputRow);
    inputEdit_->setPlaceholderText("음성/영상 파일 또는 기존 .srt/.txt 파일");
    auto *inputButton = new QPushButton("선택", inputRow);
    inputLayout->addWidget(inputEdit_, 1);
    inputLayout->addWidget(inputButton);
    fileForm->addRow("입력 파일", inputRow);

    auto *outputRow = new QWidget(fileGroup);
    auto *outputLayout = new QHBoxLayout(outputRow);
    outputLayout->setContentsMargins(0, 0, 0, 0);
    outputEdit_ = new QLineEdit(outputRow);
    outputEdit_->setPlaceholderText("저장할 .srt 파일 위치");
    auto *outputButton = new QPushButton("저장", outputRow);
    outputLayout->addWidget(outputEdit_, 1);
    outputLayout->addWidget(outputButton);
    fileForm->addRow("저장 위치", outputRow);
    root->addWidget(fileGroup);

    auto *settingsRow = new QHBoxLayout();
    settingsRow->setSpacing(12);

    auto *transcriptionGroup = new QGroupBox("전사 설정", central);
    auto *transcriptionForm = new QFormLayout(transcriptionGroup);
    transcriptionForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    transcriptionForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    auto *modelRow = new QWidget(transcriptionGroup);
    auto *modelLayout = new QHBoxLayout(modelRow);
    modelLayout->setContentsMargins(0, 0, 0, 0);
    modelCombo_ = new QComboBox(modelRow);
    for (const WhisperModel &model : modelManager_.models()) {
        modelCombo_->addItem(model.label, model.id);
    }
    downloadButton_ = new QPushButton("Whisper 모델 다운로드", modelRow);
    modelLayout->addWidget(modelCombo_, 1);
    modelLayout->addWidget(downloadButton_);
    transcriptionForm->addRow("Whisper 모델", modelRow);

    languageCombo_ = new QComboBox(transcriptionGroup);
    languageCombo_->addItem("한국어", "ko");
    languageCombo_->addItem("영어", "en");
    languageCombo_->addItem("일본어", "ja");
    languageCombo_->addItem("자동 감지", "");
    transcriptionForm->addRow("언어", languageCombo_);

    vadCheck_ = new QCheckBox("VAD 필터 사용", transcriptionGroup);
    vadCheck_->setChecked(false);
    vadCheck_->setVisible(false);

    modelStatus_ = new QLabel(transcriptionGroup);
    modelStatus_->setWordWrap(true);
    modelStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    transcriptionForm->addRow("상태", modelStatus_);
    settingsRow->addWidget(transcriptionGroup, 1);

    auto *aiGroup = new QGroupBox("AI 보정", central);
    auto *aiForm = new QFormLayout(aiGroup);
    aiForm->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    aiForm->setLabelAlignment(Qt::AlignRight | Qt::AlignVCenter);

    aiCorrectionCheck_ = new QCheckBox("전사 후 Ollama로 텍스트 보정", aiGroup);
    aiForm->addRow("사용", aiCorrectionCheck_);

    memoryStatus_ = new QLabel("메모리 확인 중", aiGroup);
    aiForm->addRow("시스템", memoryStatus_);

    auto *recommendedRow = new QWidget(aiGroup);
    auto *recommendedLayout = new QHBoxLayout(recommendedRow);
    recommendedLayout->setContentsMargins(0, 0, 0, 0);
    recommendedOllamaModel_ = new QLabel("권장 모델 확인 중", recommendedRow);
    downloadOllamaButton_ = new QPushButton("권장 모델 받기", recommendedRow);
    recommendedLayout->addWidget(recommendedOllamaModel_, 1);
    recommendedLayout->addWidget(downloadOllamaButton_);
    aiForm->addRow("권장", recommendedRow);

    auto *ollamaModelRow = new QWidget(aiGroup);
    auto *ollamaModelLayout = new QHBoxLayout(ollamaModelRow);
    ollamaModelLayout->setContentsMargins(0, 0, 0, 0);
    ollamaModelCombo_ = new QComboBox(ollamaModelRow);
    ollamaModelCombo_->setMinimumWidth(220);
    refreshOllamaButton_ = new QPushButton("새로고침", ollamaModelRow);
    ollamaModelLayout->addWidget(ollamaModelCombo_, 1);
    ollamaModelLayout->addWidget(refreshOllamaButton_);
    aiForm->addRow("사용 모델", ollamaModelRow);

    ollamaStatus_ = new QLabel("Ollama 상태 확인 전", aiGroup);
    ollamaStatus_->setWordWrap(true);
    aiForm->addRow("상태", ollamaStatus_);
    settingsRow->addWidget(aiGroup, 1);
    root->addLayout(settingsRow);

    auto *runGroup = new QGroupBox("실행", central);
    auto *runLayout = new QHBoxLayout(runGroup);
    progress_ = new QProgressBar(runGroup);
    progress_->setRange(0, 1);
    progress_->setValue(0);
    clearLogButton_ = new QPushButton("로그 지우기", runGroup);
    runButton_ = new QPushButton("SRT 만들기", runGroup);
    runButton_->setMinimumWidth(150);
    runLayout->addWidget(progress_, 1);
    runLayout->addWidget(clearLogButton_);
    runLayout->addWidget(runButton_);
    root->addWidget(runGroup);

    auto *logGroup = new QGroupBox("로그", central);
    auto *logLayout = new QVBoxLayout(logGroup);
    log_ = new QPlainTextEdit(logGroup);
    log_->setReadOnly(true);
    log_->setMinimumHeight(220);
    logLayout->addWidget(log_);
    root->addWidget(logGroup, 1);

    setCentralWidget(central);

    connect(inputButton, &QPushButton::clicked, this, [this]() { chooseInput(); });
    connect(outputButton, &QPushButton::clicked, this, [this]() { chooseOutput(); });
}

void MainWindow::wireEvents() {
    connect(modelCombo_, qOverload<int>(&QComboBox::currentIndexChanged), this, [this]() { refreshModelStatus(); });
    connect(downloadButton_, &QPushButton::clicked, this, [this]() { downloadModel(); });
    connect(downloadOllamaButton_, &QPushButton::clicked, this, [this]() { downloadRecommendedOllamaModel(); });
    connect(refreshOllamaButton_, &QPushButton::clicked, this, [this]() { refreshOllamaModels(); });
    connect(clearLogButton_, &QPushButton::clicked, this, [this]() { clearLog(); });
    connect(runButton_, &QPushButton::clicked, this, [this]() { startOrCancel(); });

    modelManager_.onLog = [this](const QString &message) { appendLog(message); };
    modelManager_.onError = [this](const QString &message) {
        appendLog(message);
        QMessageBox::critical(this, "SRTForge", message);
        refreshModelStatus();
    };
    modelManager_.onFinished = [this](const QString &) { refreshModelStatus(); };
    modelManager_.onDownloadProgress = [this](qint64 received, qint64 total) {
        if (total > 0) {
            progress_->setRange(0, 100);
            progress_->setValue(static_cast<int>((received * 100) / total));
        }
    };

    runner_.onLog = [this](const QString &message) { appendLog(message); };
    runner_.onError = [this](const QString &message) {
        setBusy(false);
        appendLog(message);
        QMessageBox::critical(this, "SRTForge", message);
    };
    runner_.onFinished = [this](const QString &path) {
        finishTranscription(path);
    };

    ollamaCorrector_.onLog = [this](const QString &message) { appendLog(message); };
    ollamaCorrector_.onError = [this](const QString &message) {
        setBusy(false);
        appendLog(message);
        QMessageBox::warning(this, "SRTForge", message);
    };
    ollamaCorrector_.onModelsChanged = [this](const QString &status) { refreshOllamaUi(status); };
    ollamaCorrector_.onPullFinished = [this](const QString &model) {
        refreshOllamaModels();
        const int index = ollamaModelCombo_->findText(model);
        if (index >= 0) {
            ollamaModelCombo_->setCurrentIndex(index);
        }
    };
    ollamaCorrector_.onFinished = [this](const QString &path) {
        setBusy(false);
        appendLog("완료했습니다: " + path);
        QMessageBox::information(this, "SRTForge", "전사 파일을 만들었습니다.");
    };

    updateRecommendedOllamaModel();
    refreshOllamaModels();
}

void MainWindow::chooseInput() {
    const QString path = QFileDialog::getOpenFileName(this, "음성/영상 파일 선택");
    if (path.isEmpty()) {
        return;
    }
    inputEdit_->setText(path);
    if (outputEdit_->text().trimmed().isEmpty()) {
        outputEdit_->setText(defaultOutputPath(path));
    }
}

void MainWindow::chooseOutput() {
    const QString path = QFileDialog::getSaveFileName(this, "SRT 저장 위치", outputEdit_->text(), "SRT 자막 (*.srt)");
    if (!path.isEmpty()) {
        outputEdit_->setText(path);
    }
}

void MainWindow::downloadModel() {
    modelManager_.download(selectedModel());
    progress_->setRange(0, 0);
    refreshModelStatus();
}

void MainWindow::startOrCancel() {
    if (runner_.isRunning()) {
        runner_.cancel();
        return;
    }

    const QString input = inputEdit_->text().trimmed();
    if (input.isEmpty()) {
        QMessageBox::warning(this, "SRTForge", "입력 파일을 선택하세요.");
        return;
    }

    const QFileInfo inputInfo(input);
    QString output = outputEdit_->text().trimmed();
    if (output.isEmpty()) {
        output = defaultOutputPath(input);
        outputEdit_->setText(output);
    }

    const QString inputSuffix = inputInfo.suffix().toLower();
    if (inputSuffix == "txt" || inputSuffix == "srt") {
        if (!aiCorrectionCheck_->isChecked()) {
            QMessageBox::warning(this, "SRTForge", "이미 전사된 파일은 전사할 필요가 없습니다. AI 보정 사용을 체크하세요.");
            return;
        }
        if (ollamaModelCombo_->currentText().trimmed().isEmpty()) {
            QMessageBox::warning(this, "SRTForge", "Ollama 모델을 선택하세요.");
            return;
        }
        if (output != input) {
            QFile::remove(output);
            if (!QFile::copy(input, output)) {
                QMessageBox::critical(this, "SRTForge", "입력 파일을 출력 위치로 복사하지 못했습니다.");
                return;
            }
        }
        setBusy(true);
        appendLog("기존 전사 파일 보정을 시작합니다: " + output);
        ollamaCorrector_.correctFile(output, ollamaModelCombo_->currentText().trimmed(), selectedLanguage());
        return;
    }

    const WhisperModel model = selectedModel();
    if (!modelManager_.hasModel(model)) {
        QMessageBox::warning(this, "SRTForge", "먼저 모델을 다운로드하세요.");
        return;
    }

    setBusy(true);
    appendLog("전사를 시작합니다.");
    runner_.start({input, output, modelManager_.modelPath(model), selectedLanguage(), false});
}

void MainWindow::clearLog() {
    log_->clear();
}

void MainWindow::refreshOllamaModels() {
    ollamaStatus_->setText("Ollama 확인 중...");
    ollamaModelCombo_->clear();
    ollamaCorrector_.refreshModels();
}

void MainWindow::refreshOllamaUi(const QString &status) {
    ollamaStatus_->setText(status);
    const QString previous = ollamaModelCombo_->currentText();
    ollamaModelCombo_->clear();
    ollamaModelCombo_->addItems(ollamaCorrector_.models());
    const int recommendedIndex = ollamaModelCombo_->findText(recommendedOllamaModelName_);
    const int previousIndex = ollamaModelCombo_->findText(previous);
    if (recommendedIndex >= 0) {
        ollamaModelCombo_->setCurrentIndex(recommendedIndex);
    } else if (previousIndex >= 0) {
        ollamaModelCombo_->setCurrentIndex(previousIndex);
    }
}

void MainWindow::downloadRecommendedOllamaModel() {
    const QString model = recommendedOllamaModelName_.trimmed();
    if (model.isEmpty()) {
        QMessageBox::warning(this, "SRTForge", "권장 모델을 확인하지 못했습니다.");
        return;
    }
    appendLog("권장 Ollama 모델 다운로드 요청: " + model);
    ollamaCorrector_.pullModel(model);
}

void MainWindow::updateRecommendedOllamaModel() {
    const double memoryGiB = SystemMemory::totalGiB();
    const QString recommended = recommendedOllamaModel(memoryGiB);
    recommendedOllamaModelName_ = recommended;

    if (memoryGiB > 0) {
        memoryStatus_->setText(QString("메모리: %1GB").arg(memoryGiB, 0, 'f', 1));
    } else {
        memoryStatus_->setText("메모리: 감지 실패");
    }
    recommendedOllamaModel_->setText("권장: " + recommended);
}

void MainWindow::finishTranscription(const QString &path) {
    appendLog("전사 완료: " + path);
    if (!aiCorrectionCheck_->isChecked()) {
        setBusy(false);
        QMessageBox::information(this, "SRTForge", "전사 파일을 만들었습니다.");
        return;
    }

    if (ollamaModelCombo_->currentText().trimmed().isEmpty()) {
        setBusy(false);
        appendLog("Ollama 모델이 없어 AI 보정을 건너뜁니다.");
        QMessageBox::warning(this, "SRTForge", "전사는 완료됐지만 Ollama 모델이 없어 AI 보정을 건너뜁니다.");
        return;
    }

    appendLog("AI 보정을 시작합니다.");
    ollamaCorrector_.correctFile(path, ollamaModelCombo_->currentText().trimmed(), selectedLanguage());
}

void MainWindow::refreshModelStatus() {
    const WhisperModel model = selectedModel();
    const bool controlsEnabled = !operationBusy_ && !runner_.isRunning() && !ollamaCorrector_.isBusy();
    if (modelManager_.hasModel(model)) {
        modelStatus_->setText("모델 준비됨: " + modelManager_.modelPath(model));
        downloadButton_->setEnabled(controlsEnabled);
    } else {
        modelStatus_->setText("모델 없음: " + model.label);
        downloadButton_->setEnabled(controlsEnabled && !modelManager_.isDownloading());
    }
}

void MainWindow::setBusy(bool busy) {
    operationBusy_ = busy;
    inputEdit_->setEnabled(!busy);
    outputEdit_->setEnabled(!busy);
    modelCombo_->setEnabled(!busy);
    languageCombo_->setEnabled(!busy);
    aiCorrectionCheck_->setEnabled(!busy);
    ollamaModelCombo_->setEnabled(!busy);
    downloadOllamaButton_->setEnabled(!busy);
    refreshOllamaButton_->setEnabled(!busy);
    vadCheck_->setEnabled(!busy);
    downloadButton_->setEnabled(!busy);
    runButton_->setText(busy ? "취소" : "SRT 만들기");
    progress_->setRange(busy ? 0 : 0, busy ? 0 : 1);
    progress_->setValue(0);
    refreshModelStatus();
}

void MainWindow::appendLog(const QString &message) {
    if (!message.trimmed().isEmpty()) {
        log_->appendPlainText(message);
    }
}

WhisperModel MainWindow::selectedModel() const {
    const QString id = modelCombo_->currentData().toString();
    for (const WhisperModel &model : modelManager_.models()) {
        if (model.id == id) {
            return model;
        }
    }
    return modelManager_.models().first();
}

QString MainWindow::selectedLanguage() const {
    return languageCombo_->currentData().toString();
}

QString MainWindow::defaultOutputPath(const QString &inputPath) const {
    QFileInfo info(inputPath);
    const QString suffix = info.suffix().toLower();
    if (suffix == "txt" || suffix == "srt") {
        return info.dir().filePath(info.completeBaseName() + "_corrected.srt");
    }
    return info.dir().filePath(info.completeBaseName() + ".srt");
}

QString MainWindow::recommendedOllamaModel(double memoryGiB) const {
    if (memoryGiB <= 0) {
        return "gemma3:4b";
    }
    if (memoryGiB <= 8.0) {
        return "gemma3:1b";
    }
    if (memoryGiB <= 24.0) {
        return "gemma3:4b";
    }
    if (memoryGiB <= 48.0) {
        return "gemma3:12b";
    }
    return "gemma3:27b";
}
