# Caracterizacion de la API publica de scripting FLX

Este documento audita la superficie publica actual que recibe el desarrollador
JavaScript en FLX.

No propone cambios de produccion. Su objetivo es congelar una fotografia fiable
para poder disenar despues una API estable sin asumir que JavaScript debe
reflejar C++ ni reproducir la estructura declarativa JSON.

Fuentes revisadas:

- `engine/scripting/ScriptEngine.*`
- `engine/scripting/ScriptBindings.*`
- `engine/scripting/bindings/*`
- `docs/scripts/flx.d.ts`
- documentacion publica `docs/en` y `docs/es`
- ejemplos reales en `examples`
- tests existentes y tests de caracterizacion anadidos

## A. Superficie publica actual completa

### Callbacks de ciclo de vida

Los scripts se envuelven como modulo por archivo. FLX captura estas funciones si
existen:

| Nombre JS | Firma real | Retorno | Implementacion | Fase | RuntimeObject | Modifica runtime | d.ts | Docs | Examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| `born` | `born(object)` | ignorado | `ScriptEngine::loadScript`, `callScriptFunction` | lifecycle | si | mediante objeto/API | si | si | si |
| `action` | `action(object)` | ignorado | `ScriptEngine::loadScript`, `callScriptFunction` | lifecycle | si | mediante objeto/API | si | si | si |
| `motion` | `motion(object)` | ignorado | `ScriptEngine::loadScript`, `callScriptFunction` | lifecycle/movement | si | mediante objeto/API | si | si | si |
| `collision` | `collision(object, other)` | ignorado | `ScriptEngine::loadScript`, `callScriptFunction` | collision | si, dos referencias | mediante ambos objetos/API | si | si | si |
| `draw` | `draw(object)` | ignorado | `ScriptEngine::loadScript`, `callScriptFunction` | drawing | si | mediante objeto/API y dibujo inmediato | si | si | si |
| `dead` | `dead(object)` | ignorado | `ScriptEngine::loadScript`, `callScriptFunction` | lifecycle | si | los cambios no resucitan | si | si | si |

Observaciones:

- Las funciones no son llamadas como metodos del objeto; el objeto se pasa como
  sujeto explicito.
- Una excepcion en callback se registra en `Logger` y el runtime continua.
- El modulo JS conserva variables de archivo compartidas por todas las
  instancias que usan ese mismo script.

### CoreBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Necesita id runtime | Modifica runtime | Efectos externos | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `console.log` | `console.log(...values)` | `undefined` | logging | no | no | no | log `script` | parcial/ambiental | si | 0 |
| `UP` | constante number | number | motion/input | no | no | no | no | si | si | 2 |
| `DOWN` | constante number | number | motion/input | no | no | no | no | si | si | 2 |
| `LEFT` | constante number | number | motion/input | no | no | no | no | si | si | 7 |
| `RIGHT` | constante number | number | motion/input | no | no | no | no | si | si | 8 |
| `STOP` | constante number | number | motion/input | no | no | no | no | si | si | 0 |
| `kill` | `kill(object)` | `undefined` | lifecycle | si | si, lee `object.id` | si | no | si | si | 23 |
| `show` | `show(object)` | `undefined` | visibility | si | si, lee `object.id` | si | no | si | si | 0 |
| `hide` | `hide(object)` | `undefined` | visibility | si | si, lee `object.id` | si | no | si | si | 0 |
| `keep_only` | `keep_only(object)` | `undefined` | lifecycle/world | si | si, lee `object.id` | si | no | si | si | 4 |
| `delta` | `delta()` | number | time | no | no | no | no | si | si | 6 |
| `random` | `random(min, max)` | number | randomness | no | no | no | no | si | si | 24 |
| `probability` | `probability(chance, base?)` | boolean | randomness | no | no | no | no | si | si | 7 |
| `ray` | `ray(source, angle, distance)` | `RayResult` | collision/raycast | si | si, lee `source.id` | no | no | si | si | 0 |
| `exit` | `exit()` | `undefined` | runtime control | no | no | si, solicita salida | cierre ordenado | si | si | 0 |
| `save` | `save(name, key, value)` | `undefined` | persistence | no | no | no runtime world | escribe save binario | si | si | 0 |
| `load` | `load(name, key, defaultValue)` | valor/default | persistence | no | no | no | lee save binario | si | si | 0 |

### MotionBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Necesita id runtime | Modifica runtime | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `move_x` | `move_x(object, direction)` | `undefined` | movement | si | no | si, escribe `x` JS | si | si | 5 |
| `move_y` | `move_y(object, direction)` | `undefined` | movement | si | no | si, escribe `y` JS | si | si | 4 |
| `advance` | `advance(object)` | `undefined` | movement | si | no | si, escribe `x/y` y a veces `velocityX/Y` | si | si | 16 |
| `follow_x` | `follow_x(object, target)` | `undefined` | movement | si, dos refs | no | si, escribe `x` runtime | si | si | 1 |
| `follow_y` | `follow_y(object, target)` | `undefined` | movement | si, dos refs | no | si, escribe `y` runtime | si | si | 1 |
| `attach` | `attach(object)` | `undefined` | relation/movement | si | no | si, cambia `RuntimeObject.attached` | si | si | 1 |
| `detach` | `detach(object)` | `undefined` | relation/movement | si | no | si, cambia `RuntimeObject.attached` | si | si | 1 |
| `attach_active` | `attach_active(object)` | boolean | relation query | si | no | no | si | si | 0 |
| `carry` | `carry(object, carrier)` | `undefined` | relation/movement | si, dos refs | no | si, cambia posicion runtime | si | si | 0 |
| `bounce_x` | `bounce_x(object)` | `undefined` | movement | si | no | si, escribe `angle` JS | si | si | 3 |
| `bounce_y` | `bounce_y(object)` | `undefined` | movement | si | no | si, escribe `angle` JS | si | si | 5 |
| `accelerate` | `accelerate(object, amount?)` | `undefined` | movement | si | no | si, escribe `speed` o `velocityX/Y` | si | si | 2 |
| `rotate` | `rotate(object, direction)` | `undefined` | movement | si | no | si, escribe `angle` JS | si | si | 7 |
| `to_origin` | `to_origin(object)` | `undefined` | movement/reset | si | no | si, escribe `x/y/speed` JS | si | si | 1 |

Notas:

- `follow_x` y `follow_y` reciben una referencia viva de `RuntimeObject`.
- La vista `RuntimeObject` JS es temporal, readonly y viva durante el hook.
  `advance`, `accelerate` y `rotate` trabajan contra estado runtime y
  configuracion interna, no contra un subobjeto `object.motion`.

### DrawBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Modifica runtime | Efectos externos | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `draw_text` | `draw_text(x, y, text, size?, color?)` | `undefined` | drawing | no | no | dibuja pantalla | si | si | 16 |
| `draw_pixel` | `draw_pixel(x, y, color?)` | `undefined` | drawing | no | no | dibuja pantalla | si | si | 2 |
| `draw_line` | `draw_line(x, y, x1, y1, color?)` | `undefined` | drawing | no | no | dibuja pantalla | si | si | 3 |
| `draw_rectangle` | `draw_rectangle(x, y, width, height, color?)` | `undefined` | drawing | no | no | dibuja pantalla | si | si | 0 |
| `fade_on` | `fade_on(color?)` | `undefined` | fade/drawing | no | si, estado global fade | overlay | si | si | 2 |
| `fade_off` | `fade_off(color?)` | `undefined` | fade/drawing | no | si, estado global fade | overlay | si | si | 6 |
| `fade_set` | `fade_set(alpha, color?)` | `undefined` | fade/drawing | no | si, estado global fade | overlay | si | si | 6 |
| `fade_active` | `fade_active()` | boolean | fade query | no | no | no | si | si | 0 |
| `fade_done` | `fade_done()` | boolean | fade query | no | no | no | si | si | 1 |
| `fade_alpha` | `fade_alpha()` | number | fade query | no | no | no | si | si | 0 |

Notas:

- Las funciones `draw_*` usan coordenadas logicas y aplican `screen.scale`.
- El color pasa por `ScriptEngine::parseColor`, por tanto se proyecta con el
  Video Chip actual.
