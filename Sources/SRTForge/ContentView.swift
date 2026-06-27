import SwiftUI

struct ContentView: View {
    @EnvironmentObject private var model: AppModel

    var body: some View {
        VStack(spacing: 0) {
            header
            ScrollView {
                VStack(spacing: 16) {
                    fileSection
                    transcriptionSection
                    logSection
                }
                .padding(20)
            }
            actionBar
        }
        .background(Color(nsColor: .windowBackgroundColor))
        .alert("SRTForge", isPresented: Binding(get: { model.alertMessage != nil }, set: { if !$0 { model.alertMessage = nil } })) {
            Button("확인") { model.alertMessage = nil }
        } message: { Text(model.alertMessage ?? "") }
    }

    private var header: some View {
        HStack {
            VStack(alignment: .leading, spacing: 3) {
                Text("SRTForge").font(.title2.bold())
                Text("macOS 로컬 전사 및 자막 보정").font(.subheadline).foregroundStyle(.secondary)
            }
            Spacer()
            Label("로컬 전사", systemImage: "waveform")
                .foregroundStyle(.secondary)
        }
        .padding(.horizontal, 20).padding(.vertical, 14)
        .background(.bar)
    }

    private var fileSection: some View {
        GroupBox("파일") {
            VStack(spacing: 10) {
                fileRow("입력", value: model.inputURL?.path ?? "음성 또는 영상 파일", icon: "waveform", action: model.selectInput)
                Divider()
                fileRow("출력", value: model.outputURL?.path ?? "저장할 SRT 위치", icon: "square.and.arrow.down", action: model.selectOutput)
            }.padding(6)
        }
    }

    private var transcriptionSection: some View {
        GroupBox("전사 설정") {
            VStack(alignment: .leading, spacing: 12) {
                Picker("Whisper 모델", selection: $model.whisperModel) { ForEach(WhisperModel.all) { Text($0.label).tag($0) } }
                    .disabled(model.isBusy)
                Picker("언어", selection: $model.language) { ForEach(TranscriptLanguage.allCases) { Text($0.title).tag($0) } }
                    .disabled(model.isBusy)
                HStack {
                    Text(String(format: "시스템 메모리 %.1fGB", model.memoryGiB)).foregroundStyle(.secondary)
                    Spacer()
                    Label(model.hasWhisperModel ? "모델 준비됨" : "모델 필요", systemImage: model.hasWhisperModel ? "checkmark.circle.fill" : "exclamationmark.circle")
                        .foregroundStyle(model.hasWhisperModel ? .green : .orange)
                    Button("다운로드", systemImage: "arrow.down.circle", action: model.downloadWhisperModel)
                        .disabled(model.hasWhisperModel || model.isDownloading)
                }
            }.padding(8)
        }.frame(maxWidth: .infinity)
    }

    private var logSection: some View {
        GroupBox {
            VStack(spacing: 8) {
                HStack { Text("로그").font(.headline); Spacer(); Button("지우기", systemImage: "trash", action: model.clearLog).disabled(model.logs.isEmpty) }
                ScrollViewReader { proxy in
                    ScrollView { LazyVStack(alignment: .leading, spacing: 4) { ForEach(Array(model.logs.enumerated()), id: \.offset) { index, line in Text(line).font(.system(.caption, design: .monospaced)).textSelection(.enabled).frame(maxWidth: .infinity, alignment: .leading).id(index) } }.padding(8) }
                        .background(Color(nsColor: .textBackgroundColor)).clipShape(RoundedRectangle(cornerRadius: 6))
                        .onChange(of: model.logs.count) { count in if count > 0 { proxy.scrollTo(count - 1) } }
                }
            }.padding(4)
        }.frame(minHeight: 230)
    }

    private var actionBar: some View {
        HStack {
            if model.isBusy { ProgressView().controlSize(.small); Text("작업 중...").foregroundStyle(.secondary) }
            Spacer()
            if model.isBusy { Button("취소", role: .cancel, action: model.cancel) }
            Button("SRT 만들기", systemImage: "captions.bubble.fill", action: model.run)
                .buttonStyle(.borderedProminent).controlSize(.large).disabled(!model.canRun)
        }.padding(.horizontal, 20).padding(.vertical, 12).background(.bar)
    }

    private func fileRow(_ title: String, value: String, icon: String, action: @escaping () -> Void) -> some View {
        HStack { Text(title).frame(width: 44, alignment: .leading); Image(systemName: icon).foregroundStyle(.secondary); Text(value).lineLimit(1).truncationMode(.middle).foregroundStyle(model.inputURL == nil ? .secondary : .primary); Spacer(); Button("선택", action: action) }
    }
}
