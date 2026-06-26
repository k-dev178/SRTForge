#!/bin/zsh
set -e

cd "$(dirname "$0")"

export PATH="/opt/homebrew/bin:/usr/local/bin:${PATH}"

echo "Starting SRTForge..."
if [ -d "/opt/homebrew/opt/qt" ]; then
  export CMAKE_PREFIX_PATH="/opt/homebrew/opt/qt:${CMAKE_PREFIX_PATH}"
fi
if [ -d "/usr/local/opt/qt" ]; then
  export CMAKE_PREFIX_PATH="/usr/local/opt/qt:${CMAKE_PREFIX_PATH}"
fi

cmake -S . -B build
cmake --build build
exec ./build/SRTForge.app/Contents/MacOS/SRTForge
