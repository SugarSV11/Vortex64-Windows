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

## Hito 7: Super Mario 64 (USA) corre en Windows con intérprete y con JIT (2026-11-15)
ROM aportada por el usuario (8 MB, .z64; no se incluye en el repositorio). Build Debug, MSVC x64, GPU AMD Radeon RX 9060 XT.
Verificado con capturas de la ventana del emulador y logs:
- `--interpreter`: pantalla de título (cabeza de Mario) renderizada; 40 s estable.
- `--recompiler` (JIT x64): modo demo (Bowser, "PRESS START") renderizado; 60 s estable.

Bug real del JIT corregido: `X64Abi::Arg0..Arg3` estaban fijados a la convención System V (rdi, rsi, rdx, rcx). En Windows x64
son rcx, rdx, r8, r9: el JIT llamaba a funciones miembro con `this` erróneo -> acceso a 0xFFFF...FFFF en `LW_VAddr32`.
Ahora dependen de `_WIN32` (`x64ops.h`). El shadow space de 32 bytes ya estaba reservado en las llamadas.

NO verificado todavía: audio (no se ha comprobado que suene), entrada (teclado/mando), juego más allá de la demo,
rendimiento (Debug, sin medir), save states/SRAM, otros juegos, ni compilación Release.

## Hito 8: audio y teclado verificados con SM64 (Debug, JIT) (2026-11-15)
- **Audio (SDL2):** el proceso crea una sesión de audio de Windows y su medidor de pico (IAudioMeterInformation) marca 0.83 de máximo,
  con valor > 0.001 en 60/60 muestras durante 15 s. Confirma que se emite señal; **no** valora calidad, latencia ni sincronización
  (no se ha escuchado ni medido).
- **Teclado:** al enviar Enter (Start por defecto, scancode 28) al emulador, SM64 pasa de la pantalla de título a "SELECT FILE"
  (capturas antes/después). Confirma la ruta de entrada por teclado hasta el juego.
- **Pendiente:** mando (SDL GameController) sin probar por no haber mando conectado en la prueba; resto de botones/analógico; latencia.

## Hito 9: build Release y velocidad (2026-11-15)
`cmake --preset windows-msvc-release` + build: compila y enlaza completo (664 pasos).
Velocidad medida con "Display Frame Rate" (VI/s reportados por el núcleo), SM64 (USA), AMD RX 9060 XT, ventana por defecto:

| Build | CPU | VI/s (muestras) |
|---|---|---|
| Release | JIT | 59.5 - 63.9 (~60) |
| Debug | JIT | 59.1 - 60.8 (~60) |
| Release | intérprete | 58.8 - 60.9 (~60) |
| Debug | intérprete | 58.4 - 59.9 (~60) |

Los cuatro casos van a velocidad completa (60 VI/s) en esta escena (pantalla de título/demo); el limitador de fotogramas satura la
medida, así que **no** hay datos de margen de rendimiento ni de escenas pesadas. Para medirlo hay que desactivar el limitador y
probar escenas de juego. Pendiente: optimización, perfilado, otros juegos.

## Hito 10: CI y paquete portable (2026-11-15)
- `packaging/windows/package.ps1`: genera `dist/Vortex64-Windows-<ver>-x64.zip` (3.5 MB) + SHA256 con exe, 4 plugins, SDL2/libpng/zlib,
  runtime MSVC app-local, Config/Lang/Scripts, LICENSE, THIRD_PARTY_NOTICES y licenses/.
  **Verificado:** el ZIP extraído en otra carpeta ejecuta SM64 con el JIT a ~60 VI/s.
- `.github/workflows/windows.yml` (windows-latest, MSVC, vcpkg, build Release, common-test, empaquetado, artefacto y release en tags).
  **NO ejecutado:** el repositorio remoto no existe aún, así que el workflow no se ha probado en GitHub.
