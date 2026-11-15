# Vortex64-Windows — progreso verificado

## Hito 1: `Common` compila con MSVC x64 y pasa `common-test` (2026-11-14)
Toolchain: VS 2022 Build Tools (MSVC 14.44), CMake 4.4.4, Ninja. Compilado como proyecto aislado (solo `Source/Common`).

Hecho:
- `Source/Common/Windows/`: implementaciones nativas de `MemoryManagement` (VirtualAlloc/Protect/Free), `DynamicLibrary`
  (LoadLibraryW), `CriticalSection`, `SyncEvent` (eventos Win32), `Thread` (CreateThread) + shims `compat/` (dirent, unistd, ...).
- `Platform.h/.cpp`, `path.cpp` (rutas %APPDATA%/%LOCALAPPDATA%, GetModuleFileNameW), `Util.cpp`, `Trace.cpp`,
  `HighResTimeStamp.cpp`, `md5.h`, `File.cpp` adaptados con ramas `_WIN32`.
- `CMakeLists.txt` raíz permite WIN32; `Common/CMakeLists.txt` selecciona fuentes por plataforma.
- `vcpkg.json` y `CMakePresets.json` (sin probar todavía).

Resultado de `common-test.exe`: pasa tras excluir una comprobación en Windows.

Divergencias / pendientes conocidos (no ocultos):
- `CThread::Terminate()` usa `TerminateThread`: no desenrolla la pila (en Linux sí, vía pthread_cancel). Comprobación `g_Unwound`
  desactivada en Windows.
- `Util.cpp`: detección de instancia única (`ExistingExePid`, `TerminateExe`, `RegisterRunningExe`) NO implementada en Windows (stubs).
- `path.cpp`: bloqueo de migración (`flock`) y `fchmod` son no-op en Windows.
- Corrección a la auditoría: las ramas `_WIN32` de `Common` **no** se conservaron en el port Linux; se escribieron nuevas.
- No se ha configurado aún el árbol completo (vcpkg: SDL2/Qt6/libpng), ni compilado core/RSP/plugins/GUI.

## Hito 2: `Vortex64-rsp-core` y `Vortex64-core` compilan y enlazan con MSVC x64 (Debug)
Compilados como proyecto aislado (asmjit, softfloat, 7zip-decoder, zlib, minizip, Common, rsp-core, core, Settings).
**Solo compilación/enlazado; no se ha ejecutado ninguna prueba del núcleo ni emulado nada todavía.**

Cambios:
- `Vortex64-plugin-spec/Base.h`: `EXPORT` = `__declspec(dllexport)` en Windows (`Settings.cpp` igual).
- `RspProfiling.cpp`: `<intrin.h>`, afinidad con `SetThreadAffinityMask`. `RspSystem.cpp`/`x64ops.cpp`: `__builtin_trap` -> `__debugbreak`.
- `RecompilerMemory.cpp`: reserva cercana a RDRAM con `VirtualAlloc(MEM_RESERVE)` en la dirección pedida (alineada a 64 KB).
  **Por verificar en ejecución:** que el JIT consiga la reserva a <=2 GB de RDRAM (rip-relative).
- `AudioPlugin.cpp`: prioridad `THREAD_PRIORITY_TIME_CRITICAL` (en vez de SCHED_FIFO). `Disk.cpp`: `localtime_s`.
- `Common/Windows/compat/cxxabi.h`: `__forced_unwind` ficticio (nunca se lanza; ver divergencia de `Terminate()`).
- SoftFloat: nuevo `build/Win64-MSVC/platform.h` (derivado del de Linux, modificado; sin `__int128` ni `__builtin_clz`, rutas genéricas).
  **Por verificar:** corrección numérica de esa variante con tests.
- `__PRETTY_FUNCTION__` -> `__FUNCSIG__` vía definición de compilación en `Common`.

Pendiente: plugins (video/audio/input/RSP) como DLL, CLI, GUI Qt6, tests del núcleo, ejecución con ROM, CI.

