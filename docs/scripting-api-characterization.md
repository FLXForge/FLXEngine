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
| `move_horizontal` | `move_horizontal(object, intent)` | `undefined` | movement | si | no | si, escribe posicion runtime | si | si | 6 |
| `move_vertical` | `move_vertical(object, intent)` | `undefined` | movement | si | no | si, escribe posicion runtime | si | si | 2 |
| `advance` | `advance(object)` | `undefined` | movement | si | no | si, escribe `x/y` y a veces `velocityX/Y` runtime | si | si | 17 |
| `follow_x` | `follow_x(object, target)` | `undefined` | movement | si, dos refs | no | si, escribe `x` runtime | si | si | 3 |
| `follow_y` | `follow_y(object, target)` | `undefined` | movement | si, dos refs | no | si, escribe `y` runtime | si | si | 1 |
| `attach` | `attach(object)` | `undefined` | relation/movement | si | no | si, cambia `RuntimeObject.attached` | si | si | 1 |
| `detach` | `detach(object)` | `undefined` | relation/movement | si | no | si, cambia `RuntimeObject.attached` | si | si | 1 |
| `attach_active` | `attach_active(object)` | boolean | relation query | si | no | no | si | si | 2 |
| `carry` | `carry(object, carrier)` | `undefined` | relation/movement | si, dos refs | no | si, cambia posicion runtime | si | si | 0 |
| `reflect_x` | `reflect_x(object)` | `undefined` | movement | si | no | si, escribe `angle` runtime | si | si | 5 |
| `reflect_y` | `reflect_y(object)` | `undefined` | movement | si | no | si, escribe `angle` runtime | si | si | 5 |
| `accelerate` | `accelerate(object, amount?)` | `undefined` | movement | si | no | si, escribe `speed` o `velocityX/Y` | si | si | 2 |
| `rotate` | `rotate(object, direction)` | `undefined` | movement | si | no | si, escribe `angle` runtime | si | si | 7 |
| `position` | `position(object, x, y)` | `undefined` | movement/position | si | no | si, escribe `x/y` runtime | si | si | 5 |
| `position_x` | `position_x(object, x)` | `undefined` | movement/position | si | no | si, escribe `x` runtime | si | si | 4 |
| `position_y` | `position_y(object, y)` | `undefined` | movement/position | si | no | si, escribe `y` runtime | si | si | 4 |
| `position_origin` | `position_origin(object)` | `undefined` | movement/reset | si | no | si, escribe `x/y` runtime | si | si | 1 |
| `apply_speed` | `apply_speed(object, value)` | `undefined` | movement | si | no | si, escribe `speed` runtime | si | si | 5 |
| `apply_angle` | `apply_angle(object, angle)` | `undefined` | movement | si | no | si, escribe `angle` runtime | si | si | 3 |
| `apply_rotation_speed` | `apply_rotation_speed(object, speed)` | `undefined` | movement | si | no | si, escribe `rotationSpeed` runtime | si | si | 4 |
| `apply_velocity` | `apply_velocity(object, angle, speed)` | `undefined` | movement | si | no | si, escribe `velocityX/Y` runtime | si | si | 3 |
| `resize` | `resize(object, width, height)` | `undefined` | movement/size | si | no | si, escribe `width/height` runtime | si | si | 4 |
| `resize_width` | `resize_width(object, width)` | `undefined` | movement/size | si | no | si, escribe `width` runtime | si | si | 4 |
| `resize_height` | `resize_height(object, height)` | `undefined` | movement/size | si | no | si, escribe `height` runtime | si | si | 0 |
| `restore_speed` | `restore_speed(object)` | `undefined` | movement/reset | si | no | si, escribe `speed` runtime | si | si | 2 |

Notas:

- `follow_x` y `follow_y` reciben una referencia viva de `RuntimeObject`.
- La vista `RuntimeObject` JS es temporal, readonly y viva durante el hook.
  `advance`, `accelerate` y `rotate` trabajan contra estado runtime y
  configuracion interna, no contra un subobjeto `object.motion`.
- Frase de contrato: el scripting conserva identidad; el Runtime conserva el
  mundo vivo. Los RuntimeObjects se resuelven, no se persisten.
