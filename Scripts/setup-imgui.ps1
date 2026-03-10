$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$thirdPartyDir = Join-Path $repoRoot "third_party"
$imguiDir = Join-Path $thirdPartyDir "imgui"

if (-not (Test-Path $thirdPartyDir)) {
    New-Item -Path $thirdPartyDir -ItemType Directory | Out-Null
}

if (Test-Path $imguiDir) {
    Write-Host "ImGui already exists: $imguiDir"
}
else {
    Write-Host "Cloning Dear ImGui into: $imguiDir"
    git clone --depth 1 https://github.com/ocornut/imgui.git $imguiDir | Out-Host
}

Write-Host "Done. Reconfigure/build C++ to enable ImGui:"
Write-Host "  cmake -S Backend -B Backend/build"
Write-Host "  cmake --build Backend/build --config Debug"