- Los workflows Linux heredados quedan solo manuales (`workflow_dispatch`).
- `vcpkg.json` no fija `builtin-baseline`: las versiones de dependencias no son reproducibles todavía.

## Hito 11: GUI Qt6 (`vortex64.exe`) corre en Windows (2026-11-15)
Qt 6.11.3 (vcpkg, feature `gui`: qtbase widgets/gui). Build: `cmake --preset windows-msvc-release -DVORTEX_BUILD_GUI=ON -DVCPKG_MANIFEST_FEATURES=gui`.
Verificado con capturas:
- Sin argumentos: biblioteca de juegos (Biblioteca/Recientes/Favoritos/64DD, Ajustes, Debugger, Acerca de, búsqueda, vistas).
- Con la ROM: menús (Archivo, Emulación, Ver, Configuración, Herramientas, Ayuda), barra de estado "Emulando ... VI/s: 60.30 · JIT"
  y el **vídeo embebido en la ventana de Qt** (HWND del widget nativo -> `SDL_CreateWindowFrom`), sin ventana SDL aparte.
- El ZIP portable (27.8 MB) con GUI + plugins de Qt (platforms/styles/imageformats) arranca desde una carpeta limpia y carga la ROM.

Cambios: `main.cpp`/`MainWindow.cpp` sin X11 (HWND, `SetWindowPos` para "siempre encima"), `ScreenSaverInhibit_win.cpp`
(`SetThreadExecutionState`), `stdafx.h` con `windows.h` real, `Common/Windows/compat/vortex_msvc_compat.h` (forced include MSVC),
exportación de datos entre plugins y módulos Qt de configuración (`PJ64_PLUGIN_DATA`, `WINDOWS_EXPORT_ALL_SYMBOLS`),
discord-rpc con sus ficheros Windows, motor JS del depurador con `ScriptShimWin` (hooks de PostMessage/MessageBox).

NO verificado todavía: ventanas de Ajustes y de configuración de plugins (Qt) abiertas en uso real; trucos; depurador y scripts JS
(compilan, sin probar); Discord RPC; inhibidor de suspensión; fullscreen; DPI alto; Debug build de la GUI (plugins debug de Qt);
los tests de la GUI de Linux (X11/XTest) no se construyen en Windows. Avisos de Qt en stderr: "QImage::scaled: Image is a null image"
(posible icono/imagen que no carga: pendiente de investigar). `ScriptShimTerminateProcesses` es un stub (los procesos de `exec()` no se
terminan al cerrar). Instancia única sigue sin implementar en Windows.

## Hito 12: ventanas de Ajustes y de configuración de plugins verificadas (2026-11-15)
Probado con el ejecutable Release: abrir Ajustes (General, Gráficos, Audio, Input, RSP, Plugins, Atajos, 64DD, Avanzado) y pulsar
"Configurar..." de los plugins de vídeo, audio e input (módulos Qt cargados con LoadLibrary; capturas con PrintWindow):
- Vídeo: "Glide64 settings" (resolución en ventana, vsync, OpenGL, FBO...).
- Audio: "Settings" (volumen, silencio).
- Input: asignación de teclas/mando de los 4 controles (Start = Intro, A = X, B = C, Z = Z, stick = teclado numérico...).

**Bug real encontrado y corregido:** el shim `Common/Windows/compat/dirent.h` buscaba con el patrón `bin*` en vez de `bin\*`
(se perdió una barra), de modo que `opendir/readdir` devolvía solo 1 entrada. Efecto visible: los selectores de plugin de la GUI
salían vacíos ("Ningún plugin seleccionado"); también afectaba a cualquier enumeración de ficheros/carpetas (`CPath::FindFirst`).
Tras el arreglo la lista muestra Project64 Video/Audio/Input y RSP con sus DLL.

Sigue sin verificar: cambiar plugin/ajustes y que se apliquen y persistan, escaneo de carpetas de ROMs en la biblioteca (probable que
fallara por el mismo bug; hay que reprobar), trucos, depurador, scripts JS, pantalla completa, mando, DPI alto.

