# Animation v0.4.0 — auditoría final, fase 5

Fecha de la revisión: 2026-11-19. Rama: `feature/v0.4.0`.
Base local al iniciar: `28d5e8b`; resultados sobre esa base más el diff de esta fase.
Fuente normativa: `spec/animation.md`, incluidos sus apartados 10, 10.1 y 11.

## Dictamen

**APROBADO CON RESERVAS para revisión funcional en Windows; Animation NO se declara cerrada.**
No se ha encontrado un incumplimiento funcional pendiente en los caminos examinados
que justifique rediseñar el subsistema. Se corrigieron una regresión de asignaciones
en ABSENT y una discrepancia documental heredada de Text. Se reforzaron pruebas que
antes solo permitían inferir algunos resultados de integración.

Antes del cierre faltan resultados reales de este diff en Linux GCC/Clang, incluida
la ejecución gráfica con Xvfb. No se extrapola a Linux el éxito de Windows ni se
usan ejecuciones anteriores a este diff como validación de Animation final.
La revisión de ownership y sus regresiones no sustituyen un detector exhaustivo
de fugas CPU/GPU ni un ensayo prolongado de integración con juegos.

## Alcance y leyenda de evidencia

- **D, directa:** una aserción verifica expresamente el resultado indicado.
- **I, indirecta:** inspección del camino compartido o prueba de un comportamiento
  relacionado; no se contabiliza como una aserción del comportamiento pendiente.
- **A, ausente:** no se ha ejecutado esa comprobación o falta un oráculo adecuado.
- Un D es local al caso y plataforma descritos, no una prueba del producto cartesiano
  de estados, familias, entradas y plataformas. Windows y Linux se califican aparte.

Abreviaturas de archivos de pruebas, relativas a la raíz:

| Código | Archivo |
| --- | --- |
| E | `tests/runtime/AnimationEvaluationTests.cpp` |
| D | `tests/loading/AnimationDeclarationTests.cpp` |
| I | `tests/runtime/AnimationIntegrationTests.cpp` |
| G | `tests/runtime/AnimationRenderingTests.cpp` |
| W | `tests/runtime/RuntimeDrawingTests.cpp` |
| A | `tests/runtime/AnimationAuditTests.cpp` (nuevo) |

Implementaciones: **P** = `engine/runtime/AnimationPlayback.cpp`;
**B** = `engine/graphics/RepresentationPrimitiveBuilder.cpp`;
**Q** = `engine/scripting/bindings/AnimationBindings.cpp`;
**C** = `engine/graphics/DeclarativeGeometry.cpp`;
**R** = `engine/runtime/RuntimeWorld.cpp`;
**J** = `engine/loading/JsonLoader.cpp`;
**V** = `engine/compiler/AnimationValidation.inl`;
**K** = `engine/compiler/binary/CompiledProjectCodec.cpp`.
Los nombres entre comillas son los títulos reales de casos, o su prefijo inequívoco.

## Matriz de conformidad ANIM

