# Vortex64-Windows — Auditoría técnica del repositorio original

Repositorio auditado (solo lectura): `SugarSV11/Vortex64`, commit `1f30d21` (HEAD de `main`, clon superficial).
Este documento cubre la FASE 1. Todo lo afirmado procede de inspeccionar el árbol; lo marcado **[por verificar]**
requiere compilar/ejecutar y aún no se ha comprobado.

## 0. Hallazgo clave

Vortex64 **no es un emulador escrito desde cero para Linux**: es un port a Linux de **Project64** (GPLv2), que nació
en Windows. Por tanto:

- El núcleo (`Common`, `Vortex64-core`, `Vortex64-rsp-core`) conserva ramas `_WIN32` de upstream y, además, ramas POSIX
  añadidas por el port. Portar a Windows consiste sobre todo en **reactivar/reescribir la capa de plataforma**, no en
  tocar la lógica de emulación.
- La GUI (`Vortex64-gui`, Qt6) y los plugins (vídeo/audio/input vía SDL2) ya son multiplataforma en lo esencial; los
  puntos que atan a Linux están acotados (ver §3).
- **Estado de la máquina de desarrollo actual:** no hay MSVC, ni clang-cl, ni Ninja, ni Qt6, ni SDL2 instalados
  (solo CMake 4.4.4, Git, Python, MSYS shell sin gcc). **Nada de lo de abajo se ha compilado aún.**

## 1. Arquitectura actual

| Módulo (`Source/…`) | Ficheros | Función |
|---|---|---|
| `Common` | 39 | Capa de plataforma: hilos, secciones críticas, eventos, memoria (reserva/commit), `DynamicLibrary`, rutas, INI, log, MD5 |
| `Vortex64-core` | 243 | R4300i (intérprete + recompilador x86/x64/ARM/AArch64), TLB, memoria N64 + handlers MMIO, saves (SRAM/EEPROM/Flash), RomList, carga de ROM (.z64/.n64/.v64, zip/7z), cheats/enhancements, carga de plugins, settings |
| `Vortex64-rsp-core` | 75 | RSP: intérprete y recompilador, HLE de audio |
| `Vortex64-rsp` | 12 | Plugin RSP (DLL/so) + depurador RSP |
| `Vortex64-video` | 134 | Plugin de vídeo propio (HLE RDP, OpenGL "glide wrapper"), contexto vía SDL2 |
| `GLideN64` (+ `GLideN64-linux`) | 535 + 17 | Plugin de vídeo alternativo, vendorizado; `-linux` es la capa de plataforma SDL2/GLX |
| `Vortex64-audio` | 19 | Plugin de audio; drivers `DirectSound`, `OpenSLES`, **`SDLAudio`** |
| `Vortex64-input` | 33 | Plugin de entrada; `SdlInputBackend` (SDL2), escaneo de teclado Win32/Linux |
| `Vortex64-gui` | 148 | Frontend Qt6 Widgets: ventana principal, ROM browser, ajustes, trucos, depurador, scripts JS (Duktape), save states |
| `Vortex64-cli` | 3 | Frontend de línea de comandos |
| `Vortex64-ui` | 58 | Árbol WTL/ATL original de Project64; **no se compila** (sustituido por la GUI Qt) |
| `Settings`, `Vortex64-plugin-spec` | 2 / 6 | Sistema de settings; cabeceras de la ABI de plugins (Zilmar) |
| `3rdParty` | 846 | asmjit, softfloat-3e, 7zip(+decoder), zlib+minizip, libpng, duktape, discord-rpc |

Los plugins son **bibliotecas dinámicas cargadas en tiempo de ejecución** con ABI estilo Zilmar (`DynamicLibrary`).
Flujo: `Vortex64-gui`/`-cli` → `Vortex64-core` (CPU/memoria) → plugins RSP/vídeo/audio/input.

Build actual: `CMakeLists.txt` raíz con `if(NOT UNIX) message(FATAL_ERROR …)`, 14 subdirectorios, ~25 ejecutables de test
(`EXCLUDE_FROM_ALL`, ejecutados con `ctest`/target `check`). CI de Linux en `.github/workflows` (AppImage, ubuntu-22.04).

