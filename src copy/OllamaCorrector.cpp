#include "OllamaCorrector.h"

#include <QFile>
#include <QCoreApplication>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTextStream>

namespace {
const char *kOllamaBaseUrl = "http://127.0.0.1:11434";
}

OllamaCorrector::OllamaCorrector(QObject *parent) : QObject(parent) {}

bool OllamaCorrector::isBusy() const {
    return busy_;
}

QStringList OllamaCorrector::models() const {
    return models_;
}

QString OllamaCorrector::statusText() const {
    return statusText_;
}

void OllamaCorrector::refreshModels() {
    if (busy_) {
        return;
    }

    QNetworkRequest request(QUrl(QString(kOllamaBaseUrl) + "/api/tags"));
    auto *reply = network_.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray data = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString errorText = reply->errorString();
        reply->deleteLater();

        models_.clear();
        if (!ok) {
            statusText_ = "Ollama 미실행";
            if (onLog) {
                onLog("Ollama를 찾지 못했습니다: " + errorText);
            }
            if (onModelsChanged) {
                onModelsChanged(statusText_);
            }
            return;
        }

        const QJsonDocument doc = QJsonDocument::fromJson(data);
        const QJsonArray modelArray = doc.object().value("models").toArray();
        for (const QJsonValue &value : modelArray) {
            const QString name = value.toObject().value("name").toString();
            if (!name.isEmpty()) {
                models_ << name;
            }
        }

        if (models_.isEmpty()) {
            statusText_ = "Ollama 실행 중, 모델 없음";
        } else {
            statusText_ = QString("Ollama 실행 중, 모델 %1개").arg(models_.size());
        }
        if (onModelsChanged) {
            onModelsChanged(statusText_);
        }
    });
}

void OllamaCorrector::pullModel(const QString &model) {
    const QString trimmedModel = model.trimmed();
    if (busy_) {
        if (onError) {
            onError("이미 Ollama 작업이 실행 중입니다.");
        }
        return;
    }
    if (trimmedModel.isEmpty()) {
        if (onError) {
            onError("다운로드할 Ollama 모델을 선택하세요.");
        }
        return;
    }

    QJsonObject body;
    body["name"] = trimmedModel;
    body["stream"] = true;

    QNetworkRequest request(QUrl(QString(kOllamaBaseUrl) + "/api/pull"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    busy_ = true;
    pullBuffer_.clear();
    if (onLog) {
        onLog("Ollama 모델 다운로드 시작: " + trimmedModel);
    }

    auto *reply = network_.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::readyRead, this, [this, reply]() {
        pullBuffer_.append(reply->readAll());
        while (true) {
            const int newline = pullBuffer_.indexOf('\n');
            if (newline < 0) {
                break;
            }
            const QByteArray line = pullBuffer_.left(newline).trimmed();
            pullBuffer_.remove(0, newline + 1);
            if (line.isEmpty()) {
                continue;
            }

            const QJsonDocument doc = QJsonDocument::fromJson(line);
            const QJsonObject object = doc.object();
            const QString status = object.value("status").toString();
            const qint64 completed = object.value("completed").toVariant().toLongLong();
            const qint64 total = object.value("total").toVariant().toLongLong();
            if (onLog && !status.isEmpty()) {
                if (total > 0) {
                    const int percent = static_cast<int>((completed * 100) / total);
                    onLog(QString("Ollama 다운로드: %1 (%2%)").arg(status).arg(percent));
                } else {
                    onLog("Ollama 다운로드: " + status);
                }
            }
        }
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply, trimmedModel]() {
        const QByteArray remaining = reply->readAll().trimmed();
        if (!remaining.isEmpty()) {
            pullBuffer_.append(remaining);
        }

        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString errorText = reply->errorString();
        reply->deleteLater();
        busy_ = false;
        pullBuffer_.clear();

        if (!ok) {
            if (onError) {
                onError("Ollama 모델 다운로드 실패: " + errorText);
            }
            return;
        }

        if (onLog) {
            onLog("Ollama 모델 다운로드 완료: " + trimmedModel);
        }
        refreshModels();
        if (onPullFinished) {
            onPullFinished(trimmedModel);
        }
    });
}

void OllamaCorrector::correctFile(const QString &path, const QString &model, const QString &language) {
    if (busy_) {
        if (onError) {
            onError("이미 AI 보정 중입니다.");
        }
        return;
    }
    if (model.trimmed().isEmpty()) {
        if (onError) {
            onError("Ollama 모델을 선택하세요.");
        }
        return;
    }

    QString errorMessage;
    const QList<TranscriptBlock> blocks = readTranscript(path, &errorMessage);
    if (blocks.isEmpty()) {
        if (onError) {
            onError(errorMessage.isEmpty() ? "보정할 전사 내용이 없습니다." : errorMessage);
        }
        return;
    }

    QJsonObject body;
    body["model"] = model;
    body["prompt"] = buildPrompt(blocks, language);
    body["stream"] = false;

    QNetworkRequest request(QUrl(QString(kOllamaBaseUrl) + "/api/generate"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    busy_ = true;
    if (onLog) {
        onLog("Ollama AI 보정 시작: " + model);
    }

    auto *reply = network_.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, path, blocks]() {
        const QByteArray data = reply->readAll();
        const bool ok = reply->error() == QNetworkReply::NoError;
        const QString errorText = reply->errorString();
        reply->deleteLater();
        busy_ = false;

        if (!ok) {
            if (onError) {
                onError("Ollama 보정 실패: " + errorText);
            }
            return;
        }

        applyCorrectionResponse(path, blocks, data);
    });
}

QList<OllamaCorrector::TranscriptBlock> OllamaCorrector::readTranscript(const QString &path, QString *errorMessage) const {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage) {
            *errorMessage = "전사 파일을 읽을 수 없습니다: " + path;
        }
        return {};
    }

    QTextStream in(&file);
    QList<TranscriptBlock> blocks;
    const QRegularExpression srtTimeLine(R"(^\s*(\d{2}:\d{2}:\d{2},\d{3}\s+-->\s+\d{2}:\d{2}:\d{2},\d{3}.*)\s*$)");
    const QRegularExpression transcriptTimeLine(R"(^\d{2}:\d{2}:\d{2}\s+~\s+\d{2}:\d{2}:\d{2}$)");

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QString index;
        QRegularExpressionMatch timeMatch = srtTimeLine.match(line);
        if (!timeMatch.hasMatch()) {
            const QString possibleIndex = line;
            if (!in.atEnd()) {
                const QString nextLine = in.readLine().trimmed();
                timeMatch = srtTimeLine.match(nextLine);
                if (timeMatch.hasMatch()) {
                    index = possibleIndex;
                    line = nextLine;
                } else if (transcriptTimeLine.match(possibleIndex).hasMatch()) {
                    line = possibleIndex;
                    if (!nextLine.isEmpty()) {
                        QStringList textLines;
                        textLines << nextLine;
                        while (!in.atEnd()) {
                            const QString textLine = in.readLine().trimmed();
                            if (textLine.isEmpty()) {
                                break;
                            }
                            textLines << textLine;
                        }
                        const QString text = textLines.join(" ").simplified();
                        if (!text.isEmpty()) {
                            blocks.append({"", line, text});
                        }
                    }
                    continue;
                } else {
                    continue;
                }
            }
        }

        QStringList textLines;
        while (!in.atEnd()) {
            const QString textLine = in.readLine().trimmed();
            if (textLine.isEmpty()) {
                break;
            }
            textLines << textLine;
        }

        const QString text = textLines.join(" ").simplified();
        if (!text.isEmpty()) {
            blocks.append({index, timeMatch.hasMatch() ? timeMatch.captured(1) : line, text});
        }
    }

    return blocks;
}