- `dead(object)` recibe un contexto final deliberado de una instancia muerta;
  `find_parent` y `find_children` pueden partir de ese contexto, pero siguen
  devolviendo solo objetos vivos.

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
| movement | `move_horizontal`, `move_vertical`, `advance`, `rotate`, `accelerate`, `reflect_x`, `reflect_y`, `follow_x`, `follow_y`, `position*`, `resize*`, `apply_*`, `restore_speed`, propiedades readonly `x/y/speed/angle/velocityX/velocityY/rotationSpeed` |
| relation | `attach`, `detach`, `attach_active`, `carry` |
| state | `state_to`, `state_current`, `state_active`, `state_entered`, `state_time` |
| timer | `play_timer`, `pause_timer`, `stop_timer`, `timer_active`, `timer_paused`, `timer_done`, `timer_left` |
| collision/raycast | callback `collision`, `ray`, `group`, `role` |
| drawing | callback `draw`, `draw_text`, `draw_pixel`, `draw_line`, `draw_rectangle`, `fade_*` |
| audio/music | `play_sound`, `play_music`, `stop_music`, `pause_music`, `music_active`, `music_paused` |
| input | `button`, `direction`, `player`, `system`, `input_pressed`, `input_down`, `input_released`, constantes de direccion |
| persistence | `save`, `load` |
| runtime control | `exit` |
| randomness/time | `delta`, `random`, `probability` |
| shared state | `read_global`, `write_global`, variables de modulo, `read_local`, `write_local` |

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
| `move_horizontal` / `move_vertical` | movement | move | horizontal/vertical | verbo_eje | si | claro |
| `advance` | movement | advance | - | verbo | si | consume estado runtime y mecanicas declarativas |
| `follow_x` / `follow_y` | movement | follow | x/y | verbo_eje | si | segundo argumento es `RuntimeObject` |
| `attach` / `detach` | relation | attach/detach | - | verbos opuestos | si | no acepta target arbitrario |
| `attach_active` | relation | attach | active | sustantivo_estado | medio | alineado con la familia `*_active` |
| `carry` | relation | carry | - | verbo | si | efecto solo del frame actual |
| `reflect_x` / `reflect_y` | movement | reflect | x/y | verbo_eje | si | modifica angle, no posicion |
| `accelerate` | movement | accelerate | - | verbo | si | sobrecarga por argc |
| `rotate` | movement | rotate | - | verbo | si | claro |
| `position_origin` | movement/reset | position | origin | verbo_destino | si | usa origen declarado |
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
  `move_horizontal`.
- Otras invierten el orden: `fade_set`, `state_current`.
- Timer ya no usa el sustantivo `timer` como accion. La operacion se expresa
  con `play_timer`, `pause_timer` y `stop_timer`.
- `*_active` aparece en fade, music, state, timer y attach, con significado
  bastante estable: consulta booleana de estado activo.
- `fade_done` y `timer_done` comparten el significado de proceso terminado.
- `pause_music` alterna pausa/reanuda, pero el nombre solo expresa una mitad.
- `follow_x/follow_y` reciben target objeto igual que `carry(object, carrier)`.
- `position_origin` expresa una accion de posicionamiento hacia el origen
  declarado.
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

APIs que operan sobre el `RuntimeObject` recibido tras validar su referencia:

- `move_horizontal`, `move_vertical`, `advance`, `rotate`, `accelerate`,
  `reflect_x`, `reflect_y`, `position`, `position_x`, `position_y`,
  `position_origin`, `resize`, `resize_width`, `resize_height`,
  `apply_speed`, `apply_angle`, `apply_rotation_speed`, `apply_velocity`,
  `restore_speed`, `attach`, `detach`, `attach_active` y `carry`.

APIs que localizan objetos:

- `find_id(id)`.
- `find_name(name)`.
- `find_parent(object)`.
- `find_children(object)`.

APIs globales:

- `fade_*`, `music_*`, `delta`, `random`, `probability`, `exit`,
  `save/load`, `draw_*` e input funcional.

Conclusiones de caracterizacion:

- El desarrollador usa `object.id` de forma puntual en un ejemplo de resolucion
  mediante `find_id`.
- El motor si lo necesita internamente para reencontrar el `RuntimeObject`
  despues de recibir un objeto JS.
- Tecnicamente podria ocultarse detras de una representacion JS opaca en el
  futuro, pero hoy muchas bindings dependen de leer la propiedad publica `id`.