- No crean `RuntimeObject`.
- En el ciclo actual se ejecutan dentro del draw del objeto, respetando `layer`
  del objeto que ejecuta el callback.

### AudioBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Necesita id runtime | Modifica runtime | Efectos externos | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `play_sound` | `play_sound(object, id)` | `undefined` | audio | si | si, lee `object.id` | no world | reproduce sonido | si | si | 15 |
| `play_music` | `play_music(object, id)` | `undefined` | music | si | si, lee `object.id` | no world | reproduce/reemplaza musica | si | si | 1 |
| `stop_music` | `stop_music()` | `undefined` | music | no | no | no world | detiene musica | si | si | 0 |
| `pause_music` | `pause_music()` | `undefined` | music | no | no | no world | pausa/reanuda musica | si | si | 2 |
| `music_active` | `music_active()` | boolean | music query | no | no | no | no | si | si | 0 |
| `music_paused` | `music_paused()` | boolean | music query | no | no | no | no | si | si | 0 |

### SpawnBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Necesita id runtime | Modifica runtime | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `spawn` | `spawn(object, childName)` | `undefined` | spawn/world | si | si, lee `object.id` | si, encola instancia | si | si | 44 |

### StateBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Necesita id runtime | Modifica runtime | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `state_to` | `state_to(object, stateName)` | `undefined` | state | si | si, busca por `object.id` | si | si | si | 4 |
| `state_current` | `state_current(object)` | string | state query | si | si | no | si | si | 0 |
| `state_active` | `state_active(object, stateName)` | boolean | state query | si | si | no | si | si | 3 |
| `state_entered` | `state_entered(object)` | boolean | state query | si | si | no | si | si | 0 |
| `state_time` | `state_time(object)` | number | state query | si | si | no | si | si | 0 |

### TimerBindings

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Necesita id runtime | Modifica runtime | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `play_timer` | `play_timer(object, timerName, duration?)` | `undefined` | timer | si | si | si | si | si | 17 |
| `pause_timer` | `pause_timer(object, timerName)` | `undefined` | timer | si | si | si | si | si | 0 |
| `stop_timer` | `stop_timer(object, timerName)` | `undefined` | timer | si | si | si | si | si | 7 |
| `timer_active` | `timer_active(object, timerName)` | boolean | timer query | si | si | no | si | si | 14 |
| `timer_paused` | `timer_paused(object, timerName)` | boolean | timer query | si | si | no | si | si | 0 |
| `timer_done` | `timer_done(object, timerName)` | boolean | timer query | si | si | no | si | si | 0 |
| `timer_left` | `timer_left(object, timerName)` | number | timer query | si | si | no | si | si | 6 |

### InputBindings

API consolidada:

| Nombre JS | Firma real | Retorno | Subsistema | RuntimeObject | Modifica runtime | d.ts | Docs | Usos examples |
| --- | --- | --- | --- | --- | --- | --- | --- | ---: |
| `button` | `button(index)` | descriptor | input | no | no | si | si | si |
| `direction` | `direction(index)` | descriptor | input | no | no | si | si | si |
| `player` | `player(index)` | descriptor sujeto | input | no | no | si | si | si |
| `system` | `system()` | descriptor sujeto | input | no | no | si | si | si |
| `input_pressed` | `(subject, control, component?)` | boolean | input | si, si subject es objeto | no | si | si | si |
| `input_down` | `(subject, control, component?)` | boolean | input | si, si subject es objeto | no | si | si | si |
| `input_released` | `(subject, control, component?)` | boolean | input | si, si subject es objeto | no | si | si | si |

`Input.*`, `Key.*` y los antiguos `__flx_input_*` no forman parte de la
superficie publica consolidada.

## B. Familias conceptuales