## 2. Componentes reutilizables (sin cambios o con cambios menores)

- **Lógica de emulación completa:** intérprete R4300i, TLB, handlers MMIO, saves, RSP interpreter/recompiler, HLE de
  audio, RDB/Cheats/Enhancements, RomList.
- **Recompilador x86-64 y asmjit:** emiten bytes y ya filtran por `defined(_M_X64)` además de `__amd64__`;
  MSVC es el compilador de referencia de Project64 upstream. El JIT en sí no debería necesitar reescritura; sí la
  asignación de memoria ejecutable (§3).
- **Plugin de vídeo (OpenGL puro), audio SDL2, input SDL2:** SDL2 ya es multiplataforma.
- **GUI Qt6:** reutilizable casi entera.
- **3rdParty** (asmjit, softfloat, zlib, minizip, 7zip-decoder, duktape, png) y todos los avisos de `licenses/`.
- **Tests** que no dependen de X11 (settings, ROM browser con ROMs falsas, plugins, migración, `common-test`).

## 3. Componentes / puntos incompatibles con Windows

Lista de ficheros con APIs Linux (grep fuera de 3rdParty/GLideN64):

| Área | Ficheros | Problema | Plan |
|---|---|---|---|
| Memoria / JIT | `Common/MemoryManagement.cpp`, `Vortex64-core/.../RecompilerMemory.cpp` | `mmap/mprotect/msync/munmap`; semántica `MAP_FIXED` sobre reserva | Reescribir con `VirtualAlloc(MEM_RESERVE/MEM_COMMIT)`, `VirtualProtect`, `VirtualFree`. **Ojo:** `VirtualAlloc` no permite re-mapear sobre una reserva como `MAP_FIXED`; `DecommitMemory` = `MEM_DECOMMIT` |
| Plataforma | `Common/Platform.h` | Incluye `alloca.h`, `strings.h`, define `stricmp`→`strcasecmp`, `_aligned_malloc`→`aligned_alloc`, `__debugbreak`→`raise(SIGTRAP)` | En Windows no incluir; usar CRT de MSVC (`<malloc.h>`, `_aligned_malloc` nativo) |
| Hilos y sync | `Common/Thread.cpp`, `CriticalSection.cpp`, `SyncEvent.cpp` | pthread (upstream ya traía rama `_WIN32`) | Reactivar ramas Win32 (`CreateThread`, `CRITICAL_SECTION`/SRW, `CreateEvent`); verificar contra `common-test` |
| Bibliotecas dinámicas | `Common/DynamicLibrary.cpp`, `Vortex64-core/Plugins/*` | `dlopen/dlsym`, extensión `.so` | `LoadLibraryW/GetProcAddress`, `.dll`; revisar `PluginList.cpp` |
| Rutas | `Common/path.cpp` | `/proc/self/exe` (l. 851), XDG (`$HOME`, `$XDG_*`), `dirent.h` | `GetModuleFileNameW`; `%APPDATA%`/`%LOCALAPPDATA%` (KnownFolders); `FindFirstFileW` o `std::filesystem` |
| Ficheros | `Common/File.cpp` | Verificar `open/stat` POSIX, UTF-8 vs UTF-16 | Convertir a UTF-16 en la frontera Win32 |
| Señales / fallos de memoria | `Vortex64-core/.../MemoryVirtualMem.h` (`<signal.h>`, `<sys/ucontext.h>` bajo `__arm__`) | No se encontró manejador SIGSEGV en el núcleo x86-64 **[por verificar si el JIT depende de fallos de página]** | Si lo hubiera: `AddVectoredExceptionHandler` |
| Ventana de vídeo | `Vortex64-gui/MainWindow.cpp/h`, `Vortex64-video/Renderer/OGLglitchmain.cpp`, `GLideN64-linux/WindowsWGL_sdl.cpp` | Widget nativo X11 + `SDL_CreateWindowFrom` | En Windows: `QWidget::winId()` = `HWND`; `SDL_CreateWindowFrom(hwnd)` funciona con SDL2 en Windows. **[por verificar]** con OpenGL |
| GUI específica Linux | `Vortex64-gui/CMakeLists.txt` (`find_package(X11)`, `Qt6::DBus`, `/usr/include/SDL2`, `:libXtst.so.6`), `ScreenSaverInhibit.cpp`, `main.cpp`, `QtNotification.cpp` | X11, DBus (inhibir salvapantallas, notificaciones), rutas codificadas | Condicionar por plataforma; `SetThreadExecutionState` para inhibir suspensión; quitar DBus/X11 |
| Tests de GUI | `Vortex64-gui/tests/input_*_x11*` y similares | XTest/X11 | Excluir en Windows; sustituir por tests Qt |
| Plugins GLideN64 | `GLideN64-linux/*`, `Config_linux`, `GLXSize` | GLX, `MemoryStatus_linux` | Sobre Windows se usaría la capa Win32/WGL de GLideN64 upstream (`Source/GLideN64/Source/src/Windows`) o SDL2 reutilizado. **Fase posterior; opcional (`-DVORTEX_GLIDEN64=OFF` al inicio)** |
| CLI | `Vortex64-cli/main.cpp`, `LinuxNotification.cpp` | POSIX (`unistd.h`, signals) | Port menor |
| ScriptShim | `Vortex64-gui/Debugger/ScriptShim/windows.h` | Cabecera **falsa** de `windows.h` para compilar el script engine en Linux | **Peligroso en Windows**: sombrea el `<windows.h>` real. Eliminar/renombrar y usar el real |
| Win32 compat | `Vortex64-input/Win32CompatTypes.h`, `Win32KeyboardScan_Linux.cpp` | Tipos Win32 simulados; scancodes Linux | Usar tipos reales y `Win32KeyboardScan.cpp` original |
| Audio | `Vortex64-audio/Driver/DirectSound.cpp` | Existe backend nativo Windows (heredado) | Opción de baja latencia; SDL2 primero |
| Build | `CMakeLists.txt` raíz y de cada módulo | `NOT UNIX → FATAL_ERROR`, `-ffile-prefix-map`, `pkg_check_modules(SDL2)`, `find_package(PNG)` del sistema, `Threads`, `CMAKE_DL_LIBS`, `GNUInstallDirs` con symlinks `/usr/bin`, 36 includes con backslash ya corregidos | Reescribir condicionales; usar vcpkg para SDL2/Qt6/PNG |
| CI/Packaging | `.github/workflows/*.yml`, `packaging/linux` | AppImage/ubuntu | Nuevo workflow `windows-latest` + `windeployqt` |
| Rich presence | `3rdParty/discord-rpc` | Falta o está parcial; en Windows hay `connection_win.cpp` upstream | Opcional; desactivable |

