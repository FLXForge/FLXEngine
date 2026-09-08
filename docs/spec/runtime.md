# Runtime

## Estado
- Documento: especificación normativa
- Ámbito: mundo de ejecución e instancias vivas
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito
`RuntimeWorld` transforma las definiciones de un `CompiledProject` en instancias vivas y gobierna su ciclo de vida dentro del mundo.

```text
ResourceRegistry → identidad de recurso
RuntimeWorld     → identidad de instancia
RuntimeObject    → estado vivo
```

Runtime no vuelve a `.flx`, JSON, YAML ni otras fuentes del proyecto.

## 2. Responsabilidades
RuntimeWorld construye el mundo inicial, crea instancias, asigna identidades runtime, establece relaciones, ejecuta `born()`, coordina las fases del frame, integra `spawn()`, gestiona muerte y cleanup, visibilidad y orden visual, y mantiene las invariantes del mundo vivo.

No resuelve rutas fuente, no compila recursos y no utiliza `name` como identidad global.

## 3. Identidad
`ResourceId` identifica una definición compilada. `runtimeId` identifica una instancia viva concreta. Una definición puede producir múltiples instancias.

`name` no es identidad global. Los punteros/referencias C++ a `RuntimeObject` tampoco son identidad persistente: operaciones estructurales pueden reorganizar el contenedor. La identidad estable es `runtimeId`.

## 4. Carga
Conceptualmente:

```text
validar CompiledProject
→ localizar root
→ crear raíz y descendencia auto
→ cargar scripts
→ born()
→ integrar spawn de born()
→ born() de nuevas instancias
→ repetir hasta estabilización
→ mundo listo
```

Si la construcción falla, `load()` devuelve fallo y Engine no entra en el loop.

## 5. Instanciación automática
Los ciclos de instanciación automática son inválidos:

```text
A --auto--> B
B --auto--> A
```

Los ciclos manuales pueden ser válidos:

```text
A --manual--> B
B --manual--> A
```

La validación principal pertenece al proyecto compilado y Runtime mantiene protección defensiva.

## 6. born()
`born()` se ejecuta exactamente una vez por instancia. Forma parte de su construcción. Los `spawn()` producidos durante `born()` se estabilizan antes de que `load()` termine, con límites defensivos frente a crecimiento infinito.

## 7. Ciclo del frame
Orden consolidado:

```text
action
→ integrar spawn
→ motion
→ integrar spawn
→ attachments
→ collision
→ integrar spawn
→ stateTime / timers
→ dead
→ cleanup
```

Las fronteras de integración son parte del contrato.

## 8. Spawn
- Desde `action`: la instancia participa en las fases restantes.
- Desde `motion`: entra tras motion y puede participar en fases posteriores.
- Desde `collision`: entra cuando collision ya terminó.
- Desde `born`: se integra durante la construcción antes de terminar `load()`.

Las mutaciones estructurales se realizan en fronteras seguras, no modificando directamente el contenedor durante una iteración activa.

## 9. Estado intrínseco y correlacional
FLX distingue:

```text
Estado intrínseco → propiedades
Relación con el mundo → operaciones Runtime
```

Propiedades locales como posición, ángulo o velocidad pueden exponerse para modificación cuando corresponda.

Vida/muerte, visibilidad, relaciones, attachments o participación en subsistemas son estado correlacional y deben gestionarse mediante operaciones del Runtime.

Principio: **el script puede modificar estado local; el Runtime gobierna las relaciones con el mundo.**

## 9.1. Movimiento vivo
`speed`, `velocity` y `angle` no representan el mismo concepto:

```text
speed    → magnitud escalar viva
velocity → vector lineal vivo
angle    → orientación viva
```

En mecánicas `direct`, la trayectoria lineal reside en `velocity`. `rotate()` modifica la orientación visual y no debe redirigir esa trayectoria.

En mecánicas `polar`, `advance()` usa `angle` y `speed` como movimiento polar propio. Cualquier `velocity` heredada o acumulada se suma como contribución lineal separada.

Las propiedades comunes de `mechanics.motion` se aplican a ambos ejes. `motion.horizontal` y `motion.vertical` son overrides parciales: si un eje no declara un campo concreto, conserva el valor común de `motion`.

## 9.2. Tipología emergente de RuntimeObject
FLX no exige una tipología explícita para clasificar RuntimeObjects.

Todo RuntimeObject comparte el mismo modelo de definición y las mismas capacidades declarativas disponibles. Su papel dentro del mundo emerge de las propiedades, relaciones y capacidades que declara.

Un RuntimeObject puede actuar, por ejemplo, como nodo estructural, objeto visible, elemento de UI, componente de otro objeto o entidad interactiva sin cambiar de tipo ni utilizar un esquema de definición distinto.

Propiedades como `component`, `children`, capacidades visuales, scripting, mechanics o collision modifican su comportamiento y sus relaciones, pero no crean clases públicas distintas de RuntimeObject.

## 10. Muerte
La operación es conceptualmente `kill(instance)` y actúa sobre identidad runtime, no sobre un nombre global.

```text
kill()
→ alive = false inmediatamente
→ deja de participar en cualquier procesamiento activo posterior,
  incluso dentro de la misma fase y frame
→ dead()
→ cleanup
→ eliminación
```

La muerte es terminal. `dead()` se ejecuta exactamente una vez y no puede resucitar la instancia.

`alive` es información de solo lectura desde scripting. No son válidas asignaciones como `object.alive = false` o `object.alive = true`.

