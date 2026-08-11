# Caracterizacion de State en FLX

Este documento audita la maquina de estados actual de FLX.

No propone cambios de produccion. Su objetivo es describir el contrato real en
JSON, compilacion, Runtime y JavaScript, y contrastarlo con
`docs/scripting-grammar-draft.md`.

## A. Contrato declarativo

La maquina de estados se declara con un bloque opcional `states` dentro de un
objeto FLX:

```json
"states": {
  "initial": "intro",
  "intro": { "next": [ "playing" ] },
  "playing": { "next": [ "gameover" ] },
  "gameover": {}
}
```

Estructura actual:

| Campo | Tipo | Obligatorio | Significado actual |
| --- | --- | --- | --- |
| `states` | object | no | Activa una maquina de estados declarativa en el objeto. |
| `states.initial` | string | no | Estado inicial copiado a `ObjectDefinition.initialState`. |
| `states.<stateName>` | object | no | Declara un estado disponible. |
| `states.<stateName>.next` | array de string | no | Lista de estados destino permitidos desde ese estado. |

Defaults observados:

- Si no existe `states`, el objeto no tiene maquina: `initialState == ""` y
  `stateTransitions` queda vacio.
- Si existe `states` sin `initial`, se cargan las transiciones declaradas, pero
  el estado vivo inicial queda vacio.
- Si un estado no declara `next`, se registra como estado con lista de
  transiciones vacia.
- Un estado con `next: []` equivale en runtime a un estado terminal.
- Un estado unico como `states: { "initial": "intro", "intro": {} }` es valido
  para loader/runtime y deja al objeto indefinidamente en `intro`.
- `states` puede ser referencia JSON porque el loader permite resolver bloques
  referenciables antes de parsear propiedades del objeto.

Validaciones actuales en `JsonLoader`:

| Caso | Resultado actual |
| --- | --- |
| `states` ausente | valido, sin warning. |
| `states` no es object | se ignora por la condicion actual; no hay warning especifico en `parseStates`. |
| `initial` ausente | valido; estado inicial vacio. |
| `initial` apunta a estado no declarado | warning en `Logger`, no Diagnostic fatal. |
| Estado cuyo valor no es object | warning en `Logger`; se omite ese estado. |
| `next` ausente | valido; lista vacia. |
| `next` no es array | warning en `Logger`; lista vacia. |
| Entrada de `next` no string | warning en `Logger`; se ignora esa entrada. |
| `next` apunta a estado no declarado | no se valida en loader/compiler; fallara como transicion invalida si se intenta. |
| `next` apunta al mismo estado | permitido declarativamente; en JS `state(o, mismo)` no hace nada antes de validar. |
| Ciclos de estados | permitidos; no hay deteccion ni deberia confundirse con ciclos de recursos. |
| Campos extra dentro de estado | el schema los rechaza; el loader no los consume. |

Schema actual:

- `docs/schemas/states.schema.json` permite `initial` string.
- Cada propiedad adicional de nivel superior se interpreta como estado.
- Cada estado solo admite `next`.
- No exige `initial`, ni exige que `next` exista, ni valida que los destinos
  existan.

## B. Contrato Runtime

Flujo actual:

```text
JSON
-> JsonLoader::parseStates
-> ObjectDefinition.initialState
-> ObjectDefinition.stateTransitions
-> CompiledProject / ResourceRegistry / .flxc
-> RuntimeObjectBuilder
-> RuntimeObject.state
-> RuntimeObject.stateTransitions
```

Datos conservados:

| Dato | ObjectDefinition | CompiledProject/.flxc | RuntimeObject |
| --- | --- | --- | --- |
| Estado inicial | `initialState` | serializado | copiado a `state` |
| Transiciones | `stateTransitions` | serializado | copiado a `stateTransitions` |
| Tiempo en estado | no existe | no existe | `stateTime` |
| Evento entered | no existe | no existe | `stateEnteredFrame` |

Duplicacion:

- `RuntimeObject.stateTransitions` duplica metadata declarativa ya presente en
  la definicion compilada.
- `RuntimeObject.state` es estado vivo y si debe ser por instancia.
- `RuntimeObject.stateTime` y `stateEnteredFrame` son estado vivo por instancia.

Inicializacion runtime:

- `RuntimeObjectBuilder` copia `definition.initialState` a `object.state`.
- `RuntimeWorld::createRuntimeObject` asigna `stateEnteredFrame = frameIndex`
  si `object.state` no esta vacio.
- En carga inicial `frameIndex` empieza en 0, asi que el primer `beginFrame()`
  convierte `stateEnteredFrame == 0` en el frame real actual.
- Objetos sin estado inicial conservan `state == ""` y `stateEnteredFrame == 0`.

Actualizacion temporal:

- `RuntimeWorld::update()` incrementa `frameIndex`, llama `beginFrame()`,
  ejecuta `action`, `motion`, attachments, collision, spawns y al final llama
  `updateObjectTime(delta)`.
- `stateTime` se incrementa al final del update, despues de collision y antes de
  `dead/cleanup`.
- Un objeto nacido durante una fase entra con `stateTime == 0`; participa o no
  en fases posteriores segun las reglas generales de spawn ya caracterizadas.

Contrato vivo confirmado:

| Situacion | Comportamiento actual |
| --- | --- |
| Estado inicial no vacio | `state_current(object)` devuelve ese nombre desde el primer frame. |
| Primer frame de un objeto con estado | `state_entered(object)` devuelve `true`. |
| Estado terminal | el objeto permanece indefinidamente en ese estado hasta que una API consiga cambiarlo. |
| Estado sin `next` o `next: []` | no permite salir a ningun otro estado mediante `state()`. |
| Sin maquina | estado actual `""`, tiempo `0`, `state_entered == false`. |
| Sin maquina + `state_active(o, "")` | devuelve `true`, porque solo compara `object.state == stateName`. |

## C. Contrato JS

API registrada por `StateBindings`:

| API | Firma | Retorno | Efecto | Validacion | Uso examples |
| --- | --- | --- | --- | --- | ---: |
| `state` | `state(object, stateName)` | `undefined` | intenta cambiar estado | valida contra `stateTransitions` | 4 |
| `state_current` | `state_current(object)` | string | consulta estado actual | objeto valido | 0 |
| `state_active` | `state_active(object, stateName)` | boolean | compara estado actual | objeto valido + string convertible | 3 |
| `state_entered` | `state_entered(object)` | boolean | consulta evento de entrada | objeto valido + ScriptEngine | 0 |
| `state_time` | `state_time(object)` | number | consulta segundos en estado | objeto valido | 0 |

Todas estas funciones buscan el `RuntimeObject` real leyendo `object.id` y
llamando a `ScriptEngine::findObjectByRuntimeId`.

Comportamiento con argumentos invalidos:

- Si faltan argumentos, las funciones de accion devuelven `undefined` y las
  consultas devuelven `false`, `""` o `0`.
- Si `object.id` no permite encontrar un runtime object, las consultas devuelven
  valores neutros y `state()` no cambia nada.
- `state()` con destino invalido registra warning en `Logger` y deja el estado
  sin cambios.

## D. Comportamiento correcto y util

La maquina actual es util para:

- pantallas de titulo;
- fases de nivel;
- respawn;
- transiciones sencillas;
- estados terminales como `gameover`;
- guardar tiempo dentro de estado mediante `state_time`;
- detectar entrada en estado durante un frame con `state_entered`.

La separacion principal funciona:

- JSON declara estados y transiciones posibles.
- JS decide cuando pedir una transicion.
- Runtime valida y mantiene estado vivo por instancia.
- No hay callbacks automaticos por estado.

## E. Ambiguedades

### `state(object, name)`

Semantica real:

- No es asignacion libre.
- No permite saltos arbitrarios.
- Es una transicion validada hacia un destino.
- Reinicia `stateTime` a `0`.
- Programa `state_entered` para el siguiente frame runtime.
- Si se llama con el mismo estado actual, no hace nada y no reinicia tiempo.

Por tanto, semanticamente corresponde mas a "transicionar/cambiar hacia" que a
"establecer".

### `state_active(object, name)`

`active` significa "el estado actual es exactamente este nombre".

No significa:

- que exista una maquina;
- que haya una transicion activa;
- que el estado este habilitado;
- que el estado exista declarativamente.

Caso curioso: sin maquina, `state_active(o, "")` devuelve `true`.

### `state_entered(object)`

No recibe nombre de estado. Pregunta si el objeto entro en su estado actual en
el frame runtime actual.

Puede llamarse multiples veces en el mismo frame y devolvera el mismo resultado.

Tras `state(o, "destino")` durante `action`, `state_entered(o)` no es `true` en
ese mismo callback; se vuelve `true` en el siguiente frame.

### `state_time(object)`

El tiempo se incrementa al final del frame. Por eso:

- en el primer `action` de un estado inicial, `state_time` ve `0`;
- tras llamar a `state()` en `action`, una consulta inmediata ve `0`;
- el siguiente frame ve el delta acumulado al final del frame de transicion.

## F. Deuda real

- `stateTransitions` se copia a cada `RuntimeObject`; podria consultarse por
  `definitionId` en el futuro si se consolida el registro como fuente de
  metadata inmutable.
- Las validaciones de `states` viven principalmente en `JsonLoader` como
  warnings de `Logger`, no como `Diagnostics` estructurados.
- `next` puede apuntar a estados inexistentes sin diagnosticarse en compilacion.
- `states` no object se ignora silenciosamente desde `parseStates`.
- `state_active(o, "") == true` sin maquina probablemente no expresa una
  intencion clara de lenguaje.
