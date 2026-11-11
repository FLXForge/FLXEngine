# FLX Engine v0.4.0 — Auditoría Linux

Fecha de revisión: 11 de noviembre de 2026.
Checkout auditado: `feature/v0.4.0`, base `37367bd`.
Objetivo de la siguiente fase: Ubuntu 24.04 x86-64.

## 1. Alcance y evidencia

Se revisaron CMake, todos los workflows, el punto de entrada, CLI,
compilación de proyectos, codec, persistencia, runtime, gráficos, audio,
input, scripting y pruebas. Esta revisión modifica la versión y la
documentación. Las propuestas Linux de este documento están pendientes
de implementación y ejecución.

El equipo disponible ejecuta Windows. `wsl --list --quiet` informa que
WSL no está instalado. No se encontraron Docker, Podman ni una herramienta
GCC/Clang Linux ejecutable en el entorno. No se realizó una compilación Linux.
Una compilación MSVC satisfactoria no confirma compatibilidad con GCC/Clang.

Las dependencias locales usadas para validar Windows estaban limpias y
correspondían a raylib 6.0, nlohmann/json v3.12.0, yaml-cpp 0.8.0 y
QuickJS-NG v0.10.1. Los commits locales de raylib y QuickJS eran,
respectivamente, `dbc56a87da87d973a9c5baa4e7438a9d20121d28` y
`3c9afc9943323ee9c7dbd123c0cd991448f4b6c2`.

## 2. Preparación de versión

`VERSION` contiene `0.4.0`. CMake lo lee y genera `FlxVersion.h`.
El comando de versión y el escritor del campo `producerVersion` usan esa
cabecera. No requieren cambios de código ni nuevas constantes.

Se actualizaron las referencias vigentes del README, los encabezados y
pies de documentación pública ES/EN, las presentaciones de los índices
API/JSON y la salida documentada de `flx --version` en `spec/cli.md`.
El README describe ahora la biblioteca y la API de sesión de v0.4.0.

Se conservaron estas referencias anteriores:

- `spec/V0.4.0_DIRECTION.md`, `spec/audio.md` y
  `spec/scripting/drawing.md` describen la etapa o caracterización v0.3.1.
- El README conserva una frase histórica sobre las aportaciones de v0.3.1.
- Los contratos iniciales v0.3.0, los ejemplos `engine=0.3.0`, las versiones
  de los juegos y `docs/scripts/flx.d.ts` versionan contratos o ejemplos
  independientes. La versión del contrato TypeScript conserva la decisión
  previamente acordada.
- Las páginas con fuentes auditadas en v0.3.0 conservan esas atribuciones
  y la descripción histórica de sus contratos.
- `.github/workflows/release.yml` conserva `v0.3.1` como ejemplo de sintaxis
  del campo de ejecución manual. El workflow valida el tag contra VERSION
  dinámicamente y construirá `flx-v0.4.0-windows-x64.zip` para ese tag.
  No es una constante de versión del producto.

El checkout no contiene un recurso VERSIONINFO de Windows para FLX.
El ejecutable generado tiene vacíos FileVersion, ProductVersion,
ProductName y FileDescription. Esto es una carencia previa de metadata PE,
no una versión antigua por actualizar. El versionado de distribución
existente reside en VERSION, CLI, nombre del paquete y producerVersion.

## 3. Hallazgos confirmados

| ID | Archivos | Evidencia y consecuencia |
| --- | --- | --- |
| C1 | `CMakePresets.json` | Todos los presets dependen de Windows, cl.exe y SegmentHeap de Visual Studio. No sirven para configurar Linux. CMake directo sigue siendo una entrada válida para la primera prueba. |
| C2 | `CMakeLists.txt` y raylib 6.0 | FLX declara CMake 3.20, pero la raíz de raylib requiere 3.25. El mínimo efectivo de esta combinación es 3.25. Ubuntu 24.04 proporciona CMake 3.28.3. |
| C3 | `.github/workflows/build.yml` | Compila únicamente Windows y se activa automáticamente en develop, no en esta rama. Ejecuta CTest en modo headless y smoke tests CLI. |
| C4 | `.github/workflows/release.yml` | Empaqueta EXE, ZIP y lanzadores BAT. No tiene job Linux ni pasos de tests antes de publicar. Las comprobaciones de tag, main y VERSION ya existen. |
| C5 | `.github/workflows/pages.yml` | Su runner Ubuntu solo copia y publica documentación. No aporta evidencia de compilación o ejecución de FLX en Linux. |
| C5b | `.github/workflows/version-check.yml` | Usa PowerShell sobre Windows para exigir que VERSION aumente frente a main. Su comparación semántica no aporta una validación Linux. |
| C6 | `CMakeLists.txt` | No hay reglas install/export de flx-core ni empaquetado Linux propio. Sí existen dependencias e includes PUBLIC para consumir el target desde CMake. |
| C7 | `tests/support/TestSupport.cpp` y pruebas gráficas | Varias suites omiten casos internos con FLX_TEST_HEADLESS y salen con éxito. TextRasterization y SessionDevice usan además código de omisión 77. Un CTest verde headless no cubre todo el runtime. |
| C8 | `tests/session/FlxSessionTests.cpp` | La prueba de solo lectura presupone que quitar permisos de escritura impide escribir. Ejecutarla como root en Linux puede invalidar esa premisa. El runner debe usar un usuario normal. |
| C9 | raylib `CMakeOptions.txt` | La configuración seleccionada usa X11 por defecto y Wayland OFF, aunque GLFW por separado tenga otros valores predeterminados. Activar Wayland añade requisitos. |