| Invariante | Implementación examinada | Prueba que verifica el comportamiento | Cobertura / límite |
| --- | --- | --- | --- |
| ANIM-001 una reproducción | P::play, un único selected/status; Q | E «different-name substitution»; I::testPublicTransitions | D: sustituye desde RUNNING/PAUSED/DONE; I: ausencia de blending, confirmada por el modelo |
| ANIM-002 solo apariencia | P::appearance/evaluate; B; C::LocalTransform; renderer separado de Collision | D «RuntimeObject owns independent playback without spatial or physics writes»; I::testSpawnAndColor; G::testAnimationGeometryCPU | D: posición/origin/tamaño/ángulo, velocidad/angular, collider, visibilidad/state/depth/Timer y presencia de color testados; I: resto de campos públicos por separación del código |
| ANIM-003 máscara activa | P::play calcula controlled con at<=duration | E «independent sparse components» y «selective current capture»; W::testAnimationAcceptanceOpenCompiled | D: componentes omitidos e inactivos, incluidas propiedades antiguas descartadas |
| ANIM-004 keys parciales | J::parseAnimations; V; P::sample/sampleColor | D «partial keys» y «duplicates detected»; E «independent sparse components» | D: orden normalizado, ausencia, interpolación independiente, retención tras última key |
| ANIM-005 sustitución discreta | P::evaluate, alias inmutable de key.representation | E «representation substitution»; W::testAnimationMicroexamplesGraphics | D: igualdad en at, [], longitud variable, cinco bitmaps y vuelta al primer frame |
| ANIM-006 persist base | J, V, K; P::capture separa grupos; B | D «invalid animation structures», «external palettes»; E «representation substitution»; A::stablePartitionAndOwnership | D: rechazo en key incluso por like, captura solo de sección sustituible, sin duplicaciones |
| ANIM-007 referencia capturada | P::play y capturedReference | E «current reference recaptures»; E «selective current capture»; I::testMachinePaletteNoReprojection | D: resume conserva; DONE recaptura; source map puede desaparecer; apply_color posterior no reescribe referencia current |
| ANIM-008 loop+current | P::evaluate/additive y scale por referencia fija | E «base and current loops», «scale capture», «loop color and representation» | D CPU: offset/rotación acumulativos; escala/color/representación no acumulativos. I: combinación exhaustiva de familias con todos los ciclos |
| ANIM-009 DONE/STOP | P::advance/stop; Q | E «complete non-loop states», «ongoing overflow»; I::testPublicTransitions | D: conserva pose final, stop desde RUNNING/PAUSED/DONE retira capa y respaldo |
| ANIM-010 PAUSED activo | P::pause/active/play; Q | E «complete non-loop states»; I::testPublicTransitions | D: pausa repetida no es toggle; resume no reinicia tiempo |
| ANIM-011 base runtime viva | P::appearance/sampleColor; DrawBindings cambia base | E «color interpolation, priority»; I::testSpawnAndColor y testMachinePaletteNoReprojection | D: color prioritario en RUNNING/PAUSED/DONE; base cambia; stop revela valor/ausencia; sin pista activa la base sigue viva |
| ANIM-012 tiempo no por frame | P::advance; R::updateObjectTime; render sin advance | E «determinism across temporal partitions»; W::testAnimationAcceptanceOpenCompiled | D: particiones representables, tolerancia explícita decimal, renders sin update mantienen tiempo; I: rendimiento no depende del número de ciclos, por fórmula y no iteración |
| ANIM-013 abierto/compilado | ProjectCompiler, K, World consume modelo común | D «CPU evaluator matches source»; I::testMicroexamplesOpenCompiled; W::testAnimationMicroexamplesGraphics y testAnimationAcceptanceOpenCompiled | D: ocho microejemplos, trazas y píxeles, captura/recuperación y Machine. I: todas las combinaciones de API inválida en ambos formatos; A: ejecución Linux de este diff |
| ANIM-014 captura selectiva | P::play máscara por eje y proyección del respaldo | E «selective current capture»; G::testAnimationGeometryCPU; W::testAnimationAcceptanceOpenCompiled | D: solo rotación conserva rotación, descarta offset/escala/color/rep; X no hereda Y; inactivas no amplían máscara; recuperación tampoco hereda propiedades ajenas |
| ANIM-015 snapshot draw | R::draw captura B antes del callback; Q modifica solo reproducción | W::testAnimationJavaScriptSnapshots y testAnimationAcceptanceOpenCompiled | D: play, stop y sustitución; consultas inmediatas; captura siguiente sin update; current usa apariencia nueva, no snapshot viejo; dibujo inmediato no se desplaza |
| ANIM-016 partición estable | P::capture; B ruta ABSENT prestada y ruta animada | A::stablePartitionAndOwnership; E «representation substitution»; G píxeles persist; ejemplo persist-order | D: orden exacto de seis elementos en ABSENT; [] y stop; sin persist conserva orden de entrada. I: no cambio de orden de objetos por separación respecto al sort de World y regresiones de depth |
| ANIM-017 recuperación atómica | P::evaluate/validateAppearance; B prepara todo antes de publicar; C valida conversiones | E «atomic overflow recovery» y «clock overflow»; G::testAnimationGeometryCPU; I::testMachinePaletteNoReprojection; W::testAnimationAcceptanceOpenCompiled | D: CPU y gráfica, fallo inicial/sustitución/recuperación/DONE, reloj válido continúa, reloj inválido se retiene, geometría no representable no se publica |
| ANIM-018 un diagnóstico | P::numericFailure, numericReported; R/Q solo registran warnings | E «ongoing overflow», «finite loop cancellation»; I::testMachinePaletteNoReprojection; W::testAnimationAcceptanceOpenCompiled | D: repetición/pausa/resume no repiten aviso, nueva reproducción sí; sesión continúa; no nuevo estado público |
| ANIM-019 duplicados tempranos | callback de J antes del DOM/merge; K::readAnimations antes de emplace; V tiempos ordenados | D «duplicates detected before overwrite», «binary duplicate names» | D: fuente, at repetido incluso fuera de duración, binario14, precedencia de paleta independiente |
| ANIM-020 límites de ciclo | P::advance, floor de división binary64 y producto redondeado volatile | E «base and current loops», «approved rounded decimal division», «loop color and representation»; D «rounded decimal cycle boundary»; I bitmap-invader | D: duración/cierre, saltos, n*Dend+D(t), resto exacto/decimal, keys discretas y vuelta del loop sin frame de cierre intermedio |
| ANIM-021 Machine previa | ProjectCompiler::projectObjectColors procesa keys; P::sampleColor no reproyecta | D «like overrides, shared resources and Machine projection»; I::testMachinePaletteNoReprojection; G colores propios/bitmap | D: midpoint (128,0,128,255) fuera de paleta roja/azul; current tras apply_color (192,0,64,255); explícitos y bitmap preservados |
| ANIM-022 14 / lectura12/13 | K::writeAnimations/readAnimations, FormatVersion=14; versión motor separada | D «format14 deterministic autonomous roundtrip», «legacy formats12 and13», «binary duplicate names»; CTest compiled-binary y format12-portability | D: lectura real de fixtures Windows12/13 y bitmap13, ausencia Animation/persist, píxeles guardados, corrupción/versiones inválidas y autonomía14. I: apariencia histórica completa12/13 sin golden raster previo. A: ejecución de esos fixtures en Linux con este diff |