Puntos que **no** son problema: no hay Direct3D; no hay registro de Windows en el código del port; JIT ARM/AArch64 se
descarta en x64 sin impacto.

## 4. Dependencias necesarias en Windows

- **Compilador:** MSVC (VS 2022 Build Tools, workload C++ desktop, Windows SDK). Clang-cl opcional.
- **CMake ≥ 3.16** (hay 4.4.4) y **Ninja**.
- **Qt6** (Widgets; sin DBus). Vía vcpkg (`qtbase`) o instalador oficial. Despliegue con `windeployqt`.
- **SDL2** (vcpkg `sdl2`); **libpng** y **zlib** (vcpkg o los vendorizados ya presentes).
- **OpenGL:** `opengl32` del sistema; cargador de extensiones vía `SDL_GL_GetProcAddress`.
- Opcionales: FreeType (solo GLideN64), discord-rpc.
- **vcpkg** como gestor reproducible (manifiesto `vcpkg.json` con baseline fijada).
- Licencias de dependencias ya recogidas en `licenses/`; revisar cada nueva (Qt6 LGPLv3 → enlace dinámico obligatorio).

## 5. Cambios requeridos (resumen)

1. `CMakeLists.txt`: permitir `WIN32`, crear `cmake/` con módulos de plataforma, `CMakePresets.json`, `vcpkg.json`.
2. Capa `Common`: reimplementar `MemoryManagement`, `path` (módulo y carpetas de usuario), `DynamicLibrary`, verificar
   `Thread/CriticalSection/SyncEvent`, `Platform.h` por plataforma, `File` (UTF-16).
