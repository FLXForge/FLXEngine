# Mechanics

## Estado

Especificación normativa de **FLX v0.3.0** para movimiento, rotación, estado mecánico vivo y herencia mecánica de creación.

Mechanics declara **cómo puede comportarse mecánicamente un objeto**. El script expresa **qué hace el objeto** mediante operaciones narrativas.

El bloque histórico raíz `motion` queda retirado. La configuración mecánica pertenece a `mechanics`.

## Principios

1. El JSON declara reglas mecánicas.
2. El Runtime mantiene estado vivo.
3. JavaScript expresa comportamiento mediante verbos.
4. `RuntimeObject` no es un espejo profundo de `mechanics`.
5. `speed`, `velocity` y `angle` representan estado vivo manejable; `acceleration`, `inertia`, `limit`, `step`, `diagonal` y `type` permanecen como reglas declarativas.
6. Una misma contribución de movimiento no puede contabilizarse dos veces.
7. Machine/Input expresa capacidad e intención; Mechanics interpreta movimiento. Cambiar una Machine 2way por 4way no debe obligar a cambiar el JSON ni el comportamiento JS.

## Estructura

```json
"mechanics": {
  "type": "direct",
  "motion": {
    "speed": {
      "start": 0,
      "limit": 0
    },
    "acceleration": 0,
    "inertia": 0,
    "step": 0,
    "diagonal": "independent",
    "horizontal": {},
    "vertical": {}
  },
  "rotation": {
    "angle": 0,
    "speed": {
      "start": 0,
      "limit": 0
    },
    "acceleration": 0,
    "inertia": 0,
    "step": 0
  }
}
```

No es necesario serializar todos los defaults. Son valores efectivos del contrato.

## `mechanics.type`

Tipos soportados en v0.3.0:

- `direct` — default.
- `polar`.

### `direct`

La traslación viva es independiente de la orientación del objeto. Es el modelo apropiado cuando un objeto puede desplazarse en una dirección y rotar visualmente sin que esa rotación cambie su trayectoria.

Casos de referencia: palas, power-ups, fragments de nave y asteroides que rotan mientras mantienen una trayectoria lineal.

### `polar`

La orientación participa en las operaciones de movimiento polar. Es el modelo apropiado cuando avanzar o acelerar depende de hacia dónde está orientado el objeto.

Casos de referencia: pelota de Pong, nave de Asteroids y proyectiles cuya dirección nace de un ángulo.

`type` no representa inertia, gravedad, step ni física.

# Motion

## `speed`

Forma corta:

```json
"speed": 100
```

equivale a:

```json
"speed": {
  "start": 100,
  "limit": 0
}
```

Forma completa:

```json
"speed": {
  "start": 100,
  "limit": 150
}
```

Reglas:

- `start` default = `0`.
- `limit` default = `0`.
- `limit == 0` significa sin límite.
- si `limit > 0`, debe ser `>= start`.
- el mismo modelo se aplica a `motion.speed` y `rotation.speed`.

`start` inicializa el estado vivo correspondiente. No es un valor que el Runtime reaplique continuamente de forma oculta.

`limit` es una restricción efectiva y debe respetarse al aplicar estado vivo.

## Valores comunes y overrides por eje

Las propiedades declaradas directamente en `motion` son comunes a ambos ejes. `horizontal` y `vertical` sobrescriben únicamente las propiedades que declaran.

```json
"motion": {
  "speed": 100,
  "inertia": 0.6,
  "horizontal": {
    "speed": {
      "limit": 150
    }
  }
}
```

Resultado efectivo:

- horizontal speed.start = 100
- horizontal speed.limit = 150
- horizontal inertia = 0.6
- vertical speed.start = 100
- vertical speed.limit = 0
- vertical inertia = 0.6

La resolución common + override debe completarse antes de que el Runtime use Mechanics.

`diagonal` describe la relación entre ejes y no puede sobrescribirse dentro de `horizontal` o `vertical`.

## `diagonal`

Valores v0.3.0:

- `independent` — default.
- `vector`.

`independent` conserva la contribución completa de cada eje.

`vector` normaliza/pondera la combinación para evitar que una diagonal tenga mayor magnitud que un movimiento de un solo eje.

## `acceleration`

Default = `0`.

Existe independientemente para `motion` y `rotation`; en `motion` puede sobrescribirse por eje.