## Hito 13: biblioteca de juegos verificada (2026-11-15)
Con una carpeta de ROMs configurada (clave `directories` de `%LOCALAPPDATA%\vortex64\vortex.json`; la copia temporal de la ROM se borró y el fichero
de configuración del usuario se restauró), la GUI Release:
- escanea la carpeta y muestra la tarjeta "Super Mario 64 (USA)" con región NTSC-U, 8 MB y formato .z64;
- al seleccionarla y pulsar "Iniciar" arranca el juego embebido en la ventana (barra de estado: Emulando, VI/s 60.8, JIT).
Nota: el escaneo no es recursivo por defecto (`RomList_GameDirRecursive` = false), igual que en el original.

Observación sin resolver (impacto bajo): `%APPDATA%\vortex64\Project64.cfg` apareció una vez con una línea suelta `n` tras la sección [Plugin]
(restos al acortarse el texto de las versiones de plugin Debug -> Release); posible fallo del acortado en `CIniFileBase::fInsertSpaces`/escritura.
No reproducido en una prueba aislada; pendiente.
Sin verificar aún: portadas (se ve un marcador de posición), favoritos, filtros/ordenación, vista de lista, zip/7z como ROM.

## Hito 14: batería de pruebas automáticas (2026-11-15)
Ver `Docs/development/TESTING.md`. `ctest`: 9/9 pruebas (common, core, rsp, 2x cli-boot, 4x jit-fuzz) + `jit-fuzz-directed` (PASS, 2 min 48 s).
- Diferencial intérprete-vs-JIT: 4 configuraciones x 300 programas aleatorios + programas dirigidos: **0 diferencias** en Windows.
- Nuevas: `Tests/core_test.cpp` (48 comprobaciones), `Tests/rsp_test.cpp` (57 comprobaciones), `Tests/cli_boot.cmake`.
- Las pruebas detectan regresiones: al reintroducir el bug de `dirent`, `core` falla.
- `jit-fuzz-test` (de la suite Linux) portado a Windows sin cambios de lógica (sin X11).
- El supuesto erróneo de una de mis pruebas (el RSP no respeta un presupuesto de ciclos con bucle infinito) se corrigió: `DoRspCycles` no mira el bit de halt.
- El INI acortado no deja restos en la prueba aislada (la línea `n` del Project64.cfg real sigue sin explicación).

## Mando (2026-11-15): NO verificado - no hay ningún mando conectado
SDL (`SDL_NumJoysticks`) y el listado de dispositivos de Windows no detectan ningún mando en esta máquina, así que el camino
SDL GameController -> plugin de entrada -> juego **no se ha podido probar**. Añadida la herramienta manual `gamepad-probe`
(`cmake --build --preset windows-msvc-release --target gamepad-probe`; `bin\gamepad-probe.exe [segundos]`), que lista los mandos y
muestra sus eventos; sin mando termina con código 2. Pendiente: conectar un mando, comprobar con la herramienta y luego jugar con él en Vortex64.

## Hito 15: margen de rendimiento medido sin limitador (2026-11-15)
Nuevas opciones de la CLI: `--unlimited` (desactiva el limitador de fotogramas, el "modo básico" y la sincronización por audio, que antes
cap­aban todo a 60 VI/s) y `--perf` (una vez por segundo compara el tiempo emulado, registro Count del CPU a 46.875 MHz, con el tiempo real).
Ojo: el contador "VI/s" del núcleo muestra `-.--` sin limitador, por eso se usa Count.

SM64 (USA), build Release, AMD RX 9060 XT, ventana por defecto, 36 muestras de 1 s por modo (pantalla de título + modo demo):

| CPU | mínimo | media | máximo | muestras < 1x |
|---|---|---|---|---|
| JIT (`--recompiler`) | 2.01x | 2.47x | 3.91x | 0 |
| Intérprete (`--interpreter`) | 2.02x | 2.44x | 3.47x | 0 |