| Familia | APIs actuales |
| --- | --- |
| lifecycle | `born`, `action`, `motion`, `collision`, `draw`, `dead`, `kill`, `keep_only` |
| visibility | `show`, `hide`, `visible` |
| spawn | `spawn` |
| movement | `move_x`, `move_y`, `advance`, `rotate`, `accelerate`, `bounce_x`, `bounce_y`, `follow_x`, `follow_y`, `to_origin`, propiedades `x/y/speed/angle/velocityX/velocityY` |
| relation | `attach`, `detach`, `attach_active`, `carry`, `attached` |
| state | `state_to`, `state_current`, `state_active`, `state_entered`, `state_time` |
| timer | `play_timer`, `pause_timer`, `stop_timer`, `timer_active`, `timer_paused`, `timer_done`, `timer_left` |
| collision/raycast | callback `collision`, `ray`, `group`, `role` |
| drawing | callback `draw`, `draw_text`, `draw_pixel`, `draw_line`, `draw_rectangle`, `fade_*` |
| audio/music | `play_sound`, `play_music`, `stop_music`, `pause_music`, `music_active`, `music_paused` |
| input | `button`, `direction`, `player`, `system`, `input_pressed`, `input_down`, `input_released`, constantes de direccion |
| persistence | `save`, `load` |
| runtime control | `exit` |
| randomness/time | `delta`, `random`, `probability` |
| shared state | `global`, variables de modulo, `object.local` |

Casos repartidos entre bindings:

- Lifecycle vive en callbacks de `ScriptEngine` y funciones de `CoreBindings`.
- Drawing incluye primitivas inmediatas y fade global en `DrawBindings`.
- Collision mezcla callback de lifecycle, metadata `group/role` y `ray` en
  `CoreBindings`.
- Relation mezcla attach persistente y carry temporal dentro de
  `MotionBindings`.

## C. Patrones de nomenclatura actuales

| API actual | Familia | Verbo principal | Complemento | Patron actual | Consistente | Problema semantico |
| --- | --- | --- | --- | --- | --- | --- |
| `kill` | lifecycle | kill | - | verbo | si | claro |
| `show` / `hide` | visibility | show/hide | - | verbos opuestos | si | contrasta con propiedad `visible` readonly |
| `keep_only` | lifecycle/world | keep | only | verbo_complemento | medio | nombre claro, pero opera globalmente |
| `delta` | time | - | delta | sustantivo | medio | consulta sin verbo |
| `random` | randomness | - | random | sustantivo/adjetivo | medio | consulta sin verbo |
| `probability` | randomness | - | probability | sustantivo | medio | devuelve boolean, no valor de probabilidad |
| `ray` | collision | - | ray | sustantivo | medio | consulta/accion sin verbo |
| `exit` | runtime | exit | - | verbo | si | claro |
| `save` / `load` | persistence | save/load | - | verbos opuestos | si | claro |
| `move_x` / `move_y` | movement | move | x/y | verbo_eje | si | claro |
| `advance` | movement | advance | - | verbo | si | depende de `motion` snapshot |
| `follow_x` / `follow_y` | movement | follow | x/y | verbo_eje | si | segundo argumento es `RuntimeObject` |
| `attach` / `detach` | relation | attach/detach | - | verbos opuestos | si | tambien existe `attached` mutable |
| `attach_active` | relation | attach | active | sustantivo_estado | medio | no sigue `is_attached`; pero ya esta alineado con `*_active` |
| `carry` | relation | carry | - | verbo | si | efecto solo del frame actual |
| `bounce_x` / `bounce_y` | movement | bounce | x/y | verbo_eje | si | modifica angle, no posicion |
| `accelerate` | movement | accelerate | - | verbo | si | sobrecarga por argc |
| `rotate` | movement | rotate | - | verbo | si | claro |
| `to_origin` | movement/reset | to | origin | direccion_destino | bajo | no empieza por verbo de accion claro |
| `draw_text` | drawing | draw | text | verbo_objeto | si | claro |
| `draw_pixel` | drawing | draw | pixel | verbo_objeto | si | claro |
| `draw_line` | drawing | draw | line | verbo_objeto | si | claro |
| `draw_rectangle` | drawing | draw | rectangle | verbo_objeto | si | claro |
| `fade_on` / `fade_off` | fade | fade | on/off | sustantivo_estado | si | `on` significa hacia opaco |
| `fade_set` | fade | fade | set | objeto_verbo | medio | invierte VERBO_COMPLEMENTO |
| `fade_active` | fade | fade | active | objeto_estado | si con `attach_active` | consulta |
| `fade_done` | fade | fade | done | objeto_estado | si | consulta finalizacion |
| `fade_alpha` | fade | fade | alpha | objeto_valor | medio | consulta sin verbo |
| `play_sound` | audio | play | sound | verbo_objeto | si | claro |
| `play_music` | music | play | music | verbo_objeto | si | familia inversa a `music_active` |
| `stop_music` | music | stop | music | verbo_objeto | si | claro |
| `pause_music` | music | pause | music | verbo_objeto | si | alterna pausa/reanuda |
| `music_active` | music | music | active | objeto_estado | medio | invierte respecto a `play_music` |
| `music_paused` | music | music | paused | objeto_estado | medio | `paused` vs `active` |
| `spawn` | spawn | spawn | - | verbo | si | claro |
| `state_to` | state | state | to | verbo compuesto | si | solicita transicion validada |
| `state_current` | state | state | current | objeto_estado | si | consulta |
| `state_active` | state | state | active | objeto_estado | si | consulta |
| `state_entered` | state | state | entered | objeto_evento | medio | evento de frame |
| `state_time` | state | state | time | objeto_valor | medio | consulta |
| `play_timer` | timer | play | timer | verbo_objeto | si | inicia, reanuda o reproduce temporizador |
| `pause_timer` | timer | pause | timer | verbo_objeto | si | pausa sin alternar |
| `stop_timer` | timer | stop | timer | verbo_objeto | si | elimina sin marcar done |
| `timer_active` | timer | timer | active | objeto_estado | si | consulta |
| `timer_paused` | timer | timer | paused | objeto_estado | si | consulta |
| `timer_done` | timer | timer | done | objeto_estado | si | consulta finalizacion natural |
| `timer_left` | timer | timer | left | objeto_valor | medio | consulta |
| `input_pressed` | input | input | pressed | sujeto_control | medio | botones y direcciones comparten consulta |
| `input_down` | input | input | down | sujeto_control | medio | botones y direcciones comparten consulta |
| `input_released` | input | input | released | sujeto_control | medio | botones y direcciones comparten consulta |

