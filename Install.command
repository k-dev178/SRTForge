#!/bin/zsh
set -e

cd "$(dirname "$0")"

echo "Installing ffmpeg with Homebrew if needed..."
if ! command -v brew >/dev/null 2>&1; then
  echo "Homebrew is required on macOS: https://brew.sh"
  exit 1
fi

brew install cmake qt ffmpeg whisper-cpp

echo
echo "Done. You can now double-click SRTForge.command."
read -r "?Press Enter to close."