## Hito 3: plugin RSP (`Vortex64-rsp.dll`) compila y enlaza con MSVC x64
Verificado con `dumpbin /exports`: exporta CloseDLL, DllAbout, DoRspCycles, GetDllInfo, InitiateRSP, RomOpen, RomClosed, SetSettingInfo*, etc.
No se ha cargado aún desde el emulador (solo compilación/enlazado/exports).
- `breakpoint.h`: `DWORD` real de `windows.h`; `RSPDebuggerUI.cpp`: `ExitThread` en vez de `pthread_exit`.
- Pendiente: la UI de depuración RSP sigue siendo stubs (como en Linux).
- En curso: vcpkg (SDL2, libpng) para los plugins de vídeo/audio/input.

## Hito 4: plugins de audio (`Vortex64-audio.dll`, SDL2) e input (`Vortex64-input.dll`, SDL2) compilan y enlazan con MSVC
SDL2 2.32.10 vía vcpkg (x64-windows). Solo compilación/enlazado; sin carga ni prueba con dispositivos reales.
- `Common/Windows/compat/dlfcn.h`: dlopen/dlsym/dladdr sobre LoadLibrary (usado por los plugins para cargar el módulo Qt de configuración).
- CMake de audio/input/vídeo: `find_package(SDL2 CONFIG)` en Windows, `pkg-config` en Linux.
- `Win32CompatTypes.h`: en Windows usa `<windows.h>` real (los tipos falsos chocaban).
- El audio usa el backend SDL2 (no DirectSound; sigue disponible en `Driver/DirectSound.cpp` sin integrar).
- Input: `Win32KeyboardScan_Linux.cpp` es un no-op; en Windows no se añaden teclas extra que antes aportaba `GetAsyncKeyState`. Pendiente decidir.
- Pendiente: plugin de vídeo (134 ficheros, GLX/libpng/OpenGL), CLI, GUI Qt6.

## Hito 5: plugin de vídeo (`Vortex64-video.dll`) compila y enlaza con MSVC; los 4 plugins ya son DLL
- `rdp.h`: `DECLAREALIGN16VAR` con `__declspec(align(16))`; `opengl.h`: incluye `<windows.h>` antes de `<GL/gl.h>`.
- `GLXDrawable.cpp`: en Windows devuelve `false` (se usa `SDL_GL_GetDrawableSize`).
- MSVC: `NO_ASM` -> rutas C portables de `3dmath.cpp` (el asm inline era 32-bit/GCC). **Rendimiento no medido.**
- **Sin probar:** creación del contexto OpenGL, `SDL_CreateWindowFrom(HWND)` embebido, ningún renderizado.
- No incluido: GLideN64 (desactivado en presets; hace falta su capa Win32/WGL o SDL2).
- Pendiente: CLI, árbol CMake completo con presets (hasta ahora se compila con un proyecto de prueba aparte), GUI Qt6, ejecución con ROM.

## Hito 6: la CLI arranca en Windows y carga los 4 plugins (2026-11-14)
Build completo con `cmake --preset windows-msvc-debug` (MSVC + Ninja + vcpkg manifest, sin Qt; Qt es la feature opcional `gui`).
`bin/vortex64-cli.exe` + `Vortex64-{rsp,video,audio,input}.dll` + SDL2d/libpng/zlib junto a él.

Prueba realizada (con una ROM FALSA de 4 MB de ceros y cabecera z64, no es un juego):
- `AppInit` OK, ajustes/RDB cargados, plugin de vídeo cargado, ROM cargada, emulación iniciada.
- El plugin de vídeo crea la ventana SDL2/OpenGL en una AMD Radeon RX 9060 XT (GL 4.6 compat).
- La emulación termina sola en `MemoryVirtualMem.cpp:226` (acceso a dirección inválida): esperado, el "juego" son ceros sin código
  de arranque. **No demuestra emulación correcta**: falta probar con una ROM real (la aportará el usuario).

Bug real encontrado y corregido: `Common/path.cpp` solo entendía rutas Linux (absoluta = empieza por '/'; anteponía '/' a `C:/...`),
lo que dejaba sin crear el INI de ajustes y provocaba un acceso nulo en `CIniFile` (crash). Ahora reconoce letras de unidad y los
buffers de ruta son mayores (antes 260). Añadido manejador de crashes con pila simbolizada en la CLI (usa el .pdb).

Pendiente: probar con ROM real (CPU/RSP/vídeo/audio/entrada), JIT (`--recompiler`), GUI Qt6, CI, paquete portable.