## Casos de aceptación 10.1

| Caso | Evidencia y límites |
| --- | --- |
| 1 autonomía/formato | D: D «format14 deterministic autonomous roundtrip» elimina el JSON, compara bytes y datos; W aceptación elimina game completo con JS y carga14. D: lectura de fixtures12/13 y pixels bitmap13. I: no hay golden histórico rasterizado de todos los fixtures legacy. |
| 2 snapshot | D: W::testAnimationAcceptanceOpenCompiled para ambos modelos: base → pose → current → fallo gráfico → pausa/resume/stop, seis renders sin update, consulta y píxeles exactos. W::testAnimationJavaScriptSnapshots cubre también spawn desde draw y dibujo inmediato. |
| 3 máscara current | D: E captura por componente, X sin Y; W aceptación parte de offset/rotación/escala/color/rep y conserva solo rotación45, incluso con key inactiva Y. D: I Machine comprueba referencia current de color inmutable tras apply_color en abierto/14. |
| 4 persist | D: A orden exacto middle-A,last-A,first-A,middle-B,first-B,last-B → first-A,first-B,middle-A,middle-B,last-A,last-B. E/G/ejemplos cubren [], stop y captura. I: combinación de las seis entradas con todas las operaciones en14; los ejemplos14 prueban tres grupos y sustitución real. |
| 5 recuperación | D: E estados/reloj/desbordamientos; G fallback gráfico selectivo y recuperación válida; I fallo CPU por World, un warning y nueva vida de diagnóstico en abierto/14; W fallo gráfico atómico y sesión viva en abierto/14. A: sanitizador de memoria y ejecución Linux. |
| 6 duplicados | D: D fuente y binario; paleta conserva última entrada; duplicados se rechazan antes del almacenamiento final. |
| 7 ciclos/partición | D: E expectativas analíticas y particiones, cierre discreto y cero no neutro; D traza source/14; ejemplo bitmap cinco límites exactos. |
| 8 color | D: I paleta Machine solo roja/azul produce púrpura intermedio, no lo reproyecta; E alpha/ausencia/color vivo; G bitmap/propios; ejemplos color/bitmap abierto14. |
| 9 decimales | D: E división redondeada, producto separado sin FMA, nextafter y sin epsilon; D source/14 con E=1 y E=0.3. A: repetición GCC/Clang Linux. |

Los puntos I y A no se convierten en D por pasar CTest. Las variantes no ejecutadas
se mantienen visibles; esta auditoría no afirma cobertura exhaustiva de todo valor
binary64, todo estado de driver ni toda combinación de recursos.

## Integración y recursos

