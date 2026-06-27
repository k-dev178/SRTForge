import Foundation

struct WhisperModel: Identifiable, Hashable {
    let id: String
    let label: String
    let fileName: String
    let url: URL

    static let all: [WhisperModel] = [
        .init(id: "large-v3-turbo", label: "large-v3-turbo (추천)", fileName: "ggml-large-v3-turbo.bin", url: URL(string: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3-turbo.bin")!),
        .init(id: "large-v3", label: "large-v3 (정확도 우선)", fileName: "ggml-large-v3.bin", url: URL(string: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-large-v3.bin")!),
        .init(id: "medium", label: "medium", fileName: "ggml-medium.bin", url: URL(string: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-medium.bin")!),
        .init(id: "small", label: "small", fileName: "ggml-small.bin", url: URL(string: "https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-small.bin")!)
    ]
}

enum TranscriptLanguage: String, CaseIterable, Identifiable {
    case ko, en, ja, auto
    var id: String { rawValue }
    var title: String {
        switch self { case .ko: "한국어"; case .en: "영어"; case .ja: "일본어"; case .auto: "자동 감지" }
    }
}