En las fuentes propias de FLX no se encontraron includes de Windows ni
llamadas Win32, POSIX obligatorias o funciones CRT exclusivas de MSVC.
Las opciones /EHsc, /experimental:c11atomics, WIN32_EXECUTABLE y las
definiciones Windows de QuickJS están condicionadas correctamente.
Esto es evidencia estática favorable, no una prueba de compilación Linux.

También se compararon los includes relativos C/C++ con las rutas registradas
en Git respetando el caso. No se detectaron discrepancias ni archivos del
checkout que colisionen al ignorar mayúsculas. Las referencias declarativas
de proyectos requieren una comprobación adicional sobre un filesystem Linux.

## 4. Riesgos y comprobaciones por subsistema

| Área y archivos | Resultado estático | Comprobación pendiente |
| --- | --- | --- |
| C++20, headers y `CMakeLists.txt` | Los requisitos públicos de flx-core y opciones MSVC están separados. No se confirmó un bloqueo de código específico. | Compilar todo con GCC y Clang. Detectar includes transitivos, conversiones, extensiones aceptadas por MSVC y errores reales de enlace. |
| QuickJS-NG y `ScriptEngine.cpp` | La integración usa las mismas cinco fuentes C del núcleo upstream, C11 con extensiones, _GNU_SOURCE, Threads, dl y m. No incorpora quickjs-libc ni ejecutables qjs/qjsc. | Ejecutar hooks JS, bindings, excepciones y sesiones sucesivas en GCC/Clang. Verificar ABI de JSValue en x86-64 y símbolos finales. No cambiar las flags globales para imitar MSVC. |
| `ProjectManifestLoader.cpp` | Usa streams y trim explícito de espacio, tab, CR y LF. | Manifiestos LF/CRLF, nombres UTF-8, directorios con espacios y permisos de lectura. |
| `ProjectCompiler.cpp` | Resuelve path, machine e input.mapping desde el manifiesto mediante filesystem. No normaliza una barra inversa en todos esos campos. | Un manifiesto con rutas de Windows no se convierte automáticamente en un proyecto portable. Probar rutas POSIX y documentar las rutas aceptadas antes de plantear cambios. |
| `JsonLoader.cpp` | Normaliza barras de referencias JSON/JS. Un prefijo / en referencias FLX significa raíz del proyecto JSON. | Preservar esa semántica. No reinterpretar /objeto como una ruta absoluta del sistema operativo. Probar like, children, referencias con miembro, scripts y ciclos. |
| `ProjectResolver.cpp`, `CliParser.cpp`, `RunCompiledCommand.cpp` | Las extensiones CLI se comparan sin distinguir caso. El filesystem conserva su política de nombres. | .FLX/.FLXC existentes deben abrirse. Un archivo game.json no satisface una referencia Game.json en un filesystem sensible al caso. Probar ambas situaciones separadamente. |
| Rutas estrechas y locale | Streams y numerosas interfaces usan std::string y path.string()/generic_string(). | UTF-8, acentos, CJK, espacios y locale C/C.UTF-8. Comparar comportamiento Windows/Linux sin inventar un nuevo sistema de rutas. |
| `CompiledProjectBinary.cpp` | Streams binarios, escritura temporal y respaldo en el mismo directorio, rename con error_code. | Destino nuevo/existente, solo lectura, fallos de reemplazo y restauración, symlinks y rutas con espacios. Las garantías y permisos de rename difieren por sistema. |
| `BinaryReader.cpp`, `BinaryWriter.cpp`, `CompiledProjectCodec.cpp` | Enteros explícitos little-endian y floats IEEE-754 de 32 bits. Se serializan campos y fuentes JS, no structs C++ ni bytecode QuickJS. El formato actual es 12. | Intercambio Windows/Linux en ambas direcciones y ejecución sin fuentes. La igualdad funcional importa incluso si CRLF o metadata de origen cambian los bytes producidos desde fuentes. |
| `PersistenceSystem.cpp`, `RunOptions.h`, `FlxSession.h` | savePath se resuelve una vez por sesión. El modo portable usa cwd/saves, con creación al escribir y sin fallback. | CWD posterior distinto, instalación sin permisos de escritura, archivo/directorio bloqueado y errores visibles. La aplicación host puede seleccionar otra ruta. El runtime no debe elegir automáticamente XDG u otra política nueva. |
| Formato .flxsave | Usa un double copiado en representación nativa. Windows x86-64 y Ubuntu x86-64 coinciden en el escenario objetivo. | Intercambiar valores boolean/number/string y Unicode. No inferir portabilidad a arquitecturas big-endian a partir de .flxc. |
| `DrawingRenderer.cpp`, `FlxSession.cpp`, `Engine.cpp` | Gráficos y fuentes se crean con raylib. No hay carga directa de texturas o fuentes de archivos externos en el renderer actual. | OpenGL, FBO, volteo de textura, escala, fullscreen, filtro POINT y rasterización de texto con Mesa y con una GPU real. |
| `AudioSystem.cpp`, `MusicSequencer.cpp` | Sonidos sintetizados y música en streaming. La sesión puede funcionar sin dispositivo y conserva FLX-RUNTIME-00016. | ALSA/PulseAudio, pausa, drenaje, prioridades, cierre y recursos del host. La ausencia de excepción o un backend nulo no demuestra salida audible. |
| `InputSystem.cpp`, `InputMappingLoader.cpp` | La consulta física pasa por raylib y el estado lógico pertenece a la sesión. | Teclado, layouts, códigos de tecla, mandos, hotplug y permisos de dispositivos. Xvfb no valida periféricos reales. |
| `FLXEngine.cpp` y comandos CLI | main(argc, argv), streams estándar y filesystem. No se encontró ejecución de procesos, shell o API exclusiva de Windows. | Códigos de salida, stdout/stderr, quoting del shell, rutas absolutas/relativas y ejecución desde otro CWD. |
| `flx-core` y consumidores | Target estático con C++20, includes engine/generated y dependencias PUBLIC. CoreConsumer enlaza la variante de producción. | Consumidor CMake fuera del árbol con add_subdirectory(... EXCLUDE_FROM_ALL). No basta copiar libflx-core.a. Un host compartido requiere evaluar PIC, también en raylib y qjs. El caso mínimo es un host ejecutable. |
| Pruebas temporales | Varias suites usan el mismo directorio temporal base. | CTest en serie durante la primera validación y directorios distintos para jobs concurrentes. Root y contenedores con privilegios no son pruebas válidas de denegación ordinaria de permisos. |
| Distribución | Las bibliotecas del motor pueden enlazarse estáticamente, pero quedan dependencias del sistema gráfico y del runtime C/C++. | ldd/readelf del ELF final y prueba en Ubuntu 24.04 limpio. Un tarball construido allí no implica compatibilidad con distribuciones anteriores. |