- Orden examinado: action → flush → motion → flush → attachments → collision →
  flush → tiempo State/Animation/Timer → dead/cleanup → iterator/flush.
  I::testLifecycleAndIsolation, testCollisionAndState y testPostTemporalCreation
  comprueban born, action, motion, collision, hidden/show, State, dead, creación
  antes/después de la fase temporal y aislamiento. Las suites Timer/lifecycle
  completas también se ejecutan. No se añade advance en draw ni consultas.
- Primitive/geometry/text/bitmap: G comprueba orden geométrico CPU, espejos,
  escala no uniforme, cero/negativa, UV/cuadrantes bitmap, texto contra referencia
  afín con métricas/espaciado/UTF-8/multilínea y persistencia. La visibilidad de
  algunas transformaciones de primitive/geometry se verifica por cantidad de
  píxeles, no por un golden completo de toda silueta: ese detalle es I, no D.
- `BitmapTextureCache::get` devuelve la textura ya encontrada; solo sube en miss.
  `clear` libera con contexto vigente; FlxSession/World poseen la caché. G comprueba
  identidad de textura. El nuevo invasor recorre cinco bitmaps compartidos y el
  test gráfico confirma reutilización de los cinco identificadores en ciclos
  sucesivos en una caché real. La caché del World usa la misma implementación;
  no se ha instrumentado su número global de llamadas OpenGL ni la memoria VRAM.
- Definición seleccionada inmutable copiada al comenzar, no por frame; bitmap
  compartido const; key representations mediante alias del owner. E comprueba que
  borrar el catálogo no invalida reproducción. A comprueba que stop no invalida
  un snapshot retenido y que el último snapshot libera definición/bitmap (weak
  observer expira). Q usa vistas de callback; I prueba vistas caducadas y continuidad.
  Esto acredita esos lifetimes, no una demostración universal de ausencia de fugas.

## Defectos demostrados y correcciones

### F5-01 — particiones asignadas en cada captura ABSENT (rendimiento)

Antes, ambos overloads de B invocaban AnimationBaseAppearance::capture siempre:
tres make_shared, vector(es) de elementos y copias de strings/geometry, incluso
sin reproducción. A reproduce el defecto: 3 asignaciones para un objeto vacío,
5 para un rectángulo. La regresión añadida falló antes del parche.

Corrección limitada: B recorre la representación base prestada durante la captura
ABSENT, filtrada establemente por persist. Las primitivas resultantes siguen
poseyendo sus datos; no conservan punteros prestados. Se conserva la preparación
y validación gráfica completa, sin atajos de seguridad. No cambia AnimationPlayback,
RuntimeObject, orden entre objetos, codec, APIs ni semántica.

Regresiones: A mide asignaciones (vacío0, rectángulo1), seis elementos persistentes
y supervivencia del snapshot tras borrar la representación fuente; G/W comparan
píxeles de las cuatro familias y todos los ejemplos.

### F5-02 — docs/schema Text imponían un mínimo no declarado (documentación)

El contrato VIS-010 exige fontSize; el parser existente acepta enteros y el builder
conserva `max(1,fontSize)` al dibujar. El schema y ambas páginas Visual exigían >=1,
por lo que rechazaban/documentaban incorrectamente entradas aceptadas, también en
representaciones de keys de Animation. No se añade una restricción al runtime ni
se elimina el comportamiento histórico del texto.

Corrección: schema pide integer y explica el mínimo del renderer; páginas ES/EN
coinciden. D «text font size keeps historical integer contract» prueba -1/0/1,
base/key, serialización14 y tamaño gráfico1, tanto ABSENT como con key seleccionada.

### Evidencia añadida que no es una funcionalidad ni un fallo de producción

- Aceptación de snapshot/current/recuperación con proyectos compilados reales y
  fuentes retiradas; antes parte de la evidencia era solo CPU o World construido.
- Machine discreta con píxeles intermedios no pertenecientes a su paleta, referencia
  current tras apply_color y recuperación CPU por World, en abierto y14.
- Microejemplo bitmap-invader solicitado expresamente durante esta fase.

## Medición reproducible de rendimiento

Ejecutable: `flx-animation-audit-tests`. Release x86-64, MSVC14.51.36231,
Windows. Modelo CPU no obtenido: la consulta CIM fue denegada; no se inventa hardware.
Cada caso calienta100 llamadas, cuenta una llamada y mide7 lotes de10000 llamadas
con steady_clock, informando mediana/mínimo/máximo. Sin gráficos, audio ni QuickJS
en los casos medidos. La captura validada genera geometría CPU de un rectángulo,
no incluye envío GPU ni un juego. El hook cuenta operator new/new[] ordinarios de
este ejecutable; no cuenta malloc de librerías C, aligned new, GPU ni memoria global.

