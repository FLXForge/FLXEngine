# Caracterizacion del ciclo de vida de RuntimeWorld

Este documento caracteriza el ciclo de vida actual de `RuntimeWorld`. Es una fotografia del comportamiento existente, no un contrato de diseno definitivo.

## Comportamiento confirmado

- La carga inicial limpia objetos vivos y pendientes, reinicia los runtime ids a `1`, resuelve el root compilado desde `ResourceRegistry`, crea el root, crea recursivamente los hijos `auto` y despues llama a `born()` una vez por cada objeto creado.
- La creacion inicial de hijos `auto` es recursiva y usa el grafo compilado de `childResources`. El root nace antes que sus descendientes auto. En una cadena simple padre/hijo, el orden confirmado es root, parent, child.
- Los runtime ids se generan como `<object id>_<counter>`. `parentId` y `originalParentId` se inicializan con el runtime id del parent original.
- `update()` ejecuta actualmente las fases en este orden: `beginFrame`, `action`, flush de spawn, `motion`, flush de spawn, `attach`, `collision`, flush de spawn, tiempo/timers, `dead`, cleanup.
- Un `spawn()` solicitado durante el `born()` inicial queda pendiente tras la carga y se vuelca despues de la fase `action` del siguiente frame. El objeto nace una sola vez, no participa en ese `action` y si participa en `motion` durante ese frame.
- Un `spawn()` solicitado durante `action()` nace antes de `motion` y puede participar en el `motion` del mismo frame.
- Un `spawn()` solicitado durante `motion()` nace despues de `motion` y antes de attachments/collision. No participa en `action` ni en `motion` de ese frame.
- Un `spawn()` solicitado durante `collision()` nace despues de collision. No participa en `action` ni en `motion` de ese frame.
- El tiempo de estado y los timers se actualizan despues de collision y antes de `dead()`. Los scripts de action/motion/collision ven los valores anteriores de ese frame.
- `dead()` se llama una vez para objetos que no estan vivos y que aun no han ejecutado `dead()`. El cleanup elimina los objetos que siguen con `alive=false` despues de `dead()`.
- El comportamiento actual permite que `dead()` ponga `alive=true`; si lo hace, cleanup conserva el objeto.
- `keep_only(object)` marca como muertos todos los demas objetos vivos y conserva exactamente el runtime id solicitado. Los objetos eliminados por `keep_only` no pueden resucitar desde `dead()` porque `ScriptEngine::applyJsObject` fuerza de nuevo `alive=false` en los objetos no conservados.
- El dibujo declarativo se ordena de forma estable por `layer` antes de dibujar. Los callbacks JS `draw()` se ejecutan despues, en orden de insercion de objetos vivos, no por `layer`.
- `visible=false` evita el dibujo declarativo, pero no evita el callback JS `draw()`.
- Los attachments se aplican despues de `motion` y antes de `collision`. El `motion` del hijo ve la posicion anterior al attachment. `collision` ve la posicion posterior al attachment.
- En hijos creados con offset bajo un parent rotado, `originalOffset` se guarda desde la posicion real creada respecto al parent. Por tanto attach sigue el offset almacenado, no necesariamente el offset JSON sin rotar.
- Attachment sigue siempre al parent original. Si el parent no existe o esta muerto, el attachment se omite en ese frame.
- `findByRuntimeId()` busca objetos vivos por runtime id exacto. `findByName()` devuelve el primer objeto vivo con ese nombre en el orden del vector interno.
- Los objetos pendientes de spawn no son visibles para `findByName()` hasta que la cola de spawn se vuelca.
- No deben conservarse referencias o punteros C++ a `RuntimeObject` a traves de operaciones que puedan anadir o eliminar objetos, porque `RuntimeWorld` almacena las instancias vivas en un `std::vector`.
- Las excepciones de script en `born`, `action`, `motion`, `collision`, `draw` y `dead` se registran mediante `ScriptEngine`. Actualmente no hacen fallar la carga del runtime ni detienen el update/draw loop.

## Comportamiento ambiguo

- El orden de creacion entre hermanos depende de la iteracion de `unordered_map` en `childResources`. Los tests no lo fijan como contrato estable.
- El orden declarativo por `layer` esta confirmado por la estructura del codigo, pero los tests actuales no inspeccionan pixeles ni llamadas internas de Raylib.
- La participacion exacta en collision de un objeto nacido tras `motion()` depende de filtros de collision y del orden de objetos. La garantia caracterizada es que el objeto ya ha nacido antes de la fase collision.
- Los errores de script solo aparecen en Logger. No hay diagnostics estructurados de runtime para esas excepciones.

## Defectos o deuda observada

- `visible=false` no evita el callback `draw()`. Puede ser valido si `draw()` se considera una fase de overlay, pero es incoherente con la intuicion de un objeto invisible del mundo.
- `dead()` puede resucitar un objeto mediante `alive=true`. Si se considera intencional, deberia documentarse; si no, cleanup deberia ignorar esa mutacion en una futura politica de runtime.
- Los callbacks JS `draw()` ignoran `layer`, mientras las formas declarativas si lo usan. Esto crea dos modelos de orden de dibujo dentro del mismo mundo.
- `RuntimeWorld` no tiene proteccion explicita contra ciclos en la instanciacion recursiva de hijos `auto`. Un grafo compilado con `A -> B -> A` parece inseguro de ejecutar.
- Las excepciones de script se registran en logs, pero no se representan como diagnostics estructurados. Esto dificulta expresar una politica clara de fallo en CLI/runtime.
- `RuntimeObject*` se expone en varios callbacks del host aunque el contenedor interno puede realocarse. El codigo runtime debe evitar conservar esos punteros entre limites de spawn/cleanup.

## Casos no ejecutados por riesgo

- No se ejecuto el ciclo auto `A -> B -> A` porque la ruta recursiva actual no muestra un conjunto de visitados ni condicion determinista de parada. La suite contiene una fixture segura que documenta la forma del grafo sin cargarlo.
- No se ejecuto una comprobacion pixel a pixel ni de llamadas Raylib para el orden declarativo de dibujo porque `RuntimeWorld` dibuja directamente contra Raylib y no existe todavia una separacion de renderer orientada a test.