Hay una inconsistencia documental previa: `spec/compiled-binary.md` menciona
FormatVersion 11 y `spec/compiled-project.md` menciona 2, mientras que el
codec y el contrato Creation usan 12. Se identifica para evitar usar esos
números como expectativa de intercambio. La preparación de versión conserva
el codec y sus números.

La definición manual del target qjs no hereda todas las opciones del CMake
upstream, como -funsigned-char y sus ajustes de warnings. Esa diferencia debe
tenerse en cuenta al comparar resultados GCC/Clang. No se ha demostrado que
obligue a cambiar flags para el escenario x86-64.

## 5. Cambios imprescindibles para la primera compilación Ubuntu

La revisión estática no permite justificar todavía un parche obligatorio a
las fuentes del motor. La primera acción debe ser una compilación diagnóstica
con las versiones seleccionadas y sin correcciones previas.

Requisitos del entorno para el camino X11 recomendado:

```sh
sudo apt-get update
sudo apt-get install --no-install-recommends \
  build-essential clang cmake ninja-build git ca-certificates pkg-config \
  libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libxext-dev \
  libgl1-mesa-dev libasound2-dev
```

Para pruebas gráficas automatizadas, añadir `xvfb xauth libgl1-mesa-dri`.
La guía de raylib recomienda también otros paquetes según el backend.
No se requiere instalar versiones de sistema de raylib, QuickJS, json o
yaml-cpp porque FLX ya selecciona esas fuentes con FetchContent.

