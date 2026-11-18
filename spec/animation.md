# Animation — contrato normativo para FLX v0.4.0

**Estado:** contrato aprobado para implementación; conformidad de la implementación pendiente de validación.
**Ámbito:** representación visual declarativa y reproducción por `RuntimeObject`.  
**Dependencias normativas:** `spec/visual-representation.md`, `spec/visual-bitmap-palette.md`, `spec/spatial_model.md`, `spec/scripting/runtime_object.md`, `spec/scripting/timer.md`.

**Integración de compilación:** `spec/compiler.md`, `spec/compiled-project.md` y `spec/compiled-binary.md`. Las extensiones y discrepancias documentales acotadas se identifican en la sección 12; este documento no declara implementado el formato 14.

## 1. Propósito y límites

Animation modifica exclusivamente la **apariencia dibujada** de una instancia. NO DEBE modificar `x`, `y`, `width`, `height`, `angle`, `origin`, `visible`, `depth`, colliders, velocidades, estado lógico ni física. La apariencia animada NO DEBE modificar los valores observables de `RuntimeObject` JS.

Una instancia PUEDE reproducir **una sola** animación a la vez. No hay blending, easing, eventos de key, pistas simultáneas, animación de colisiones, visibilidad ni profundidad en v0.4.0. La reproducción es explícita mediante JavaScript; no existe `initial` ni autoplay implícito.

## 2. Declaración

`animations` es un diccionario opcional en la raíz de la definición de objeto, al mismo nivel que `visual` y `states`. Cada clave es un nombre no vacío y único.

Los nombres duplicados dentro de una misma declaración de `animations` DEBEN detectarse y rechazarse antes de que un parser, mapa, normalización o lectura binaria sobrescriba una entrada. No es válido conservar silenciosamente la primera o la última. Esta regla no redefine las reglas existentes de composición y overrides entre declaraciones diferentes, ni la precedencia de claves de `visual.palette`.

```json
{
  "origin": { "x": 160, "y": 90 },
  "size": { "width": 16, "height": 16 },
  "visual": {
    "color": "white",
    "representation": [
      { "primitive": "rectangle", "persist": "first" },
      { "bitmap": { "pixels": [["red", "red"], ["red", "red"]] } },
      { "text": "!", "fontSize": 8, "persist": "last" }
    ]
  },
  "animations": {
    "pulse": {
      "duration": 1,
      "loop": true,
      "from": "base",
      "keys": [
        { "at": 0, "transform": { "scale": { "x": 1, "y": 1 } } },
        { "at": 0.5, "transform": { "scale": { "x": 1.3, "y": 1.3 } }, "color": "red" },
        { "at": 1, "transform": { "scale": { "x": 1, "y": 1 } }, "color": "white" }
      ]
    }
  }
}
```

Cada animación admite:

| Campo | Tipo | Regla |
| --- | --- | --- |
| `duration` | número | Obligatorio, finito y > 0; segundos |
| `loop` | booleano | Opcional, predeterminado `false` |
| `from` | `"base"` / `"current"` | Opcional, predeterminado `"base"` |
| `keys` | array | Obligatorio, al menos una key |

Cada key requiere `at` (segundos, finito, >= 0) y al menos una propiedad animable entre `transform`, `color`, `representation`. El orden de declaración de keys no determina el resultado: se ordenan por `at`. Dos keys con el mismo `at` en una animación son inválidas, incluso si están fuera de duración.

Las keys con `at > duration` son **válidas**, pero generan warning y quedan inactivas. Si ninguna key es activa, la animación sigue siendo válida, con warning de ausencia de efecto. `at: duration` es válido y se evalúa como cierre del ciclo. Ningún campo omitido en una key debe rellenarse con valores predeterminados que produzcan cambios implícitos.

## 3. Propiedades animables

### 3.1 Transformación local

```json
{
  "at": 0.25,
  "transform": {
    "offset": { "x": 10, "y": -5 },
    "rotation": 180,
    "scale": { "x": 1.2, "y": 0.8 }
  }
}
```

