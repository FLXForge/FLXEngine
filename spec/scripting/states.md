# State machine

## Estado
- Documento: especificación normativa
- Ámbito: máquina de estados de objetos FLX
- Versión inicial: 0.3.0
- Estado: consolidado

## 1. Propósito
La máquina de estados permite declarar estados válidos y sus transiciones. JSON declara qué puede ocurrir; JavaScript decide cuándo ocurre.

## 2. Declaración
`states` es opcional.

```json
"states": {
  "initial": "idle",
  "idle": { "next": ["warning"] },
  "warning": { "next": ["done"] },
  "done": {}
}
```

Si existe, `initial` es obligatorio, no vacío y debe referenciar un estado declarado.

`next` es opcional. Ausente/vacío = terminal. Destinos deben existir. Ciclos válidos. Self-transition sólo si se declara explícitamente.

## 3. Propiedad de la StateMachine

La StateMachine pertenece al RuntimeObject que declara `states`.

Un RuntimeObject sin `states` no crea una máquina vacía.

## 4. Effective State context

Todas las APIs y consumidores de State resuelven:

```text
effectiveStateContext(O):
1. O declara states
   → StateMachine de O
2. O no declara states y O es component
   → effectiveStateContext(parent)
3. O no es component
   → sin StateMachine
```

La búsqueda termina en la primera StateMachine o en el primer RuntimeObject no-component sin states.

```text
tank states = patrol / alert / attack
├─ body [component], sin states
│  → tank
└─ turret [component], states = idle / tracking / firing
   └─ cannon [component], sin states
      → turret
```

Un child normal no hereda State.

## 5. API JavaScript

```javascript
state_to(object, stateName)
state_current(object)
state_active(object, stateName)
state_entered(object)
state_time(object)
```

Todas operan sobre el effective State context del objeto recibido.

La antigua `state(object, stateName)` no forma parte de la API.

## 6. state_to

Solicita transición validada sobre la StateMachine efectiva.

Sólo cambia si destino existe y está permitido por `next`.

Ejemplo: `state_to(cannon,"firing")` puede modificar la máquina de turret si cannon es component sin states y turret es el primer contexto efectivo.

Transición válida cambia state, reinicia tiempo y programa `state_entered` para el siguiente frame completo.

## 7. Consultas

`state_current` devuelve estado actual; sin máquina efectiva, `""`.

`state_active` devuelve predicado; sin máquina, false.

`state_entered` es true durante el primer frame completo del estado efectivo y no dentro del callback que llamó a `state_to`.

`state_time` pertenece a la máquina efectiva, vale 0 durante ese primer frame y luego acumula.

Componentes que resuelven la misma StateMachine observan el mismo state/stateEntered/stateTime: no hay copias.

## 8. Runtime y DRY

La definición que declara states conserva reglas (`initialState`, transiciones). La instancia propietaria conserva estado vivo.

Los componentes consumidores resuelven esa máquina, no duplican su estado.

## 9. Consumidores transversales

Collision y cualquier otro subsistema condicionado por State usan el mismo effective State context; no implementan una herencia paralela.

## 10. Validación

Fallan antes de Runtime:
- tipos incorrectos;
- initial ausente/vacío/inexistente;
- next inválido;
- destino inexistente/duplicado.

Las referencias externas a states deben validarse contra effective State context; sin máquina efectiva o estado inexistente → definición inválida.

## 11. Futuro

Máximo una StateMachine propia por RuntimeObject en v0.3.

Una entidad lógica puede contener varias máquinas porque distintos components pueden declarar la suya, pero cada RuntimeObject resuelve una única máquina efectiva.

## 12. Invariantes

- STATE-001 states opcional.
- STATE-002 Si existe, initial obligatorio y válido.
- STATE-003 next sólo a destinos existentes.
- STATE-004 ciclos válidos.
- STATE-005 self-transition explícita.
- STATE-006 state_to sólo transiciones permitidas.
- STATE-007 state_entered = primer frame completo.
- STATE-008 state_time = 0 durante ese frame.
- STATE-009 Sin StateMachine efectiva no existe estado vacío implícito.
- STATE-010 La máquina pertenece al RuntimeObject declarador.
- STATE-011 Component sin states puede resolver State ascendente.
- STATE-012 La resolución asciende sólo por chains de component.
- STATE-013 RuntimeObject con states propios detiene ascenso.
- STATE-014 Child normal no hereda State.
- STATE-015 Todos los consumidores usan el mismo effective State context.

## 13. Principio final

```text
states    → qué puede ocurrir
state_to  → cuándo ocurre
component → desde qué RuntimeObjects se observa una misma máquina efectiva
```
