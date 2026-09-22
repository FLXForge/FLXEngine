# Timer

## Estado

- Documento: especificación normativa
- Ámbito: temporizadores de objetos FLX
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito

Timer permite que un comportamiento JavaScript controle procesos temporales con nombre pertenecientes a un objeto runtime.

Cada timer pertenece a una única instancia de objeto. Dos instancias PUEDEN utilizar el mismo nombre sin compartir estado.

Timer es independiente de State y de la visibilidad del objeto.

## 2. Modelo observable

Un timer se encuentra en uno de estos estados observables:

| Estado | `timer_active` | `timer_paused` | `timer_done` | `timer_left` |
| --- | --- | --- | --- | --- |
| `ABSENT` | `false` | `false` | `false` | `0` |
| `RUNNING` | `true` | `false` | `false` | `> 0` |
| `PAUSED` | `true` | `true` | `false` | valor congelado `> 0` |
| `DONE` | `false` | `false` | `true` | `0` |

`ABSENT` significa que el timer no existe. No existe un estado observable `STOPPED`.

Un timer pausado sigue activo. `DONE` significa exclusivamente que el timer terminó de forma natural.

## 3. API JavaScript

```javascript
play_timer(object, name)
play_timer(object, name, duration)
pause_timer(object, name)
stop_timer(object, name)

timer_active(object, name)
timer_paused(object, name)
timer_done(object, name)
timer_left(object, name)
```

Las antiguas funciones `timer(object, name, duration)` y `timer_clear(object, name)` NO forman parte de la API.

## 4. Nombres y duraciones

`name` DEBE ser una cadena no vacía.

Cuando se proporciona, `duration` DEBE ser un número finito estrictamente mayor que `0`. Su unidad son segundos.

Una llamada con nombre o duración inválidos:

- NO DEBE crear, eliminar ni modificar ningún timer;
- DEBE permitir que la ejecución del script continúe;
- DEBE registrar actualmente una advertencia mediante Logger.

## 5. play_timer sin duración

`play_timer(object, name)` DEBE comportarse según el estado previo:

| Estado previo | Resultado |
| --- | --- |
| `ABSENT` | advertencia y ninguna modificación |
| `RUNNING` | ninguna modificación |
| `PAUSED` | pasa a `RUNNING`, conservando duración y tiempo restante |
| `DONE` | pasa a `RUNNING` desde la duración almacenada |

La operación sin duración NO DEBE inventar una duración para un timer ausente.

## 6. play_timer con duración

`play_timer(object, name, duration)` interpreta `duration` como duración total, no como nuevo tiempo restante.

| Estado previo | Resultado |
| --- | --- |
| `ABSENT` | crea un timer `RUNNING` con el total indicado |
| `DONE` | inicia una reproducción nueva con el total indicado |
| `RUNNING` o `PAUSED` | redefine el total conservando el tiempo ya transcurrido |

Para un timer `RUNNING` o `PAUSED`, el resultado DEBE ser equivalente a:

```text
elapsed = oldDuration - oldLeft
newLeft = newDuration - elapsed
```

Si `newLeft > 0`, el timer DEBE quedar `RUNNING` con `newLeft` segundos. Si `newLeft <= 0`, DEBE quedar `DONE` con tiempo restante `0`.

Redefinir la duración de un timer existente NO equivale a reiniciarlo. Un reinicio explícito se expresa mediante:

```javascript
stop_timer(object, "name");
play_timer(object, "name", duration);
```

## 7. pause_timer

`pause_timer(object, name)` DEBE cambiar un timer `RUNNING` a `PAUSED` y conservar su duración y tiempo restante.

Sobre `ABSENT`, `PAUSED` o `DONE`, DEBE ser una operación sin efecto.

`pause_timer` NO es un toggle. La reanudación se solicita mediante `play_timer(object, name)`.

## 8. stop_timer

`stop_timer(object, name)` DEBE eliminar un timer `RUNNING`, `PAUSED` o `DONE`.

Sobre `ABSENT`, DEBE ser una operación sin efecto.