`acceleration` es una **tasa temporal**. Su integración utiliza `delta` y debe producir resultados acumulados equivalentes a diferentes framerates dentro de tolerancias numéricas razonables.

Para traslación, conceptualmente:

```text
delta_velocity = acceleration * intent * delta
```

Los valores históricos de ejemplos pre-v0.3.0 no forman parte del contrato y pueden recalibrarse para conservar el comportamiento observable.

## `inertia`

Default = `0`.

Rango válido:

```text
0 <= inertia <= 1
```

Semántica pública:

- `0` — ninguna persistencia.
- `0 < inertia < 1` — persistencia progresivamente mayor.
- `1` — persistencia completa/perpetua.

`inertia` es un control normalizado de persistencia. No es un multiplicador fijo por frame. La respuesta debe ser independiente del framerate y utilizar `delta`.

### Movimiento guiado y movimiento libre

Son operaciones guiadas:

```js
move_horizontal(object, intent);
move_vertical(object, intent);
advance(object);
accelerate(object, intent);
rotate(object, intent);
```

Una orden guiada debe poder actuar aunque el estado vivo esté en reposo.

Cuando deja de existir una orden activa en un ámbito, el movimiento restante es libre y `inertia` gobierna cuánto persiste el estado vivo ya generado.

En movimiento libre, inertia reduce/preserva el vector vivo existente. Cambiar `angle` no debe redirigir retroactivamente un momentum ya adquirido.

## `step`

Default = `0`.

- `0` — movimiento/rotación continuo.
- `step > 0` — cuantiza la operación del frame.

`step` no inicia una transición multiframe ni una animación automática. Existe en `motion` y `rotation` y puede combinarse con inertia.

Frogger es un caso futuro de referencia para movimiento discreto.

# Rotation

```json
"rotation": {
  "angle": 0,
  "speed": {
    "start": 0,
    "limit": 0
  },
  "acceleration": 0,
  "inertia": 0,
  "step": 0
}
```

`angle` inicializa la orientación viva.

`rotation.speed`, `rotation.acceleration`, `rotation.inertia` y `rotation.step` siguen la misma filosofía temporal y declarativa que sus equivalentes de motion.

La rotación de un objeto `direct` no modifica por sí misma la dirección de su `velocity` lineal.

# Estado vivo

Mechanics es declaración. El Runtime mantiene estado vivo.

Estado mecánico vivo relevante:

- posición;
- `speed`;
- `velocity`;
- `angle`;
- velocidad angular / `rotationSpeed`.

No son estado vivo copiado como propiedades planas:

- `acceleration`;
- `inertia`;
- `limit`;
- `step`;
- `diagonal`;
- `type`.

## `speed`

`speed` es la magnitud escalar viva manejable por scripting.

En `polar`, `speed + angle` expresan la contribución polar propia. Una `velocity` adicional independiente no debe destruirse ni reescalarse al aplicar/restaurar `speed`.

En `direct`, el movimiento efectivo debe responder al `speed` vivo aplicado por las operaciones de Mechanics.

## `velocity`

`velocity` es el vector lineal vivo (`velocityX`, `velocityY` en la superficie JS actual).

Puede representar:

- movimiento directo;
- momentum generado por acceleration;
- velocity heredada por `inherit`;
- contribución lineal adicional a un movimiento polar.

Una contribución no puede contabilizarse dos veces mediante `speed` y `velocity`.

## `angle`

`angle` es orientación viva.

Convención FLX:

```text
0   = arriba
90  = derecha
180 = abajo
270 = izquierda
```

En `polar` puede participar en avance/aceleración. En `direct`, cambiar `angle` no redirige automáticamente `velocity`.

# API de movimiento

## `move_horizontal`

```js
move_horizontal(object, intent);
```

Mueve horizontalmente usando Mechanics efectivo del eje. `intent` está normalizado en `[-1,+1]`.

## `move_vertical`

```js
move_vertical(object, intent);
```

Mueve verticalmente usando Mechanics efectivo del eje. `intent` está normalizado en `[-1,+1]`.

Input digital produce normalmente `-1`, `0`, `+1`; un futuro input analógico podrá usar valores intermedios sin cambiar Mechanics.

## `advance`

```js
advance(object);
```

Ordena al objeto avanzar usando su Mechanics y estado vivo.

