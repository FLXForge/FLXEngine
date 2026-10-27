# FLX Runtime Session Specification

**Versión objetivo:** FLX v0.4.0  
**Estado:** Propuesta normativa  
**Ámbito:** Modularización del runtime y ejecución integrada  
**Documento:** `spec/runtime-session.md`

---

## 1. Propósito

Esta especificación define la separación entre el motor de ejecución FLX y la aplicación anfitriona que lo utiliza.

FLX debe poder ejecutarse de dos formas:

- **Standalone:** mediante `flx.exe`, conservando el comportamiento actual del CLI.
- **Embedded:** como biblioteca C++ integrada en una aplicación que proporciona sus propios dispositivos y controla su ciclo de ejecución.

Ambas modalidades deben utilizar el mismo runtime y respetar las mismas reglas declarativas.

El objetivo principal es permitir la ejecución secuencial de distintos proyectos FLX dentro de un mismo proceso, sin necesidad de destruir y recrear la ventana ni el dispositivo físico de audio.

Esta especificación no introduce un nuevo modelo de mundo, nuevas capacidades de Machine ni cambios en el formato compilado.

## 2. Principios arquitectónicos

### RS-001 — Separación entre host y sesión

El host y la sesión tienen responsabilidades diferentes.

El host es responsable de:

- Crear y destruir la ventana y el contexto gráfico.
- Inicializar y cerrar el dispositivo físico de audio.
- Gestionar el bucle principal y proporcionar el tiempo transcurrido.
- Determinar cuándo se carga, ejecuta y descarga una sesión.
- Gestionar el cierre de la aplicación.
- Presentar el resultado gráfico de FLX en su propia superficie.
- Configurar los servicios globales de plataforma, incluidos los de logging.

La sesión FLX es responsable de:

- Consumir un `CompiledProject` válido.
- Crear y mantener el mundo de ejecución.
- Gestionar su contexto JavaScript.
- Aplicar las restricciones declaradas por Machine.
- Gestionar el estado lógico de input.
- Gestionar los recursos de audio propios del proyecto.
- Actualizar y renderizar el mundo.
- Informar de su estado y motivo de finalización.
- Liberar todos los recursos que haya adquirido.

Una sesión no debe destruir dispositivos físicos propiedad del host.

### RS-002 — Unidad de ejecución

`FlxSession` representa una ejecución independiente de un único `CompiledProject`.

Cada sesión dispone de su propio estado lógico y contexto de ejecución.

No se admite cargar otro proyecto sobre una sesión activa sin descargar previamente la ejecución anterior.

La implementación puede reutilizar una instancia de `FlxSession` después de `unload()`, siempre que el nuevo proyecto comience sin estado residual.

### RS-003 — Biblioteca reutilizable

El runtime y sus subsistemas deben poder compilarse como una biblioteca estática C++ denominada `flx-core`.

La biblioteca debe permitir que otros ejecutables utilicen FLX sin depender del punto de entrada del CLI.

No se requiere una ABI estable, una biblioteca dinámica ni compatibilidad binaria entre versiones.

La biblioteca puede depender internamente de raylib, QuickJS y las demás dependencias existentes.

No deben introducirse capas de abstracción de plataforma sin una necesidad funcional demostrada.

## 3. Proyecto compilado

### RS-004 — Fuente de ejecución

La sesión recibe un `CompiledProject` previamente construido y validado.

La sesión no debe necesitar acceder a los archivos originales `.flx`, YAML, JSON, JavaScript o `.input`.

Se mantienen las invariantes definidas en `spec/compiled-project.md`.

La carga desde fuentes y la carga desde `.flxc` deben converger en la misma representación compilada.

### RS-005 — Propiedad del proyecto

Los datos del proyecto deben permanecer válidos durante toda la vida de la sesión.

La sesión debe poseer el proyecto o mantener una propiedad compartida explícita que garantice su supervivencia.

No se permiten referencias o punteros a datos de un proyecto cuya vida no esté garantizada.

La forma concreta de transferencia de propiedad se determinará durante la implementación, sin modificar el contrato de `CompiledProject`.

### RS-006 — Compatibilidad del formato

La modularización no modifica el formato binario `.flxc`, su versión ni sus reglas de validación.

Tampoco modifica la semántica de Machine, objetos, recursos, scripts o input mapping.

## 4. Ciclo de vida

### RS-007 — Estados

La sesión debe distinguir, como mínimo, los siguientes estados conceptuales:

| Estado | Significado |
|---|---|
| `Empty` | No existe un proyecto cargado ni recursos activos de sesión |
| `Ready` | Proyecto cargado y preparado para ejecutar |
| `Running` | Sesión en ejecución |
| `Finished` | Ejecución finalizada correctamente o por solicitud explícita |
| `Failed` | Carga o ejecución terminada por error |

La implementación puede utilizar nombres o estructuras equivalentes, siempre que conserve estas distinciones.

### RS-008 — Transiciones

Las transiciones admitidas son:

- `Empty → Ready`: carga correcta.
- `Empty → Failed`: carga fallida.
- `Ready → Running`: comienzo de ejecución.
- `Running → Finished`: finalización normal o solicitada.
- `Ready/Running → Failed`: error de ejecución.
- `Ready/Running/Finished/Failed → Empty`: descarga explícita.

No se admite ejecutar ni renderizar un proyecto inexistente.

Una sesión terminada no debe volver a ejecutarse sin una nueva carga.

### RS-009 — Descarga

`unload()` debe liberar todos los recursos propios de la sesión.

La operación debe ser segura después de una carga parcial o fallida.

Las llamadas repetidas a `unload()` no deben provocar errores ni dobles liberaciones.

La descarga no debe cerrar la ventana ni el dispositivo físico de audio del host.

### RS-010 — Destrucción segura

Los recursos deben destruirse antes que los servicios de los que dependen.

En particular:

- Los recursos gráficos deben liberarse mientras exista el contexto gráfico.
- Los streams y sonidos deben liberarse mientras exista el dispositivo de audio.
- Las referencias JavaScript a objetos deben invalidarse de forma segura.
- Los callbacks no deben acceder a objetos ya destruidos.
- Las referencias a recursos del proyecto no deben sobrevivir a su propietario.

La implementación debe establecer y verificar explícitamente el orden de destrucción.

### RS-011 — Finalización diferida

Una solicitud de finalización realizada durante un callback JavaScript no debe destruir inmediatamente el contexto desde el que se está ejecutando.

La sesión debe registrar la solicitud y completar la finalización cuando el control regrese a un punto seguro del ciclo de ejecución.

## 5. Actualización y renderizado

### RS-012 — Control del tiempo

El host proporciona el tiempo transcurrido entre actualizaciones.

La sesión no debe depender de un bucle principal propio ni consultar obligatoriamente `GetFrameTime()`.

La ejecución standalone debe conservar las reglas temporales existentes, incluida cualquier limitación del delta aplicada actualmente por `Engine`.

### RS-013 — Renderizado lógico

La sesión debe poder generar su salida gráfica sin iniciar ni finalizar el frame físico del host.

El renderizado lógico debe respetar:

- La resolución definida por Machine.
- Las restricciones de color y representación.
- Las capas y el orden de dibujo.
- Los efectos y transiciones existentes.
- Los filtros de salida configurados.

El host controla la presentación final, su posición, escala y composición con otras superficies.

### RS-014 — Superficie de salida

La sesión debe proporcionar acceso controlado a su resultado gráfico.

El host podrá utilizarlo mientras la sesión esté cargada y el contexto gráfico permanezca válido.

La interfaz inicial puede utilizar tipos de raylib.

No se requiere una API gráfica independiente de raylib.

La sesión no debe asumir que ocupa toda la ventana del host.

## 6. Audio

### RS-015 — Dispositivo físico

El host es propietario del dispositivo físico de audio.

La sesión puede utilizarlo, pero no debe inicializarlo ni cerrarlo cuando se ejecute en modo integrado.

La ejecución standalone seguirá inicializando y cerrando el dispositivo mediante su host correspondiente.

### RS-016 — Recursos lógicos

Cada sesión es propietaria de sus sonidos, música, streams, voces y estados de reproducción.

La descarga debe detener y liberar todos esos recursos sin afectar al dispositivo físico.

No debe quedar reproducción residual de una sesión después de su descarga.

Las restricciones declaradas por el Audio Chip continúan siendo obligatorias.

## 7. Input

### RS-017 — Entrada física y lógica

La lectura de dispositivos físicos corresponde a los servicios de plataforma disponibles para la sesión.

El mapping, los estados de botones, las direcciones y las restricciones de Input Chip pertenecen a la ejecución FLX.

Debe conservarse la abstracción `PhysicalInputProvider` existente.

### RS-018 — Aislamiento

Una sesión nueva no debe heredar estados lógicos de botones, acciones o direcciones de la anterior.

El estado físico actual de un dispositivo puede seguir siendo observable, pero no debe confundirse con estado lógico persistente del mundo anterior.

## 8. Estado y scripting

### RS-019 — Aislamiento del mundo

Cada sesión debe mantener independientes:

- Objetos y relaciones.
- Identificadores de runtime.
- Estados de mecánicas.
- Temporizadores.
- Colisiones.
- Attachments.
- Estados de efectos y transiciones.
- Estado JavaScript.

Los datos temporales asociados a objetos deben pertenecer al mundo correspondiente y no a almacenamiento global compartido entre ejecuciones.

### RS-020 — Contexto JavaScript

Cada nueva ejecución debe disponer de un contexto JavaScript limpio.

No se permite reutilizar parcialmente un contexto anterior conservando módulos, referencias, variables globales o callbacks de otra sesión.

El mecanismo de creación y destrucción debe respetar las garantías de QuickJS.

### RS-021 — Persistencia