- `offset.x/y`: números finitos en unidades lógicas FLX, sobre ejes locales del objeto.
- `rotation`: número finito en **grados**; se interpola sin normalización ni camino angular mínimo. `720` expresa dos vueltas.
- `scale.x/y`: números finitos, independientes por eje. Cero y negativos son válidos (incluido espejo).
- Todos los componentes son opcionales, pero un objeto `transform` vacío no constituye una propiedad animable válida.
- Los componentes se interpolan **linealmente e independientemente**.

Animation no modifica el pivot del objeto. La composición geométrica es, conceptualmente, **scale → rotation animada → offset animado → rotación/traslación mundial del RuntimeObject**. En notación de puntos columna:

```text
worldPoint = object.position
           + rotate(object.angle,
               animated.offset + rotate(animated.rotation,
                   scale(animated.scale, localPoint)))
```

El escalado y la rotación se aplican respecto al pivot local existente. Los colliders NO consumen esta transformación.

### 3.2 Color

`color` es un token de color declarativo válido según el sistema Visual/Palette existente. No se introduce una nueva sintaxis JSON RGBA en este contrato. El motor resuelve los colores con la paleta efectiva y Machine según los contratos vigentes; la interpolación es lineal **por canal RGBA normalizado** entre colores ya resueltos. El redondeo final a canales enteros debe ser consistente en todas las rutas de ejecución.

Los destinos declarados y el color declarado de referencia pasan por el procesamiento Machine existente antes de la reproducción. Una referencia runtime se captura como su color efectivo, sin reprocesarla. La interpolación NO DEBE volver a proyectarse por Machine en cada frame: los colores intermedios pueden no pertenecer a una paleta discreta. El color por defecto se captura tal como lo usa el renderer; no se introduce una proyección adicional ni una declaración de color para suplir su ausencia.

Solo afecta al color base efectivo `visual.color` heredado por `primitive`, `geometry` y `text` que no declaren color propio. No tiñe bitmaps ni elementos con color explícito. Si el objeto carece de `visual.color`, la referencia es el color efectivo por defecto del renderer, sin convertir esa ausencia en una declaración permanente.

`apply_color()` y `restore_color()` siguen modificando el color runtime. Cuando la animación contiene al menos una key activa de `color`, su capa visual tiene prioridad durante RUNNING, PAUSED y DONE. El color runtime puede cambiar sin recalcular la interpolación ya iniciada. Al detener la animación, se muestra el color runtime vigente, incluida la ausencia de color declarado.

Si la animación no tiene keys **activas** de color, no controla el color: las operaciones JS se reflejan normalmente.

### 3.3 Representation

Una key puede declarar `"representation": [...]` con un array de elementos Visual válidos. Es una **sustitución discreta completa** de la sección animable, no una interpolación ni un parche de elementos. `[]` es válido y vacía solo dicha sección. La nueva representación se activa en su instante `at`; si se omite, continúa la anterior.

`persist` solo se admite en elementos de `visual.representation` base, con valores `"first"` o `"last"`. `persist` dentro de `keys[].representation` es error de validación. Los elementos base sin `persist` constituyen la sección sustituible. Orden efectivo:

```text
base persist:first (orden relativo original)
+ representación sustituible efectiva
+ base persist:last (orden relativo original)
```

Los elementos persistentes no se duplican al cambiar de animación ni al capturar `from: "current"`; se dibujan una vez y reciben la misma transformación visual animada que el resto. No se admite intercalarlos entre elementos animables.

Este orden se aplica también en ABSENT, antes de reproducir y después de `stop_animation()`. Sin reproducción, la sección sustituible es la sección base sin `persist`. La partición es estable: conserva el orden declarado dentro de cada uno de los tres grupos y no modifica el array fuente. Sin elementos `persist`, el orden es exactamente el de Visual Representation. La agrupación es una extensión explícita para elementos que optan por `persist`, no una ordenación de objetos por profundidad ni una alteración del dibujo inmediato.

La sustitución de representación NO altera el tamaño, posición, collider ni profundidad del RuntimeObject. Los elementos sustitutos respetan las reglas de tamaño, color, bitmap y paleta existentes.