- Para obtener otras instancias hoy solo existen:
  - el segundo parametro de `collision`;
  - `find_id`, `find_name`, `find_parent`, `find_children`;
  - retorno indirecto de `spawn` no existe;
  - `ray` no devuelve objeto ni id.
- Las referencias de `RuntimeObject` no se deben almacenar entre hooks; se debe
  almacenar `object.id` como string y resolver mediante `find_id()`.

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

Conteo aproximado en los scripts versionados de `examples`:

| API | Usos |
| --- | ---: |
| `draw_text` | 52 |
| `spawn` | 43 |
| `kill` | 27 |
| `play_timer` | 22 |
| `random` | 20 |
| `advance` | 17 |
| `play_sound` | 15 |
| `timer_active` | 15 |
| `input_pressed` | 12 |
| `stop_timer` | 10 |
| `timer_left` | 8 |
| `rotate` | 7 |
| `probability` | 7 |
| `delta` | 6 |
| `fade_off` | 6 |
| `fade_set` | 6 |
| `move_horizontal` | 6 |
| `reflect_x` | 5 |
| `reflect_y` | 5 |
| `position` | 5 |
| `apply_speed` | 5 |
| `keep_only` | 4 |
| `input_down` | 4 |
| `state_to` | 6 |
| `position_x` | 4 |
| `position_y` | 4 |
| `resize` | 4 |
| `resize_width` | 4 |
| `apply_rotation_speed` | 4 |
| `follow_x` | 3 |
| `draw_line` | 3 |
| `apply_angle` | 3 |
| `apply_velocity` | 3 |
| `draw_pixel` | 2 |
| `fade_on` | 2 |
| `pause_music` | 2 |
| `accelerate` | 2 |
| `move_vertical` | 2 |
| `restore_speed` | 2 |
| `attach_active` | 2 |
| `follow_y`, `attach`, `detach`, `position_origin`, `fade_done`, `play_music` | 1 cada una |

Propiedades de objeto usadas en `examples/*.js`:

| Propiedad | Usos |
| --- | ---: |
| `group` | 20 |
| `x` | 29 |
| `y` | 17 |
| `width` | 11 |
| `name` | 5 |
| `speed` | 1 |
| `height` | 1 |
| `id` | 1 |

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

- `alive`
- `visible`
- `role`
- `angle`
- `rotationSpeed`
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
| `id/name/group/role/x/y/width/height/speed/angle/velocity*/rotationSpeed` | vista viva readonly | readonly | si | alineado tras refactor |
| `origin*/previous*/layer/motion.*` | no expuesto | no | no | alineado tras retirar restos documentales antiguos |
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
- `previousPosition`: util para `carry`, pero no expuesto como propiedad JS.
- `originSpeed`: soporte interno de `restore_speed`, no API publica.

## N. Familias que requieren mini-auditoria propia

- Input: quedan pendientes 8way, analog, pointer, text y remapeo avanzado.
- Movimiento: convivencia entre speed/angle, velocity, acceleration y metadata
  declarativa.
- Identidad/referencias: si `id` debe seguir llamandose asi y si se necesita
  una alternativa a busqueda global para obtener otras instancias.
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
5. Debe existir una forma publica de obtener la ultima instancia spawneada?
6. Como se normaliza la nomenclatura de consultas: `*_active`, `is_*`,
    `*_done`, `*_left`, `*_current`?
7. Debe exponerse en el futuro una API de remapeo o introspeccion de controles?
8. Debe existir un sistema de nombres de acciones por encima de los controles
    logicos numerados?

## Tests de caracterizacion anadidos

La suite `flx-runtime-lifecycle-tests` incorpora:

- `script module variables are shared between instances`: confirma que una
  variable `var shared` a nivel de script se comparte entre instancias del mismo
  modulo.
- `collision callback receives concrete other reference`: confirma que el
  segundo argumento de `collision(object, other)` representa la instancia
  concreta y que las operaciones funcionales aplican al `RuntimeObject`
  correspondiente.
- `public scripting functions are registered`: confirma que las funciones
  principales y `console.log` existen en el contexto JS.

Estas pruebas se suman a las de `docs/runtime-object-characterization.md` sobre
vista readonly viva, referencias temporales, identidad, metadata, `alive`,
`visible`, `attached` mediante API funcional y contexto final de `dead(object)`.
