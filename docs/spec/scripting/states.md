# State machine

## Estado
- Documento: especificación normativa
- Ámbito: máquina de estados de objetos FLX
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito
La máquina de estados permite declarar estados válidos y sus transiciones. JSON declara qué puede ocurrir; JavaScript decide cuándo ocurre.

## 2. Declaración
`states` es opcional. Si no existe, el objeto no tiene máquina de estados.

```json
"states": {
  "initial": "idle",
  "idle": { "next": ["warning"] },
  "warning": { "next": ["done"] },
  "done": {}
}
```

Si existe `states`, `initial` es obligatorio, no puede estar vacío y debe referenciar un estado declarado.

## 3. Estados y transiciones
`next` es opcional. Ausente o vacío significa estado terminal.

Todos los destinos de `next` deben existir. Los ciclos son válidos. Una self-transition solo es válida cuando está declarada explícitamente.

Una máquina con un único estado es válida.

## 4. API JavaScript
```javascript
state_to(object, stateName)
state_current(object)
state_active(object, stateName)
state_entered(object)
state_time(object)
```

La antigua función `state(object, stateName)` no forma parte de la API.

## 5. state_to
`state_to` solicita una transición validada. Solo cambia de estado cuando el destino existe y está permitido por `next`.

Una transición válida cambia el estado, reinicia su tiempo y programa el evento de entrada para el siguiente frame completo.

Una transición inválida se ignora y actualmente se registra mediante Logger.

## 6. state_current
Devuelve el estado actual. Sin máquina devuelve `""`; ese valor no representa un estado implícito.

## 7. state_active
Devuelve `true` cuando el estado indicado es el actual. Sin máquina devuelve siempre `false`.

## 8. state_entered
Es `true` durante el primer frame completo de ejecución del estado actual. No pasa a `true` dentro del mismo callback que llamó a `state_to`.

## 9. state_time
Se reinicia a `0` al entrar en un estado. Durante el primer frame completo:
```text
state_entered(object) == true
state_time(object) == 0
```
Después comienza a acumular tiempo.

## 10. Runtime y DRY
La definición compilada conserva `initialState` y `stateTransitions`.

`RuntimeObject` mantiene solo estado vivo:
```text
definitionId
state
stateTime
stateEnteredFrame
```

Las reglas se consultan desde `ResourceRegistry`; no se duplican por instancia.

## 11. Validación
Las declaraciones inválidas fallan antes de Runtime: tipos incorrectos, `initial` ausente/vacío/inexistente, `next` inválido, destinos inexistentes o duplicados.

## 12. Gramática
```text
state_to       → transición
state_current  → proyección de valor
state_active   → predicado
state_entered  → evento
state_time     → proyección temporal
```

## 13. Futuro
La v0.3.0 define una única máquina por objeto. Múltiples máquinas quedan fuera del contrato actual.

## 14. Invariantes
- STATE-001 `states` es opcional.
- STATE-002 Si existe, `initial` es obligatorio.
- STATE-003 `initial` debe existir.
- STATE-004 Todo destino `next` debe existir.
- STATE-005 `next` ausente/vacío define estado terminal.
- STATE-006 Los ciclos son válidos.
- STATE-007 La self-transition requiere declaración explícita.
- STATE-008 `state_to` solo realiza transiciones permitidas.
- STATE-009 `state_entered` corresponde al primer frame completo.
- STATE-010 `state_time` vale 0 durante ese frame.
- STATE-011 Sin `states` no existe estado vacío implícito.
- STATE-012 Las reglas viven en la definición; la instancia conserva estado vivo.

## 15. Principio final
```text
states   → qué puede ocurrir
state_to → cuándo ocurre
```

JSON declara posibilidades. JavaScript decide el comportamiento.
