# RuntimeObject, ObjectDefinition y contrato JavaScript

Este documento caracteriza el contrato actual entre `ObjectDefinition`,
`RuntimeObject` y el objeto JavaScript creado por `ScriptEngine::createJsObject`.

No propone cambios de produccion. Su objetivo es dejar una fotografia fiable para
disenar el futuro contrato JS sin asumir que debe ser un espejo de C++ ni una
reproduccion del JSON.

## A. Contrato C++ actual

Clasificacion provisional:

- A. Estado vivo intrinseco.
- B. Estado correlacional del mundo.
- C. Metadata/definicion inmutable.
- D. Relacion runtime.
- E. Infraestructura interna.
- F. Posible duplicacion/deuda.

| Campo RuntimeObject | Tipo | Origen | Cambia en runtime | Lectores principales | Modificadores principales | Clase |
| --- | --- | --- | --- | --- | --- | --- |
| `local` | `unordered_map<string,double>` | runtime vacio | si | scripts via JS, tests | JS `object.local`, `applyJsObject` | A |
| `children` | `unordered_map<string,ObjectDefinition>` | `ObjectDefinition.children` | no esperado | legado/runtime indirecto | builder | F |
| `childResources` | `unordered_map<string,string>` | `ObjectDefinition.childResources` | no esperado | spawn, auto/grid creation | builder | C/F |
| `music` | `unordered_map<string,MusicDefinition>` | `ObjectDefinition.music` | no esperado | `play_music(object,id)` | builder | C/F |
| `sounds` | `unordered_map<string,SoundDefinition>` | `ObjectDefinition.sounds` | no esperado | `play_sound(object,id)` | builder | C/F |
| `timers` | `unordered_map<string,RuntimeTimer>` | runtime vacio | si | timer bindings, lifecycle | timer bindings, update time | A |
| `creationMode` | `string` | `ObjectDefinition.creationMode` | no esperado | grid creation | builder | C/F |
| `gridRules` | `GridCreationRules` | `ObjectDefinition.gridRules` | no esperado | grid creation | builder | C/F |
| `gridPatternIsRows` | `bool` | `ObjectDefinition.gridPatternIsRows` | no esperado | grid creation | builder | C/F |
| `gridPattern` | `vector<string>` | `ObjectDefinition.gridPattern` | no esperado | grid creation | builder | C/F |
| `gridRowPattern` | `vector<vector<string>>` | `ObjectDefinition.gridRowPattern` | no esperado | grid creation | builder | C/F |
| `name` | `string` | `ObjectDefinition.id` | no esperado | JS, findByName, logs | constructor/builder | B |
| `runtimeId` | `string` | runtime allocation | no | JS `id`, findByRuntimeId, APIs | builder/runtime allocation | B/D |
| `definitionId` | `string` | resource id passed by runtime | no | runtime defensive cycle/load | RuntimeWorld creation path | E |
| `parentId` | `string` | parent runtime id | no esperado | JS APIs indirectly, attachments | builder | D |
| `originalParentId` | `string` | original parent runtime id | no esperado | attachments | builder | D |
| `sourcePath` | `string` | `ObjectDefinition.sourcePath` | no esperado | diagnostics/debug | builder | C/E |
| `group` | `string` | `ObjectDefinition.group` | no esperado | collision, JS | builder | C/F |
| `role` | `string` | `ObjectDefinition.role` | no esperado | JS/game logic | builder | C/F |
| `state` | `string` | `ObjectDefinition.initialState` | si | state bindings | state bindings | A |
| `stateTime` | `float` | runtime zero | si | state_time, tests | update time/state change | A |
| `stateEnteredFrame` | `uint64_t` | runtime zero | si | state_entered | state change/runtime frame | A/E |
| `visible` | `bool` | `ObjectDefinition.visible` | si | draw, JS snapshot | show/hide runtime API | A |
| `alive` | `bool` | runtime true | si | all phases, JS snapshot | kill/keep_only/runtime cleanup | A |
| `deadCalled` | `bool` | runtime false | si | dead phase | dead phase | E |
| `attached` | `bool` | `ObjectDefinition.attachOnCreate` | si | attachments, JS | JS direct write, attach/detach | A/D |
| `layer` | `int` | `ObjectDefinition.layer` | si | draw sorting, JS | JS direct write | A/C |
| `origin` | `Vector2` | `ObjectDefinition.origin` | no esperado | JS originX/Y, to_origin, attach math | builder | C/F |
| `position` | `Vector2` | `ObjectDefinition.origin` | si | draw, collision, motion, JS | JS x/y, movement, attach, carry | A |
| `previousPosition` | `Vector2` | initial origin | si | carry, JS previousX/Y | beginFrame | A |
| `size` | `Vector2` | `ObjectDefinition.size` | si | draw/collision/JS | JS width/height | A/C |
| `originalOffset` | `Vector2` | `ObjectDefinition.offset` | no esperado | attachments | builder | D |
| `attachFollowX` | `bool` | `ObjectDefinition.attachFollowX` | no esperado | attachments | builder | C/F |
| `attachFollowY` | `bool` | `ObjectDefinition.attachFollowY` | no esperado | attachments | builder | C/F |
| `attachFollowAngle` | `bool` | `ObjectDefinition.attachFollowAngle` | no esperado | attachments | builder | C/F |
| `hasOrigin` | `bool` | `ObjectDefinition.hasOrigin` | no esperado | creation/init semantics | builder | C/F |
| `color` | `Color` | `ObjectDefinition.color` | no esperado | draw | builder | C/F |
| `shapeMode` | `string` | `ObjectDefinition.shapeMode` | no esperado | draw | builder | C/F |
| `radius` | `float` | `ObjectDefinition.radius` | no esperado | draw/collision | builder | C/F |
| `speed` | `float` | `ObjectDefinition.speed` | si | motion helpers, JS | JS speed, movement helpers | A |
| `angle` | `float` | `ObjectDefinition.angle` | si | draw/motion/ray, JS | JS angle, movement helpers | A |
| `originSpeed` | `float` | `ObjectDefinition.speed` | no esperado | JS, to_origin | builder | C/F |
| `velocity` | `Vector2` | runtime zero/inheritance | si | motion/carry/JS | JS velocity, motion helpers | A |
| `rotationSpeed` | `float` | `ObjectDefinition.rotationSpeed` | no via JS actual | motion helpers | builder | C/F |
| `acceleration` | `float` | `ObjectDefinition.acceleration` | no via JS actual | accelerate/advance | builder | C/F |
| `maxSpeed` | `float` | `ObjectDefinition.maxSpeed` | no via JS actual | accelerate/advance | builder | C/F |
| `inertia` | `float` | `ObjectDefinition.inertia` | no via JS actual | advance | builder | C/F |
| `shapeType` | `string` | `ObjectDefinition.shapeType` | no esperado | draw | builder | C/F |
| `textContent` | `string` | `ObjectDefinition.textContent` | no esperado | draw text shape | builder | C/F |
| `points` | `vector<Vector2>` | `ObjectDefinition.points` | no esperado | draw polygon | builder | C/F |
| `boundsMode` | `string` | `ObjectDefinition.boundsMode` | no esperado | applyBounds/draw wrap | builder | C/F |
| `boundsOverflow` | `bool` | `ObjectDefinition.boundsOverflow` | si | draw wrap | applyBounds | A |
| `collisionActive` | `bool` | `ObjectDefinition.collisionActive` | no esperado | collision system | builder | C/F |
| `collisionWith` | `vector<string>` | `ObjectDefinition.collisionWith` | no esperado | collision/ray | builder | C/F |
| `collisionType` | `string` | `ObjectDefinition.collisionType` | no esperado | collision/ray/debug draw | builder | C/F |
| `collisionRadius` | `float` | `ObjectDefinition.collisionRadius` | no esperado | collision/ray/debug draw | builder | C/F |
| `scripts` | `vector<string>` | `ObjectDefinition.scripts` | no esperado | currently little/no runtime use | builder | F |
| `resolvedScriptPaths` | `vector<string>` | `ObjectDefinition.resolvedScriptPaths` | no esperado | all script phases | builder | C/F |

