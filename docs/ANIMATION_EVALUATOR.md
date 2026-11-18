# Animation — núcleo temporal y renderer, fases 2 y 3

Esta nota describe la implementación CPU de `spec/animation.md`. El contrato
normativo no cambia. No existe todavía una API JavaScript de Animation ni
conexión del evaluador con `RuntimeWorld::update`. La fase 3 conecta su apariencia
con el renderer declarativo, comprobada mediante control interno C++. Los proyectos
siguen sin poder iniciar reproducción por JavaScript. `persist` está disponible
sin reproducción y tiene un microejemplo ejecutable.
El formato sigue siendo 14, con lectura de 12/13; la versión del motor no cambia.

## Arquitectura

- `engine/runtime/AnimationPlayback.h/.cpp` contiene el evaluador sin dependencias
  de QuickJS, CLI o llamadas a dispositivos. Reutiliza los tipos de datos Visual.
- Cada `RuntimeObject` tiene un miembro `animation` inicialmente ABSENT. Crear otra
  instancia no copia una reproducción existente ni cambia sus campos espaciales.
- Una reproducción nueva conserva una copia inmutable de la definición validada;
  sus números y representaciones siguen la fase 1. Los bitmaps permanecen
  compartidos e inmutables. No se interpretan colores ni matrices durante avances.
- `AnimationReference` contiene solo componentes controlados por keys activas.
  `AnimationControls` conserva esa máscara por componente.
- La última `Layer` válida conserva transformaciones locales, color heredable y
  referencia a la sección sustituible. Su publicación es atómica. No contiene
  posición, ángulo, tamaño mundial, colliders, texturas ni callbacks.
- `AnimationBaseAppearance::capture` particiona de forma estable first/replaceable/
  last. `appearance(base)` combina la contribución con la base runtime viva; no
  captura persistentes dentro de la sección sustituible ni toca el renderer.

## Operaciones internas

`play` resuelve un nombre en el mapa compilado. RUNNING no reinicia; PAUSED reanuda
sin nueva captura; DONE o un nombre distinto inician una reproducción nueva.
La pose cero se evalúa antes de devolver. Los nombres inválidos dejan intacta la
reproducción. `pause` no es toggle; `stop` elimina también el respaldo numérico.

`advance(delta, diagnostics)` es una operación explícita del propietario de la
instancia. No se ha conectado al World en esta fase. El futuro adaptador deberá
respetar la fase temporal de Timer, el nacimiento/muerte y el snapshot de draw;
no debe avanzar desde render ni desde consultas. Las pruebas CPU no acreditan
esa integración pendiente del lifecycle o de JS.

`appearance(base)` y las consultas de estado, nombre y tiempo son puras. La base
aportada debe describir la misma instancia, con los grupos de representación
inmutables y el color runtime actualizado. Sin control de color, sus cambios se
reflejan incluso con reproducción pausada o finalizada. Con control de color, la
trayectoria usa su referencia capturada. El flag de presencia del color runtime
se conserva separado y nunca se convierte en una declaración por animar.

## Tiempo, interpolación y ciclos

Se usan `double` y `std::lerp` para los cinco componentes. La interpolación de
extremos de signos opuestos evita desbordar una resta intermedia. Rotación no se
normaliza; escala cero y negativa son válidas. Las keys inactivas no participan.
Si una suma de referencia y desplazamiento cíclico desborda como intermedio pero
puede cancelarse en el resultado final, se reintenta con aritmética escalada por
potencias de dos; esto no limita ni cambia la escala visual.
Los números de ciclo y el tiempo local se calculan desde elapsed/duration, sin
integrar transformaciones por frame ni iterar una vez por ciclo atravesado.
La precisión de límites sigue la decisión aprobada: `floor(E/d)` se aplica a la
división binary64 redondeada y el producto `n*d` se redondea antes de restarlo.
El producto se materializa para impedir que una contracción FMA cambie el resto.
Con duration=0.1 y elapsed=1 se obtienen 10 ciclos y resto cero; no se usa el
cociente exacto de los racionales representados ni epsilon para corregir 0.3/0.1.
Los ciclos usan `[0,duration)`; un límite exacto comienza el ciclo nuevo y deja
`left=duration`. Sin loop, el endpoint se limita a duration y produce DONE.

En loop+current, offset/rotación usan `referencia + n*Dend + D(t)`. No se resta la
key de cero a Dend. Escala usa el factor del ciclo actual, sin acumulación.
Color y representación vuelven a la referencia capturada en cada ciclo.

El color se interpola como RGBA normalizado ya procesado por Machine. Se convierte
a byte con `std::round` al entero más cercano (mitades hacia arriba para canales
no negativos), siguiendo la convención existente de VideoColorProcessor. No se
reproyecta por frame ni se modifican los colores propios o píxeles del bitmap.
`from: current` captura el color efectivo así obtenido.

