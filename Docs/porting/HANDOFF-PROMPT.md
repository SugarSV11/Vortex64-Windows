# Prompt para una sesión nueva: terminar Vortex64-Windows y entregar .exe + código fuente

Copia todo lo de abajo en una sesión nueva de Claude Code abierta en `C:\Users\Sugar\OneDrive\Desktop`.

---

Continúa y TERMINA el port a Windows de mi emulador de Nintendo 64 "Vortex64". El trabajo está en
`C:\Users\Sugar\OneDrive\Desktop\Vortex64-Windows` (repositorio Git local, sin remoto). Responde en español.

## Reglas obligatorias
1. NUNCA modifiques, sobrescribas ni hagas push a `https://github.com/SugarSV11/Vortex64` (el original, solo referencia).
2. NO subas nada a GitHub (ni crees el repositorio remoto `SugarSV11/Vortex64-Windows`) hasta que yo lo diga. Trabaja solo en local.
3. No declares que algo funciona sin haberlo probado; registra en la documentación qué se probó y qué no. No ocultes errores.
4. El mando (gamepad) y compilar en Linux quedan FUERA de alcance (yo programé la versión de Linux y no uso mando).
   Lo que falta se PORTA; no hace falta demostrar cada cosa a mano.
5. Respeta GPLv2: conserva `LICENSE`, `THIRD_PARTY_NOTICES.md`, `licenses/` y los avisos de copyright.
6. Descargas: pide confirmación antes de descargar o instalar algo nuevo (ya autorizadas y hechas: VS Build Tools, Ninja, vcpkg,
   Qt6 vía vcpkg, 7-Zip, el GLideN64 oficial por `packaging/windows/fetch-gliden64.ps1`).
7. Los tests de GUI se ejecutan sin ventanas (`QT_QPA_PLATFORM=minimal:enable_fonts`) y con perfil de usuario aislado
   (`APPDATA`/`LOCALAPPDATA` temporales); no uses ni modifiques mi perfil real (`%APPDATA%\vortex64`, `%LOCALAPPDATA%\vortex64`).
   No mates procesos con `Stop-Process -Force` mientras escriben ajustes. Evita abrir diálogos de error en mi pantalla
   (los tests ya enlazan `Source/Common/Windows/compat/crash_report.h`).
8. Herramienta: en este entorno las secuencias `\n` y `\\` dentro de scripts Python pasados por heredoc se corrompen. Escribe los
   parches como ficheros (`Write`) o usa `Edit`.

## Estado actual (léelo antes de tocar nada)
Lee `Docs/porting/00-AUDIT.md`, `01-PROGRESS.md` (hitos 1-19), `02-PARITY.md` (tabla de paridad), `Docs/development/BUILD-WINDOWS.md`
y `TESTING.md`. Resumen:
- Compila con MSVC x64 (Debug/Release), CMake presets `windows-msvc-debug|release`, Ninja, vcpkg (SDL2, libpng, zlib; feature `gui` = Qt6).
- Funciona: núcleo (intérprete + JIT x64, 0 diferencias en fuzz), plugins DLL (vídeo, audio, input, RSP), CLI, GUI Qt6, SM64 a 60 VI/s
  (hasta ~9x sin limitador), save states, ROM en zip/7z, instancia única, pantalla completa, siempre visible, GLideN64 (binario oficial).
- ctest: 88 pruebas pasan (10 núcleo/RSP/CLI/instancia/JIT + 78 GUI) más `jit-fuzz-directed` (~3 min).
- ZIP portable (`packaging/windows/package.ps1`) probado desde carpeta limpia.
- Hay CAMBIOS SIN COMMITEAR (último commit `21dbab2`): tests portados (`audio`, `lifecycle`, `mainwindow` con named pipe de Discord,
  `first-run`, `migration`), arreglo de junction para la migración de Textures en `Source/Common/path.cpp`, `vcpkg.json` con
  `testlib`, registros en `Tests/CMakeLists.txt`. NADA de eso está compilado ni probado todavía.
- Qt se estaba reconstruyendo con la feature `testlib` (log: `...scratchpad\qttest.log`). Si ya terminó, sigue; si no, espera
  (`vcpkg` ejecutándose en segundo plano; no compiles el proyecto a la vez).

## Tareas pendientes (en orden)
1. Comprobar que `vcpkg_installed` tiene Qt6 con `Qt6Test.dll`. Si la reconstrucción no terminó o se cortó, relanzarla:
   `cmake --preset windows-msvc-release -DVORTEX_BUILD_GUI=ON -DVCPKG_MANIFEST_FEATURES=gui` (con `VCPKG_ROOT=C:\Users\Sugar\vcpkg`
   y `vcvars64.bat` cargado).
