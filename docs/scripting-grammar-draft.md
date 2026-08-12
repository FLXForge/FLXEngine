# FLX Script Grammar — Draft

## Estado

- Documento: borrador de diseño interno
- Ámbito: gramática y coherencia semántica de la API JavaScript de FLX
- Ubicación recomendada actual: `docs/`
- Estado: experimental / sujeto a auditoría
- No constituye todavía contrato público estable

## 1. Propósito

Este documento no define todavía una lista cerrada de funciones.

Define una gramática de diseño para que la API JavaScript de FLX tenga coherencia lingüística, semántica y estética.

La intención es que el código pueda leerse como comportamiento:

```javascript
kill(enemy);
spawn(ship, "laser");
move_x(ship, 10);
play_music(game, "theme");
state_to(ship, "moving");
group_has(enemy, "hostile");
```

La API no pretende imitar C++, Java ni la estructura declarativa JSON.

Pretende expresar acciones, estados, preguntas y relaciones de forma consistente.

## 2. Programación de comportamiento

La API de scripting de FLX puede describirse provisionalmente como una programación orientada al comportamiento.

La estructura conceptual es:

```text
función      → verbo o construcción verbal
objeto       → sujeto
argumentos   → complementos
```

Ejemplo:

```javascript
spawn(ship, "laser");
```

Lectura conceptual:

```text
spawn
→ verbo

ship
→ sujeto

laser
→ complemento directo dinámico
```

El objetivo no es crear frases gramaticalmente perfectas en inglés.

El objetivo es que toda la API responda a reglas internas previsibles.

## 3. Principio fundamental

No todas las funciones tienen que parecerse.

Todas deben poder explicarse por las mismas reglas.

Por tanto, FLX no impone una única forma universal.

Reconoce varias construcciones semánticas.

## 4. Acción directa

Forma general:

```text
VERBO[_CC][_CD](sujeto [, complementos])
```

Donde:

- `VERBO` expresa la acción;
- `CC` representa un complemento circunstancial lexicalizado;
- `CD` representa un complemento directo lexicalizado;
- el primer argumento suele representar el sujeto;
- los argumentos posteriores contienen complementos dinámicos.

Ejemplos:

```javascript
kill(enemy);
spawn(ship, "laser");
move_x(ship, 10);
play_music(game, "theme");
draw_pixel(x, y, color);
```

## 5. Complemento fijo y complemento dinámico

Un complemento puede formar parte del nombre de la función cuando define permanentemente la especialización de la operación.

Ejemplo:

```text
draw_pixel
draw_line
draw_text
```

`pixel`, `line` y `text` son complementos directos fijos.

Por el contrario:

```javascript
spawn(ship, "laser");
```

no debe transformarse en `spawn_laser`, porque `laser` es información variable de esa llamada.

Regla:

> Un complemento fijo puede formar parte del nombre. Un complemento dinámico pertenece a los argumentos.

## 6. Complementos circunstanciales

Algunos complementos especializan la acción por lugar, eje, dirección, modo u otra circunstancia.

Ejemplos espaciales:

```text
move_x
move_y
follow_x
follow_y
bounce_x
bounce_y
```

`_x` y `_y` no constituyen sufijos universales.

Solo tienen sentido en operaciones cuya naturaleza admite un eje.

## 7. Transición

Algunas operaciones expresan un cambio hacia un destino.

Forma conceptual:

```text
CONCEPTO_to(sujeto, destino [, complementos])
```

Ejemplos potenciales:

```javascript
state_to(ship, "moving");
fade_to(...);
```

`_to` significa destino o transición.

No significa asignación genérica.

## 8. Predicados

Una consulta booleana puede expresarse mediante:

```text
CONCEPTO_PREDICADO(...)
```

Ejemplos:

```text
fade_active
state_active
group_has
role_is
```

El predicado debe poseer un significado estable en toda la API.

## 9. Eventos

Algunas consultas no preguntan por un estado continuo, sino por un hecho ocurrido.

Ejemplos:

```text
fade_done
state_entered
button_pressed
```

`_done` significa proceso terminado.

`_entered` significa entrada en un estado.

`_pressed` significa flanco de pulsación.

## 10. Proyecciones de valor

Algunas funciones no ordenan una acción ni preguntan un booleano.

Proyectan un valor perteneciente a una familia conceptual.

Forma:

```text
CONCEPTO_VALOR(...)
```

Ejemplos:

```text
state_current
state_time
timer_left
fade_alpha
```