### Observaciones C++

- `RuntimeObject` mezcla estado vivo, relaciones runtime, metadata compilada e
  infraestructura interna.
- Muchos campos son copias directas de `ObjectDefinition` y no cambian tras
  instanciar.
- `definitionId` ya apunta hacia la definicion compilada, pero la instancia aun
  conserva muchas copias locales de esa definicion.
- Algunos campos parecen metadata pero hoy son estado mutable real:
  `size`, `layer`, `speed`, `angle`, `attached`.

## B. RuntimeObjectBuilder

`RuntimeObjectBuilder::build(definition, runtimeId, parentId)` realiza estas
operaciones:

### Inicializa o transforma

| RuntimeObject | Regla |
| --- | --- |
| `name` | usa `definition.id`, no `runtimeId`. |
| `runtimeId` | usa el argumento `runtimeId`. |
| `definitionId` | se inicializa con `definition.id`; despues RuntimeWorld puede sobrescribirlo con ResourceId. |
| `parentId` | usa el argumento `parentId`. |
| `originalParentId` | usa el argumento `parentId`. |
| `position` | se inicializa desde `definition.origin`. |
| `previousPosition` | se inicializa desde `definition.origin`. |
| `originSpeed` | se inicializa desde `definition.speed`. |
| `attached` | se inicializa desde `definition.attachOnCreate`. |
| `originalOffset` | se inicializa desde `definition.offset`. |

