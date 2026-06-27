# SRTForge

macOS 전용 SwiftUI 앱입니다. 음성·영상 파일을 로컬 `whisper.cpp`로 전사해 SRT 자막 파일을 만듭니다.

## 설치

처음 한 번 실행합니다.

```bash
./Install.command
```

Homebrew를 통해 `ffmpeg`와 `whisper.cpp`를 설치합니다.

## 실행

```bash
./SRTForge.command
```

Swift로 앱을 빌드한 뒤 자동으로 실행합니다. 생성된 앱은 다음 위치에 있습니다.

```text
build/SRTForge.app
```

이후 Finder에서 `build/SRTForge.app`을 직접 실행할 수도 있습니다.

## 사용 순서

1. 음성 또는 영상 파일을 선택합니다.
2. Whisper 모델과 전사 언어를 선택합니다.
3. `SRT 만들기`를 누릅니다.

Whisper 모델은 앱에서 다운로드하며 `~/Library/Application Support/SRTForge/models`에 저장됩니다.
