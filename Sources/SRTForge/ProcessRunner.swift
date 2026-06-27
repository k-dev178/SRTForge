import Foundation

actor ProcessRunner {
    private var process: Process?

    func run(_ executable: URL, arguments: [String], onOutput: @escaping @Sendable (String) -> Void) async throws {
        let process = Process()
        let pipe = Pipe()
        process.executableURL = executable
        process.arguments = arguments
        process.standardOutput = pipe
        process.standardError = pipe
        self.process = process

        pipe.fileHandleForReading.readabilityHandler = { handle in
            let data = handle.availableData
            guard !data.isEmpty, let text = String(data: data, encoding: .utf8) else { return }
            onOutput(text)
        }
        try process.run()
        await withCheckedContinuation { continuation in
            process.terminationHandler = { _ in continuation.resume() }
        }
        pipe.fileHandleForReading.readabilityHandler = nil
        self.process = nil
        guard process.terminationStatus == 0 else { throw AppError.processFailed(executable.lastPathComponent) }
    }

    func cancel() { process?.terminate() }
}

enum AppError: LocalizedError {
    case message(String)
    case processFailed(String)
    var errorDescription: String? {
        switch self {
        case .message(let text): text
        case .processFailed(let name): "\(name) 실행에 실패했습니다. 로그를 확인하세요."
        }
    }
}

enum ToolLocator {
    static func find(_ names: [String]) -> URL? {
        let roots = [Bundle.main.executableURL?.deletingLastPathComponent(), URL(fileURLWithPath: "/opt/homebrew/bin"), URL(fileURLWithPath: "/usr/local/bin")].compactMap { $0 }
        for root in roots {
            for name in names {
                let candidate = root.appendingPathComponent(name)
                if FileManager.default.isExecutableFile(atPath: candidate.path) { return candidate }
            }
        }
        return nil
    }
}