bool OllamaCorrector::writeTranscript(const QString &path, const QList<TranscriptBlock> &blocks, QString *errorMessage) const {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        if (errorMessage) {
            *errorMessage = "보정된 전사 파일을 쓸 수 없습니다: " + path;
        }
        return false;
    }

    QTextStream out(&file);
    for (int i = 0; i < blocks.size(); ++i) {
        if (i > 0) {
            out << "\n\n";
        }
        const QString index = blocks[i].index.isEmpty() ? QString::number(i + 1) : blocks[i].index;
        out << index << "\n";
        out << blocks[i].timestamp << "\n";
        out << blocks[i].text;
    }
    out << "\n";
    return true;
}

QString OllamaCorrector::buildPrompt(const QList<TranscriptBlock> &blocks, const QString &language) const {
    QStringList lines;
    for (int i = 0; i < blocks.size(); ++i) {
        lines << QString("%1. %2").arg(i + 1).arg(blocks[i].text);
    }

    return correctionSystemText(language) + "\n\n입력:\n" + lines.join("\n");
}

QString OllamaCorrector::loadPromptTemplate() const {
    const QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath("prompts/correction_prompt.txt"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../Resources/prompts/correction_prompt.txt"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../../../../prompts/correction_prompt.txt"),
        QDir::current().filePath("prompts/correction_prompt.txt")
    };

    for (const QString &path : candidates) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString prompt = QString::fromUtf8(file.readAll()).trimmed();
            if (!prompt.isEmpty()) {
                return prompt;
            }
        }
    }

    return defaultPromptTemplate();
}

QString OllamaCorrector::defaultPromptTemplate() const {
    return QString(
        "너는 전사문 교정기다.\n"
        "목표:\n"
        "- 음성 전사 결과를 읽기 좋은 문장으로 보정한다.\n"
        "- 의미를 바꾸지 않는다.\n"
        "- 없던 내용을 추가하지 않는다.\n"
        "보정 범위:\n"
        "- 맞춤법, 띄어쓰기, 문장부호, 명백한 전사 오탈자\n"
        "출력 규칙:\n"
        "- 입력 줄 개수와 같은 개수의 줄만 출력한다.\n"
        "- 각 줄은 반드시 '번호. 보정문' 형식으로 출력한다.\n"
        "- 설명, 제목, 마크다운, 코드블록을 출력하지 않는다."
    );
}

