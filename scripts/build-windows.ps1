# Build xquest.exe and package a Windows installer via NSIS, bundling the
# game data from assets/ (see assets/README).
# Expects: cmake, a VS toolchain, vcpkg (with VCPKG_INSTALLATION_ROOT set),
# and makensis on PATH.
$ErrorActionPreference = "Stop"

$RepoRoot = Resolve-Path "$PSScriptRoot\.."
$BuildDir = Join-Path $RepoRoot "build-windows"
$StageDir = Join-Path $BuildDir "stage"
$Version  = if ($env:XQUEST_VERSION) { $env:XQUEST_VERSION } else { "0.0.0" }

Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $StageDir | Out-Null

$Toolchain = Join-Path $env:VCPKG_INSTALLATION_ROOT "scripts\buildsystems\vcpkg.cmake"

cmake -B "$BuildDir\cmake" -A x64 `
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain" `
    -DVCPKG_TARGET_TRIPLET=x64-windows `
    -DCMAKE_BUILD_TYPE=Release `
    $RepoRoot
cmake --build "$BuildDir\cmake" --config Release --parallel

Copy-Item "$BuildDir\cmake\Release\xquest.exe" "$StageDir\xquest.exe"

$Sdl2Dll = Get-ChildItem -Path (Join-Path $env:VCPKG_INSTALLATION_ROOT "installed\x64-windows\bin") `
    -Filter "SDL2.dll" -ErrorAction SilentlyContinue | Select-Object -First 1
if (-not $Sdl2Dll) {
    throw "SDL2.dll not found under vcpkg installed\x64-windows\bin -- did 'vcpkg install sdl2:x64-windows' run?"
}
Copy-Item $Sdl2Dll.FullName "$StageDir\SDL2.dll"

$DataDir = Join-Path $StageDir "data"
New-Item -ItemType Directory -Force -Path $DataDir | Out-Null
foreach ($f in "xquest.gfx", "xquest.enm", "xquest.fnt", "xquest2.fnt", "xquest.snd", "startpic.raw") {
    Copy-Item (Join-Path $RepoRoot "assets\$f") (Join-Path $DataDir $f)
}

$OutName = "XQuest-$Version-Setup.exe"
$OutPath = Join-Path $RepoRoot $OutName

makensis "/DXQUEST_STAGE_DIR=$StageDir" "/DXQUEST_VERSION=$Version" "/DXQUEST_OUT=$OutPath" `
    (Join-Path $RepoRoot "installer\windows\xquest.nsi")

Write-Host "Done: $OutPath"
