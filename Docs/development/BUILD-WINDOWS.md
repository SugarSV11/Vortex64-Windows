# Compilar Vortex64-Windows (Windows 10/11 x64)

## Requisitos
- Visual Studio 2022 Build Tools (workload "Desarrollo para el escritorio con C++", MSVC 14.4x, Windows SDK)
- CMake >= 3.16, Ninja
- vcpkg (`git clone https://github.com/microsoft/vcpkg` + `bootstrap-vcpkg.bat`) y la variable `VCPKG_ROOT`

## Compilar (GUI Qt6 + CLI + plugins)
Desde "x64 Native Tools Command Prompt" (o tras `vcvars64.bat`):

    set VCPKG_ROOT=C:\ruta\a\vcpkg
    cmake --preset windows-msvc-release -DVORTEX_BUILD_GUI=ON -DVCPKG_MANIFEST_FEATURES=gui
    cmake --build --preset windows-msvc-release

Las dependencias (SDL2, libpng, zlib y, con la feature `gui`, Qt6 base; la primera vez Qt tarda ~25 min) las instala vcpkg automáticamente desde `vcpkg.json`.
Salida: `build/windows-msvc-release/bin/` (`vortex64-cli.exe` + `Vortex64-{rsp,video,audio,input}.dll`).

## Ejecutar
    vortex64-cli.exe "ruta\juego.z64" [--recompiler | --interpreter]
Los datos de usuario van a `%APPDATA%\vortex64` y `%LOCALAPPDATA%\vortex64`.

## Paquete portable
    powershell -File packaging\windows\package.ps1 -Version 0.1.0
Genera `dist/Vortex64-Windows-<version>-x64.zip` (+ `.sha256`) con las DLL de SDL2/libpng/zlib y el runtime de MSVC (app-local).

## Tests
    cmake --build --preset windows-msvc-release --target common-test
    ctest --test-dir build/windows-msvc-release --output-on-failure

## Estado
Ver `Docs/porting/01-PROGRESS.md` (qué está verificado y qué no). La GUI Qt6 corre en Windows (ver Hito 11).