La propuesta mínima configura X11 explícitamente. Wayland puede evaluarse
después con `GLFW_BUILD_WAYLAND=ON`, las bibliotecas `libwayland-dev` y
`libxkbcommon-dev` y un wayland-scanner disponible. Un binario X11 bajo una
sesión Wayland depende de XWayland y requiere una prueba propia.

```sh
cmake -S . -B out/build/linux-gcc -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++ \
  -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_X11=ON -DGLFW_BUILD_WAYLAND=OFF
cmake --build out/build/linux-gcc --parallel 2
FLX_TEST_HEADLESS=1 ctest --test-dir out/build/linux-gcc --output-on-failure -j 1
```

Repetir en otro árbol con clang/clang++. Un preset Linux puede añadirse en la
implementación posterior para reproducir estos argumentos. El entorno,
presets y workflow se pueden preparar sin modificar los presets Windows.
Solo los errores reales de GCC/Clang justificarán correcciones puntuales de
includes, tipos o enlace.

## 6. Pruebas funcionales y de rutas necesarias

1. Validar y compilar los cinco ejemplos desde la raíz y desde otro CWD,
   pasando el manifiesto con ruta absoluta. Ejecutarlos con un límite de frames.
2. Copiar un proyecto a una ruta con espacios y Unicode. Probar machine,
   mapping, like y scripts relativos, ./, ../ y referencias FLX desde /.
3. Crear referencias con caso correcto e incorrecto en un filesystem sensible
   al caso. Probar extensiones en mayúsculas sin renombrar implícitamente archivos.
4. Guardar con ruta predeterminada, absoluta y relativa. Cambiar CWD después de
   load, probar sesiones aisladas y compartidas y verificar ausencia de fallback
   ante permisos denegados. Ejecutar como usuario normal.
5. Comprobar lectura de fuentes y .flxc sin permisos, sustitución de un .flxc
   existente y recuperación del original tras fallos de publicación local.
6. Ejecutar CTest sin FLX_TEST_HEADLESS bajo Xvfb con Mesa. Registrar todas las
   omisiones. Separar la comprobación gráfica sin audio de la prueba de audio
   físico para que una omisión no esconda el resultado del caso silencioso.
7. En escritorio Ubuntu real, comprobar ventana, fullscreen, foco, entrada,
   mandos, sonido y música. Repetir la sesión sin audio mediante un mecanismo
   determinista y comprobar warning, update, render, exit y unload.
8. Usar un host externo de producción que mantenga dispositivos y ejecute
   proyectos sucesivos. Confirmar que no necesita FLX_TESTING ni comandos CLI.

Xvfb prueba gráficos virtualizados. No sustituye pruebas de Wayland, periféricos
ni reproducción audible.

## 7. Compatibilidad cruzada de .flxc

Preparar fixtures desde el mismo commit y VERSION en Windows y Linux,
incluyendo los cinco ejemplos y un proyecto determinista con scripts,
mapping, estados, Creation, audio y valores numéricos.

- Windows escribe, Linux lee y ejecuta. Linux escribe, Windows lee y ejecuta.
- Mover el archivo a otra ruta, retirar las fuentes y ejecutar run-compiled.
- Comprobar magic, formatVersion=12 y producerVersion=0.4.0.
- Comparar raíz, recursos, IDs, scripts y resultados lógicos, usando tolerancias
  explícitas para resultados de coma flotante cuando corresponda.
- Repetir rechazo de truncados, versión de formato desconocida, duplicados y
  límites. El texto producerVersion por sí solo no restringe la lectura.
- Comparar hashes solo para fixtures con entradas y metadata normalizadas.
  No asumir que compilar el mismo checkout con CRLF/LF distintos produce
  archivos idénticos.

La compatibilidad del archivo depende del codec, no de la ABI de MSVC,
GCC o Clang. No deben serializarse structs nativos ni introducir bytecode JS.

## 8. Diseño propuesto del workflow y artefacto

Un workflow de validación separado, en Ubuntu 24.04, debería activarse por PR
y manualmente. Matriz GCC/Clang en árboles independientes, permisos contents:read,
checkout del commit, instalación de dependencias y configuración explícita.
Inicialmente usar acciones fijadas por commit y registrar sus versiones.

Orden recomendado de los jobs:

1. Configuración y compilación diagnóstica de ambos compiladores.
2. CTest lógico/headless y smoke CLI con comprobación de cada código de salida.
3. CTest gráfico bajo Xvfb sin la variable headless, conservando JUnit/logs y
   revisando omisiones. El audio físico requiere un entorno apto adicional.
