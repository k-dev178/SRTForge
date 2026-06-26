param(
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Configuration = "Release"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $Root "build-windows"
$DistDir = Join-Path $Root "dist\windows\SRTForge"

Set-Location $Root

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "CMake was not found. Install CMake first."
}

cmake -S . -B $BuildDir
cmake --build $BuildDir --config $Configuration

$ExeCandidates = @(
    (Join-Path $BuildDir "$Configuration\SRTForge.exe"),
    (Join-Path $BuildDir "SRTForge.exe")
)
$ExePath = $ExeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $ExePath) {
    throw "SRTForge.exe was not created."
}

if (Test-Path $DistDir) {
    Remove-Item $DistDir -Recurse -Force
}
New-Item -ItemType Directory -Path $DistDir | Out-Null

Copy-Item $ExePath $DistDir
Copy-Item (Join-Path $Root "prompts") $DistDir -Recurse

$ToolsDir = Join-Path $Root "tools"
if (Test-Path $ToolsDir) {
    Copy-Item $ToolsDir $DistDir -Recurse
} else {
    Write-Warning "tools folder not found. Put ffmpeg.exe and whisper-cli.exe in dist\windows\SRTForge\tools before running on another PC."
}

$WinDeployQtCommand = Get-Command windeployqt -ErrorAction SilentlyContinue
$WinDeployQtPath = if ($WinDeployQtCommand) { $WinDeployQtCommand.Source } else { $null }
if (-not $WinDeployQtPath) {
    $QtRoots = @(
        $env:Qt6_DIR,
        $env:CMAKE_PREFIX_PATH,
        "C:\Qt"
    ) | Where-Object { $_ }

    foreach ($QtRoot in $QtRoots) {
        $Found = Get-ChildItem -Path $QtRoot -Filter "windeployqt.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($Found) {
            $WinDeployQtPath = $Found.FullName
            break
        }
    }
}

if ($WinDeployQtPath) {
    & $WinDeployQtPath --release --no-translations (Join-Path $DistDir "SRTForge.exe")
} else {
    Write-Warning "windeployqt was not found. Run this script from a Qt command prompt or add Qt bin to PATH."
}

Write-Host ""
Write-Host "Windows package created:"
Write-Host $DistDir