## 4. Referencia inicial, keys parciales y evaluación

Al comenzar una reproducción nueva se captura una **referencia inmutable para esa reproducción**:

- `from: "base"`: apariencia runtime sin la capa de Animation anterior, incluyendo modificaciones runtime vigentes (por ejemplo `apply_color`).
- `from: "current"`: apariencia efectiva en el instante de la sustitución, incluida la animación anterior si estaba RUNNING, PAUSED o DONE, limitada a las propiedades controladas por la nueva animación.

El conjunto controlado se determina exclusivamente por las keys activas (`at <= duration`) de la nueva animación. Para transformación, se calcula por componente: `offset.x`, `offset.y`, `rotation`, `scale.x` y `scale.y`. Color y representación son propiedades independientes. Una key inactiva no amplía este conjunto.

La referencia incluye únicamente los componentes controlados: color efectivo, sección sustituible o componentes de transformación, según corresponda. Los componentes de transformación no controlados toman su valor neutro; el color no controlado sigue el color runtime vigente y la representación no controlada usa la sección sustituible base. No se heredan contribuciones de una animación anterior para propiedades que la nueva no controla. Una animación con todas las keys inactivas no conserva efectos de la reproducción sustituida.

La captura es inmutable solo para las propiedades controladas. Los elementos `persist` permanecen anclados a la representación base y nunca se copian en la sección sustituible. Capturar representación no captura una textura ni el estado de reproducción de otra instancia.

La transformación neutra de Animation es `offset=(0,0)`, `rotation=0`, `scale=(1,1)`. Para una reproducción iniciada desde `base`, estos valores describen la apariencia sin Animation. Para una reproducción desde `current`, los valores de keys de `offset` y `rotation` se interpretan como **desplazamientos respecto a la referencia capturada**; los de `scale` como factores respecto a ella. Esto permite que una key `rotation: 0` en `at: 0` conserve la orientación capturada y que `rotation: 180` la incremente en 180°.

Para cada componente numérico:

1. Se considera su valor neutro de referencia en `t=0` cuando no existe una key de ese componente en `at=0`.
2. Se interpola linealmente entre las keys que **sí declaran ese componente**, incluyendo la referencia inicial cuando corresponda.
3. Tras su última key activa, se mantiene el último valor hasta `duration`.
4. Omitir un componente en una key de otra propiedad NO reinicia ni modifica su trayectoria.

Para color, las keys contienen **destinos absolutos**; se interpola desde el color de referencia hasta el primer destino y entre destinos sucesivos. La representación se conserva hasta una key explícita.

**Nota de precisión:** el valor de una key de transformación se expresa en el espacio de la reproducción (relativo a su referencia capturada), no como una escritura de `RuntimeObject.angle` ni como una nueva posición mundial.

## 5. Repetición

El tiempo `at` es relativo al **ciclo actual**. La reproducción usa tiempo transcurrido y número de ciclos atravesados, nunca sumas por frame de transformaciones o escalas.

### 5.0 Límites temporales deterministas

Sea `E >= 0` el tiempo transcurrido de reproducción y `d > 0` su duración. Sin loop, se evalúa `t = min(E, d)` y `E >= d` produce DONE. Con loop, se usan `n = floor(E / d)` y `t = E - n * d`, con `0 <= t < d`. En un múltiplo exacto `E = k * d`, se evalúa el inicio del ciclo `k`, con `t = 0`; `animation_left()` devuelve `d`, no cero. Al iniciar también se usa `n = 0`, `t = 0`.

Una key discreta se aplica cuando `at <= t`; la igualdad no se difiere al frame siguiente. La key de cierre `at = d` se evalúa al finalizar una reproducción sin loop y al calcular el cierre/acumulación de un ciclo con loop, aunque no se dibuje por separado. No se introducen ventanas epsilon que adelanten o retrasen keys ni un límite de ciclos procesados por update.