4. Jobs de intercambio de fixtures con Windows y consumidor externo.
5. Crear el paquete con el mismo ejecutable validado. Extraerlo en otro
   directorio, comprobar permisos, versión, dependencias y ejecutar el Pong
   empaquetado antes de subir el artefacto.

Los workflows de publicación existentes se mantienen fuera de esta fase.
Una futura publicación deberá reutilizar el artefacto validado y conservar
las verificaciones de tag, pertenencia a main y VERSION.

Artefacto recomendado para la siguiente fase:

```text
flx-v0.4.0-linux-x86_64.tar.gz
  flx-v0.4.0-linux-x86_64/
    flx
    examples/
    schemas/
    scripts/flx.d.ts
    README.md
    LICENSE
    THIRD_PARTY_NOTICES.txt
    BUILD_INFO.txt
```

`BUILD_INFO.txt` debería registrar VERSION, commit, compilador, Ubuntu objetivo,
backend gráfico, commits de dependencias y resultados de validación.
Incluir las licencias de dependencias distribuidas y un SHA-256 del tarball.
No se propone un SDK binario flx-core en esta primera entrega.

El tar debe preservar el permiso ejecutable de flx. Un lanzador opcional
debe usar LF y resolver su ruta respecto a su propio archivo. Subir el
tarball como un archivo evita depender de los permisos del ZIP de Actions.
El registro de dependencias runtime se obtiene del ELF real con ldd/readelf,
incluidas bibliotecas cargadas dinámicamente por los backends cuando proceda.

## 9. Riesgos de regresión Windows y secuencia mínima

Los riesgos principales de una implementación posterior son normalizar rutas
declarativas cambiando su significado, alterar la integración C de QuickJS,
modificar opciones globales CMake, cambiar el formato para resolver un problema
de ruta, o relajar pruebas de audio/gráficos para obtener un resultado verde.
Cada corrección debe tener un fallo Linux reproducible y una validación
equivalente en Windows.

Secuencia propuesta:

1. Revisar este diagnóstico y acordar X11/Ubuntu 24.04 como alcance inicial.
2. Ejecutar la compilación base Linux con las fuentes actuales. Guardar errores
   completos, paquetes instalados, commit y configuración.
3. Corregir únicamente bloqueos demostrados, uno por uno, y repetir GCC/Clang
   más las pruebas Windows afectadas.
4. Validar rutas, permisos, intercambio .flxc y consumidor externo.
5. Validar gráficos virtualizados y después audio/input/escritorio reales.
6. Implementar tarball y smoke test después de extraerlo.
7. Añadir el workflow de validación. Aprobar por separado la publicación Linux.

## 10. Validación ejecutada en esta preparación

- Configuración limpia Ninja/MSVC x64 Release en
  `out/build/v040-audit-release`, con las dependencias locales seleccionadas.
- Compilación completa de flx, flx-core, variante de pruebas y consumidores.
- CTest en serie con gráficos y audio disponibles: 30/30 suites aprobadas,
  sin omisiones, en 9,74 segundos.
- `flx --version` devolvió exactamente `0.4.0`.
- La cabecera generada contiene Major=0, Minor=4, Patch=0 y Text=0.4.0.
- Compilación de Pong a `out/v040-pong.flxc`. Su cabecera contiene
  formatVersion=12 y producerVersion=0.4.0.
- Metadata PE inspeccionada: los cuatro campos de versión/producto indicados
  en la sección 2 están vacíos.
- La compilación y ejecución Linux, el ELF final y un tarball Linux real
  quedan pendientes de la siguiente fase.

## Fuentes externas contrastadas

- [Opciones de raylib 6.0](https://github.com/raysan5/raylib/blob/6.0/CMakeOptions.txt)
  para selección X11/Wayland.
- [Guía oficial GNU/Linux de raylib](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux)
  para requisitos de desarrollo.
- [CMake de QuickJS-NG v0.10.1](https://github.com/quickjs-ng/quickjs/blob/v0.10.1/CMakeLists.txt)
  para fuentes C, definiciones y bibliotecas de la integración.
- [CMake en Ubuntu 24.04](https://packages.ubuntu.com/noble/cmake),
  [Clang en Ubuntu 24.04](https://packages.ubuntu.com/noble/clang) y
  [ALSA de desarrollo en Ubuntu 24.04](https://packages.ubuntu.com/noble/libasound2-dev).
- [Permisos de artefactos de Actions](https://github.com/actions/upload-artifact#permission-loss)
  para el transporte del tarball.
