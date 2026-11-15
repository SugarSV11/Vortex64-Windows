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