Datos de las ejecuciones antes/después guardadas en los logs:

| Caso | Asignaciones antes → después | Mediana µs antes → después |
| --- | --- | --- |
| Snapshot ABSENT vacío | 3 → 0 | 0.17383 → 0.00688 |
| Snapshot ABSENT rectángulo, sin viewport | 5 → 1 | 0.27201 → 0.06587 |
| Snapshot ABSENT rectángulo validado | 12 → 8 | 0.55168 → 0.33117 |
| Advance ABSENT | 0 → 0 | 0.00167 → 0.00171 |
| Advance RUNNING, una pista | 0 → 0 | 0.01373 → 0.01320 |
| Snapshot RUNNING rectángulo validado | 12 → 12 | 0.55386 → 0.50438 |

Las diferencias pequeñas de tiempo no demuestran mejoras en rutas no modificadas.
No hay umbral temporal en los tests, porcentajes extrapolados a FPS ni estimaciones
presentadas como benchmarks. La aserción estable es el conteo de asignaciones del
caso controlado. Repeticiones posteriores pueden variar por carga y planificación.

**Costes aún presentes:** captura animada reconstruye grupos base; cada snapshot
prepara vectores/meshes, incluso en PAUSED/DONE, y reevalúa sin avanzar tiempo para
validar recuperación. En el caso mínimo RUNNING se midieron12 asignaciones. Algunas
podrían reducirse con caches/scratch buffers, pero requerirían política de
invalidación/ownership: esta fase no rediseña el renderer. No se acredita ausencia
de toda asignación evitable en toda ruta crítica ni un presupuesto de frame de juego.

## Compilación, pruebas y plataformas

Configuración local: `out/build/v040-audit-release`, CMAKE_BUILD_TYPE=Release.
Desde un entorno de desarrollo MSVC x64, con las dependencias ya configuradas:

```text
cmake --build out/build/v040-audit-release --clean-first -j 6
ctest --test-dir out/build/v040-audit-release --output-on-failure -j 4
out/build/v040-audit-release/flx-animation-audit-tests.exe
```

- Compilación limpia de flx, flx-core y consumidores: correcta.
- Tras las últimas ampliaciones de tests, recompilación incremental correcta y
  **CTest completo38/38**, 0 fallos. La última pasada tarda3.70s; es tiempo de esta
  ejecución, no un benchmark de rendimiento del motor.
- Pasada dirigida con FLX_TEST_HEADLESS=1: **5/5 suites**, 0 fallos. Los cuatro grupos
  gráficos de Animation se omiten explícitamente en esa pasada. Se ejecutan con
  contexto real en el CTest nativo, incluidos snapshots y todos los microejemplos.
- `flx --version`: 0.4.0. Formato14 permanece sin modificación. diff --check limpio.
- **Linux/GCC/Clang/Xvfb: A, no ejecutados.** WSL no está instalado y no se encontró
  GCC, Clang ni Docker utilizables. No se instala un sistema ni se publica el diff.
- Workflow linux-diagnostics examinado estáticamente: matriz GCC/Clang, CTest
  headless, después Xvfb con software GL y cinco suites, incluida runtime-drawing.
  Las nuevas suites entran por CTest headless; los nuevos casos gráficos están
  dentro de runtime-drawing y entran en el regex ya existente. Sin cambios YAML.
  El guard comprueba suites JUnit; aún se debe inspeccionar el log interno para
  confirmar que no aparece `[SKIP] Animation...` en la pasada gráfica.
- Xvfb no se ha probado aquí ni representa audio físico, Wayland nativo o hardware real.

Logs locales ignorados, en `out/build/`:
`animation-phase5-clean-build.log`, `animation-phase5-build.log`,
`animation-phase5-ctest.log`, `animation-phase5-ctest-summary.log`,
`animation-phase5-headless.log`, `animation-phase5-before.log`,
`animation-phase5-after.log` y `animation-phase5-doc-check.log`. El baseline del benchmark falla intencionalmente
su nueva aserción antes de corregir F5-01; no es un fallo pendiente del diff final.

## Documentación, tipos y ejemplos

