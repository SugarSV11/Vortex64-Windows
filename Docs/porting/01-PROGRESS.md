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