## D. Inconsistencias de nomenclatura

- Algunas familias usan `verbo_complemento`: `draw_text`, `play_sound`,
  `move_x`.
- Otras invierten el orden: `fade_set`, `state_current`.
- Timer ya no usa el sustantivo `timer` como accion. La operacion se expresa
  con `play_timer`, `pause_timer` y `stop_timer`.
- `*_active` aparece en fade, music, state, timer y attach, con significado
  bastante estable: consulta booleana de estado activo.
- `fade_done` y `timer_done` comparten el significado de proceso terminado.
- `pause_music` alterna pausa/reanuda, pero el nombre solo expresa una mitad.
- `follow_x/follow_y` reciben target objeto igual que `carry(object, carrier)`.
- `to_origin` expresa destino, no accion (`reset`, `return`, etc.).
- Input usa `pressed` para botones y sistema, pero no para direcciones.

## E. Contrato actual del objeto JS

La matriz completa esta desarrollada en
`docs/runtime-object-characterization.md`. Resumen:

| Propiedad JS | Lectura | Escritura real | Fuente runtime | Categoria |
| --- | --- | --- | --- | --- |
| `local` | si | si, numerica | `RuntimeObject.local` | propiedad local |
| `name` | si | no | `RuntimeObject.name` | metadata/identidad logica |
| `id` | si | no | `RuntimeObject.runtimeId` | identidad tecnica |
| `alive` | si | no | `RuntimeObject.alive` | estado vivo readonly |
| `visible` | si | no | `RuntimeObject.visible` | estado vivo readonly |
| `attached` | no | no | `RuntimeObject.attached` | consultar con `attach_active()` |
| `group` | si | no | `RuntimeObject.group` | metadata |
| `role` | si | no | `RuntimeObject.role` | metadata |
| `layer` | no | no | `RuntimeObject.layer` | no expuesto como propiedad runtime |
| `x`, `y` | si | no | `RuntimeObject.position` | estado vivo readonly; cambiar con funciones |
| `previousX`, `previousY` | no | no | `RuntimeObject.previousPosition` | no expuesto como propiedad runtime |
| `width`, `height` | si | no | `RuntimeObject.size` | estado vivo/tamano readonly; cambiar con funciones |
| `speed`, `angle` | si | no | `RuntimeObject.speed/angle` | estado vivo readonly; cambiar con funciones |
| `velocityX`, `velocityY` | si | no | `RuntimeObject.velocity` | estado vivo readonly; cambiar con funciones |
| `originX`, `originY` | no | no | `RuntimeObject.origin` | no expuesto como propiedad runtime |
| `originSpeed` | no | no | `RuntimeObject.originSpeed` | no expuesto como propiedad runtime |
| `motion.*` | no | no | - | no expuesto como propiedad runtime |

