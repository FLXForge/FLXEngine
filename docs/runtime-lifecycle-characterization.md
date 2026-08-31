# Contrato del ciclo de vida de RuntimeWorld

Este documento registra el contrato de `RuntimeWorld` despues de la
consolidacion del ciclo de vida. Ya no describe comportamiento accidental:
separa reglas consolidadas, reglas futuras deliberadamente pendientes y riesgos
ya eliminados.

## A. Reglas consolidadas

### Carga

- `RuntimeWorld::load()` obtiene el objeto raiz desde `ResourceRegistry`.
- La raiz y todos los hijos `spawn: "auto"` alcanzables se instancian durante la
  carga.
- `born(object)` se ejecuta exactamente una vez por instancia.
- Todo `spawn()` solicitado durante `born()` se vuelca y estabiliza antes de que
  `RuntimeWorld::load()` devuelva resultado.
- El `spawn()` recursivo durante `born()` tambien se estabiliza antes de que la
  carga termine.
- Existe un limite defensivo para proteger la carga de cadenas de spawn que no
  llegan a estabilizarse.
- Si el volcado de spawn durante carga supera ese limite, la carga falla y el
  loop de `Engine` no debe empezar.
- Los runtime ids son unicos por instancia.
- `parentId` se inicializa con el runtime id del parent original cuando un
  objeto nace como child. FLX v0.3.0 no implementa reparenting, asi que esta
  relacion permanece estable durante toda la vida de la instancia.

### Spawn Durante Update

- Un `spawn()` solicitado durante `action()` nace despues de la fase `action` y
  participa en las fases restantes del mismo frame, incluido `motion()`.
- Un `spawn()` solicitado durante `motion()` nace despues de la fase `motion`.
  No ejecuta `action()` ni `motion()` en ese mismo frame.
- Un `spawn()` solicitado durante `collision()` nace despues de la fase
  `collision`. No ejecuta `action()` ni `motion()` en ese mismo frame.
- Se conserva el orden de ciclo aprobado:

```text
beginFrame
action
flush spawn
motion
flush spawn
attachments
collision
flush spawn
time/timers
dead
cleanup
```

### Muerte

- `kill(object)` es terminal, explicito, idempotente y diferido.
- Los objetos muertos no se eliminan del vector de objetos vivos durante una
  iteracion activa.
- Una vez muerto, un objeto salta fases activas posteriores, recibe
  `dead(object)` exactamente una vez y se elimina fisicamente durante cleanup.
- `dead(object)` no puede resucitar un objeto.
- `keep_only(object)` usa la misma ruta de muerte que `kill(object)`.
- `keep_only(object)` conserva exactamente el runtime object pasado. No conserva
  hijos automaticamente.

### Alive

- `object.alive` es de solo lectura desde JavaScript.
- Escrituras directas como `object.alive = false` se ignoran.
- Los scripts deben usar `kill(object)` para solicitar muerte.

### Visible

- `object.visible` es de solo lectura desde JavaScript.
- Los scripts deben usar `show(object)` y `hide(object)` para cambiar
  visibilidad.
- `hide(object)` suprime el dibujo declarativo de la shape.
- `hide(object)` suprime el callback JavaScript `draw(object)`.
- Los objetos ocultos siguen ejecutando `action()`, `motion()`, collision,
  timers y state time.
- `show(object)` restaura el dibujo declarativo y `draw(object)`.

### Dibujo

- Los objetos visibles se dibujan en orden estable por `layer`.
- Los valores menores de `layer` se dibujan antes que los mayores.
- Los objetos con el mismo `layer` mantienen el orden runtime estable actual.
- Para cada objeto, primero se dibuja la shape declarativa y justo despues se
  ejecuta su callback JavaScript `draw(object)`.
- No existen dos pipelines separados del tipo "todas las shapes declarativas" y
  despues "todos los callbacks draw de objeto".
- El `layer` actual representa el orden relativo de objetos del mundo.
- Las futuras video layers introduciran un nivel superior de composicion por
  encima de este orden de dibujo por objeto.

### Ciclos De Instanciacion Automatica

