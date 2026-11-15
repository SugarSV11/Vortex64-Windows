# Paridad Vortex64 (Linux) vs Vortex64-Windows — estado real (2026-11-15)

Leyenda: ✅ verificado en Windows (con la prueba indicada) · 🟡 compila/abre pero sin verificar en uso · ❌ no portado / no disponible

| Área | Estado | Cómo se verificó / nota |
|---|---|---|
| Núcleo R4300i intérprete | ✅ | SM64 (título, demo, menú); `jit-fuzz` |
| Recompilador x64 (JIT) | ✅ | 4 configuraciones x 300 programas aleatorios + suite dirigida: 0 diferencias con el intérprete; SM64 |
| TLB, excepciones, memoria | ✅ (parcial) | cubiertos por los programas del fuzz y SM64; sin tests dedicados de TLB |
| RSP (interpreter, plugin DLL) | ✅ | `rsp-test` (escalares, vectoriales VADD/VAND/VMULF, BREAK) y SM64 |
| RSP recompilador | 🟡 | sin prueba diferencial |
| Vídeo propio (OpenGL/SDL2) | ✅ | SM64 embebido en la ventana Qt; sin comparación de imágenes |
| GLideN64 (plugin alternativo) | ❌ | requiere portar su capa de plataforma (7 ficheros Linux), FreeType, su UI Qt y 535 ficheros; no iniciado |
| Audio (SDL2) | ✅ (señal) | el medidor de pico de Windows marca señal; calidad/latencia no medidas |
| Audio DirectSound | 🟡 | el backend original existe en el código, no integrado |
| Entrada: teclado | ✅ | Enter (Start) lleva de título a "SELECT FILE" |
| Entrada: mando (SDL GameController) | 🟡 | no hay mando en esta máquina; herramienta `gamepad-probe` lista |
| Ratón | 🟡 | sin probar |
| ROM z64/n64/v64 | ✅ | `core-test` (misma imagen en las tres ordenaciones) |
| ROM en .zip | ✅ | SM64 dentro de un zip corre con el JIT |
| ROM en .7z | 🟡 | decodificador compilado, sin probar |
| Guardados EEPROM | ✅ (creación) | `SUPER MARIO 64.eep` (512 B) creado por el emulador; no se jugó una partida |
| SRAM / FlashRAM | 🟡 | sin juego que los use para probar |
| Save states | ✅ | F5 crea `*.pj.zip` (1,08 MB), F7 restaura sin caídas (no se comprobó visualmente el estado restaurado) |
| GUI: biblioteca de juegos | ✅ | escanea carpeta, muestra tarjeta, "Iniciar" arranca |
| GUI: menús, barra de estado | ✅ | capturas; VI/s y CPU en la barra |
| GUI: Ajustes (todas las páginas) | ✅ (abren) | no se probó que cada cambio se aplique y persista |
| GUI: configuración de plugins (vídeo/audio/input) | ✅ | las tres ventanas abren y se pintan |
| GUI: Trucos | ✅ (abre) | diálogo "Trucos" (Ctrl+C) abre; aplicar un truco no probado |
| GUI: Mejoras | 🟡 | entrada de menú presente; sin abrir |
| Depurador (ventanas) | ✅ (abre) | tras activarlo en Avanzado abre "Vortex64 - Debugger"; sin usarlo con un juego |
| Scripts JS del depurador | 🟡 | compila; `exec()` no termina procesos al cerrar (stub) |
| Discord Rich Presence | 🟡 | compila con sus ficheros Windows; sin Discord para probar |
| Inhibir suspensión (SetThreadExecutionState) | 🟡 | implementado, sin comprobar con el sistema |
| Pantalla completa | 🟡 | sin probar |
| "Siempre encima" | 🟡 | implementado (SetWindowPos), sin probar |
| Instancia única | ✅ | `instance-test` con procesos reales (detecta, termina, sin restos) |
| CLI | ✅ | arranque, `--interpreter/--recompiler/--unlimited/--perf/--profile` |
| Tests automáticos | ✅ (parcial) | 10 pruebas ctest + suite dirigida del JIT; ~25 tests Linux de la GUI (X11/XTest) sin portar |
| Empaquetado | ✅ (ZIP portable) | ZIP 27,8 MB probado en carpeta limpia; sin instalador ni asociación de archivos |
| CI GitHub Actions | 🟡 | escrita, nunca ejecutada (no hay repositorio remoto) |
| Linux sigue compilando | 🟡 | los cambios de código compartido llevan `#ifdef _WIN32`; no se compiló en Linux |
| Rendimiento | ✅ | 60 VI/s en todos los casos; hasta 9,4x sin limitador (JIT, vsync off) |

## Qué NO está al nivel de Linux (resumen)
GLideN64; verificación del mando, SRAM/FlashRAM, 7z, pantalla completa, Discord, trucos aplicados, scripts JS completos; instalador/asociación de
archivos; ~25 tests de GUI; ejecución de la CI; compilación en Linux tras los cambios.