Clasificacion del objeto JS:

- A. Operacion de comportamiento: funciones como `kill`, `spawn`,
  `play_sound`.
- B. Consulta de comportamiento: `timer_active`, `state_active`,
  `fade_done`.
- C. Vista viva readonly: `id`, `name`, `group`, `role`, `alive`, `visible`,
  `x/y`, `speed`, `angle`, `velocityX/Y`, `width/height`, `rotationSpeed`.
- D. Exposicion accidental: `id` como runtime id.
- E. Estructura heredada del JSON: ya no se expone como objeto runtime.
- F. Infraestructura: `global`, callbacks, `console.log`.

## F. Identidad y referencias

Identidad actual:

- `object.id` expone `RuntimeObject.runtimeId`.
- `object.name` expone el nombre logico de instancia.
- No se expone `definitionId`.
- No se expone `parentId`.

APIs que dependen del `id` visible:

- `kill`, `show`, `hide`, `keep_only`.
- `spawn`, `play_sound`, `play_music`.
- `state*`, `timer*`.
- `ray`.

APIs que usan referencias JS directas sin busqueda runtime:

- `move_x`, `move_y`, `advance`, `rotate`, `accelerate`, `bounce_x`,
  `bounce_y`, `attach`, `detach`, `attach_active`, `carry`, `to_origin`.

APIs que localizan objetos:

- `find_id(id)`.
- `find_name(name)`.
- `find_parent(object)`.
- `find_children(object)`.

APIs globales:

- `fade_*`, `music_*`, `delta`, `random`, `probability`, `exit`,
  `save/load`, `draw_*`, `Input`.

Conclusiones de caracterizacion:

- El desarrollador no usa `object.id` en los ejemplos actuales.
- El motor si lo necesita internamente para reencontrar el `RuntimeObject`
  despues de recibir un objeto JS.
- Tecnicamente podria ocultarse detras de una representacion JS opaca en el
  futuro, pero hoy muchas bindings dependen de leer la propiedad publica `id`.
- Para obtener otras instancias hoy solo existen:
  - el segundo parametro de `collision`;
  - `find_id`, `find_name`, `find_parent`, `find_children`;
  - retorno indirecto de `spawn` no existe;
  - `ray` no devuelve objeto ni id.

## G. JSON vs JS

| Concepto JSON | Exposicion JS actual | Clasificacion |
| --- | --- | --- |
| `motion.speed` | `object.speed` readonly + `apply_speed()` | estado vivo consultable; escritura funcional |
| `motion.angle` | `object.angle` readonly + `apply_angle()`/`rotate()` | estado vivo consultable; escritura funcional |
| `motion.rotationSpeed` | `object.rotationSpeed` readonly + `apply_rotation_speed()` | estado vivo consultable; escritura funcional |
| `motion.acceleration` | no directo | declarativo consumido por runtime |
| `motion.inertia` | no directo | declarativo consumido por runtime |
| `motion.maxSpeed` | no directo | declarativo consumido por runtime |
| `shape.size` | `object.width/height` | estado vivo de tamano |
| `group` | `object.group` | metadata de juego/collision |
| `role` | `object.role` | metadata de juego |
| `layer` | no directo | declarativo consumido por draw order |
| `children` | no directo; `spawn(object,id)` | declaracion operada por API |
| `sounds` | no directo; `play_sound(object,id)` | declaracion operada por API |
| `music` | no directo; `play_music(object,id)` | declaracion operada por API |
| `states` | no directo; `state*` | declaracion operada por API |
| `collision` | no directo; callback/ray/group/role | metadata consumida por runtime |
| `creation.grid` | no directo | declaracion consumida por runtime |