(1x = 60 VI/s, velocidad real de la consola.) Una pasada anterior mostró caídas puntuales (0.5x-0.75x en alguna muestra, intérprete y JIT) y picos de hasta 4.7x:
la variación entre pasadas es grande y no se ha repetido de forma controlada.

Lectura honesta: en esta escena el JIT **no** supera al intérprete, lo que indica que el límite no es la emulación de CPU sino
otra parte (vídeo HLE/OpenGL, sincronización de ventana o el propio bucle). No se puede concluir nada sobre el beneficio del JIT con esta escena;
tampoco hay datos de escenas de juego pesadas (solo título/demo), de otros juegos ni de la GPU/CPU usadas. Pendiente: perfilado real (VTune/ETW/Superluminal)
y escenas pesadas antes de optimizar.

Aclaración sobre la línea `n` del `Project64.cfg`: no se reproduce acortando varias claves seguidas en una prueba aislada (290 bytes correctos);
lo más probable es que apareciera porque yo terminaba el proceso de la GUI con `Stop-Process -Force` mientras escribía el fichero (la escritura del INI no es atómica). Sin confirmar.

## Hito 16: perfilado (2026-11-15)
Nuevo en la CLI: `--profile=N` (muestreador propio: suspende cada hilo ~1000 veces/s, anota RIP y recorre la pila del hilo más ocupado; símbolos
por .pdb, que ahora se generan también en Release) y `--perf` (velocidad emulada/real con el registro Count). SM64 (USA), Release, RX 9060 XT.

Hallazgo: con `--unlimited` pero vsync del plugin de vídeo activo (por defecto, intervalo de intercambio -1 = adaptativo), el hilo de emulación pasaba
**~48 % del tiempo esperando** en `Vortex64-video!gfxBufferSwap` (driver OpenGL de AMD, `atio6axx`), limitado por el refresco del monitor (100 Hz).
Con `[Video] vsync=0` (perfil de usuario aislado) ese tiempo de espera desaparece (0.8 %) y el hilo queda 99 % en cómputo.

| Modo (sin limitador, vsync desactivado) | mín | media | máx |
|---|---|---|---|
| JIT | 7.43x | 9.42x | 12.33x |
| Intérprete | 4.76x | 5.77x | 7.58x |
(35 muestras de 1 s; 1x = 60 VI/s; JIT = 1.63x el intérprete.) Con vsync activo: JIT media 4.13x (aisl.) / 2.47x (perfil del usuario con "Display Frame Rate").

Perfil del hilo de emulación con el JIT (99 % cómputo): código generado 19 %, `CRecompiler::RecompilerMain_Lookup_validate` 19 % (búsqueda/validación de bloques),
`R4300iOp::ExecuteFallbackOpcode` 6.6 % + operaciones FPU en intérprete (LWC1/COP1, softfloat), `LW_Memory`/`VAddrToPAddr` ~5 %, HLE de audio del RSP ~3 %.
Candidatos de optimización (NO aplicados; a velocidad normal ya sobra margen, 9x): reducir el coste de la búsqueda de bloques, recompilar más operaciones FPU
sin perder precisión. Limitaciones: una sola escena (título/demo) de un juego; sin comparar con otros emuladores.

## Hito 17: cierre de paridad (2026-11-15)
Ver `Docs/porting/02-PARITY.md` (tabla completa con lo verificado y lo no verificado).
Nuevo: instancia única real en Windows (archivo mapeado con nombre; `Tests/instance_test.cpp` con procesos reales: PASS);
verificados save states (F5/F7), ROM en .zip, diálogo de Trucos, depurador (al activarlo) y menús.
GLideN64 NO portado (alcance de varias horas; el plugin de vídeo propio funciona y es el predeterminado).
ctest: common, core, rsp, instance, 2x cli-boot, 4x jit-fuzz = 10/10, más `jit-fuzz-directed` (PASS, 2 min 48 s).