- Los ciclos `auto -> auto` son invalidos y deben fallar.
- Los ciclos `manual -> manual` son validos porque no son ciclos de
  instanciacion automatica.
- Los grafos mixtos son validos cuando el ciclo queda cortado por una arista
  manual.
- Los ciclos de referencia de recursos y los ciclos de instanciacion automatica
  siguen siendo conceptos separados.
- `CompiledProjectValidator` es la capa principal de validacion de ciclos de
  instanciacion automatica.
- `RuntimeWorld` mantiene una proteccion defensiva para proyectos compilados
  invalidos.

### Errores De Script

- Las excepciones lanzadas por callbacks de script se registran mediante
  `Logger`.
- La ejecucion runtime continua despues de excepciones en `born`, `action`,
  `motion`, `collision`, `draw` y `dead`.
- Esta es la politica explicita actual. Los diagnostics runtime para fallos de
  script quedan pendientes para una auditoria futura.

### Tiempo

- State time y timers empiezan con sus valores de nacimiento.
- Durante un frame, los scripts observan state time y timers antes de que se
  aplique el incremento/decremento del frame.
- State time y timers se actualizan despues de collision y antes de `dead()`.
- Los objetos ocultos siguen actualizando timers y state time.

### Identidad Y Busqueda

- `findByRuntimeId()` devuelve un objeto vivo por runtime id exacto.
- `findByName()` devuelve el primer objeto vivo con ese nombre en el orden
  runtime actual.
- Los objetos pendientes de spawn no son visibles para busqueda hasta que se
  vuelca la cola de spawn.
- Los objetos muertos no se devuelven despues de cleanup.

## B. Reglas futuras deliberadamente pendientes

- Video layers: el `layer` actual solo es orden de dibujo por objeto. Un sistema
  futuro de video layers debe definir composicion por encima del dibujo de
  objetos.
- Attachments: el contrato actual confirma el timing aprobado: attachments se
  aplican despues de `motion()` y antes de collision, usando el parent original.
  Queda pendiente una auditoria mas amplia de attachments.
- States: las transiciones de estado existen, pero queda pendiente una auditoria
  completa de timing de entrada y diagnostics.
- Timers: los timers pertenecen al objeto y se actualizan dentro del ciclo, pero
  persistencia, diagnostics y semanticas avanzadas quedan pendientes.
- Collision semantics: este documento solo cubre cuando participa collision en
  el ciclo. Filtrado y politica de respuesta requieren auditoria propia.
- Runtime diagnostics: muchos fallos runtime y de script siguen registrandose
  como logs en lugar de diagnostics estructurados.
- Politica de errores de script: actualmente la ejecucion continua tras errores
  de script. Una decision futura podria introducir comportamiento estricto o
  configurable.
- Almacenamiento de RuntimeObject: el contenedor vivo sigue siendo un vector
  interno. El codigo externo no debe conservar punteros C++ a `RuntimeObject`
  entre operaciones que puedan hacer spawn o cleanup.
- Verificacion de dibujo: los tests cubren orden critico a nivel de pixel, pero
  no existe todavia una abstraccion de renderer para inspeccionar todos los
  comandos de dibujo.

## C. Riesgos ya eliminados

- El spawn durante `born()` ya no deja instancias pendientes despues de
  `RuntimeWorld::load()`.
- El spawn recursivo durante `born()` se estabiliza o falla con limite
  defensivo.
- `kill()` ya no permite resurreccion mediante `dead()`.
- `keep_only()` ya no usa un comportamiento de eliminacion separado.
- `object.alive` ya no es mutable desde JavaScript.
- `object.visible` ya no es mutable desde JavaScript.
- `hide()` ya no suprime solo parte del render; suprime shape declarativa y
  `draw(object)`.
- El dibujo de objetos ya no se comporta como dos pipelines de layer
  independientes.
- Los ciclos manuales de children ya no se clasifican como
  `AutomaticInstantiationCycle`.
- La deteccion de ciclos automaticos usa identidad de recurso, evitando falsos
  positivos por nombres logicos cortos repetidos en archivos distintos.