La misma referencia y el mismo tiempo transcurrido representado DEBEN producir la misma pose y consultas, independientemente del número de renders o de cómo se haya atravesado los ciclos. Las pruebas de partición temporal usarán totales exactamente representables o una tolerancia numérica explícita para el error de acumulación; no exigirán igualdad binaria entre sumas flotantes distintas ni permitirán cambiar de ciclo mediante tolerancias. Las operaciones temporales y sus conversiones DEBEN comprobar representabilidad antes de consumir un resultado inválido.

### 5.1 `loop: false`

La reproducción llega a `duration`, evalúa su pose final, pasa a DONE y retiene la apariencia final hasta que se reinicie, sustituya o detenga.

### 5.2 `loop: true`, `from: "base"`

Cada ciclo se evalúa desde la **misma referencia capturada al iniciar la reproducción**. No se acumula ningún componente. Se permiten discontinuidades al cruzar el límite de ciclo.

### 5.3 `loop: true`, `from: "current"`

- **Offset y rotación:** se acumula el **desplazamiento neto de la trayectoria de un ciclo**. Sea `D(t)` la trayectoria local interpolada desde su valor neutro y `Dend = D(duration)`: en el ciclo `n` (empezando en cero), `valor = referencia + n * Dend + D(t)`.
- **Scale:** no se acumula. En todos los ciclos se aplica el factor local `S(t)` a la escala de referencia capturada al comenzar la reproducción.
- **Color:** no se acumula; en cada ciclo se interpola desde el mismo color de referencia capturado al comenzar la reproducción. Los destinos son absolutos.
- **Representation:** no se acumula; cada ciclo comienza con la sección sustituible de referencia capturada y aplica sus keys discretas.

El motor DEBE evitar desbordamientos y valores no finitos en geometría y transformación, conforme a la recuperación de la sección 8.1. No se define un límite artificial de escala ni una normalización angular silenciosa.

La key en `at: duration` participa en la pose de cierre y en el cálculo del desplazamiento neto, incluso si un frame salta directamente al siguiente ciclo. La evaluación del ciclo nuevo no exige renderizar separadamente el fotograma final del anterior.

## 6. API JavaScript

```javascript
play_animation(object, name)
pause_animation(object)
stop_animation(object)

animation_current(object)
animation_active(object)
animation_paused(object)
animation_done(object)
animation_left(object)
```

El nombre debe ser una cadena no vacía que identifique una animación declarada en el objeto. Las llamadas inválidas registran diagnóstico y NO modifican la reproducción existente. Las consultas no crean ni modifican estado.

| Estado previo | `play_animation` mismo nombre | `play_animation` nombre diferente |
| --- | --- | --- |
| ABSENT | Inicia | Inicia |
| RUNNING | Sin efecto | Sustituye inmediatamente |
| PAUSED | Reanuda, sin recapturar referencia | Sustituye inmediatamente |
| DONE | Reinicia y recaptura referencia | Sustituye inmediatamente |

Al sustituir una animación, la nueva aplica su propia política `from` y la anterior deja de contribuir sin restauración intermedia visible. `stop_animation()` retira la capa Animation desde RUNNING, PAUSED o DONE; sobre ABSENT no tiene efecto.

**Coherencia normativa con Timer:** `pause_animation()` **NO es toggle**. Cambia RUNNING a PAUSED; sobre ABSENT, PAUSED o DONE no tiene efecto. Para reanudar se usa `play_animation(object, mismoNombre)`.

| Estado | `animation_current` | `animation_active` | `animation_paused` | `animation_done` | `animation_left` |
| --- | --- | --- | --- | --- | --- |
| ABSENT | `null` | `false` | `false` | `false` | `0` |
| RUNNING, sin loop | nombre | `true` | `false` | `false` | segundos restantes > 0 |
| PAUSED, sin loop | nombre | `true` | `true` | `false` | restante congelado > 0 |
| DONE | nombre | `false` | `false` | `true` | `0` |
| RUNNING, con loop | nombre | `true` | `false` | `false` | restantes del ciclo actual |
| PAUSED, con loop | nombre | `true` | `true` | `false` | restantes del ciclo congelados |