La descarga de una sesión no debe borrar automáticamente datos persistentes que hayan sido guardados legítimamente mediante las capacidades de persistencia de FLX.

La persistencia explícita debe distinguirse del estado temporal de ejecución.

Esta especificación no introduce nuevas capacidades de almacenamiento ni define una política de aislamiento entre aplicaciones.

## 9. Finalización y errores

### RS-022 — `exit()`

La función JavaScript `exit()` conserva su semántica de solicitar la finalización de la ejecución FLX.

En modo standalone, el host finalizará su ejecución de acuerdo con el comportamiento actual.

En modo integrado, finalizará únicamente la sesión FLX.

`exit()` no debe cerrar automáticamente la aplicación anfitriona.

### RS-023 — Detención por el host

El host debe poder solicitar la finalización de una sesión.

Esta solicitud debe diferenciarse de una salida solicitada desde JavaScript.

La detención debe respetar las mismas garantías de destrucción segura.

### RS-024 — Resultados

La sesión debe informar, al menos, de:

- Éxito o fallo.
- Estado final.
- Motivo de finalización.
- Diagnósticos.
- Número de frames ejecutados, cuando corresponda.

Los motivos propios del host, como el cierre de una ventana, no deben confundirse con errores internos de FLX.

La implementación podrá adaptar estos resultados a `EngineResult` para mantener la interfaz standalone.

### RS-025 — Recuperación tras errores

Un error de carga o ejecución no debe inutilizar el host.

Después de descargar una sesión fallida debe ser posible crear y ejecutar una nueva sesión, siempre que los dispositivos físicos continúen disponibles.

No deben quedar recursos parcialmente inicializados sin liberar.

## 10. Compatibilidad standalone

### RS-026 — Compatibilidad de `Engine`

La clase `Engine` puede adaptarse para utilizar internamente `FlxSession`.

Debe conservar el comportamiento observable del CLI existente.

La modularización no debe modificar innecesariamente las opciones públicas de ejecución ni los códigos de salida.

### RS-027 — Equivalencia

`flx run` y `flx run-compiled` deben mantener su equivalencia semántica.

La misma Machine y el mismo proyecto deben producir el mismo comportamiento independientemente de que la ejecución sea standalone o integrada, salvo diferencias propias de presentación y control del host.

## 11. Pruebas de aceptación

### RS-028 — Regresión

Todas las suites existentes deben continuar funcionando.

La extracción de `flx-core` no debe modificar el comportamiento del runtime.

### RS-029 — Ciclo de vida

Deben existir pruebas que cubran:

- Carga correcta.
- Carga fallida.
- Ejecución y finalización.
- Descarga repetida.
- Destrucción después de un error.
- Nueva carga después de descargar.
- Finalización solicitada por JavaScript.
- Finalización solicitada por el host.

### RS-030 — Aislamiento

Deben existir pruebas que demuestren que una ejecución nueva no hereda estado temporal de la anterior.

En particular, deben comprobarse los estados de objetos, attachments, input, scripting y audio.

### RS-031 — Propiedad de dispositivos

Debe verificarse que descargar una sesión no destruye los dispositivos físicos del host.

La liberación de recursos gráficos y de audio debe realizarse antes de destruir dichos dispositivos.

### RS-032 — Compatibilidad CLI

Deben mantenerse las pruebas de compilación, validación, ejecución y lectura de `.flxc`.

Los resultados observables del CLI deben seguir siendo compatibles.

## 12. Fuera de alcance

Esta especificación no exige:

- Ejecución simultánea de varios mundos.
- Composición de mundos en una misma escena.
- Transiciones automáticas entre proyectos.
- Gestión de niveles o campañas.
- Nuevas funciones JavaScript para cambiar de mundo.
- Modificación del formato `.flxc`.
- Nuevas capacidades de Machine.
- Sustitución de raylib.
- Biblioteca dinámica o ABI estable.
- Editor visual.
- Integración específica con el hub de 80's Like.
- Sistema de plugins o servicios de plataforma extensible.

La ejecución secuencial de sesiones es el objetivo de esta versión.

## 13. Criterio de cierre

La modularización se considerará implementada cuando:

1. `flx-core` pueda enlazarse desde un ejecutable independiente del CLI.
2. `FlxSession` permita cargar, actualizar, renderizar, finalizar y descargar un proyecto.
3. El host conserve la propiedad de ventana y audio.
4. Las sesiones no compartan estado temporal accidentalmente.
5. Los errores no impidan ejecutar sesiones posteriores.
6. El CLI conserve su comportamiento y compatibilidad.
7. Las pruebas automatizadas pertinentes estén superadas.

La demostración manual de ejecución multiworld y su posterior integración en 80's Like se realizarán como actividades independientes.

---

## 14. Principio rector

**FLX define y ejecuta mundos. El host decide dónde, cuándo y durante cuánto tiempo se ejecutan.**

La modularización debe preservar la sencillez del motor, evitar responsabilidades duplicadas y mantener intacta la semántica declarativa consolidada en versiones anteriores.