### Copia directamente desde ObjectDefinition

`sourcePath`, `hasOrigin`, `origin`, `size`, `color`, `visible`, `layer`,
`attachFollowX`, `attachFollowY`, `attachFollowAngle`, `shapeMode`, `shapeType`,
`textContent`, `radius`, `points`, `speed`, `angle`, `rotationSpeed`,
`acceleration`, `maxSpeed`, `inertia`, `boundsMode`, `boundsOverflow`, `group`,
`role`, `collisionType`, `collisionActive`, `collisionRadius`, `collisionWith`,
`scripts`, `resolvedScriptPaths`, `music`, `sounds`, `children`,
`childResources`, `creationMode`, `gridRules`,
`gridPatternIsRows`, `gridPattern`, `gridRowPattern`.

### No copia desde ObjectDefinition

| ObjectDefinition | Resultado runtime |
| --- | --- |
| `spawnMode` | queda en la definicion, se consulta desde registry antes de instanciar. |
| `hasOffset` | no se copia; solo se copia `offset` como `originalOffset`. |
| `hasVisual` | no se copia. |
| `hasSpeed` | no se copia. |
| `hasAngle` | no se copia. |
| `inheritParentAngle` | no se copia al objeto; se aplica durante creacion/spawn. |
| `scriptSourcePaths` | no se copia; se usan en compilacion. |
| `childSourcePaths` | no se copia; se usa en loading/compilacion. |
| `initialState` | no se conserva como campo; se transforma en `RuntimeObject.state`. |

### Puntos especiales

- `children` se copia aunque el proyecto compilado consolidado deberia usar
  `childResources`. Esto parece deuda/compatibilidad interna.
- `music` y `sounds` pertenecen al objeto runtime porque las APIs actuales
  `play_music(object,id)` y `play_sound(object,id)` los consultan desde la
  instancia.
- `resolvedScriptPaths` se copia y es esencial para ejecutar callbacks.
- `stateTransitions` permanece en `ObjectDefinition`; las bindings de estado
  consultan la definicion compilada mediante `definitionId`.
- Grid se copia completo; `creation.grid` no consulta la definicion original.

## C. Contrato JavaScript actual

`ScriptEngine::createJsObject()` crea un objeto nuevo por callback. Al terminar
el callback, `applyJsObject()` copia de vuelta solo algunas propiedades.