`loop: true` no produce DONE por finalización de ciclo. `animation_left()` informa segundos restantes **del ciclo actual**, no de una reproducción infinita.

## 7. Ciclo de vida e integración

- Cada instancia mantiene su propia reproducción; dos instancias del mismo objeto declarado no comparten estado animado.
- El avance de Animation usa el tiempo de simulación del objeto y NO depende de visibilidad. `hide()` no pausa Animation; `state_to()` no la reinicia.
- Una instancia destruida no sigue avanzando Animation. La animación no sobrevive a la destrucción ni se transfiere a otra instancia.
- Una llamada `play_animation()` debe establecer inmediatamente el nuevo estado y evaluar la pose de `at=0`, sujeta a la recuperación numérica de la sección 8.1. Las consultas posteriores del callback observan el estado nuevo; no se añade una API de consulta de pose. Esto no reconstruye primitivas ya capturadas por el dibujo.
- Se conserva el snapshot existente: la representación declarativa de un objeto se captura antes de sus callbacks `draw`. Un play, pause, stop o cambio de animación solicitado desde `draw` no modifica retroactivamente ese snapshot; su efecto visual aparece en la siguiente captura. Un `from: "current"` posterior captura el estado efectivo actualizado, no una imagen del snapshot ya emitido. No se reordena ni transforma por Animation el dibujo inmediato.
- El avance temporal ocurre una sola vez por update, en la fase temporal posterior a collision y anterior a dead/eliminación, como Timer. Un inicio en `action`, `motion` o `collision` conserva `at=0` durante ese callback y, si sigue RUNNING, recibe el delta en esa fase del mismo update. Un inicio en `born` durante la carga conserva `at=0` hasta el primer update; una creación durante un update recibe avance solo si alcanza viva la fase temporal pendiente. Un inicio desde `draw`, posterior a esa fase, no recibe un avance retroactivo. Renderizar o consultar no avanza el reloj. `dead` puede consultar el estado final antes de eliminar la instancia.
- La renderización consume la apariencia efectiva, pero nunca escribe la transformación animada en `RuntimeObject` ni en Collision.

## 8. Validación y diagnóstico

Errores de definición (antes de runtime): `animations` no diccionario; nombres vacíos; `duration` no finita o <=0; `loop` o `from` inválidos; `keys` vacío/no array; `at` inválido/duplicado; key sin propiedad animable; `transform` vacío o mal tipado; componentes no finitos; color desconocido; representación inválida; `persist` inválido o declarado dentro de una key. Las keys de `representation` se validan con las mismas reglas de familias y bitmaps que Visual Representation.

Warnings no bloqueantes: keys con `at > duration`; ninguna key activa. No se descartan keys fuera de duración del modelo compilado: deben poder activarse si cambia la duración de la declaración en una recompilación.

Las entradas JS inválidas no deben mutar la animación vigente y deben permitir continuar el script, siguiendo el patrón de Timer. Los códigos concretos de diagnóstico se asignarán conforme al registro existente, sin inventar códigos que puedan colisionar.

### 8.1 Recuperación numérica

Una evaluación que produzca componentes, geometría o conversiones no finitos o no representables NO DEBE entregar esos valores al renderer ni finalizar el proceso o la sesión. Se conserva la última apariencia válida de la capa Animation de esa instancia, de forma atómica: no se publica una mezcla de componentes nuevos y anteriores. Se conservan datos visuales locales válidos, no píxeles de pantalla ni una copia de las propiedades espaciales del objeto.

Antes de una primera pose válida, la apariencia de respaldo es la apariencia efectiva válida inmediatamente anterior al inicio o sustitución. Esta retención de emergencia no incorpora propiedades a la referencia de `from: "current"` ni permite heredarlas después de una evaluación válida. `stop_animation()` siempre elimina la capa, incluido el respaldo, y vuelve a la apariencia runtime base con el orden de `persist`.