## Recuperación numérica

Se comprueban reloj derivado, componentes y resultados finitos/representables en
binary64. Un delta positivo que ya no puede incrementar el reloj representado se
diagnostica y no se confirma. No hay epsilon temporal, límite artificial de ciclos,
clamp de escala ni normalización de ángulos. El endpoint no-loop se calcula sin
provocar primero un overflow innecesario en la suma del tiempo.

Un fallo de reloj mantiene reloj, estado y última contribución válidos. Un fallo
de pose con reloj representable confirma ese reloj/estado, incluso DONE, pero
retiene la contribución visual anterior completa. Una primera pose inválida tras
sustitución usa la contribución previa limitada a los componentes controlados por
la reproducción nueva como respaldo; su referencia selectiva permanece independiente. Una evaluación posterior válida
sustituye todo el respaldo. Stop siempre lo retira.

`FLX-RUNTIME-00020 InvalidAnimationOperation` informa entradas internas inválidas.
`FLX-RUNTIME-00021 AnimationNumericRecovery` es warning recuperable, como máximo
una vez por reproducción e instancia, incluso si el fallo desaparece y reaparece.
Reanudar no reinicia esta supresión; una reproducción nueva sí. No se crea estado
público ERROR ni se marca una sesión como fallida por estos warnings.

La geometría mundial y su conversión posterior a los tipos gráficos todavía no
se calculan aquí. Una transformación finita binary64 no garantiza que esa futura
conversión o geometría sea representable. La fase de renderer deberá comprobarlo
antes de emitir datos gráficos y respetar la misma recuperación normativa; las
pruebas de fase 2 no validan esa frontera ni resultados rasterizados.

## Pruebas y fixtures

`flx-animation-evaluation-tests` compila únicamente el evaluador, Diagnostics y la
suite; no enlaza raylib, QuickJS, flx-test-engine ni CLI. Usa los headers raylib
solo por los tipos de datos existentes y nlohmann/json para la traza de prueba.
No crea ventana, audio, contexto gráfico ni motor JS.

`tests/fixtures/animation/temporal-loop.json` es una declaración mínima real:
loop+current con offset, rotación y escala. `temporal-loop.trace.json` contiene
expectativas analíticas y deltas de prueba, no sintaxis pública FLX. Son base para
un microejemplo futuro, no una demostración visual ejecutable en esta fase.
La suite declarativa compila el JSON real, lo serializa y compara el evaluador de
ambas definiciones con la misma traza, sin perder su autonomía compilada.

La regresión de RuntimeObject comprueba aislamiento entre instancias y ausencia
de cambios en origen, posición, tamaño, ángulo, velocidades, colliders, visibilidad,
estado, profundidad, presencia de color y timers. Las suites existentes protegen
el dibujo, Timer, lifecycle, consumidores flx-core y ejemplos ya ejecutables.

## Validación local de fase 2 (histórica)

Windows x86-64, MSVC y CMake Release: compilación limpia de flx, flx-core,
consumidores y pruebas. CTest completo: 36/36; suite CPU: 18 grupos sin skips.
Se verifica equivalencia abierta/formato 14, incluidos límites decimales aprobados,
y no mutación de estado espacial/físico al usar el miembro de RuntimeObject.
El target CPU enlaza solo sus tres unidades C++ y las bibliotecas estándar;
raylib y QuickJS no aparecen como bibliotecas de enlace de ese ejecutable.
Linux/GCC/Clang no se han ejecutado en esta entrega; deberá repetirse el runner.
La geometría/rasterización animada, el lifecycle temporal de World y los bindings
siguen pendientes y no se consideran validados por estos resultados.

```text
cmake --build out/build/v040-audit-release
ctest --test-dir out/build/v040-audit-release --output-on-failure
```

Cambios de infraestructura: CMake registra el evaluador en flx-core y su variante
de pruebas, además del ejecutable CPU aislado. El registro de diagnósticos y sus
pruebas incorporan los dos códigos. Las páginas Object/Diagnostics ES y EN reflejan
el estado parcial sin publicar APIs JS. La única precisión normativa añadida en
esta fase es la división binary64 redondeada aprobada explícitamente para los
límites temporales, con su caso de aceptación en `spec/animation.md`.

## Renderer declarativo — fase 3

`RepresentationPrimitiveBuilder` consume `appearance(base)` sin escribir en el
estado espacial del RuntimeObject. World captura y valida todas las primitivas
antes de los callbacks draw. El snapshot conserva recursos inmutables y geometría;
play/stop posteriores no lo reescriben. El dibujo inmediato no pasa por esta capa.