| JS property | Readable | Writable real | Documented | Runtime source | Observacion |
| --- | --- | --- | --- | --- | --- |
| `local` | si | si, numerico | si | `RuntimeObject.local` | Solo persisten valores convertibles a number. |
| `name` | si | no | si | `RuntimeObject.name` | Nombre logico de instancia. |
| `id` | si | no | si | `RuntimeObject.runtimeId` | Expone runtime id, aunque se llama `id`. |
| `alive` | si | no | si | `RuntimeObject.alive` | Escritura ignorada; usar `kill()`. |
| `visible` | si | no | si | `RuntimeObject.visible` | Escritura ignorada; usar `show()/hide()`. |
| `attached` | si | si | si | `RuntimeObject.attached` | Mutable real, ademas de `attach()/detach()`. |
| `group` | si | no | si | `RuntimeObject.group` | Metadata de collision/logica. |
| `role` | si | no | si | `RuntimeObject.role` | Metadata logica dentro de group. |
| `layer` | si | si | si | `RuntimeObject.layer` | Cambia orden de dibujo runtime. |
| `x` | si | si | si | `RuntimeObject.position.x` | Estado vivo. |
| `previousX` | si | no | si | `RuntimeObject.previousPosition.x` | Escritura ignorada. |
| `y` | si | si | si | `RuntimeObject.position.y` | Estado vivo. |
| `previousY` | si | no | si | `RuntimeObject.previousPosition.y` | Escritura ignorada. |
| `width` | si | si | si | `RuntimeObject.size.x` | Estado vivo/tamano de draw/collision. |
| `height` | si | si | si | `RuntimeObject.size.y` | Estado vivo/tamano de draw/collision. |
| `speed` | si | si | si | `RuntimeObject.speed` | Usado por movimiento clasico. |
| `angle` | si | si | si | `RuntimeObject.angle` | Usado por movimiento/draw/ray. |
| `originX` | si | no | si | `RuntimeObject.origin.x` | Escritura ignorada. |
| `originY` | si | no | si | `RuntimeObject.origin.y` | Escritura ignorada. |
| `originSpeed` | si | no | si | `RuntimeObject.originSpeed` | Escritura ignorada. |
| `motion` | si | no real | si | snapshot de varios campos | Es objeto anidado nuevo; sus escrituras no se aplican. |
| `motion.speed` | si | no | si | `RuntimeObject.speed` | Duplicado de `speed`, solo snapshot. |
| `motion.angle` | si | no | si | `RuntimeObject.angle` | Duplicado de `angle`, solo snapshot. |
| `motion.rotationSpeed` | si | no | si | `RuntimeObject.rotationSpeed` | Escritura ignorada. |
| `motion.acceleration` | si | no | si | `RuntimeObject.acceleration` | Escritura ignorada. |
| `motion.inertia` | si | no | si | `RuntimeObject.inertia` | Escritura ignorada. |
| `motion.maxSpeed` | si | no | si | `RuntimeObject.maxSpeed` | Escritura ignorada. |
| `velocityX` | si | si | si | `RuntimeObject.velocity.x` | Estado vivo. |
| `velocityY` | si | si | si | `RuntimeObject.velocity.y` | Estado vivo. |

### Propiedades C++ no expuestas a JS

No se exponen directamente: `definitionId`, `parentId`, `originalParentId`,
`sourcePath`, `state`, `stateTime`, `stateEnteredFrame`, `deadCalled`,
`origin`, `position`, `previousPosition`, `size`, `originalOffset`,
`attachFollowX/Y/Angle`, `hasOrigin`, `color`, `shapeMode`, `radius`,
`shapeType`, `textContent`, `points`, `boundsMode`, `boundsOverflow`,
`collisionActive`, `collisionWith`, `collisionType`, `collisionRadius`,
`scripts`, `resolvedScriptPaths`, `children`, `childResources`, `music`,
`sounds`, `timers`, `creationMode`, `gridRules`,
`gridPattern*`.

Algunas de estas capacidades se exponen mediante funciones:

- `state_current`, `state_active`, `state_entered`, `state_time`.
- `play_timer`, `pause_timer`, `stop_timer`, `timer_active`,
  `timer_paused`, `timer_done`, `timer_left`.
- `spawn`, `play_sound`, `play_music`.
- `attach`, `detach`, `attach_active`.

## D. Duplicidades JS

| Duplicidad | Comportamiento actual |
| --- | --- |
| `speed` vs `motion.speed` | `speed` es mutable real; `motion.speed` es snapshot ignorado. |
| `angle` vs `motion.angle` | `angle` es mutable real; `motion.angle` es snapshot ignorado. |
| `width/height` vs `shape.size` JSON | JS modifica `RuntimeObject.size`; no modifica definicion. |
| `x/y` vs `originX/originY` | `x/y` son posicion viva; `originX/originY` son snapshot de origen. |
| `attached` vs `attach()/detach()` | Ambos modifican `RuntimeObject.attached`; la funcion expresa mejor la intencion. |
| `id` vs `name` | `id` es runtime id unico; `name` es id logico de instancia. |

La duplicidad mas peligrosa es `motion`: parece una configuracion viva, esta
documentada como `MotionConfig`, pero `applyJsObject()` no lee el subobjeto.

## E. Identidad expuesta

Actualmente JS recibe:

- `object.name`: nombre logico de instancia, procedente de `ObjectDefinition.id`.
- `object.id`: runtime id unico, procedente de `RuntimeObject.runtimeId`.

No se expone:

- `runtimeId` con ese nombre.
- `definitionId`.
- `parentId` u `originalParentId`.
- `sourcePath`.

Usos reales:

- Documentacion publica muestra `ship.id` y `ship.name`.
- Ejemplos usan `name` en Arkanoid para diferenciar powerups:
  `powerup.name == "broken"`, `gun`, `big`, `small`.