`keep_only()` utiliza el mismo mecanismo interno de muerte que `kill()`.

## 11. Visibilidad
La visibilidad se gestiona mediante:

```text
show(instance)
hide(instance)
```

`visible` es de solo lectura desde scripting.

Una instancia oculta sigue viva y continúa con action, motion, collision, attachments, stateTime y timers, pero no produce salida visual: ni dibujo declarativo ni callback JS `draw()`.

`show()` restaura su participación visual.

## 12. Dibujo
En 0.3.0:

```text
orden estable por object.layer
→ para cada objeto visible
   → dibujo declarativo
   → draw() JS
```

Para igual `layer` se conserva el orden de inserción.

`object.layer` no es un orden global definitivo. Es el orden relativo del objeto dentro de su futura capa de vídeo:

```text
video layer
→ object.layer
→ orden estable
```

Las video layers no forman parte de 0.3.0.

## 13. Tiempo, attachments y collision
`stateTime` y timers avanzan después de action, motion, attachments y collision. Se conserva esta política en 0.3.0.

Attachments mantienen el orden `motion → attachments → collision`; su semántica detallada pertenece a su especificación correspondiente.

Collision se ejecuta después de motion y attachments. Su contrato normativo, incluidos colliders, contactos, activación, mutaciones durante la fase, Ray y debug, se define en:

```text
docs/spec/scripting/collision.md
```

La visibilidad no altera la participación en Collision.

## 14. Búsqueda
`findByRuntimeId()` representa una búsqueda exacta. Las búsquedas por nombre son utilidades contextuales, no identidad global.

## 15. Relación con ScriptEngine
La frontera es:

```text
Script
→ solicita operación
→ RuntimeWorld
→ modifica el mundo
```

No:

```text
Script
→ modifica arbitrariamente estructura interna del Runtime
```

## 16. Errores de scripting
En 0.3.0 una excepción en `born`, `action`, `motion`, `collision`, `draw` o `dead` se registra y no detiene automáticamente el juego.

La integración formal de incidencias runtime recuperables con Diagnostics queda pendiente de la auditoría de scripting.

## 17. Relación con Engine
Engine aloja la ejecución; RuntimeWorld gobierna el mundo vivo.

```text
CompiledProject
→ Engine
→ RuntimeWorld
→ RuntimeObject
```

## 18. Reglas deliberadamente pendientes
No constituyen por sí mismas deuda accidental:
- video layers;
- semántica completa de attachments;
- Runtime Diagnostics recuperables;
- audio;
- operaciones adicionales sobre hijos;
- modularización interna de RuntimeWorld.

No deben elevarse a contrato definitivo sin su auditoría específica.

## 19. Riesgos eliminados en 0.3.0
- spawn de `born()` pendiente hasta el primer update;
- resurrección desde `dead()`;
- escritura JS directa de `alive`;
- escritura JS directa de `visible`;
- muerte especial de `keep_only`;
- visibilidad aplicada solo al dibujo declarativo;
- `draw()` JS fuera del orden del objeto;
- confusión entre ciclos de referencia y ciclos de instanciación automática;
- uso de `name` como identidad precisa;
- tipología pública rígida de RuntimeObject;
- semántica diferida de `alive` tras `kill()`.

## 20. Invariantes
- RT-001 Cada instancia posee `runtimeId` único.
- RT-002 `ResourceId` identifica definición; `runtimeId`, instancia.
- RT-003 `name` no es identidad global.
- RT-004 `born()` se ejecuta exactamente una vez.
- RT-005 Al terminar `load()` no quedan nacimientos iniciales pendientes.
- RT-006 Las mutaciones estructurales se integran en fronteras seguras.
- RT-007 El orden de integración de spawn forma parte del contrato.
- RT-008 La muerte es terminal.
- RT-009 `dead()` se ejecuta exactamente una vez.
- RT-010 `alive` no es escribible desde scripting.
- RT-011 `kill()` actúa sobre identidad de instancia.
- RT-012 `keep_only()` comparte semántica de muerte.
- RT-013 `visible` no es escribible directamente desde scripting.
- RT-014 `hide()` no altera fases no visuales.
- RT-015 Una instancia oculta no produce salida visual.
- RT-016 Dibujo declarativo y `draw()` JS comparten el orden relativo del objeto.
- RT-017 `object.layer` es relativo, no una video layer global.
- RT-018 Los ciclos de instanciación automática son inválidos.
- RT-019 Los ciclos manuales no son por sí mismos ciclos de instanciación.
- RT-020 Los errores JS recuperables no detienen automáticamente Runtime.
- RT-021 Los punteros C++ a RuntimeObject no son identidad persistente.
- RT-022 Runtime no vuelve a las fuentes.
- RT-023 La tipología funcional de un RuntimeObject emerge de su configuración y no de un tipo estructural declarado.
- RT-024 `kill()` establece `alive = false` inmediatamente y excluye la instancia de toda participación posterior.

## 21. Tests mínimos
El contrato debe proteger carga/nacimiento, spawn por fase, muerte terminal e inmediata, `keep_only`, visibilidad, orden de dibujo, ciclos auto/manual, identidad runtime, continuidad ante errores recuperables de scripting y la frontera con Collision.

## 22. Principio final
RuntimeWorld no representa el proyecto. Representa el mundo vivo que existe cuando ese proyecto se ejecuta.

```text
Definición compilada
→ instanciación
→ RuntimeObject
→ nacimiento / vida / relaciones
→ muerte
→ cleanup
```

El script transforma estado local. Las operaciones del Runtime transforman su relación con el mundo.
