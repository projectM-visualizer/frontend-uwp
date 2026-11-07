# Combines the x64 and x86 app packages into one .appxbundle.
#
# CMake configures one architecture per build folder, so a multi-architecture
# bundle has to be assembled afterwards. Build both presets first:
#
#   cmake --build --preset x64-release
#   cmake --build --preset x86-release
#   powershell -File scripts/make-bundle.ps1
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'

$root = Split-Path $PSScriptRoot -Parent
$suffix = if ($Configuration -eq 'Debug') { '_Debug' } else { '' }
$output = Join-Path $root "build\bundle\$Configuration"
$packages = Join-Path $output 'packages'

if (Test-Path $output) { Remove-Item $output -Recurse -Force }
New-Item -ItemType Directory -Path $packages | Out-Null

$version = $null
foreach ($build in @(@{ Preset = 'x64'; Arch = 'x64' }, @{ Preset = 'x86'; Arch = 'Win32' })) {
    $folder = Join-Path $root "build\$($build.Preset)\AppPackages"
    $appx = Get-ChildItem $folder -Recurse -Filter "projectM-UWP_*_$($build.Arch)$suffix.appx" -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if (-not $appx) {
        throw "No $Configuration package in $folder. Build the $($build.Preset) preset first."
    }
    Copy-Item $appx.FullName $packages
    $version = ($appx.Name -split '_')[1]
}

$makeappx = Get-ChildItem "${env:ProgramFiles(x86)}\Windows Kits\10\bin" -Recurse -Filter makeappx.exe |
    Where-Object { $_.FullName -match '\\x64\\' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1
if (-not $makeappx) {
    throw 'makeappx.exe not found. Install the Windows SDK.'
}

$bundle = Join-Path $output "projectM-UWP_${version}_x86_x64$suffix.appxbundle"
& $makeappx.FullName bundle /d $packages /p $bundle /bv $version /o
if ($LASTEXITCODE -ne 0) {
    throw "makeappx failed with exit code $LASTEXITCODE."
}

Write-Host "Created $bundle"
