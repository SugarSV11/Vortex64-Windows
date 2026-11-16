<div align="center">

# 🌀 Vortex64 for Windows

**Emulador de Nintendo 64 nativo para Windows 10/11 (x64)** · *Native Nintendo 64 emulator for Windows*

![Plataforma](https://img.shields.io/badge/plataforma-Windows%2010%20%7C%2011%20x64-0078D4?logo=windows&logoColor=white)
![Compilador](https://img.shields.io/badge/MSVC-C%2B%2B17-5C2D91)
![GUI](https://img.shields.io/badge/GUI-Qt%206-41CD52?logo=qt&logoColor=white)
![Licencia](https://img.shields.io/badge/licencia-GPL--2.0-blue)

[**⬇ Descargar**](../../releases/latest) · [Compilar](Docs/development/BUILD-WINDOWS.md) · [Estado del port](Docs/porting/01-PROGRESS.md) · [Paridad con Linux](Docs/porting/02-PARITY.md)

</div>

---

## ✨ ¿Qué es?

**Vortex64-Windows** es el port a **Windows** de [Vortex64](https://github.com/SugarSV11/Vortex64), un emulador de Nintendo 64 creado por **SugarSV11** para Linux.
Este repositorio lo lleva a Windows con **MSVC x64, CMake, vcpkg y Qt 6**, sin Wine ni capas de compatibilidad: es un ejecutable nativo.

> Necesitas tus **propias ROMs**. Este proyecto no incluye ni distribuye ninguna ROM ni contenido de Nintendo.

## 🎮 Características

- **Núcleo MIPS R4300i** con intérprete y **recompilador JIT x64** (verificado contra el intérprete con miles de programas aleatorios: 0 diferencias)
- **RSP** y TLB, guardados (EEPROM/SRAM/Flash) y **save states**
- **Vídeo**: plugin propio OpenGL/SDL2 embebido en la ventana, y **GLideN64** (binario oficial) incluido en el paquete
- **Audio** SDL2 y **entrada** por teclado y mando (SDL2)
- **Interfaz Qt 6**: biblioteca de ROMs, favoritos, ajustes, trucos y mejoras, pantalla completa, "siempre visible", instancia única
- **Depurador** con ventanas y **scripts JavaScript** (sistema de archivos, procesos, sockets, temporizadores)
- ROMs `.z64`, `.n64`, `.v64`, y dentro de `.zip` / `.7z`
- Interfaz en español e inglés

## 🆕 Novedades 1.1.0

- **Sin ventana de CMD**: `vortex64.exe` es ahora una aplicación de Windows.
- **GLideN64 siempre en la lista** de plugins de vídeo (la compilación lo deja junto al `.exe`).
- **Teclas con su nombre real** en la configuración de controles (antes W salía como "8", A como "U"...). Las teclas guardadas con la 1.0 se corrigen solas la primera vez.
- **SM64 Living ROM** ([hack beta](https://github.com/SugarSV11/SM64-Living-ROM)): Vortex64 ya la reconoce y la abre sin configurar nada (EEPROM de 16 Kbit). Ese hack solo funciona en Vortex64.

## 🚀 Instalación

1. Descarga **`Vortex64-Windows-x64.zip`** desde [Releases](../../releases/latest).
2. Descomprímelo donde quieras (es portable; incluye el runtime de Visual C++, no hay que instalar nada).
3. Ejecuta **`vortex64.exe`**.

También hay una versión de línea de comandos:

```powershell
vortex64-cli.exe "C:\ruta\juego.z64" --recompiler   # o --interpreter
```

Tus ajustes y partidas se guardan en `%APPDATA%\vortex64` y `%LOCALAPPDATA%\vortex64`.

## 🛠 Compilar desde el código

Requisitos: Visual Studio 2022 Build Tools (MSVC 14.4x + Windows SDK), CMake, Ninja y [vcpkg](https://github.com/microsoft/vcpkg).

```bat
set VCPKG_ROOT=C:\ruta\a\vcpkg
cmake --preset windows-msvc-release -DVORTEX_BUILD_GUI=ON -DVCPKG_MANIFEST_FEATURES=gui
cmake --build --preset windows-msvc-release
```

Detalles en [`Docs/development/BUILD-WINDOWS.md`](Docs/development/BUILD-WINDOWS.md). Para generar el ZIP portable:
`powershell -File packaging\windows\package.ps1 -Version 1.1.0`.

## ✅ Pruebas

Más de 120 pruebas automáticas (núcleo, RSP, CLI, recompilador contra intérprete, GUI, ajustes, plugins, scripts JS, ventana principal),
todas pasando en Windows. Ver [`Docs/development/TESTING.md`](Docs/development/TESTING.md).

## 📊 Estado honesto

Verificado con Super Mario 64 (USA): título, demo y menús a velocidad completa (60 VI/s; hasta ~9x sin limitador), audio con señal, teclado,
save states, pantalla completa. **Sin probar todavía**: mando, SRAM/FlashRAM en un juego real, aplicar trucos, Discord Rich Presence,
ROMs `.7z`, pantallas de DPI alto. No hay instalador (solo ZIP portable). El detalle completo está en
[`Docs/porting/02-PARITY.md`](Docs/porting/02-PARITY.md).

## 📜 Licencia y créditos

Software libre bajo la **GNU GPL v2** ([`LICENSE`](LICENSE)). Port del Vortex64 original para Linux, de SugarSV11. Los ficheros que conservan cabeceras de copyright de terceros mantienen sus avisos (la GPL lo exige); usa GLideN64, AsmJit,
SDL2, Duktape, Discord RPC, 7-Zip C SDK, zlib/minizip, Berkeley SoftFloat y Qt 6. Atribución completa en
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) y [`licenses/`](licenses/). El README de la versión Linux original está en
[`Docs/README-LINUX-ORIGINAL.md`](Docs/README-LINUX-ORIGINAL.md).
Nintendo 64 es marca de Nintendo; este proyecto no está afiliado a Nintendo.

---

<sub>**English:** Vortex64-Windows is the native Windows (x64, MSVC, Qt 6) port of the Vortex64 N64 emulator by SugarSV11 (x64 JIT recompiler, SDL2/OpenGL,
GLideN64 bundled). Download the portable ZIP from Releases, extract and run `vortex64.exe`; bring your own ROMs. GPLv2.</sub>
