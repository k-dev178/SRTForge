#pragma once

#include "ModelManager.h"
#include "OllamaCorrector.h"
#include "SystemMemory.h"
#include "TranscriptionRunner.h"

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void wireEvents();
    void chooseInput();
    void chooseOutput();
    void downloadModel();
    void startOrCancel();
    void clearLog();
    void refreshOllamaModels();
    void refreshOllamaUi(const QString &status);
    void downloadRecommendedOllamaModel();
    void updateRecommendedOllamaModel();
    void finishTranscription(const QString &path);
    void refreshModelStatus();
    void setBusy(bool busy);
    void appendLog(const QString &message);
    WhisperModel selectedModel() const;
    QString selectedLanguage() const;
    QString defaultOutputPath(const QString &inputPath) const;
    QString recommendedOllamaModel(double memoryGiB) const;

    ModelManager modelManager_;
    OllamaCorrector ollamaCorrector_;
    TranscriptionRunner runner_;

    QLineEdit *inputEdit_ = nullptr;
    QLineEdit *outputEdit_ = nullptr;
    QComboBox *modelCombo_ = nullptr;
    QComboBox *languageCombo_ = nullptr;
    QCheckBox *aiCorrectionCheck_ = nullptr;
    QLabel *memoryStatus_ = nullptr;
    QLabel *recommendedOllamaModel_ = nullptr;
    QComboBox *ollamaModelCombo_ = nullptr;
    QLabel *ollamaStatus_ = nullptr;
    QPushButton *downloadOllamaButton_ = nullptr;
    QPushButton *refreshOllamaButton_ = nullptr;
    QCheckBox *vadCheck_ = nullptr;
    QLabel *modelStatus_ = nullptr;
    QProgressBar *progress_ = nullptr;
    QPushButton *downloadButton_ = nullptr;
    QPushButton *runButton_ = nullptr;
    QPushButton *clearLogButton_ = nullptr;
    QPlainTextEdit *log_ = nullptr;
    QString recommendedOllamaModelName_;
    bool operationBusy_ = false;
};