void OllamaCorrector::applyCorrectionResponse(const QString &path, const QList<TranscriptBlock> &blocks, const QByteArray &data) {
    const QJsonDocument responseDoc = QJsonDocument::fromJson(data);
    const QString response = responseDoc.object().value("response").toString().trimmed();
    QList<TranscriptBlock> corrected = blocks;
    if (!applyJsonCorrection(response, &corrected) && !applyLineCorrection(response, &corrected)) {
        if (onLog) {
            onLog("Ollama 원본 응답:\n" + response.left(2000));
        }
        if (onError) {
            onError("Ollama 보정 응답을 해석하지 못했습니다. 원본 전사 파일은 유지됩니다.");
        }
        return;
    }

    QString errorMessage;
    if (!validateCorrections(blocks, corrected, &errorMessage)) {
        if (onLog) {
            onLog("Ollama 원본 응답:\n" + response.left(2000));
        }
        if (onError) {
            onError(errorMessage + " 원본 전사 파일은 유지됩니다.");
        }
        return;
    }

    const QString backupPath = path + ".bak";
    QFile::remove(backupPath);
    QFile::copy(path, backupPath);

    if (!writeTranscript(path, corrected, &errorMessage)) {
        if (onError) {
            onError(errorMessage);
        }
        return;
    }

    if (onLog) {
        onLog("Ollama AI 보정 완료");
    }
    if (onFinished) {
        onFinished(path);
    }
}

bool OllamaCorrector::applyJsonCorrection(const QString &response, QList<TranscriptBlock> *corrected) const {
    const int start = response.indexOf('[');
    const int end = response.lastIndexOf(']');
    if (start < 0 || end < start) {
        return false;
    }

    const QByteArray jsonBytes = response.mid(start, end - start + 1).toUtf8();
    const QJsonDocument correctionsDoc = QJsonDocument::fromJson(jsonBytes);
    if (!correctionsDoc.isArray()) {
        return false;
    }

    int applied = 0;
    for (const QJsonValue &value : correctionsDoc.array()) {
        const QJsonObject object = value.toObject();
        const int index = object.value("index").toInt(-1);
        const QString text = object.value("text").toString().simplified();
        if (index >= 0 && index < corrected->size() && !text.isEmpty()) {
            (*corrected)[index].text = text;
            applied += 1;
        }
    }
    return applied > 0;
}

bool OllamaCorrector::applyLineCorrection(const QString &response, QList<TranscriptBlock> *corrected) const {
    const QRegularExpression numberedLine(R"(^\s*(\d+)\s*[\.\):：-]\s*(.+?)\s*$)");
    const QStringList lines = response.split('\n');
    int applied = 0;

    QStringList plainTextLines;
    for (const QString &rawLine : lines) {
        QString line = rawLine.trimmed();
        if (line.isEmpty() || line.startsWith("```")) {
            continue;
        }

        const QRegularExpressionMatch match = numberedLine.match(line);
        if (match.hasMatch()) {
            const int index = match.captured(1).toInt() - 1;
            const QString text = match.captured(2).simplified();
            if (index >= 0 && index < corrected->size() && !text.isEmpty()) {
                (*corrected)[index].text = text;
                applied += 1;
            }
            continue;
        }

        if (!line.startsWith("[") && !line.startsWith("{") && !line.contains("입력") && !line.contains("출력")) {
            plainTextLines << line;
        }
    }

    if (applied == 0 && plainTextLines.size() == corrected->size()) {
        for (int i = 0; i < corrected->size(); ++i) {
            const QString text = plainTextLines[i].simplified();
            if (!text.isEmpty()) {
                (*corrected)[i].text = text;
                applied += 1;
            }
        }
    }

    return applied > 0;
}

bool OllamaCorrector::validateCorrections(const QList<TranscriptBlock> &original, const QList<TranscriptBlock> &corrected, QString *errorMessage) const {
    if (original.size() != corrected.size()) {
        if (errorMessage) {
            *errorMessage = "보정 결과의 자막 블록 수가 원본과 다릅니다.";
        }
        return false;
    }

    for (int i = 0; i < original.size(); ++i) {
        if (original[i].timestamp != corrected[i].timestamp) {
            if (errorMessage) {
                *errorMessage = "보정 결과가 시간 정보를 변경했습니다.";
            }
            return false;
        }

        const QString originalText = original[i].text;
        const QString correctedText = corrected[i].text;
        if (correctedText.isEmpty()) {
            if (errorMessage) {
                *errorMessage = "보정 결과에 빈 자막이 있습니다.";
            }
            return false;
        }
        if (originalText.size() > 0 && correctedText.size() > originalText.size() * 3 + 40) {
            if (errorMessage) {
                *errorMessage = "보정 결과가 원문보다 과하게 길어졌습니다.";
            }
            return false;
        }
    }

    return true;
}

QString OllamaCorrector::correctionSystemText(const QString &language) const {
    QString languageName = "원문 언어";
    if (language == "ko") {
        languageName = "한국어";
    } else if (language == "ja") {
        languageName = "일본어";
    } else if (language == "en") {
        languageName = "영어";
    }

    return QString(
        "언어: %1\n"
        "%2"
    ).arg(languageName, loadPromptTemplate());
}