Se contrastan las ocho funciones de Q, flx.d.ts, página Drawing ES/EN y entradas
A–Z: nombres únicos, anclas y enlaces, orden alfabético y retornos coherentes.
Esquemas JSON y scripts de ejemplo se comprueban sintácticamente. El schema Text
corregido se comparte con representation de Animation; no cambia el formato.
La comprobación estructural no es traducción lingüística exhaustiva ni una pasada
de TypeScript tsc o de un validador JSON Schema externo (no disponibles aquí).

Ocho microejemplos Animation, ejecutados en abierto y compilado:
bitmap-invader, color, interpolation, loop-base, loop-current, pause-resume,
representation y persist-order. I compara trazas y expectativas temporales;
W compara imágenes y expectativas concretas en los siete temporales; G comprueba
persist-order estático en ambos modos. No se afirma haber ejecutado todos los
microejemplos ajenos a Animation del repositorio fuera de las suites existentes.

**bitmap-invader:** cinco bitmaps11×8, paleta local verde/transparente, sustitución
cada0.25s, ciclo1.25s, inicio explícito desde born, sin imágenes externas/audio/input.
Se comprueban los88 centros de celda de cada imagen, cinco imágenes distintas,
igualdad de píxeles al repetir ciclo, límite temporal exacto y reutilización de
texturas. Índice de microexamples y página Examples ES/EN enlazan el README bilingüe.
No se duplica persist-order ni se introducen APIs o funciones de motor nuevas.

## Riesgos pendientes y puertas de cierre

| Gravedad | Estado / acción necesaria |
| --- | --- |
| Alta para cierre multiplataforma | Linux GCC/Clang y Xvfb sin resultados de este diff. Publicar por la desarrolladora y ejecutar linux-diagnostics; exigir cero fallos y ningún skip interno Animation en Xvfb. No cerrar antes. |
| Media, rendimiento | Se eliminan particiones en ABSENT, pero siguen asignaciones/meshes en rutas activas y preparación de snapshots. Medir escenas representativas antes de prometer capacidad/FPS; no introducir caches sin reglas de invalidación. |
| Media, recursos | No se ejecutaron ASan/LSan/Valgrind ni un perfil de heap/VRAM prolongado. Ownership examinado y regresión weak_ptr pasan, pero la ausencia global de fugas permanece sin acreditar. |
| Baja, compatibilidad histórica | Se comprueban datos12/13, pixel bitmap13 y regresiones de dibujo; no existe un golden histórico completo rasterizado de cada fixture legacy. No equiparar lectura correcta a identidad visual de cualquier proyecto antiguo. |
| Baja, documentación | tsc y validador externo de schemas no ejecutados; se validan estructura/enlaces/símbolos/sintaxis y carga real por FLX. Sin evidencia de incoherencia funcional restante en las referencias Animation examinadas. |
| Baja, límites gráficos existentes | Texto pequeño/atlas/point filtering mantienen sus límites previos; no se presenta la nitidez como corregida. Ensayos de drivers/hardware/Wayland siguen fuera de esta validación Windows. |

La recomendación con reservas no autoriza publicación ni declara Animation cerrada.
No hay commits, push, tags, releases, cambios remotos, dependencias nuevas ni cambios
de contratos normativos. La desarrolladora conserva la revisión del diff y el cierre.

## Archivos de esta fase

- `engine/graphics/RepresentationPrimitiveBuilder.cpp`: F5-01.
- `CMakeLists.txt` y nuevo `tests/runtime/AnimationAuditTests.cpp`: conteo/medición,
  partición estable y ownership.
- `tests/loading/AnimationDeclarationTests.cpp`: contrato Text histórico.
- `tests/runtime/AnimationIntegrationTests.cpp`: Machine/current/CPU recovery y bitmap.
- `tests/runtime/RuntimeDrawingTests.cpp`: aceptación abierta14, píxeles y bitmap loop.
- `docs/schemas/visual.schema.json`, `docs/html/{es,en}/json/visual.html`: F5-02.
- `microexamples/animation/bitmap-invader/{bitmap-invader.flx,bitmap-invader.json,
  bitmap-invader.js,README.md}`: ejemplo solicitado, cinco fotogramas.
- `microexamples/README.md`, `docs/html/{es,en}/examples.html`: enlaces al ejemplo.
- Esta auditoría y notas en ANIMATION_EVALUATOR/LINUX_PORTABILITY_AUDIT para enlazar
  el estado actual sin atribuirle validaciones de fases anteriores.
