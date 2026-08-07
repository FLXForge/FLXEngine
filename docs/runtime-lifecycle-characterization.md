# Caracterizacion del ciclo de vida de RuntimeWorld

Este documento resume la caracterizacion realizada sobre `RuntimeWorld` y el
contrato consolidado despues de corregir los puntos inseguros detectados.

## Contrato consolidado

- La carga inicial resuelve el root desde `ResourceRegistry`, crea el root,
  crea recursivamente los hijos `auto` y llama a `born()` exactamente una vez
  por instancia.
- Los `spawn()` solicitados durante `born()` se estabilizan antes de que
  `RuntimeWorld::load()` devuelva exito. La cola se vacia hasta quedar estable.
- Si la estabilizacion de `spawn()` durante carga supera el limite defensivo, la
  carga falla con un diagnostic estable y el loop de `Engine` no empieza.
- Los ciclos de instanciacion automatica se validan en `CompiledProjectValidator`.
  `RuntimeWorld` mantiene una defensa adicional para proyectos compilados
  invalidos.
- `kill(object)` solicita muerte de forma explicita, segura, idempotente y
  diferida. No elimina objetos del vector durante una iteracion.
- La muerte es terminal: el objeto saltara fases activas posteriores,
  ejecutara `dead(object)` una sola vez y sera eliminado en cleanup. `dead()` no
  puede resucitarlo.
- `object.alive` es de solo lectura para JavaScript. Escribir
  `object.alive = false` no mata el objeto.
- `keep_only(object)` usa el mismo mecanismo de muerte que `kill(object)` y
  conserva exactamente el runtime id indicado. No conserva hijos
  automaticamente.
- `hide(object)` y `show(object)` controlan visibilidad sin alterar vida,
  colision, motion, action, timers ni state time.
- Un objeto oculto no dibuja su shape declarativa ni ejecuta su callback
  JavaScript `draw(object)`.
- `object.visible` es de solo lectura para JavaScript. La API publica para
  cambiar visibilidad es `hide(object)` / `show(object)`.
- El dibujo de objetos visibles se ordena de forma estable por `layer`. Para
  cada objeto se dibuja primero la shape declarativa y despues su callback
  JavaScript `draw(object)`.
- `layer` solo expresa orden relativo entre objetos del mundo en el renderer
  actual. No es un sistema global definitivo de capas de video.

## Comportamiento mantenido

- `update()` conserva el orden general: `beginFrame`, `action`, flush de spawn,
  `motion`, flush de spawn, `attach`, `collision`, flush de spawn,
  tiempo/timers, `dead`, cleanup.
- Un `spawn()` solicitado durante `action()` nace antes de `motion` y puede
  participar en el `motion` del mismo frame.
- Un `spawn()` solicitado durante `motion()` nace despues de `motion` y antes de
  attachments/collision. No participa en `action` ni en `motion` de ese frame.
- Un `spawn()` solicitado durante `collision()` nace despues de collision. No
  participa en `action` ni en `motion` de ese frame.
- Los runtime ids siguen el formato `<object id>_<counter>`.
- `parentId` y `originalParentId` se inicializan con el runtime id del parent
  original.
- Los attachments se aplican despues de `motion` y antes de `collision`.
- Attachment sigue siempre al parent original. Si el parent no existe o esta
  muerto, el seguimiento se omite en ese frame.
- `findByRuntimeId()` busca objetos vivos por runtime id exacto.
- `findByName()` devuelve el primer objeto vivo con ese nombre en el orden
  interno actual.
- Los objetos pendientes de spawn no son visibles para busqueda hasta que la
  cola de spawn se vuelca.
- Las excepciones de script en `born`, `action`, `motion`, `collision`, `draw`
  y `dead` se registran y el runtime continua.

## Deuda pendiente

- El orden de creacion entre hermanos sigue dependiendo del orden de
  `childResources`. No se fija como contrato estable.
- Las excepciones de script aun no se representan como diagnostics
  estructurados de runtime.
- `RuntimeWorld` almacena instancias vivas en un `std::vector`; no deben
  conservarse punteros C++ a `RuntimeObject` entre operaciones que puedan hacer
  spawn o cleanup.
- No existe todavia un renderer separable para verificar orden visual mediante
  llamadas capturables o pixeles en tests unitarios puros.