`DeclarativeGeometry` prepara posiciones en binary64 y comprueba conversiones
world/raster/clip-space antes de emitir datos. Cada presentación wrap se valida
con los offsets históricos de la instancia; no se cambian bounds, colliders ni
su selección de presentaciones. La transformación es escala → rotación animada
→ offset → rotación/posición mundial → raster, manteniendo el pivot.

Los quads bitmap y de glifos conservan UV al corregir winding tras espejos. Bitmap
usa la caché existente del World y color WHITE; no se reinterpreta la matriz ni
se suben texturas nuevas por frame. Text usa el atlas predeterminado existente,
sin crear render textures o fuentes para animar.

El texto parte de las métricas y el espaciado raster históricos, incluidos centrado
por línea, multilínea, UTF-8, tabs y terminación NUL. Ese contorno base se expresa
en coordenadas locales mediante la inversa de la proyección y rotación del objeto;
a partir de ahí se aplica la composición normativa. Una transformación identidad
mantiene el camino histórico. No se corrige ni se promete corregir la nitidez de
texto pequeño. Los trazos de primitive/geometry conservan su grosor raster histórico;
se transforman sus puntos, sin introducir una API de grosor.

Para geometría fill se conserva par-impar, incluido el contorno. El barrido se
recorta al viewport: es clipping de raster, no un clamp de valores Animation.
Esto limita el trabajo a filas visibles y evita casts enteros fuera de rango.
Los trazos preparados se calculan con aritmética double antes de estrechar a
float, evitando overflow del cálculo de longitud de DrawLineEx. Una geometry de
un punto transforma su celda histórica completa, también bajo escala cero.

`AnimationPlayback::validateAppearance` es el puente numérico sin dependencia de
raylib: acepta un callback de validación de la contribución completa. No avanza
elapsed, ciclos o estados. Permite reevaluar también PAUSED/DONE para recuperar
una pose válida sin recapturar referencia. El respaldo gráfico solo se confirma
cuando todas las familias/presentaciones han pasado. Tanto el respaldo CPU como
el gráfico se proyectan a la máscara de la reproducción nueva: no pueden retener
color, representación, offset o rotación que esta no controle (ANIM-003/014).

Si el candidato no es representable, se conserva el respaldo local válido y se
usa la supresión de diagnóstico de la misma reproducción. World registra el
código y la identidad de instancia mediante Logger, como los avisos runtime
existentes; el overload validado también recibe Diagnostics para consumidores
internos. Si el contexto/estado base actual tampoco permite emitir el respaldo,
se omite la contribución declarativa completa, sin clamp, geometría parcial ni
fallo de proceso. La retirada de Animation sigue mostrando la base runtime.

El overload de build con contexto es la ruta de producción para validar y recuperar
antes de emitir. El overload const histórico produce snapshots sin validar contra
un viewport; un consumidor que reproduzca Animation debe usar la ruta con contexto
para obtener recuperación por instancia. Los overloads históricos de renderer sin
caché siguen siendo conveniencias; World siempre aporta su caché persistente.

## Pruebas y ejemplos — fase 3

Se amplía `flx-runtime-drawing-tests` con geometría CPU y gráficos de contexto real,
sin añadir otro workflow. Su ejecución Xvfb existente incluye estas pruebas; bajo
FLX_TEST_HEADLESS se omite explícitamente solo el caso gráfico nuevo.

Se comprueban analíticamente orden de transformaciones, salida anisotrópica, métricas
de una fuente sintética sin dispositivo, presencia de persist y respaldo filtrado.
Con contexto real se comparan píxeles para identidad de todas las familias, ambos
espejos y UV, transformación combinada, texto contra referencia afín independiente,
colores, sección vacía, celda de geometry, snapshot, dibujo inmediato y caché.
`renderer-object.json` produce los mismos píxeles abierto y compilado. El ejemplo
persist-order se ejecuta por FlxSession en ambos modos y comprueba azul/rojo/verde.

No se añaden bindings, avance automático, formatos nuevos ni workflows de release.
La validación Linux/GCC/Clang real deberá repetirse en el runner. La fase gráfica
no elimina las limitaciones del atlas/fuente predeterminada y el muestreo puntual.

### Resultado local de fase 3

Windows x86-64 / MSVC / Release: compilación limpia de flx, flx-core y consumidores;
CTest completo 36/36. La suite de dibujo incluye sus casos CPU y de contexto real.
La ejecución dirigida sin dispositivo pasa el evaluador temporal y los casos CPU
de dibujo; el caso nuevo de píxeles se omite explícitamente en headless y se ejecuta
con contexto real en la pasada nativa. No se ha ejecutado Linux localmente.

Se añadieron regresiones de clipping con segmentos de magnitud 1e30 y polígonos
1e8: deben producir el trazo y el relleno visibles esperados sin perder precisión
al intersectar el viewport ni recorrer filas fuera de pantalla. La banda raster
de un píxel conserva las reglas de extremos de GL_LINES, sin alterar Animation.
