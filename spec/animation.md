# Animation — contrato normativo propuesto para FLX v0.4.0

**Estado:** propuesta consolidada, pendiente de validación contra implementación y aprobación en repositorio.  
**Ámbito:** representación visual declarativa y reproducción por `RuntimeObject`.  
**Dependencias normativas:** `spec/visual-representation.md`, `spec/visual-bitmap-palette.md`, `spec/spatial_model.md`, `spec/scripting/runtime_object.md`, `spec/scripting/timer.md`.

## 1. Propósito y límites

Animation modifica exclusivamente la **apariencia dibujada** de una instancia. NO DEBE modificar `x`, `y`, `width`, `height`, `angle`, `origin`, `visible`, `depth`, colliders, velocidades, estado lógico ni física. La apariencia animada NO DEBE modificar los valores observables de `RuntimeObject` JS.

Una instancia PUEDE reproducir **una sola** animación a la vez. No hay blending, easing, eventos de key, pistas simultáneas, animación de colisiones, visibilidad ni profundidad en v0.4.0. La reproducción es explícita mediante JavaScript; no existe `initial` ni autoplay implícito.

## 2. Declaración

`animations` es un diccionario opcional en la raíz de la definición de objeto, al mismo nivel que `visual` y `states`. Cada clave es un nombre no vacío y único.

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

La sustitución de representación NO altera el tamaño, posición, collider ni profundidad del RuntimeObject. Los elementos sustitutos respetan las reglas de tamaño, color, bitmap y paleta existentes.

## 4. Referencia inicial, keys parciales y evaluación

Al comenzar una reproducción nueva se captura una **referencia inmutable para esa reproducción**:

- `from: "base"`: apariencia runtime sin la capa de Animation anterior, incluyendo modificaciones runtime vigentes (por ejemplo `apply_color`).
- `from: "current"`: apariencia efectiva en el instante de la sustitución, incluida la animación anterior si estaba RUNNING, PAUSED o DONE.

La referencia incluye color, sección sustituible de representación y transformación visual efectiva. Los elementos `persist` permanecen anclados a la representación base y nunca se copian en la sección sustituible.

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

### 5.1 `loop: false`

La reproducción llega a `duration`, evalúa su pose final, pasa a DONE y retiene la apariencia final hasta que se reinicie, sustituya o detenga.

### 5.2 `loop: true`, `from: "base"`

Cada ciclo se evalúa desde la **misma referencia capturada al iniciar la reproducción**. No se acumula ningún componente. Se permiten discontinuidades al cruzar el límite de ciclo.

### 5.3 `loop: true`, `from: "current"`

- **Offset y rotación:** se acumula el **desplazamiento neto de la trayectoria de un ciclo**. Sea `D(t)` la trayectoria local interpolada desde su valor neutro y `Dend = D(duration)`: en el ciclo `n` (empezando en cero), `valor = referencia + n * Dend + D(t)`.
- **Scale:** no se acumula. En todos los ciclos se aplica el factor local `S(t)` a la escala de referencia capturada al comenzar la reproducción.
- **Color:** no se acumula; en cada ciclo se interpola desde el mismo color de referencia capturado al comenzar la reproducción. Los destinos son absolutos.
- **Representation:** no se acumula; cada ciclo comienza con la sección sustituible de referencia capturada y aplica sus keys discretas.

El motor DEBE evitar desbordamientos y valores no finitos en geometría y transformación, diagnosticando valores no representables sin provocar un fallo del proceso. No se define un límite artificial de escala ni una normalización angular silenciosa.

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
- Una llamada `play_animation()` debe hacer observable inmediatamente el nuevo estado y la pose de `at=0` durante el callback que la solicita. El avance posterior debe respetar la misma política de fases temporales que Timer, evitando dobles avances en un frame.
- La renderización consume la apariencia efectiva, pero nunca escribe la transformación animada en `RuntimeObject` ni en Collision.

## 8. Validación y diagnóstico

Errores de definición (antes de runtime): `animations` no diccionario; nombres vacíos; `duration` no finita o <=0; `loop` o `from` inválidos; `keys` vacío/no array; `at` inválido/duplicado; key sin propiedad animable; `transform` vacío o mal tipado; componentes no finitos; color desconocido; representación inválida; `persist` inválido o declarado dentro de una key. Las keys de `representation` se validan con las mismas reglas de familias y bitmaps que Visual Representation.

Warnings no bloqueantes: keys con `at > duration`; ninguna key activa. No se descartan keys fuera de duración del modelo compilado: deben poder activarse si cambia la duración de la declaración en una recompilación.

Las entradas JS inválidas no deben mutar la animación vigente y deben permitir continuar el script, siguiendo el patrón de Timer. Los códigos concretos de diagnóstico se asignarán conforme al registro existente, sin inventar códigos que puedan colisionar.

## 9. Compilación, formato y rendimiento

- Las definiciones y representaciones de keys se validan y normalizan antes de ejecución.
- Los colores y bitmaps se procesan conforme al contrato Machine existente; **no** se regeneran texturas ni se reinterpretan matrices de píxeles en cada frame.
- El formato compilado y el loader DEBEN conservar duración, loop, from, keys, representaciones y orden semántico. Modo abierto y compilado DEBEN producir el mismo resultado.
- Los recursos compartidos de representación son inmutables; la reproducción por instancia solo guarda referencia temporal, estado y apariencia efectiva.
- El renderer DEBE aplicar transformaciones locales a las cuatro familias (`primitive`, `geometry`, `text`, `bitmap`) y a los elementos persistentes.
- El formato de serialización, número de versión y migraciones se decidirán durante la integración del compilador, sin alterar este contrato público.

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

## 12. Aspectos de implementación que requieren comprobación, no nuevas funciones

1. Confirmar el orden exacto de las fases de actualización frente a Timer y la disponibilidad de pose `at=0` dentro de hooks.
2. Confirmar la representación interna del color efectivo por defecto y su proyección Machine para interpolación RGBA.
3. Confirmar que el renderer puede componer escalas negativas y no uniformes en las cuatro familias sin alterar Collision.
4. Confirmar el mecanismo de versionado del formato compilado y la vida útil de las texturas de keys.
5. Definir política diagnóstica para overflow numérico y límites de precisión temporal sin introducir clamps arbitrarios.

Estas comprobaciones son requisitos de integración y pruebas; no justifican ampliar la API pública.