No necesitan introducir artificialmente verbos `get_*`.

## 11. Capacidades activables

`_on` y `_off` se reservan para capacidades que pueden activarse o desactivarse.

Ejemplo:

```text
fade_on
fade_off
```

No equivalen genéricamente a `true` y `false`.

No deben usarse para cualquier condición binaria.

Cuando existen verbos naturales mejores, deben preferirse:

```text
show / hide
attach / detach
play / stop
```

## 12. Vocabulario transversal inicial

### `_to`
Destino o transición.

### `_on`
Activar una capacidad.

### `_off`
Desactivar una capacidad.

### `_active`
La capacidad o proceso está activo actualmente.

### `_done`
El proceso ha finalizado.

### `_entered`
Se produjo la entrada en un estado.

### `_pressed`
Se produjo el evento de pulsación.

### `_has`
Pertenencia, posesión o inclusión.

### `_is`
Clasificación o identidad semántica.

## 13. Términos no transversales

Elementos como:

```text
_x
_y
_origin
_music
_sound
_pixel
_line
_text
```

no pertenecen automáticamente al vocabulario transversal.

Pueden representar complemento circunstancial, directo, espacial, de medio o de especialización del verbo.

## 14. Sobre `_set`

`_set` no se adopta como verbo universal.

Su uso puede introducir una estética getter/setter ajena al lenguaje que FLX intenta construir.

Antes de utilizar `concept_set` debe buscarse la semántica real de la operación.

Puede tratarse de una transición, un inicio, una activación o una acción específica del dominio.

No se prohíbe `_set`, pero no debe utilizarse por defecto.

## 15. Sobre `_has` e `_is`

`_has` e `_is` pueden actuar como predicados del lenguaje.

La diferencia provisional es:

```text
_has → pertenencia / posesión / inclusión
_is  → clasificación / identidad semántica
```

La auditoría de `group` y `role` deberá validarlo.

## 16. `state_to`

La API consolidada:

```javascript
state_to(object, "moving");
```

expresa una transicion hacia un destino y sustituye a la forma antigua basada
en `state`.

Las consultas pueden permanecer agrupadas:

```text
state_current
state_active
state_entered
state_time
```

Este renombre queda cerrado: no se registra alias heredado.

## 17. Timer

La API consolidada de Timer:

```javascript
play_timer(object, "reload", 1);
pause_timer(object, "reload");
stop_timer(object, "reload");
```

valida `play`, `pause` y `stop` como vocabulario de control de proceso.

Timer representa una capacidad que puede ejecutarse, suspenderse, terminar
voluntariamente y terminar naturalmente.

Por eso combina acciones verbales:

```text
play_timer
pause_timer
stop_timer
```

con consultas agrupadas por concepto:

```text
timer_active
timer_paused
timer_done
timer_left
```

`play_timer(object, name, duration)` no significa reinicio automatico. Cambia
la duracion total conservando el tiempo ya transcurrido. Un reinicio completo
se expresa con:

```javascript
stop_timer(object, "reload");
play_timer(object, "reload", 1);
```

Esta consolidacion no implica que toda capacidad futura deba soportar siempre
`play`, `pause` y `stop`; solo confirma que Timer encaja con esa familia.

## 18. Audio

La familia actual:

```text
play_music
stop_music
pause_music
play_sound
```

responde correctamente a `VERBO + complemento directo fijo`.

No es obligatorio invertirla a `music_play` solo para mejorar la indexación.

La coherencia semántica prevalece sobre el orden alfabético.

## 19. `pause_music`

Una función llamada `pause_music` debe pausar.

Si también reanuda, existe una incoherencia semántica.

La auditoría de Audio deberá decidir entre construcciones como:

```text
pause_music
resume_music
```

o una operación explícitamente de alternancia.

## 20. `to_origin`

`to_origin(object)` carece de verbo explícito.

`origin` representa un destino o referencia espacial, no una acción.

La API deberá auditar qué operación real expresa.

Posibles formas:

```text
move_origin(object)
move(object, origin)
```

sin decidir todavía cuál es correcta.

## 21. Follow

`follow_x` y `follow_y` poseen una estructura lingüística razonable: verbo + complemento espacial.

El problema actual no es necesariamente el nombre, sino la forma de identificar el objetivo.

Una futura forma como:

```javascript
follow_x(object, target);
```

sería coherente si `target` es una referencia semántica real y no un nombre global ambiguo.

## 22. Referencias e identidad

