<#
  Vortex64-Windows: builds a portable ZIP from a finished Release build.

  Usage (from the repository root, after `cmake --preset windows-msvc-release` and `cmake --build --preset windows-msvc-release`):
      pwsh packaging/windows/package.ps1 [-BuildDir build/windows-msvc-release] [-OutDir dist] [-Version dev]

  The ZIP is app-local: it carries the MSVC runtime DLLs (Microsoft.VC143.CRT) so no redistributable has to be installed.
#>
param(
    [string]$BuildDir = "build/windows-msvc-release",
    [string]$OutDir = "dist",
    [string]$Version = "dev"
)
$ErrorActionPreference = "Stop"
$root = Resolve-Path (Join-Path $PSScriptRoot "../..")
$bin = Join-Path $root "$BuildDir/bin"
if (-not (Test-Path (Join-Path $bin "vortex64-cli.exe"))) { throw "vortex64-cli.exe not found in $bin - build the Release preset first." }

$name = "Vortex64-Windows-$Version-x64"
$stage = Join-Path $root "$OutDir/$name"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force $stage | Out-Null

# executables and plugins (no .pdb/.ilk/.exp/.lib; no per-user runtime folders)
Get-ChildItem $bin -File | Where-Object { $_.Extension -in ".exe", ".dll", ".html" } | Copy-Item -Destination $stage
foreach ($d in "Config", "Lang", "Scripts", "platforms", "styles", "imageformats") {
    if (Test-Path (Join-Path $bin $d)) { Copy-Item (Join-Path $bin $d) (Join-Path $stage $d) -Recurse }
}

# MSVC runtime (app-local deployment)
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$crt = $null
if (Test-Path $vswhere) {
    $vs = & $vswhere -latest -products * -property installationPath
    if ($vs) { $crt = Get-ChildItem "$vs\VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT" -Directory -ErrorAction SilentlyContinue | Sort-Object FullName | Select-Object -Last 1 }
}
if ($crt) { Get-ChildItem $crt.FullName -Filter *.dll | Copy-Item -Destination $stage } else { Write-Warning "MSVC runtime not found; the target PC needs the VC++ 2015-2022 x64 redistributable." }

# GLideN64 (official upstream build, fetched by fetch-gliden64.ps1; GPL-2.0-or-later: licence and source pointer travel with it)
$gl = Join-Path $root "third_party_bin/GLideN64"
if (Test-Path (Join-Path $gl "GLideN64.dll")) {
    Copy-Item (Join-Path $gl "GLideN64.dll") $stage
    Copy-Item (Join-Path $gl "GLideN64.custom.ini") $stage -ErrorAction SilentlyContinue
    if (Test-Path (Join-Path $gl "translations")) { Copy-Item (Join-Path $gl "translations") (Join-Path $stage "translations") -Recurse }
    Copy-Item (Join-Path $gl "SOURCE.txt") (Join-Path $stage "GLideN64-SOURCE.txt") -ErrorAction SilentlyContinue
    Write-Host "GLideN64 included"
} else { Write-Host "GLideN64 not included (run packaging/windows/fetch-gliden64.ps1 first to include it)" }

# licences (GPLv2 requires them to travel with the binaries)
foreach ($f in "LICENSE", "THIRD_PARTY_NOTICES.md", "README.md") { Copy-Item (Join-Path $root $f) $stage }
Copy-Item (Join-Path $root "licenses") (Join-Path $stage "licenses") -Recurse
@"
Vortex64-Windows $Version (x64)
GUI (library, settings, cheats, debugger):   vortex64.exe   (only present if the build had VORTEX_BUILD_GUI=ON)
Command line:                                vortex64-cli.exe "path\to\game.z64" [--recompiler|--interpreter]
No ROMs are included; use your own legally obtained ROMs.
User data (settings, saves, logs) goes to %APPDATA%\vortex64 and %LOCALAPPDATA%\vortex64.
Source code (GPLv2): https://github.com/SugarSV11/Vortex64-Windows
"@ | Set-Content (Join-Path $stage "README-WINDOWS.txt") -Encoding UTF8

$zip = Join-Path $root "$OutDir/$name.zip"
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path $stage -DestinationPath $zip
$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
"$hash  $name.zip" | Set-Content (Join-Path $root "$OutDir/$name.zip.sha256")
Write-Host "Created $zip ($([math]::Round((Get-Item $zip).Length/1MB,1)) MB)"
Write-Host "SHA256 $hash"