- En `polar`, la orientación participa en la contribución polar.
- En `direct`, el avance consume el vector lineal vivo correspondiente.
- Una nueva orden `advance` debe poder actuar aunque el objeto hubiera llegado al reposo.
- No debe existir doble contabilización entre `speed` y `velocity`.

## `accelerate`

```js
accelerate(object, intent);
```

`intent` es opcional cuando el comportamiento no necesita expresarlo explícitamente.

Acelerar:

1. modifica estado vivo según `acceleration`, intención, `limit` y `delta`;
2. aplica desplazamiento en el mismo frame.

No requiere una llamada posterior a `advance()` para mover ese frame.

Cuando cesa `accelerate`, la velocity adquirida entra en movimiento libre y queda gobernada por `inertia`.

## `rotate`

```js
rotate(object, intent);
```

`intent` puede omitirse cuando el comportamiento utiliza directamente el estado angular vivo configurado. La operación utiliza el bloque `rotation` y puede actuar desde velocidad angular `0`.

# Aplicación y restauración de estado vivo

La familia `apply_*` aplica explícitamente estado vivo sin modificar la declaración Mechanics. La familia `restore_*` devuelve una magnitud a su referencia inicial declarada cuando existe un caso real que lo justifica.

No se crean miembros por simetría artificial.

## `apply_speed`

```js
apply_speed(object, value);
```

Aplica una nueva velocidad viva.

No modifica `speed.start`, `speed.limit`, `acceleration` ni `inertia`.

Si `limit > 0` y `value` lo supera, se aplica `limit` y el Runtime emite warning.

En `polar`, aplicar speed no destruye ni reescala una `velocity` adicional independiente.

## `restore_speed`

```js
restore_speed(object);
```

Restaura la velocidad viva al `motion.speed.start` efectivo con el que fue creada la instancia.

No deshace el último `apply_speed`; siempre vuelve a `start`.

No modifica posición, angle, inertia, acceleration ni otras magnitudes.

## `apply_velocity`

```js
apply_velocity(object, direction, speed);
```

Sustituye el vector lineal vivo por un vector con dirección `direction` y magnitud `speed`.

No desplaza inmediatamente el objeto.

No modifica:

- `angle`;
- `rotationSpeed`;
- `motion.speed.start`;
- `acceleration`;
- `inertia`.

Respeta `motion.speed.limit`. Si la magnitud supera un `limit > 0`, se aplica el límite efectivo y se emite warning. `limit == 0` significa sin límite.

Ejemplo `direct` con rotación visual independiente:

```js
function born(asteroid) {
    apply_velocity(asteroid, random(0, 360), random(20, 70));
}

function motion(asteroid) {
    advance(asteroid);
    rotate(asteroid);
}
```

# Reflection

Las antiguas `bounce_x` / `bounce_y` quedan retiradas.

## `reflect_x`

```js
reflect_x(object);
```

Invierte la componente horizontal de la trayectoria viva.

## `reflect_y`

```js
reflect_y(object);
```

Invierte la componente vertical de la trayectoria viva.

Reflection no decide colisiones, no pierde energía y no implementa restitución/materiales/peso. Un verdadero bounce pertenece a una futura auditoría Collision/Physics.

# Positioning

## `position`

```js
position(object, x, y);
```

Coloca instantáneamente el objeto en coordenadas lógicas.

## `position_origin`

```js
position_origin(object);
```

Coloca el objeto en su `origin` efectivo.

Positioning no usa delta y no modifica speed, velocity, angle, timers o `local`.

Si también se necesita restaurar velocidad:

```js
position_origin(ball);
restore_speed(ball);
```

Son responsabilidades distintas.

# `inherit`

`inherit` es un bloque del objeto, separado de `mechanics`.

```json
"inherit": {
  "creation": {
    "angle": "copy",
    "velocity": "compose"
  },
  "live": {
    "angle": "copy"
  }
}
```

Transmite **estado vivo**, no configuración Mechanics.

## `inherit.creation`

Se aplica durante la creación y después la propiedad se independiza.

Estados soportados en v0.3.0:

### `angle`

```json
"angle": "copy"
```

Copia la orientación viva del padre.

### `velocity`

```json
"velocity": "copy"
```

El hijo recibe la velocity lineal viva del padre.

```json
"velocity": "compose"
```