La identidad interna de C++ no forma parte automática del lenguaje del desarrollador.

El desarrollador trabaja con referencias a instancias:

```javascript
function action(ship) {
    kill(ship);
}
```

`ship` representa la instancia concreta.

No debería necesitar conocer `runtimeId` salvo necesidad real.

Principio:

> El contexto y las referencias semánticas tienen prioridad sobre la identidad técnica explícita.

## 23. Contexto del comportamiento

Un script pertenece al comportamiento de una definición.

Las variables de módulo pueden ser compartidas por todas las instancias que utilizan ese script.

Los parámetros de callback representan la instancia concreta de esa invocación.

El script no debe tratarse automáticamente como un observador global omnisciente del mundo.

## 24. Objeto JavaScript

El objeto que recibe JavaScript no es:

- un espejo de `RuntimeObject` C++;
- un espejo de `ObjectDefinition`;
- una reproducción del JSON.

Es un contrato específico del lenguaje de comportamiento.

Las propiedades directas deben representar estado local que tenga sentido consultar y modificar directamente.

Las relaciones con el mundo deben tender a operaciones del Runtime.

## 25. JSON y JavaScript

La estructura JSON organiza declaraciones.

La API JavaScript organiza comportamiento.

Por tanto:

```json
{
  "motion": {
    "angle": 90
  }
}
```

puede ser correcto declarativamente.

Pero durante ejecución:

```javascript
object.angle += 5;
```

puede ser la forma correcta de comportamiento.

No debe trasladarse automáticamente `object.motion.angle` solo porque exista esa estructura en JSON.

## 26. Group y Role

`group` y `role` son candidatos prioritarios para probar la gramática.

Posibles construcciones conceptuales:

```text
group_has(object, "enemy")
role_is(object, "player")
```

No están cerradas.

Antes debe decidirse si group puede ser múltiple, si role es singular, si pueden cambiar y qué subsistemas los consumen.

## 27. Principios de diseño de nombres

1. La semántica prevalece sobre la indexación alfabética.
2. Una función debe describir la operación real.
3. No se fuerza una forma única para acciones, predicados y valores.
4. Los complementos fijos pueden lexicalizarse en el nombre.
5. Los complementos dinámicos pertenecen a los argumentos.
6. Los complementos circunstanciales solo existen donde tienen sentido.
7. Un sufijo transversal debe conservar siempre el mismo significado.
8. No se introducen sinónimos arbitrarios.
9. No se adoptan patrones getter/setter por defecto.
10. La API debe poder leerse como comportamiento.

## 28. Formas gramaticales provisionales

### Acción

```text
VERBO[_CC][_CD](sujeto [, complementos])
```

### Transición

```text
CONCEPTO_to(sujeto, destino [, complementos])
```

### Predicado

```text
CONCEPTO_PREDICADO(...)
```

### Evento

```text
CONCEPTO_EVENTO(...)
```

### Proyección de valor

```text
CONCEPTO_VALOR(...)
```

### Capacidad activable

```text
CONCEPTO_on(...)
CONCEPTO_off(...)
```

Estas formas son herramientas de análisis, no una especificación cerrada.

## 29. Criterio de validación futura

Una función nueva debe responder:

1. ¿Qué acción, pregunta o valor representa?
2. ¿Cuál es el verbo o concepto principal?
3. ¿Qué complementos son fijos?
4. ¿Qué complementos son dinámicos?
5. ¿Pertenece a una familia existente?
6. ¿Utiliza vocabulario con significado definido?
7. ¿Se puede leer con naturalidad dentro de FLX?
8. ¿Permite inferir otras funciones relacionadas?
9. ¿Expone una capacidad real o una implementación accidental?

## 30. Auditorías que deben poner a prueba esta gramática

El borrador debe contrastarse al menos con:

```text
State
Timer
Motion
Follow
Group / Role
References
Audio / Music
Input
Drawing
Collision
Spawn
Attachments
```

Cada auditoría puede confirmar, ampliar o corregir la gramática.

## 31. Principio final

FLX no busca una API uniforme por apariencia.

Busca un lenguaje coherente por significado.

La meta es que el desarrollador no memorice llamadas aisladas.

La meta es que aprenda cómo se construyen las frases.

Cuando las reglas estén suficientemente maduras, nuevas expresiones deberían poder deducirse antes incluso de consultar la documentación.

Ese será el punto en el que la API deje de ser una colección de funciones y empiece a comportarse como un idioma.