Principio observado:

- Cuando la API usa funciones, el lenguaje se acerca a "funcion = verbo,
  objeto = sujeto".
- La vista JS ya no reproduce bloques declarativos como `motion.*`; expone solo
  estado runtime consultable y operaciones funcionales.

## H. API realmente utilizada

Conteo aproximado en `examples/*.js`:

| API | Usos |
| --- | ---: |
| `spawn` | 44 |
| `random` | 24 |
| `kill` | 23 |
| `play_timer` | 17 |
| `advance` | 16 |
| `draw_text` | 16 |
| `play_sound` | 15 |
| `timer_active` | 14 |
| `input_down` | ejemplos migrados |
| `input_pressed` | ejemplos migrados |
| `rotate` | 7 |
| `probability` | 7 |
| `delta` | 6 |
| `fade_off` | 6 |
| `fade_set` | 6 |
| `timer_left` | 6 |
| `move_x` | 5 |
| `bounce_y` | 5 |
| `keep_only` | 4 |
| `move_y` | 4 |
| `state_to` | 4 |
| `bounce_x` | 3 |
| `draw_line` | 3 |
| `state_active` | 3 |
| `draw_pixel` | 2 |
| `fade_on` | 2 |
| `pause_music` | 2 |
| `stop_timer` | 7 |
| `accelerate` | 2 |
| `follow_x`, `follow_y`, `attach`, `detach`, `to_origin`, `fade_done`, `play_music` | 1 cada una |

Propiedades de objeto usadas en `examples/*.js`:

| Propiedad | Usos |
| --- | ---: |
| `local` | 24 |
| `group` | 20 |
| `width` | 17 |
| `x` | 12 |
| `y` | 10 |
| `angle` | 10 |
| `speed` | 5 |
| `height` | 5 |
| `name` | 4 |
| `attached` | 2 |
| `originY` | 2 |

Nota: los conteos son busquedas textuales aproximadas, no un AST JS.

## I. API expuesta pero no utilizada

Implementada y documentada, pero sin uso claro en `examples`:

- `console.log`
- `show`
- `hide`
- `ray`
- `exit`
- `save`
- `load`
- `attach_active`
- `carry`
- `draw_rectangle`
- `fade_active`
- `fade_alpha`
- `stop_music`
- `music_active`
- `music_paused`
- `state_current`
- `state_entered`
- `state_time`
- `STOP`

Propiedades expuestas sin uso claro en `examples`:

- `id`
- `alive`
- `visible`
- `role`
- `layer`
- `previousX`
- `previousY`
- `originX`
- `originSpeed`
- `velocityX`
- `velocityY`

API retirada o no implementada:

- `Input.*`.
- `Key.*`.
- `__flx_input_*`.

## J. Diferencias implementation / d.ts / docs

| Elemento | Implementacion | d.ts | Docs publicas | Observacion |
| --- | --- | --- | --- | --- |
| funciones principales | si | si | si | alineadas en lo esencial |
| callbacks | capturados por wrapper | declarados como funciones | si | correcto para authoring |
| `console.log` | implementado | no declarado localmente | ambiental TS | aceptable, pero no propio FLX |
| `global` | solo numeros persistentes | `Record<string, number>` | si | alineado |
| `local` | solo numeros persistentes | `Record<string, number>` | si | alineado |
| `id/name/group/role/x/y/width/height/speed/angle/velocity*/rotationSpeed` | vista viva readonly | readonly | si | alineado tras refactor |
| `origin*/previous*/layer/motion.*` | no expuesto | no | parcialmente historico | retirar restos documentales antiguos |
| `attached` | API funcional | no directo | si | alineado con `attach_active()` |
| `Input.*` / `Key.*` / `__flx_input_*` | retirado | no | no actual | ejemplos migrados |
| audio config types | tipos d.ts | no son objetos runtime JS | JSON authoring | no forman API runtime directa |

## K. Candidatos claros a propiedad JS

No son decisiones, solo candidatos segun el comportamiento actual:

- `x`, `y`: posicion viva.
- `width`, `height`: tamano vivo, mientras siga afectando dibujo/colision.
- `speed`, `angle`: movimiento clasico vivo.
- `velocityX`, `velocityY`: movimiento vectorial vivo.
- `group`, `role`: lectura de clasificacion usada por gameplay.
- `name`: lectura de identidad logica, si se mantiene ese concepto.

## L. Candidatos claros a funcion del motor

No son decisiones, solo candidatos segun el tipo de efecto:

- Vida: `kill`, `keep_only`.
- Visibilidad: `show`, `hide`.
- Relaciones: `attach`, `detach`, `attach_active`, `carry`.
- Instanciacion: `spawn`.
- Audio/musica: `play_sound`, `play_music`, `stop_music`, `pause_music`.
- Estados y timers: familias `state_*`, `play_timer`, `pause_timer`,
  `stop_timer` y consultas `timer_*`.
- Persistencia y salida: `save`, `load`, `exit`.
- Consultas de mundo: `ray`.

## M. Candidatos a ocultar

- `id` como runtime id publico: hoy es necesario para bindings, pero no aparece
  como dato de juego en ejemplos.
- `motion.*`: estructura declarativa; ya no debe aparecer como objeto runtime.
- `previousX/previousY`: util para `carry`, pero puede ser detalle interno si
  no hay caso de gameplay directo.
- `originSpeed`: parece soporte de `to_origin`, no necesariamente API publica.

## N. Familias que requieren mini-auditoria propia

- Input: quedan pendientes 8way, analog, pointer, text y remapeo avanzado.
- Movimiento: convivencia entre speed/angle, velocity, acceleration y metadata
  declarativa.
- Identidad/referencias: si `id` debe existir, si se exponen parent/children, y
  como obtener referencias a otras instancias sin busqueda global.
- Drawing/video layers: relacion futura entre `draw_*`, callbacks de mundo y
  capas de video superiores.
- Audio/music: nomenclatura `play_music` vs `music_active`, pausa toggle, y
  consultas futuras.
- Persistence: ubicacion final de saves y tipos soportados.
- Estado/timers: `state_to` ya usa verbo compuesto; Timer consolida el control
  de proceso con `play_timer`, `pause_timer` y `stop_timer`.

## O. Preguntas de diseno pendientes

1. Debe el desarrollador ver `object.id`, o basta con una referencia JS opaca?
2. Si existe `id`, debe llamarse `runtimeId` para no confundirse con `name` o
   `definitionId`?
3. `name` representa identidad de instancia, etiqueta logica, o deberia ser
   reemplazado por otra palabra?
4. `group` y `role` deben seguir como propiedades readonly o migrar hacia API
   funcional?
5. `attached` debe poder escribirse directamente o solo mediante
   `attach/detach`?
6. `layer` debe ser estado mutable del objeto o declaracion inmutable?
7. `motion.*` debe desaparecer de la API JS, hacerse readonly real, o aplicar
   escrituras al runtime?
8. Debe existir una forma publica de obtener parent, children o ultima instancia
   spawneada?
9. Resuelto: `follow_x/follow_y` reciben una referencia viva de `RuntimeObject`.
10. Como se normaliza la nomenclatura de consultas: `*_active`, `is_*`,
    `*_done`, `*_left`, `*_current`?
11. Debe exponerse en el futuro una API de remapeo o introspeccion de controles?
12. Debe existir un sistema de nombres de acciones por encima de los controles
    logicos numerados?

## Tests de caracterizacion anadidos

La suite `flx-runtime-lifecycle-tests` incorpora:

- `script module variables are shared between instances`: confirma que una
  variable `var shared` a nivel de script se comparte entre instancias del mismo
  modulo.
- `collision callback receives concrete other reference`: confirma que el
  segundo argumento de `collision(object, other)` representa la instancia
  concreta y que sus escrituras aplican al `RuntimeObject` correspondiente.
- `public scripting functions are registered`: confirma que las funciones
  principales, `console.log` e `Input` existen en el contexto JS.

Estas pruebas se suman a las de `docs/runtime-object-characterization.md` sobre
propiedades mutables reales, propiedades aparentemente mutables pero ignoradas,
identidad, metadata, `alive`, `visible` y `attached`.
