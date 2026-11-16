<#
  Vortex64-Windows: downloads the official GLideN64 plugin for Windows x64 (Project64 plugin format) from the GLideN64
  project's own GitHub release, verifies its SHA-256 and extracts it to third_party_bin/GLideN64 (ignored by git).

  GLideN64 is GPL-2.0-or-later software (https://github.com/gonetz/GLideN64). The binary is NOT stored in this repository.
  Its source code for this build: https://github.com/gonetz/GLideN64/tree/a04081a (see licenses/GLideN64.txt).

  Usage:  powershell -File packaging/windows/fetch-gliden64.ps1 [-Force]
  Then:   packaging/windows/package.ps1 includes it automatically when third_party_bin/GLideN64/GLideN64.dll exists.
#>
param([switch]$Force)
$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "../..")
$dest = Join-Path $root "third_party_bin/GLideN64"
$asset = "GLideN64-a04081a-Windows-PJ64Legacy-WTL-x64.zip"
$url = "https://github.com/gonetz/GLideN64/releases/download/github-actions/$asset"
$sha256 = "c3f4a9a6d6844061dd60950ca9eebf08e4c2a7db0c5e054e9e1a9a008c39d1f9"

if ((Test-Path (Join-Path $dest "GLideN64.dll")) -and -not $Force) { Write-Host "GLideN64 already present in $dest (use -Force to download again)"; return }

$tmp = Join-Path ([System.IO.Path]::GetTempPath()) ("v64-gliden64-" + [guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory $tmp | Out-Null
try {
    $zip = Join-Path $tmp $asset
    Write-Host "Downloading $url"
    Invoke-WebRequest -Uri $url -OutFile $zip -UseBasicParsing
    $actual = (Get-FileHash $zip -Algorithm SHA256).Hash.ToLower()
    if ($actual -ne $sha256) { throw "SHA-256 mismatch: expected $sha256, got $actual. Not extracting." }
    Write-Host "SHA-256 verified"
    if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
    New-Item -ItemType Directory $dest -Force | Out-Null
    Expand-Archive $zip -DestinationPath $dest -Force
    "GLideN64 a04081a (Windows x64, PJ64Legacy WTL)`r`nSource: https://github.com/gonetz/GLideN64/tree/a04081a`r`nLicense: GPL-2.0-or-later`r`nArchive SHA-256: $sha256" | Set-Content (Join-Path $dest "SOURCE.txt") -Encoding UTF8
    Write-Host "GLideN64 extracted to $dest"
} finally {
    Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
