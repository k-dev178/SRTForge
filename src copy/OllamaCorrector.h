#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class OllamaCorrector : public QObject {
public:
    explicit OllamaCorrector(QObject *parent = nullptr);

    bool isBusy() const;
    QStringList models() const;
    QString statusText() const;

    void refreshModels();
    void pullModel(const QString &model);
    void correctFile(const QString &path, const QString &model, const QString &language);

    std::function<void(const QString &)> onLog;
    std::function<void(const QString &)> onError;
    std::function<void(const QString &)> onModelsChanged;
    std::function<void(const QString &)> onPullFinished;
    std::function<void(const QString &)> onFinished;

private:
    struct TranscriptBlock {
        QString index;
        QString timestamp;
        QString text;
    };

    QList<TranscriptBlock> readTranscript(const QString &path, QString *errorMessage) const;
    bool writeTranscript(const QString &path, const QList<TranscriptBlock> &blocks, QString *errorMessage) const;
    QString buildPrompt(const QList<TranscriptBlock> &blocks, const QString &language) const;
    QString loadPromptTemplate() const;
    QString defaultPromptTemplate() const;
    void applyCorrectionResponse(const QString &path, const QList<TranscriptBlock> &blocks, const QByteArray &data);
    bool applyJsonCorrection(const QString &response, QList<TranscriptBlock> *corrected) const;
    bool applyLineCorrection(const QString &response, QList<TranscriptBlock> *corrected) const;
    bool validateCorrections(const QList<TranscriptBlock> &original, const QList<TranscriptBlock> &corrected, QString *errorMessage) const;
    QString correctionSystemText(const QString &language) const;

    QNetworkAccessManager network_;
    QStringList models_;
    QByteArray pullBuffer_;
    QString statusText_ = "Ollama 상태 확인 전";
    bool busy_ = false;
};
