# Checks that a bundle from make-bundle.ps1 holds complete x64 and x86 packages with
# the app's identity. A missing DLL or preset folder would otherwise only show up
# when someone launched the app.
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release'
)

$ErrorActionPreference = 'Stop'

$expectedName = 'coaster.projectM-visualizer'
$expectedPublisher = 'CN=D9ED5A20-CFEF-4A74-9651-3E6DDAF76CC5'
$expectedDisplayName = 'projectM-visualizer'
$requiredFiles = @(
    'AppxManifest.xml',
    'projectM-UWP.WindowsUniversal.Application.exe',
    'libEGL.dll',
    'libGLESv2.dll',
    'projectM-4.dll',
    'projectM-4-playlist.dll'
)

$root = Split-Path $PSScriptRoot -Parent
$packages = Join-Path $root "build\bundle\$Configuration\packages"

Add-Type -AssemblyName System.IO.Compression.FileSystem

$failures = @()
foreach ($expected in @(@{ Pattern = '*_x64*.appx'; Arch = 'x64' }, @{ Pattern = '*_Win32*.appx'; Arch = 'x86' })) {
    $appx = Get-ChildItem $packages -Filter $expected.Pattern | Select-Object -First 1
    if (-not $appx) {
        $failures += "No $($expected.Arch) package in $packages"
        continue
    }

    $zip = [IO.Compression.ZipFile]::OpenRead($appx.FullName)
    try {
        $names = $zip.Entries | ForEach-Object { $_.FullName }

        foreach ($file in $requiredFiles) {
            if ($names -notcontains $file) {
                $failures += "$($appx.Name) is missing $file"
            }
        }

        if (-not ($names | Where-Object { $_ -like 'p/*.milk' })) {
            $failures += "$($appx.Name) has no presets"
        }

        $entry = $zip.GetEntry('AppxManifest.xml')
        if ($entry) {
            $reader = New-Object IO.StreamReader($entry.Open())
            [xml]$manifest = $reader.ReadToEnd()
            $reader.Dispose()

            $identity = $manifest.Package.Identity
            if ($identity.Name -ne $expectedName) {
                $failures += "$($appx.Name) has identity $($identity.Name)"
            }
            if ($identity.Publisher -ne $expectedPublisher) {
                $failures += "$($appx.Name) has publisher $($identity.Publisher)"
            }
            $displayName = $manifest.Package.Properties.DisplayName
            if ($displayName -ne $expectedDisplayName) {
                $failures += "$($appx.Name) has display name $displayName"
            }
            if ($identity.ProcessorArchitecture -ne $expected.Arch) {
                $failures += "$($appx.Name) has architecture $($identity.ProcessorArchitecture)"
            }
        }
    }
    finally {
        $zip.Dispose()
    }
}

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Host "FAILED: $_" }
    exit 1
}

Write-Host "Packages are valid."