El fallo visual no pausa ni reinicia la reproducción: cuando el tiempo candidato es representable, se actualizan normalmente reloj, estado y consultas, incluido DONE. Si el propio avance temporal no es representable, no se confirma ese avance y se conservan el último reloj y estado válidos. La evaluación puede recuperarse al obtener un candidato válido; no hay saltos artificiales de tiempo, clamps ni sustitución de NaN por cero.

Se emite un diagnóstico en el primer fallo numérico de cada reproducción, como máximo uno por reproducción e instancia, aunque se repita, cambie de causa o desaparezca y reaparezca en frames posteriores. Reanudar no reinicia esta supresión; iniciar una reproducción nueva sí. Las consultas y renders repetidos no deben multiplicar el diagnóstico. Esta supresión no afecta a diagnósticos de argumentos JS ni errores de definición. No se añade un estado público de error ni se cambia la semántica de Timer.

## 9. Compilación, formato y rendimiento

- Las definiciones y representaciones de keys se validan y normalizan antes de ejecución.
- Los colores y bitmaps se procesan conforme al contrato Machine existente; **no** se regeneran texturas ni se reinterpretan matrices de píxeles en cada frame.
- El formato compilado y el loader DEBEN conservar duración, loop, from, keys, componentes presentes/ausentes, representaciones, `persist` y orden semántico, incluidas las keys inactivas. Modo abierto y compilado DEBEN producir el mismo resultado.
- Los recursos compartidos de representación son inmutables; la reproducción por instancia solo guarda referencia temporal, estado y apariencia efectiva.
- El renderer DEBE aplicar transformaciones locales a las cuatro familias (`primitive`, `geometry`, `text`, `bitmap`) y a los elementos persistentes.
- La integración Animation escribirá `.flxc` con `formatVersion = 14`. El lector conservará compatibilidad con formatos 12 y 13, además de 14. Los archivos antiguos se interpretan sin Animation ni `persist`, conservando su apariencia y semántica originales; no se inventan animaciones o datos perdidos. Un binario 14 se identifica como 14 aunque no declare animaciones. No se afirma que lectores antiguos puedan leerlo.
- `formatVersion` es independiente de la versión del motor y de `producerVersion`; adoptar 14 no cambia por sí mismo la versión del producto. La disposición binaria detallada se documentará al implementar el codec, respetando endianness, límites, presupuestos, validación de duplicados y serialización determinista existentes. El Reader valida el modelo 14 sin consultar fuentes ni ejecutar JS; los nombres duplicados no se reparan sobrescribiendo entradas.

## 10. Pruebas mínimas de conformidad

1. Declaración sin `animations`; nombre desconocido; duración inválida; `at` duplicado.
2. Keys parciales no ordenadas, una sola key, primera key posterior a cero, última anterior a duración.
3. Keys en `at:duration`, posteriores a duración y todas inactivas.
4. Interpolación de offset X/Y, rotación de 720°, escala 0/negativa y canales de color.
5. Sustitución de representación por arrays de distinta longitud y por `[]`.
6. `persist:first/last` con cambio de representación y `from:current`, sin duplicación.
7. `play` RUNNING/PAUSED/DONE, sustitución, `pause` no toggle, `stop` desde todos los estados.
8. `animation_active/paused/done/left/current` en ABSENT, RUNNING, PAUSED, DONE y loop.
9. `apply_color` y `restore_color` durante RUNNING/PAUSED/DONE; animación sin keys activas de color.
10. `loop+base` frente a `loop+current`, incluidos múltiples ciclos atravesados en un solo update y escala no acumulativa.
11. Aislamiento entre instancias, `hide`, `state_to`, `kill` y preservación de colliders y propiedades JS.
12. Equivalencia de ejecución abierta/compilada y de diferentes tamaños de delta temporal.
13. Renderizado con primitivas, geometría, texto, bitmap y paleta Machine; sin recreación de textura por frame.

### 10.1 Casos de aceptación de integración

Estos casos son obligatorios y complementan las pruebas anteriores. Salvo las comprobaciones específicas de lectura antigua o errores de fuente, deben cubrir ejecución abierta y formato 14 cuando corresponda.

