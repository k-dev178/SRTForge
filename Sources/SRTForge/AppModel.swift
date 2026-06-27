import AppKit
import Foundation
import UniformTypeIdentifiers

@MainActor
final class AppModel: ObservableObject {
    @Published var inputURL: URL?
    @Published var outputURL: URL?
    @Published var whisperModel = WhisperModel.all[0]
    @Published var language = TranscriptLanguage.ko
    @Published var logs: [String] = []
    @Published var isBusy = false
    @Published var isDownloading = false
    @Published var progress: Double = 0
    @Published var alertMessage: String?

    private let runner = ProcessRunner()
    let memoryGiB = Double(ProcessInfo.processInfo.physicalMemory) / 1_073_741_824

    var modelPath: URL {
        modelsDirectory.appendingPathComponent(whisperModel.fileName)
    }
    var hasWhisperModel: Bool { FileManager.default.fileExists(atPath: modelPath.path) }
    var canRun: Bool { inputURL != nil && outputURL != nil && !isBusy && hasWhisperModel }

    func selectInput() {
        let panel = NSOpenPanel()
        panel.allowedContentTypes = [.audio, .movie]
        guard panel.runModal() == .OK, let url = panel.url else { return }
        inputURL = url
        outputURL = url.deletingPathExtension().appendingPathExtension("srt")
    }

    func selectOutput() {
        let panel = NSSavePanel()
        panel.allowedContentTypes = [.init(filenameExtension: "srt")!]
        panel.nameFieldStringValue = outputURL?.lastPathComponent ?? "전사.srt"
        guard panel.runModal() == .OK else { return }
        outputURL = panel.url
    }

    func downloadWhisperModel() {
        guard !isDownloading else { return }
        isDownloading = true; progress = 0
        append("Whisper 모델 다운로드 시작: \(whisperModel.label)")
        Task {
            do {
                try FileManager.default.createDirectory(at: modelsDirectory, withIntermediateDirectories: true)
                let (temporary, _) = try await URLSession.shared.download(from: whisperModel.url)
                try? FileManager.default.removeItem(at: modelPath)
                try FileManager.default.moveItem(at: temporary, to: modelPath)
                append("모델 다운로드 완료: \(modelPath.path)")
            } catch { fail(error) }
            isDownloading = false; progress = 0
        }
    }

    func run() {
        guard let inputURL, let outputURL, canRun else { return }
        isBusy = true; append("전사를 시작합니다.")
        Task {
            do {
                try await transcribe(input: inputURL, output: outputURL)
                append("완료: \(outputURL.path)")
            } catch { fail(error) }
            isBusy = false
        }
    }

    func cancel() { Task { await runner.cancel() }; isBusy = false; append("작업을 취소했습니다.") }
    func clearLog() { logs.removeAll() }

    private func transcribe(input: URL, output: URL) async throws {
        guard let ffmpeg = ToolLocator.find(["ffmpeg"]) else { throw AppError.message("ffmpeg를 찾을 수 없습니다. Install.command를 실행하세요.") }
        guard let whisper = ToolLocator.find(["whisper-cli"]) else { throw AppError.message("whisper-cli를 찾을 수 없습니다. Install.command를 실행하세요.") }
        let wav = FileManager.default.temporaryDirectory.appendingPathComponent("srtforge-\(UUID().uuidString).wav")
        defer { try? FileManager.default.removeItem(at: wav) }
        append("오디오 변환 시작")
        try await runner.run(ffmpeg, arguments: ["-y", "-i", input.path, "-ar", "16000", "-ac", "1", "-c:a", "pcm_s16le", wav.path], onOutput: outputHandler)
        append("Whisper 전사 시작")
        let prefix = output.deletingPathExtension().path
        var args = ["-m", modelPath.path, "-f", wav.path, "-l", language.rawValue, "-osrt", "-of", prefix]
        if whisperModel.id == "large-v3" && memoryGiB <= 8.5 { args.append("--no-gpu") }
        try await runner.run(whisper, arguments: args, onOutput: outputHandler)
        let generated = URL(fileURLWithPath: prefix + ".srt")
        guard FileManager.default.fileExists(atPath: generated.path) else { throw AppError.message("생성된 SRT 파일을 찾을 수 없습니다.") }
        if generated != output { try FileManager.default.moveReplacingItem(at: generated, to: output) }
    }

    private var outputHandler: @Sendable (String) -> Void { { [weak self] text in Task { @MainActor in self?.append(text.trimmingCharacters(in: .whitespacesAndNewlines)) } } }
    private var modelsDirectory: URL { FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask)[0].appendingPathComponent("SRTForge/models") }
    private func append(_ text: String) { if !text.isEmpty { logs.append(text) } }
    private func fail(_ error: Error) { let text = error.localizedDescription; append(text); alertMessage = text }
}

private extension FileManager {
    func moveReplacingItem(at source: URL, to destination: URL) throws { try? removeItem(at: destination); try moveItem(at: source, to: destination) }
}