- No se han encontrado usos reales de `object.id` en `examples`.
- Las APIs funcionales internas usan `id` del objeto JS para resolver runtime id
  en `kill`, `spawn`, `keep_only`, `show`, `hide`, `attach`, etc.

Conclusiones de caracterizacion:

- El runtime id explicito es necesario para las funciones nativas actuales, pero
  no parece necesario como dato de juego en ejemplos.
- `id` como nombre publico es ambiguo porque no es el id logico del JSON ni el
  nombre de instancia; es identidad runtime.

## F. Metadata y consumidores reales

| Metadata | Expuesta a JS | Mutable real | Consumidores reales |
| --- | --- | --- | --- |
| `group` | si | no | colisiones en Pong/Asteroids/Arkanoid/Invaders; ray result usa group. |
| `role` | si | no | documentada; sin uso claro en examples actuales. |
| `layer` | si | si | tests; posible uso dinamico futuro; examples no lo usan por JS. |
| `originX/Y` | si | no | Arkanoid title usa `originY`. |
| `size` como `width/height` | si | si | Asteroids fades, Arkanoid paddle, collision positioning. |
| collision metadata | no directa | no | collision system/ray; JS solo ve `group` y `role`. |
| `music/sounds` | no directa | no | APIs `play_music/play_sound`. |
| `children/childResources` | no directa | no | API `spawn(object,id)` y creation runtime. |
| states | no directa | via API | state bindings. |

## G. Accesos reales en examples

Conteo aproximado de propiedades JS relevantes encontradas en `examples`:

| Propiedad | Frecuencia aproximada | Uso |
| --- | ---: | --- |
| `local` | 24 | estado por objeto. |
| `group` | 20 | filtrado en collision. |
| `width` | 17 | resize, colision manual, efectos. |
| `x` | 15 | limites, posicion, draw helpers. |
| `angle` | 12 | direccion de movimiento/disparo/rebote. |
| `y` | 11 | limites, posicion. |
| `speed` | 5 | inicializacion aleatoria/efectos. |
| `height` | 5 | resize/colision manual. |
| `name` | 4 | powerups de Arkanoid. |
| `attached` | 2 | logica de bola en Arkanoid. |
| `originY` | 2 | retorno de titulo en Arkanoid. |

Propiedades expuestas pero sin uso claro en `examples`: `id`, `visible`, `layer`,
`previousX`, `previousY`, `originX`, `originSpeed`, `motion.*`, `velocityX`,
`velocityY`, `role`.

## H. Inconsistencias

- El objeto JS parece un espejo parcial de `RuntimeObject`, pero solo algunas
  escrituras se aplican.
- `motion` parece editable por estructura, pero no tiene semantica real de
  escritura.
- `alive` y `visible` aparecen como propiedades, pero la semantica real es
  funcional (`kill`, `show`, `hide`).
- `attached` contradice parcialmente esa regla: existe API funcional
  (`attach/detach/attach_active`) pero tambien escritura directa real.
- `group` y `role` son metadata copiadas a cada instancia y expuestas como
  propiedades planas, aunque no se pueden cambiar realmente desde JS.
- `layer` procede de definicion pero se comporta como estado vivo mutable.
- `id` expone runtime id, no id logico; el nombre puede inducir a error.
- `RuntimeObject.children` mantiene definiciones embebidas aunque el modelo
  compilado consolidado usa `childResources`.

## I. Datos duplicados

Candidatos evidentes a consultar por `definitionId`/`ResourceRegistry` en el
futuro, si el coste y ergonomia lo permiten:

- Shape/render metadata: `color`, `shapeMode`, `shapeType`, `textContent`,
  `radius`, `points`.
- Collision metadata: `collisionType`, `collisionRadius`, `collisionWith`,
  posiblemente `collisionActive` si no se quiere toggling runtime.
- Audio declarativo: `music`, `sounds`.
- Spawn/creation declarativo: `childResources`, `creationMode`, `gridRules`,
  `gridPattern*`.
- Script metadata: `resolvedScriptPaths`.
- FSM declarativa: `stateTransitions`.
- Attach config: `attachFollowX/Y/Angle`, `originalOffset`.
- Bounds config: `boundsMode`.

Campos que claramente deben seguir siendo locales por instancia:

- `runtimeId`, `name`, `parentId`, `originalParentId`.
- `local`.
- `position`, `previousPosition`, `velocity`.
- `speed`, `angle` si siguen siendo estado vivo de movimiento.
- `size` mientras JS pueda modificar `width/height`.
- `visible`, `alive`, `deadCalled`.
- `attached`.
- `timers`, `state`, `stateTime`, `stateEnteredFrame`.

