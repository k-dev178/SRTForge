#!/bin/zsh
set -e

cd "$(dirname "$0")"

echo "SRTForge 필수 도구를 설치합니다..."
if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required on macOS: https://brew.sh"
  exit 1
fi

brew install ffmpeg whisper-cpp

echo
echo "설치 완료. 이제 SRTForge.command를 실행하세요."
read -r "?Enter를 누르면 닫힙니다."
