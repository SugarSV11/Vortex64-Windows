# Pruebas automáticas (Windows)

Se ejecutan con `ctest --test-dir build/windows-msvc-release --output-on-failure` (tras compilar los objetivos
`common-test core-test rsp-test jit-fuzz-test fake-test-rom`). Ninguna prueba necesita una ROM comercial: usan ROMs sintéticas
(`Tests/make_fake_rom.cpp` genera un z64 de 4 MB con cabecera válida y ceros). Los datos de usuario de cada prueba están
aislados (`APPDATA`/`LOCALAPPDATA` apuntan a carpetas del directorio de build).

| Prueba (ctest) | Qué comprueba | Fichero |
|---|---|---|
| `common` | Hilos, eventos de sincronización y terminación de `CThread` (capa de plataforma) | `Source/Common/tests/common_test.cpp` |
| `core` | Memoria (VirtualAlloc, protección, **código ejecutable del JIT**), rutas (letras de unidad, enumeración), INI (ida y vuelta, acortado sin restos), arranque `AppInit`, carga de ROM en z64/n64/v64 (misma imagen), cabeceras válidas/ inválidas, ROM inexistente | `Tests/core_test.cpp` |
| `rsp` | Plugin RSP por su ABI pública: identificación, instrucciones escalares, bucles con delay slot, anchos de carga/almacenado y extensión de signo, vectoriales (VADD/VAND/VMULF), BREAK + interrupción | `Tests/rsp_test.cpp` |
| `cli-boot-interpreter` / `cli-boot-recompiler` | La CLI arranca, carga los 4 plugins y la ROM, inicia la emulación y no se cae | `Tests/cli_boot.cmake` |
| `jit-fuzz-*` (4) | **CPU:** 300 programas MIPS aleatorios por configuración (sin/ con caché de registros, núcleo 32/64 bits, enlazado de bloques) ejecutados por intérprete y por JIT; se compara todo el estado | `Source/Vortex64-gui/tests/jit_fuzz_test.cpp` |
| `jit-fuzz-directed` (lenta, ~3 min) | Programas dirigidos: ramas, saltos, memoria, aritmética, inmediatos, desplazamientos, bucles | idem |

## Resultados (2026-11-15, Release, MSVC 14.44, AMD RX 9060 XT)
`100% tests passed out of 9` (la suite dirigida se ejecutó aparte: PASS en 2 min 48 s; ahora también está en ctest como `jit-fuzz-directed`).

## Verificación de las propias pruebas
Se reintrodujo a propósito el fallo del shim `dirent` (patrón de búsqueda sin `\`): `core` falla en `Count == 2`, como debe.

## Limitaciones conocidas
- Las pruebas de CPU/JIT cargan los plugins como la aplicación, por lo que necesitan un contexto OpenGL (en CI de GitHub no se ha ejecutado aún).
- Solo se prueba el RSP con el intérprete/ruta por defecto del plugin; no hay prueba diferencial del recompilador RSP.
- No hay pruebas de RDP/vídeo (comparación de imágenes), audio con dispositivo real, mando ni de las ventanas de la GUI (las pruebas Qt de Linux
  dependen de X11/XTest y no se han portado).
- No hay medición de cobertura.