2. Compilar todo (`cmake --build --preset windows-msvc-release`) y los tests: `core-test rsp-test instance-test jit-fuzz-test migration-test
   first-run-test audio-test lifecycle-test mainwindow-test rombrowser-test mainmenu-test` + los de GUI. Arreglar errores de compilación.
3. Ejecutar `ctest --test-dir build/windows-msvc-release -LE window` y corregir fallos (muchos serán bugs reales de la app, como pasó
   con los de plugin-config y migración). Después `ctest -L window` (abren una ventana unos segundos; avísame antes).
   `rombrowser-test` y `mainmenu-test` están dentro de `if(NOT WIN32 OR TARGET Qt6::Test)` en `Source/Vortex64-gui/CMakeLists.txt`:
   registrarlos en `Tests/CMakeLists.txt` siguiendo el patrón `vortex_gui_test` (mira `Scripts/linux/run-offscreen-tests.sh`).
4. Portar los tests que aún no están: `script-test` (ScriptShim POSIX -> Windows), `debugger-test` (necesita ROM: usar la ROM falsa
   `build/.../fake-test-rom.z64` o `Tests/make_fake_rom.cpp`). Los de X11/XTest/uinput (`input-e2e`, `input-capture-x11`, `input-ingame`,
   `hotkey-pause`, `input-gamepad`, `present-sync`) no aplican a Windows: documéntalo.
5. Verificar con un test automático (si es posible) que SRAM/FlashRAM se guardan y que un truco se aplica (ver
   `Source/Vortex64-core/N64System/Enhancement/Enhancements.h`, `ApplyActive`). Si no se puede sin una ROM real, documéntalo.
6. Actualizar `Docs/porting/02-PARITY.md`, `01-PROGRESS.md` y `TESTING.md` con lo real. Commits locales con mensajes claros, terminados en
   `Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>`.
7. Ejecutar la suite completa una última vez: `ctest ...` (incluido `jit-fuzz-directed`) y copiar el resultado a la documentación.

## ENTREGA FINAL (obligatoria)
Crea una carpeta nueva `C:\Users\Sugar\OneDrive\Desktop\Vortex64-Windows-ENTREGA` con EXACTAMENTE dos elementos:

1. **`Vortex64-Windows-x64.zip`** — el ejecutable listo para usar. Generarlo con `powershell -File packaging\windows\fetch-gliden64.ps1`
   y después `powershell -File packaging\windows\package.ps1 -Version 1.0.0` (build Release con la GUI). Debe contener `vortex64.exe`,
   `vortex64-cli.exe`, las DLL de los plugins, SDL2/libpng/zlib/Qt, runtime MSVC, `Config`, `Lang`, `Scripts`, `GLideN64.dll`,
   licencias y `README-WINDOWS.txt`. Comprueba que arranca desde una carpeta limpia con un perfil aislado antes de entregarlo.
2. **`Vortex64-Windows-source.zip`** — el código fuente completo del repositorio, SIN `build/`, `dist/`, `vcpkg_installed/`,
   `third_party_bin/`, `.git` ni ROMs: usa `git archive --format=zip -o ... HEAD` tras commitear todo. Incluye `LICENSE`,
   `THIRD_PARTY_NOTICES.md`, `Docs/` y las instrucciones de compilación (`Docs/development/BUILD-WINDOWS.md`).

Dentro de la carpeta de entrega NO pongas nada más (ni ROMs, ni binarios sueltos, ni logs). Al terminar dime: la ruta de la carpeta, el
tamaño y SHA-256 de cada zip, qué pruebas pasaron (cifras reales de ctest), y una lista honesta de lo que NO está al nivel de Linux.
NO subas nada a GitHub: la subida la decido yo después de revisar la entrega.

---

## Actualización (final de la sesión del 2026-11-15)
- Último commit: ver `git log`. Qt con `testlib` ya está instalado y TODO compila. `ctest -LE window -E directed` = 113/113.
- Con `-DVORTEX_TEST_ROM=<SM64 real>` también pasan `gui-rombrowser` y `gui-debugger`. `lifecycle-test` pasa (ventana real, `-Qpa windows`).
- PENDIENTE real: `mainwindow-test` (ventana real + Discord falso por named pipe): una vez falló solo por la semántica de FatalError (ya ajustada en el
  test) y otra vez se quedó colgado tras "mainwindow: status bar"; hay que repetirlo para ver si es intermitente o un bug (matar `mainwindow-test.exe` si se cuelga).
- NO portado: `script-test` (916 líneas, sockets BSD + shells POSIX + contadores del ScriptShim de Linux). En Windows `ScriptShimTerminateProcesses` es un stub (los
  procesos de `exec()` no se terminan al cerrar): una solución es un Job Object con KILL_ON_JOB_CLOSE para los procesos creados por los scripts.
- Falta la ENTREGA (carpeta `Vortex64-Windows-ENTREGA` con los dos zip) y el informe final.