- `state()` programa `state_entered` para el frame siguiente; esto evita perder
  el evento tras cambiar estado en action, pero debe quedar fijado
  explicitamente si se acepta como contrato.
- La API `state()` no expresa en el nombre que valida una transicion.

## G. Documentacion divergente

Documentacion alineada:

- `docs/en/api.html` y `docs/es/api.html` explican que `state()` cambia el
  estado y valida contra `next`.
- `state_current`, `state_active`, `state_entered` y `state_time` aparecen en
  API y d.ts.
- `docs/en/json-reference.html` y `docs/es/json-reference.html` explican que
  JSON declara estados y transiciones, no condiciones ni acciones.

Divergencias o faltantes:

- No se documenta que `state_entered` tras `state()` se observa en el siguiente
  frame, no en el mismo callback.
- No se documenta que estados sin `next` son terminales de facto.
- No se documenta el comportamiento sin maquina.
- No se documenta que `initial` inexistente solo produce warning de Logger y no
  falla compilacion.
- No se documenta que `next` hacia estado inexistente no se valida al cargar.
- `docs/scripting-grammar-draft.md` presenta `state_to` como candidato mas
  expresivo, pero aun no es API real.

## H. Gramatica actual

Contraste con `docs/scripting-grammar-draft.md`:

| API actual | Forma gramatical | Encaje |
| --- | --- | --- |
| `state(object, "moving")` | sustantivo usado como verbo | ambiguo; la semantica real es transicion validada |
| `state_current(object)` | concepto_valor | encaja bien |
| `state_active(object, "moving")` | concepto_predicado | encaja si `_active` significa "es estado actual" |
| `state_entered(object)` | concepto_evento | encaja bien |
| `state_time(object)` | concepto_valor | encaja bien |
| `advance(object)` | verbo de movimiento | no pertenece a State actualmente |

Sobre `advance`:

- La unica funcion publica `advance(object)` esta en `MotionBindings`.
- Mueve el objeto usando `speed/angle` o `velocity` con `motion.acceleration`.
- No lee `states`.
- No lee `next`.
- No cambia estado.
- No reinicia `stateTime`.
- No guarda relacion con `state()`.

Por tanto, si en el futuro State necesita una operacion de avance por `next`, no
puede asumirse que el nombre `advance` esta libre semanticamente: hoy ya
significa movimiento.

## I. Candidatos de diseno

No son decisiones cerradas:

- `state_to(object, name)` describe mejor la operacion real que `state(object,
  name)`.
- `state_current`, `state_entered` y `state_time` encajan bien con el borrador
  de gramatica.
- `state_active` es aceptable si se define como "el estado actual coincide con
  este nombre"; podria ser ambiguo si `_active` se reserva para procesos en
  curso.
- Podria convenir una validacion de compilacion para `initial` y destinos
  `next` inexistentes, si se quiere que JSON sea contrato fuerte.
- Podria distinguirse entre objeto sin maquina y maquina con estado vacio para
  evitar `state_active(o, "") == true`.
- Los estados terminales parecen un concepto valido y util; solo necesitan
  documentarse mejor.

## J. Preguntas pendientes

1. Debe `initial` ser obligatorio cuando existe `states`?
2. Debe fallar compilacion si `initial` no existe?
3. Debe fallar compilacion si `next` apunta a un estado inexistente?
4. Un estado sin `next` debe documentarse oficialmente como terminal?
5. Debe permitirse una maquina de un unico estado?
6. Debe `state()` seguir existiendo o migrar a un nombre de transicion como
   `state_to()`?
7. Debe `state_entered` aceptar opcionalmente un nombre de estado?
8. Debe el evento de entrada activarse en el mismo frame de `state()` o en el
   siguiente como ahora?
9. Debe `state_time` empezar a contar desde el mismo frame de transicion o desde
   el siguiente?
10. Debe `state_active(o, "")` devolver `true` en objetos sin maquina?
11. Debe RuntimeObject conservar `stateTransitions` o consultarlas por
    `definitionId`?
12. Debe State producir `Diagnostics` estructurados en vez de warnings de
    `Logger` durante carga/compilacion?

## Tests de caracterizacion

La suite `flx-runtime-lifecycle-tests` cubre ahora:

- Estado inicial visible desde el primer frame.
- `state_entered` activo en el primer frame del estado inicial.
- `state_time` a `0` antes del primer incremento de tiempo.
- Transicion invalida sin cambio de estado.
- Transicion valida con cambio de estado y reinicio de `state_time`.
- `state_entered` no se activa en el mismo callback que llama a `state()`.
- `state_entered` se activa en el siguiente frame tras `state()`.
- Estado terminal sin `next` rechazando salida.
- Maquina de un unico estado.
- Objeto sin `states`.
- Comportamiento actual de `state_active(o, "")` sin maquina.