Después de detener un timer, las cuatro consultas DEBEN producir los valores de `ABSENT`. Detener NO DEBE producir `DONE`.

## 9. Finalización natural

El tiempo restante de un timer `RUNNING` DEBE disminuir con el avance temporal del objeto.

Cuando alcanza o atraviesa `0`, el tiempo restante DEBE fijarse en `0` y el timer DEBE pasar a `DONE`.

`DONE` es persistente: DEBE conservarse hasta una reproducción posterior o hasta `stop_timer`.

Los timers `PAUSED` y `DONE` NO DEBEN decrementar.

## 10. Consultas

- `timer_active(object, name)` DEBE devolver `true` únicamente para `RUNNING` y `PAUSED`.
- `timer_paused(object, name)` DEBE devolver `true` únicamente para `PAUSED`.
- `timer_done(object, name)` DEBE devolver `true` únicamente para `DONE`.
- `timer_left(object, name)` DEBE devolver los segundos restantes; para `ABSENT` y `DONE` DEBE devolver `0`.

Las consultas NO DEBEN crear ni modificar timers.

## 11. Frames y ciclo de vida

- Un timer creado en `born()` DEBE conservar su duración completa hasta el primer update.
- Un timer creado o modificado en `action`, `motion` o `collision` DEBE conservar el valor definido durante ese callback.
- Si queda `RUNNING`, DEBE decrementar en la fase de timers de ese mismo frame.
- Ocultar un objeto NO DEBE pausar sus timers.
- `state_to` NO DEBE modificar sus timers.
- Un objeto muerto NO DEBE seguir avanzando timers después de morir.
- `dead()` PUEDE consultar el estado final de sus timers antes de que se destruya la instancia.

Timer no define en esta versión una política de pausa global del juego.

## 12. Aislamiento

Cada nombre identifica un timer dentro de una instancia concreta.

Crear, reproducir, redefinir, pausar o detener un timer NO DEBE modificar:

- timers con otros nombres de la misma instancia;
- timers del mismo nombre pertenecientes a otras instancias.

## 13. Gramática

```text
play_timer    → iniciar, reanudar o reproducir
pause_timer   → suspender
stop_timer    → detener voluntariamente y eliminar

timer_active  → vigencia
timer_paused  → suspensión
timer_done    → finalización natural
timer_left    → cantidad restante
```

`STOP` y `DONE` representan hechos distintos: el primero es una intervención del comportamiento; el segundo, el agotamiento natural del tiempo.

## 14. Invariantes

- TIMER-001 Todo timer pertenece a una única instancia de objeto.
- TIMER-002 Un nombre identifica como máximo un timer dentro de esa instancia.
- TIMER-003 `RUNNING` implica duración y tiempo restante mayores que `0`.
- TIMER-004 `PAUSED` implica duración y tiempo restante mayores que `0`.
- TIMER-005 `DONE` implica duración mayor que `0` y tiempo restante igual a `0`.
- TIMER-006 Un timer pausado está activo.
- TIMER-007 Solo la finalización natural produce `DONE`.
- TIMER-008 Detener elimina el timer y produce el estado observable `ABSENT`.
- TIMER-009 Redefinir un timer activo conserva el tiempo transcurrido.
- TIMER-010 Las entradas inválidas no modifican el timer.
- TIMER-011 Los timers de nombres o instancias diferentes están aislados.
- TIMER-012 `DONE` persiste hasta reproducir o detener el timer.

## 15. Fuera del contrato actual

La versión 0.3.0 no define:

- callbacks o eventos de timer;
- consultas públicas de progreso o tiempo transcurrido;
- grupos de timers;
- límite máximo de timers por objeto;
- política de pausa global;
- diagnósticos runtime estructurados para llamadas inválidas.

Estas posibilidades no forman parte de Timer mientras no se especifiquen expresamente.

## 16. Principio final

```text
play / pause / stop → control del proceso temporal
active / paused / done / left → observación del proceso
```

Timer conserva la diferencia entre detener voluntariamente y terminar de forma natural.