1. **Formato y autonomía:** escribir 14, retirar las fuentes y cargarlo con las mismas keys parciales, inactivas y `persist`. Cargar fixtures válidos 12 y 13 sin Animation conserva su resultado anterior; una versión no soportada sigue rechazándose.
2. **Snapshot de draw:** un objeto capturado con apariencia A inicia B desde `draw`. Las consultas del mismo hook identifican B; la captura emitida mantiene A. Una segunda captura sin update muestra B en `at=0`, y renders adicionales no avanzan el reloj. Stop y sustitución desde `draw` respetan la misma frontera.
3. **Captura selectiva:** A aporta offset, escala, color y representación. B con `from: "current"` solo declara rotación: captura únicamente la rotación efectiva; offset y escala quedan neutros y reaparecen color y sección sustituible runtime base. Si B controla solo `offset.x`, no hereda `offset.y`. Keys inactivas no amplían lo capturado. Si B controla color, su referencia capturada no cambia con un `apply_color` posterior.
4. **Persistencia sin reproducción:** una base declarada en orden `[sustituible-A, last-A, first-A, sustituible-B, first-B, last-B]` se dibuja `[first-A, first-B, sustituible-A, sustituible-B, last-A, last-B]` en ABSENT y tras stop. Una key `representation: []` deja solo los grupos persistentes. Cambiar o recapturar animación no los duplica; sin `persist` se conserva el orden original.
5. **Recuperación numérica:** después de una pose válida, provocar un resultado no finito o no representable y comprobar retención atómica local, continuidad de la sesión, ausencia de valores inválidos enviados al renderer y un único diagnóstico durante muchos updates/renders de esa reproducción. Cubrir fallo inicial, sustitución, fallo del reloj, recuperación posterior, DONE, pausa/reanudación sin nuevo diagnóstico y stop retirando el respaldo. Una reproducción nueva puede emitir su propio diagnóstico.
6. **Duplicados antes de sobrescritura:** dos miembros con el mismo nombre dentro del mismo `animations` fuente, incluso idénticos, producen error antes de construir una tabla que los colapse. Entradas duplicadas en el binario 14 también se rechazan. La composición de paleta mantiene su precedencia existente y no se confunde con esta prohibición.
7. **Límites y partición temporal:** con referencia neutra, duración 1 y offset X desde 0 hasta 10, `loop+current` produce 10 en `E=1` y 25 en `E=2.5`; `loop+base` produce 0 y 5 respectivamente. En `E=1`, ambos loops tienen `left=1` y no DONE. Sin loop, `E>=1` da offset 10, DONE y `left=0`. Un update de 2.5 y diez de 0.25 producen el mismo resultado. Una representación declarada solo en `at=1` queda como cierre sin loop y no persiste en el inicio del ciclo siguiente con loop. Cubrir también keys de transformación no neutras en cero conforme a la fórmula de la sección 5.3, sin redefinir `Dend` como diferencia entre keys.
8. **Color sin reproyección:** entre destinos efectivos rojo `(255,0,0,255)` y azul `(0,0,255,255)`, el punto medio tiene RGBA normalizado `(0.5,0,0.5,1)`, convertido a canales enteros con el mismo criterio de redondeo en abierto y compilado, aunque una paleta discreta no contenga ese resultado. No se ejecuta una nueva proyección Machine por frame; los elementos de color propio y bitmap no cambian. La ausencia de `visual.color` permanece ausente tras stop.

## 11. Invariantes

