#include "TranscriptionRunner.h"
#include "SystemMemory.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QUuid>

TranscriptionRunner::TranscriptionRunner(QObject *parent) : QObject(parent) {}

bool TranscriptionRunner::isRunning() const {
    return ffmpeg_ != nullptr || whisper_ != nullptr;
}

void TranscriptionRunner::start(const TranscriptionOptions &options) {
    if (isRunning()) {
        if (onError) {
            onError("이미 전사 작업이 실행 중입니다.");
        }
        return;
    }

    const QString ffmpeg = findProgram({"ffmpeg", "ffmpeg.exe"});
    if (ffmpeg.isEmpty()) {
        if (onError) {
            onError("ffmpeg를 찾을 수 없습니다. Install.command 또는 시스템 패키지 관리자로 ffmpeg를 설치하세요.");
        }
        return;
    }

    tempWav_ = tempWavPath();
    ffmpeg_ = new QProcess(this);
    ffmpeg_->setProgram(ffmpeg);
    ffmpeg_->setArguments({"-y", "-i", options.inputPath, "-ar", "16000", "-ac", "1", "-c:a", "pcm_s16le", tempWav_});
    ffmpeg_->setProcessChannelMode(QProcess::MergedChannels);

    connect(ffmpeg_, &QProcess::readyReadStandardOutput, this, [this]() {
        appendProcessLog(ffmpeg_);
    });
    connect(ffmpeg_, &QProcess::finished, this, [this, options](int exitCode, QProcess::ExitStatus status) {
        appendProcessLog(ffmpeg_);
        ffmpeg_->deleteLater();
        ffmpeg_ = nullptr;

        if (status != QProcess::NormalExit || exitCode != 0) {
            finish(false, options.outputPath);
            return;
        }

        startWhisper(options, tempWav_);
    });

    if (onLog) {
        onLog("오디오 변환 시작");
    }
    ffmpeg_->start();
}

void TranscriptionRunner::cancel() {
    if (ffmpeg_) {
        ffmpeg_->kill();
    }
    if (whisper_) {
        whisper_->kill();
    }
}

QString TranscriptionRunner::findProgram(const QStringList &names) const {
    const QStringList appDirs = {
        QCoreApplication::applicationDirPath(),
        QDir(QCoreApplication::applicationDirPath()).filePath("tools"),
        "/opt/homebrew/bin",
        "/usr/local/bin"
    };
    for (const QString &dir : appDirs) {
        for (const QString &name : names) {
            const QString candidate = QDir(dir).filePath(name);
            if (QFileInfo::exists(candidate) && QFileInfo(candidate).isExecutable()) {
                return candidate;
            }
        }
    }

    for (const QString &name : names) {
        const QString candidate = QStandardPaths::findExecutable(name);
        if (!candidate.isEmpty()) {
            return candidate;
        }
    }

    return {};
}

QString TranscriptionRunner::tempWavPath() const {
    const QString name = "srtforge-" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".wav";
    return QDir(QDir::tempPath()).filePath(name);
}

QString TranscriptionRunner::outputPrefix(const QString &outputPath) const {
    QFileInfo info(outputPath);
    return QDir(info.absolutePath()).filePath(info.completeBaseName());
}

QString TranscriptionRunner::srtPathForOutput(const QString &outputPath) const {
    return outputPrefix(outputPath) + ".srt";
}

void TranscriptionRunner::startWhisper(const TranscriptionOptions &options, const QString &wavPath) {
    const QString whisper = findProgram({"whisper-cli", "whisper-cli.exe", "main", "main.exe"});
    if (whisper.isEmpty()) {
        if (onError) {
            onError("whisper-cli를 찾을 수 없습니다. Install.command 또는 whisper.cpp 빌드 결과물을 tools 폴더에 넣으세요.");
        }
        finish(false, options.outputPath);
        return;
    }

    whisper_ = new QProcess(this);
    whisper_->setProgram(whisper);

    QStringList args = {
        "-m", options.modelPath,
        "-f", wavPath,
        "-l", options.language.isEmpty() ? "auto" : options.language,
        "-osrt",
        "-of", outputPrefix(options.outputPath)
    };
    const QFileInfo modelInfo(options.modelPath);
    const bool isFullLargeV3 = modelInfo.fileName() == "ggml-large-v3.bin";
    const double memoryGiB = SystemMemory::totalGiB();
    if (isFullLargeV3 && memoryGiB > 0 && memoryGiB <= 8.5) {
        args << "--no-gpu";
        if (onLog) {
            onLog("8GB 메모리에서 large-v3 GPU 실행은 메모리 부족 위험이 있어 CPU 모드로 실행합니다. 속도가 필요하면 large-v3-turbo를 사용하세요.");
        }
    }
    if (options.vadFilter && onLog) {
        onLog("VAD 옵션은 별도 VAD 모델이 필요해서 현재 실행에서는 사용하지 않습니다.");
    }

    whisper_->setArguments(args);
    whisper_->setProcessChannelMode(QProcess::MergedChannels);

    connect(whisper_, &QProcess::readyReadStandardOutput, this, [this]() {
        appendProcessLog(whisper_);
    });
    connect(whisper_, &QProcess::finished, this, [this, options](int exitCode, QProcess::ExitStatus status) {
        appendProcessLog(whisper_);
        whisper_->deleteLater();
        whisper_ = nullptr;
        finish(status == QProcess::NormalExit && exitCode == 0, options.outputPath);
    });

    if (onLog) {
        onLog("Whisper 전사 시작");
    }
    whisper_->start();
}

void TranscriptionRunner::finish(bool ok, const QString &outputPath) {
    if (!tempWav_.isEmpty()) {
        QFile::remove(tempWav_);
        tempWav_.clear();
    }

    const QString srtPath = srtPathForOutput(outputPath);
    if (ok && QFile::exists(srtPath)) {
        if (srtPath != outputPath) {
            QFile::remove(outputPath);
            if (!QFile::rename(srtPath, outputPath)) {
                if (!QFile::copy(srtPath, outputPath)) {
                    if (onError) {
                        onError("SRT 파일을 저장하지 못했습니다: " + outputPath);
                    }
                    return;
                }
                QFile::remove(srtPath);
            }
        }
        if (onFinished) {
            onFinished(outputPath);
        }
        return;
    }

    if (ok) {
        if (onError) {
            onError("전사는 끝났지만 임시 SRT 파일을 찾을 수 없습니다: " + srtPath);
        }
        return;
    }

    if (onError) {
        onError("전사에 실패했습니다. 로그를 확인하세요.");
    }
}

void TranscriptionRunner::appendProcessLog(QProcess *process) {
    if (!process || !onLog) {
        return;
    }
    const QString text = QString::fromUtf8(process->readAllStandardOutput()).trimmed();
    if (!text.isEmpty()) {
        onLog(text);
    }
}