Campos dudosos:

- `layer`: podria ser metadata o estado vivo; hoy JS lo modifica realmente.
- `origin` y `originSpeed`: parecen metadata de nacimiento, pero `to_origin`
  depende de ellos.
- `boundsOverflow`: se calcula en runtime aunque depende de `boundsMode`.

Costes de consultar metadata por `definitionId`:

- Ventaja: reduce duplicacion y separa mejor declaracion de estado vivo.
- Coste: draw, collision, audio, scripts y spawn consultarian registry con mas
  frecuencia.
- Riesgo: si alguna metadata se quiere hacer mutable runtime, habria que crear
  override local o API funcional especifica.
- Necesidad: el runtime ya necesita `ResourceRegistry` para spawn y load, pero
  no todo codigo actual esta escrito como consulta a registry.

## J. Candidatos a API funcional

Estas capacidades afectan relacion con el mundo o infraestructura, por lo que
parecen mejores como funciones que como escritura directa:

- Vida: mantener `kill(object)`; evitar mutabilidad directa de `alive`.
- Visibilidad: mantener `show(object)` / `hide(object)`.
- Attach: preferir `attach(object)`, `detach(object)`, `attach_active(object)`
  frente a `object.attached = ...`.
- Spawn/children: mantener `spawn(object,id)`.
- Audio: mantener `play_sound(object,id)` y `play_music(object,id)`.
- Estado: mantener `state`, `state_current`, `state_active`, `state_time`.
- Timers: mantener API funcional de timers.
- Parent/relaciones: no exponer `parentId` hasta tener un contrato de lectura
  claro.

## K. Candidatos a permanecer como propiedades

Propiedades que representan estado local natural y tienen uso real:

- `local`.
- `name` como nombre logico de instancia, aunque conviene decidir si el nombre
  exacto es el correcto.
- `x`, `y`.
- `width`, `height`.
- `speed`, `angle`.
- `velocityX`, `velocityY`, si el modelo de movimiento vectorial sigue siendo
  parte de la API JS.
- `originX`, `originY`, si se consideran referencias utiles de solo lectura.
- `group`, `role`, si se mantienen como lectura de clasificacion logica.

## L. Detalles internos accidentalmente expuestos

- `id` como runtime id bajo un nombre generico.
- `previousX` / `previousY`: utiles para `carry` y debug, pero pueden ser
  detalle de ciclo.
- `originSpeed`: poco usado y no mutable; parece detalle de reset.
- `motion` anidado: expone estructura declarativa sin escritura real.
- `layer`: hoy mutable, pero conceptualmente podria pertenecer a renderer/API
  funcional en el futuro.

## M. Preguntas de diseno pendientes

1. Debe existir una propiedad publica `runtimeId`, o `id` debe seguir
   significando runtime id?
2. `name` debe llamarse `name`, `instance`, `instanceId` o similar?
3. Queremos permitir mutacion directa de `attached`, o solo API funcional?
4. `layer` debe ser estado vivo mutable o metadata declarativa?
5. `motion.*` debe eliminarse de JS, hacerse realmente mutable, o marcarse como
   snapshot readonly?
6. `group` y `role` deben ser readonly explicitos en typings?
7. `width/height` deben seguir modificando tambien collision, o separar shape
   size y collision size?
8. Debe poder leerse la metadata de collision desde JS, o solo `group/role`?
9. Debe `RuntimeObject` conservar copias de metadata compilada o consultarlas
   por `definitionId`?
10. Que politica de compatibilidad aplica antes de v0.3.0 para limpiar esta
    superficie?

## Tests de caracterizacion anadidos

La suite `flx-runtime-lifecycle-tests` confirma ahora:

- Propiedades planas mutables reales: `x`, `y`, `speed`, `angle`,
  `velocityX`, `velocityY`, `width`, `height`, `layer`, `attached`, `local`.
- Propiedades aparentemente mutables pero ignoradas: `motion.speed`,
  `motion.angle`, `motion.rotationSpeed`, `motion.acceleration`,
  `motion.inertia`, `motion.maxSpeed`.
- `alive` y `visible` son de solo lectura efectiva.
- Identidad y metadata son legibles, pero escrituras sobre `id`, `name`,
  `group`, `role`, `originX`, `originY`, `originSpeed`, `previousX`,
  `previousY` no vuelven al runtime.