La velocity lineal viva del padre se incorpora a la velocity propia del hijo.

Casos de referencia:

- Asteroids laser: `angle = copy`, `velocity = compose`.
- Ship fragments: `velocity = copy`.

No se hereda configuración Mechanics.

## `inherit.live`

v0.3.0 soporta:

```json
"live": {
  "angle": "copy"
}
```

La propiedad continúa derivándose del padre mientras exista la relación.

Attachments existentes no se convierten implícitamente en `inherit.live`.

# Creación y `born()`

Orden conceptual:

1. Definition/JSON es la base.
2. Se construye el `RuntimeObject`.
3. Se resuelven Mechanics inicial, origin/offset/grid e `inherit.creation`.
4. El objeto existe pero todavía no ha entrado al mundo.
5. Se ejecuta `born()`.
6. El objeto entra en el mundo.

`born()` es la última palabra sobre el estado inicial vivo. Puede sobrescribir valores procedentes de definición, Mechanics inicial o `inherit.creation`.

El Runtime no re-aplica después `inherit.creation` para proteger valores previos. `inherit.live` es distinto porque continúa derivándose durante la relación.

# Input y Mechanics

Mechanics recibe intención; no conoce dispositivos ni detalles físicos de la Machine.

```js
input_direction(subject, direction, HORIZONTAL);
input_direction(subject, direction, VERTICAL);
```

devuelve intención normalizada.

Convención digital:

```text
RIGHT = +1
UP    = +1
LEFT  = -1
DOWN  = -1
```

En 4way:

- HORIZONTAL consulta únicamente LEFT/RIGHT.
- VERTICAL consulta únicamente UP/DOWN.

En 2way, la única vía POSITIVE/NEGATIVE se proyecta sobre cualquiera de los ejes solicitados.

Consultar `docs/spec/scripting/input.md` para el contrato completo.

# Defaults efectivos

```text
mechanics.type                         direct

motion.speed.start                    0
motion.speed.limit                    0
motion.acceleration                   0
motion.inertia                        0
motion.step                           0
motion.diagonal                       independent

rotation.angle                        0
rotation.speed.start                  0
rotation.speed.limit                  0
rotation.acceleration                 0
rotation.inertia                      0
rotation.step                         0
```

Los valores comunes de motion se propagan a horizontal/vertical salvo override explícito.

# Validación y errores

Debe rechazarse:

- `inertia < 0` o `inertia > 1`;
- `limit > 0` y `limit < start`;
- valores no soportados de `mechanics.type`;
- valores no soportados de `diagonal`;
- operaciones `inherit` no soportadas.

Aplicar speed/velocity superior a un límite explícito no invalida el objeto: se aplica el límite efectivo y se genera warning.

# Fuera de alcance de v0.3.0

Mechanics v0.3.0 no define:

- gravity;
- física general;
- friction / drag;
- weight / mass;
- restitution / materiales;
- bounce físico;
- orbital mechanics;
- paths;
- navegación;
- steering;
- zonas;
- API general parent/child/neighbor;
- operaciones step multiframe;
- callbacks/eventos de movimiento;
- input analógico;
- 2.5D / 3D / Mode7.

Estas capacidades deben emerger de casos reales posteriores.

# Familias resultantes

## Mechanics / Translation

```text
move_horizontal
move_vertical
advance
accelerate
```

## Mechanics / Rotation

```text
rotate
```

## Live state

```text
apply_speed
apply_velocity
restore_speed
```

## Geometry / Reflection

```text
reflect_x
reflect_y
```

## Positioning

```text
position
position_origin
```

## Future Navigation / Context

`follow_*` permanece como deuda funcional/legacy hasta una auditoría específica de Navigation/References.

# Casos de referencia v0.3.0

- Pong paddle — direct.
- Pong ball — polar.
- Arkanoid paddle — direct.
- Arkanoid ball — polar.
- Arkanoid power-ups — direct.
- Asteroids ship — polar.
- Asteroids laser — polar + inherit creation.
- Asteroids grandes/pequeños — direct + velocity propia + rotación independiente.
- Asteroids ship fragments — direct + velocity copy + inertia.
- Invaders player/enemies — direct.
- Invaders projectiles — polar cuando su trayectoria se expresa mediante orientación + speed.

Estos juegos validan el lenguaje; sus números históricos no forman parte del contrato.
