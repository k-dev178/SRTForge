#pragma once

#include <QNetworkAccessManager>
#include <QObject>
#include <QList>
#include <QString>
#include <QUrl>

#include <functional>

struct WhisperModel {
    QString id;
    QString label;
    QString fileName;
    QUrl url;
};

class ModelManager : public QObject {
public:
    explicit ModelManager(QObject *parent = nullptr);

    QList<WhisperModel> models() const;
    QString modelsDirectory() const;
    QString modelPath(const WhisperModel &model) const;
    bool hasModel(const WhisperModel &model) const;
    bool isDownloading() const;

    void download(const WhisperModel &model);

    std::function<void(qint64, qint64)> onDownloadProgress;
    std::function<void(const QString &)> onLog;
    std::function<void(const QString &)> onError;
    std::function<void(const QString &)> onFinished;

private:
    QList<WhisperModel> modelList_;
    QNetworkAccessManager network_;
    bool downloading_ = false;
};
