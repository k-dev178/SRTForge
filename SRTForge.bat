@echo off
setlocal

cd /d "%~dp0"

where cmake >nul 2>nul
if errorlevel 1 (
  echo CMake was not found. Install CMake and run this file again.
  pause
  exit /b 1
)

echo Building SRTForge...
cmake -S . -B build-windows
if errorlevel 1 (
  pause
  exit /b 1
)

cmake --build build-windows --config Release
if errorlevel 1 (
  pause
  exit /b 1
)

if exist "build-windows\Release\SRTForge.exe" (
  start "" "build-windows\Release\SRTForge.exe"
) else if exist "build-windows\SRTForge.exe" (
  start "" "build-windows\SRTForge.exe"
) else (
  echo SRTForge.exe was not created.
  pause
  exit /b 1
)
