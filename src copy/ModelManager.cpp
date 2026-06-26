#include "ModelManager.h"

#include <QDir>
#include <QFile>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStandardPaths>

ModelManager::ModelManager(QObject *parent) : QObject(parent) {
    const QString base = "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/";
    modelList_ = {
        {"large-v3-turbo", "large-v3-turbo (추천)", "ggml-large-v3-turbo.bin", QUrl(base + "ggml-large-v3-turbo.bin")},
        {"large-v3", "large-v3 (정확도 우선)", "ggml-large-v3.bin", QUrl(base + "ggml-large-v3.bin")},
        {"medium", "medium", "ggml-medium.bin", QUrl(base + "ggml-medium.bin")},
        {"small", "small", "ggml-small.bin", QUrl(base + "ggml-small.bin")},
    };
}

QList<WhisperModel> ModelManager::models() const {
    return modelList_;
}

QString ModelManager::modelsDirectory() const {
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(base).filePath("models");
}

QString ModelManager::modelPath(const WhisperModel &model) const {
    return QDir(modelsDirectory()).filePath(model.fileName);
}

bool ModelManager::hasModel(const WhisperModel &model) const {
    return QFile::exists(modelPath(model));
}

bool ModelManager::isDownloading() const {
    return downloading_;
}

void ModelManager::download(const WhisperModel &model) {
    if (downloading_) {
        if (onError) {
            onError("이미 모델을 다운로드 중입니다.");
        }
        return;
    }

    QDir().mkpath(modelsDirectory());
    const QString finalPath = modelPath(model);
    const QString partialPath = finalPath + ".part";
    auto *file = new QFile(partialPath, this);
    if (!file->open(QIODevice::WriteOnly)) {
        if (onError) {
            onError("모델 파일을 쓸 수 없습니다: " + partialPath);
        }
        file->deleteLater();
        return;
    }

    downloading_ = true;
    if (onLog) {
        onLog("모델 다운로드 시작: " + model.label);
    }

    QNetworkRequest request(model.url);
    auto *reply = network_.get(request);
    connect(reply, &QNetworkReply::downloadProgress, this, [this](qint64 received, qint64 total) {
        if (onDownloadProgress) {
            onDownloadProgress(received, total);
        }
    });
    connect(reply, &QNetworkReply::readyRead, this, [reply, file]() {
        file->write(reply->readAll());
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, file, partialPath, finalPath]() {
        file->write(reply->readAll());
        file->close();

        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString errorText = reply->errorString();
        reply->deleteLater();
        file->deleteLater();
        downloading_ = false;

        if (!ok) {
            QFile::remove(partialPath);
            if (onError) {
                onError("모델 다운로드 실패: " + errorText);
            }
            return;
        }

        QFile::remove(finalPath);
        if (!QFile::rename(partialPath, finalPath)) {
            QFile::remove(partialPath);
            if (onError) {
                onError("모델 파일 저장에 실패했습니다: " + finalPath);
            }
            return;
        }

        if (onLog) {
            onLog("모델 다운로드 완료: " + finalPath);
        }
        if (onFinished) {
            onFinished(finalPath);
        }
    });
}
