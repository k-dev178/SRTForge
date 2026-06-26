#pragma once

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>

#include <functional>

struct TranscriptionOptions {
    QString inputPath;
    QString outputPath;
    QString modelPath;
    QString language;
    bool vadFilter = true;
};

class TranscriptionRunner : public QObject {
public:
    explicit TranscriptionRunner(QObject *parent = nullptr);

    bool isRunning() const;
    void start(const TranscriptionOptions &options);
    void cancel();

    std::function<void(const QString &)> onLog;
    std::function<void(const QString &)> onError;
    std::function<void(const QString &)> onFinished;

private:
    QString findProgram(const QStringList &names) const;
    QString tempWavPath() const;
    QString outputPrefix(const QString &outputPath) const;
    QString srtPathForOutput(const QString &outputPath) const;
    void startWhisper(const TranscriptionOptions &options, const QString &wavPath);
    void finish(bool ok, const QString &outputPath);
    void appendProcessLog(QProcess *process);

    QProcess *ffmpeg_ = nullptr;
    QProcess *whisper_ = nullptr;
    QString tempWav_;
};