3. Núcleo: comprobar warnings/errores MSVC (el port de Linux introdujo `#ifdef` que pueden haber roto ramas Win32).
4. Plugins: sufijo `.dll`, exportación `__declspec(dllexport)`/`.def` según ABI Zilmar (`__stdcall` en x86; en x64 la
   convención es única).
5. GUI: quitar X11/DBus, adaptar embebido de vídeo (`winId`/`HWND`), inhibidor de suspensión, rutas.
6. Tests: portar la suite headless (`QT_QPA_PLATFORM=offscreen`) a ctest en Windows.
7. CI y empaquetado portable.

## 6. Riesgos técnicos

| Riesgo | Impacto | Mitigación |
|---|---|---|
| El port de Linux alteró código que antes compilaba con MSVC (ifdefs, includes) | Medio | Compilar pronto, corregir ramas |
| Semántica de memoria del JIT: `MAP_FIXED` sobre reserva vs `VirtualAlloc`; mapeo de RDRAM en espejo | **Alto** | Prueba unitaria específica de `MemoryManagement` y fuzz de JIT (`jit-fuzz-test` existente) |
| Fallos de página / excepciones usadas por el JIT para accesos a memoria | Alto **[por verificar]** | Buscar dependencias del handler; vectored exception handler |
| Embebido SDL+OpenGL en widget Qt (focus, redimensionado, DPI, doble búfer con DWM) | Medio | Prototipo temprano |
| Convención de ABI de plugins (`__stdcall` x86 vs x64) | Bajo en x64 | Solo x64 |
| Ramas con tipos Win32 sustituidos (`Win32CompatTypes.h`, `ScriptShim/windows.h`) chocan con los reales | Medio | Aislar por `#if !defined(_WIN32)` |
| GLideN64 en Windows: configuración (GLideNUI) y WGL | Medio | Fase posterior |
| Rutas > 260 caracteres, Unicode, OneDrive | Bajo | UTF-16 + `\\?\` si hace falta |
| Toolchain ausente en la máquina actual | Bloqueante hasta instalar | Instalar VS Build Tools + Ninja + vcpkg (descarga de varios GB) |
| Calidad "no verificada" de parte del propio original (los docs admiten funciones que solo mostraban `QMessageBox`; audio/input sin validar en hardware) | Medio | Contrastar con `Docs/dev/PROJECT64_PARITY.md`; no heredar afirmaciones sin probar |

## 7. Plan de implementación ordenado

1. **Preparación (hecho):** copia independiente, `git init` nuevo, sin remoto. Licencias y avisos conservados.
2. **Toolchain:** instalar VS 2022 Build Tools, Ninja, vcpkg; `vcpkg.json` (sdl2, qtbase, libpng, zlib).
3. **CMake Windows:** presets `windows-msvc-{debug,release}`, quitar `FATAL_ERROR`, condicionar X11/DBus/pkg-config.
4. **Common compila y pasa `common-test`** (primer hito verificable).
5. **`Vortex64-rsp-core` y `Vortex64-core` compilan en MSVC;** recompilador x64 + `jit-fuzz-test`.
6. **Plugins** vídeo/audio/input/RSP como DLL; carga con `LoadLibrary`.
7. **CLI** arranca y carga una ROM de prueba (header-only para tests; ROM legal del usuario para humo).
8. **GUI Qt6** en Windows; embebido de vídeo; inhibidor de suspensión; rutas de usuario.
9. **Suite de tests en ctest + GitHub Actions `windows-latest`.**
10. **GLideN64** (opcional), DirectSound como alternativa de audio, optimización del JIT, empaquetado portable
    (`windeployqt`, ZIP) y documentación.

## 8. Reglas de trabajo aplicadas

- `SugarSV11/Vortex64` solo se ha **leído** (clon en el directorio temporal de la sesión). La copia de trabajo es
  `Vortex64-Windows`, con `git init` propio y sin remoto configurado.
- No se ha subido nada a GitHub. Crear/empujar a `SugarSV11/Vortex64-Windows` requiere tu confirmación.
- `LICENSE` (GPLv2), `THIRD_PARTY_NOTICES.md` y `licenses/` se mantienen intactos.
