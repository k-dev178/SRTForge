# SRTForge

SRTForge는 음성/영상 파일을 `whisper.cpp`로 전사해서 `.srt` 자막 파일을 만들고, 선택하면 Ollama로 전사문을 보정하는 데스크톱 프로그램입니다.

## 바로 실행

macOS:

```bash
./SRTForge.command
```

Windows:

```bat
SRTForge.bat
```

Windows 배포용 exe 만들기:

```powershell
powershell -ExecutionPolicy Bypass -File .\Build-Windows.ps1
```

생성 위치:

```text
dist\windows\SRTForge\SRTForge.exe
```

## macOS에서 실행

처음 한 번:

```bash
./Install.command
```

실행:

```bash
./SRTForge.command
```

빌드 결과 앱은 아래에 생성됩니다.

```text
build/SRTForge.app
```

## Windows에서 실행

필요한 프로그램:

```text
Qt 6
CMake
Visual Studio Build Tools 또는 Visual Studio
ffmpeg.exe
whisper-cli.exe
Ollama 선택 사항
```

개발 PC에서 바로 빌드 후 실행:

```bat
SRTForge.bat
```

배포용 exe 폴더 만들기:

```powershell
powershell -ExecutionPolicy Bypass -File .\Build-Windows.ps1
```

완성된 Windows 실행 폴더:

```text
dist\windows\SRTForge
```

다른 Windows PC에 보낼 때는 이 폴더 전체를 압축해서 보내면 됩니다.

## Windows 도구 파일 위치

Windows에서 `ffmpeg.exe`, `whisper-cli.exe`를 자동으로 찾게 하려면 프로젝트 루트에 아래처럼 넣습니다.

```text
tools\
  ffmpeg.exe
  whisper-cli.exe
```

그 상태로 `Build-Windows.ps1`을 실행하면 `dist\windows\SRTForge\tools` 안으로 같이 복사됩니다.

`tools` 폴더를 쓰지 않아도, `ffmpeg.exe`와 `whisper-cli.exe`가 Windows PATH에 등록되어 있으면 실행됩니다.

## 모델

Whisper 모델은 GUI에서 다운로드합니다. 저장 위치는 OS별 앱 데이터 폴더입니다.

AI 보정은 Ollama가 켜져 있을 때만 동작합니다. Ollama 모델도 GUI에서 권장 모델을 받을 수 있습니다.