- ANIM-001 Una instancia reproduce como máximo una animación.
- ANIM-002 Animation no modifica propiedades espaciales, físicas o lógicas de RuntimeObject.
- ANIM-003 Solo las propiedades declaradas en keys activas están controladas por Animation.
- ANIM-004 Las keys son parciales, independientes por componente y ordenadas temporalmente.
- ANIM-005 Las representaciones son discretas y sustituyen únicamente la sección animable.
- ANIM-006 `persist` pertenece solo a la representación base y no se duplica.
- ANIM-007 `from` captura una referencia al comenzar, no una referencia viva.
- ANIM-008 `loop+current` acumula offset/rotación, pero no escala/color/representación.
- ANIM-009 DONE conserva la última apariencia; STOP retira la contribución Animation.
- ANIM-010 PAUSED es activo; `pause_animation` no es toggle.
- ANIM-011 Los cambios runtime no reescriben una interpolación ya iniciada.
- ANIM-012 La evaluación temporal no depende del número de frames.
- ANIM-013 El comportamiento público es equivalente en modo abierto y compilado.
- ANIM-014 La captura de `from: "current"` no retiene propiedades sin keys activas en la nueva animación; transformación se controla por componente.
- ANIM-015 Las llamadas desde `draw` actualizan reproducción, pero nunca reescriben el snapshot ya capturado ni avanzan tiempo por renderizar.
- ANIM-016 La partición estable `first + sustituible + last` se aplica también en ABSENT, sin duplicar elementos ni cambiar el orden de objetos.
- ANIM-017 Un resultado numérico inválido conserva atómicamente la última apariencia local válida; nunca se entrega geometría no finita al renderer.
- ANIM-018 Los fallos numéricos generan como máximo un diagnóstico por reproducción e instancia, sin abortar la sesión ni crear un estado público nuevo.
- ANIM-019 Los nombres duplicados se rechazan antes de cualquier sobrescritura, tanto en fuente como en el binario.
- ANIM-020 Los límites de ciclo usan intervalos `[0,duration)`; la key de cierre participa en el cierre y la acumulación, sin exigir un render intermedio.
- ANIM-021 La interpolación usa colores efectivos ya procesados y no reproyecta por Machine durante la reproducción.
- ANIM-022 La escritura usa formato 14 y la lectura conserva 12/13; no se confunde versión de formato con versión del motor.

## 12. Compatibilidad normativa y comprobaciones de implementación

1. **RuntimeObject y modelo espacial:** permanecen intactos sus valores públicos, identidad, vigencia de wrappers, pivot, mecánicas y colliders. La apariencia y su respaldo numérico son locales y separados de esas propiedades.
2. **Timer:** se conservan sus convenciones de pausa, DONE, stop y fase temporal. Animation no modifica sus funciones ni introduce una política global de pausa. La recuperación numérica aquí definida pertenece únicamente a Animation.
3. **Visual y Bitmap:** se conserva snapshot, composición entre objetos, familias, reglas de tamaño, color heredado y ausencia de tintado bitmap. La proyección Machine sigue siendo previa a ejecución; las texturas siguen siendo recursos internos reutilizados por mundo y no se serializan. Debe verificarse la transformación firmada/no uniforme de las cuatro familias sin alterar Collision ni el dibujo inmediato.
4. **Extensión de orden:** VIS-005 de `spec/visual-representation.md` establece sin excepción que `representation` conserva el orden declarado. La agrupación `persist` aprobada en este documento es una excepción explícita para declaraciones con `persist`, incluso sin reproducción; el orden relativo se conserva dentro de cada grupo y las declaraciones anteriores no cambian. La redacción de VIS-005 y la referencia al orden en Bitmap requieren armonización documental posterior, sin revertir esta decisión funcional. No se modifican esos documentos en esta intervención.
5. **Compilación:** se mantienen autonomía de `CompiledProject`, resolución previa, validación sin ejecución, límites de recursos y determinismo binario. `spec/compiled-binary.md` describe actualmente el formato implementado 13 y `spec/compiled-project.md` conserva una referencia a formato persistente v2. El formato 14 aprobado aquí es el objetivo de integración; ambas referencias deberán armonizarse con el codec al implementarlo. No se afirma una migración ya realizada ni se cambian documentos ajenos en esta intervención.
6. **Comprobaciones pendientes:** verificar la representación y redondeo de color efectivo, precisión y representabilidad temporal, recuperación numérica, detección temprana de duplicados, vida útil de bitmaps y compatibilidad real 12/13/14 mediante los casos de la sección 10.1. Estas obligaciones no autorizan clamps, APIs adicionales ni cambios de semántica para simplificar la implementación.

Estas comprobaciones son requisitos de integración y pruebas; no justifican ampliar la API pública.
