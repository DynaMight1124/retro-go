param([string]$Python = 'python', [switch]$ReleasePackage)
$ErrorActionPreference = 'Stop'
Push-Location $PSScriptRoot
try {
    & $Python -c "import tkinter, PyInstaller; print('Tk and PyInstaller available')"
    if ($LASTEXITCODE -ne 0) { throw 'Use Python 3.11+ with Tk, and install requirements-package.txt into a dedicated environment.' }
    & $Python -m PyInstaller --noconfirm --distpath build/output --workpath build/pyinstaller image_builder.spec
    if ($LASTEXITCODE -ne 0) { throw 'Image builder packaging failed' }
    $report = Join-Path $PSScriptRoot 'build/runtime-check.json'
    if (Test-Path -LiteralPath $report) { Remove-Item -LiteralPath $report }
    $exe = Join-Path $PSScriptRoot 'build/output/Tiny386-Image-Builder.exe'
    $check = Start-Process -FilePath $exe -ArgumentList @('--check-runtime',('"{0}"' -f $report)) -WindowStyle Hidden -Wait -PassThru
    if ($check.ExitCode -ne 0 -or !(Test-Path -LiteralPath $report)) { throw 'Packaged runtime check failed' }
    Get-Content -LiteralPath $report
    $standalone = Join-Path (Split-Path $PSScriptRoot -Parent) 'Tiny386-Image-Builder.exe'
    Copy-Item -LiteralPath $exe -Destination $standalone -Force
    Write-Output "Standalone executable: $standalone"
    if ($ReleasePackage) {
        New-Item -ItemType Directory -Force -Path (Join-Path $PSScriptRoot 'dist') | Out-Null
        Copy-Item -LiteralPath $exe -Destination (Join-Path $PSScriptRoot 'dist/Tiny386-Image-Builder.exe') -Force
        & $Python package_sources.py
        if ($LASTEXITCODE -ne 0) { throw 'Source/licence packaging failed' }
    }
} finally { Pop-Location }
