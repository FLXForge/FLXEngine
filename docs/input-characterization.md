# FLX Input Consolidation

This document describes the consolidated digital Input contract for FLX v0.3.0.

```text
Machine / Input Chip -> input mapping -> InputSystem -> JavaScript API
```

## A. Consolidated Rules

- Input is digital in this iteration.
- The Machine declares capabilities, not gameplay actions.
- The mapping connects physical inputs to logical controls.
- JavaScript queries logical controls through functions.
- Gameplay names such as `fire`, `jump` or `start` do not appear in C++,
  Machine YAML or mapping files.
- Pointer, text, analog and 8way input are deliberately not implemented yet.

## B. Input Chip

Current YAML shape:

```yaml
input:
  system:
    buttons: 2
  players:
    count: 1
    controls:
      directions:
        - type: 4way
          simultaneous: last
          buffer: 0
      buttons: 1
```

Fields:

| Field | Meaning |
| --- | --- |
| `system.buttons` | Number of logical system buttons. |
| `players.count` | Number of available players. Player indexes start at 1. |
| `players.controls.directions` | Ordered list of logical direction controls per player. |
| `players.controls.directions[].type` | `2way` or `4way`. |
| `players.controls.directions[].simultaneous` | `first`, `neutral` or `last`. |
| `players.controls.directions[].buffer` | Direction-selection buffer used by `first`. |
| `players.controls.buttons` | Number of logical player buttons per player. |

If no input chip is declared, FLX uses a permissive internal default:
16 players, 16 system buttons, 16 player buttons and one 4way direction using
`last`.

Unsupported fields such as `pointer`, `text`, legacy `direction`, legacy
`buttons.player`, legacy `buttons.system`, `8way` and `analog` are diagnostics.
They are not silently degraded.

## C. Input Mapping

Current mapping syntax:

```properties
system.buttons.0=KEY_ESCAPE,KEY_LEFT_ALT+KEY_Q

players.1.directions.0.up=KEY_W,JOY1_UP
players.1.directions.0.down=KEY_S,JOY1_DOWN
players.1.directions.0.left=KEY_A,JOY1_LEFT
players.1.directions.0.right=KEY_D,JOY1_RIGHT
players.1.buttons.0=KEY_SPACE,JOY1_A
```

For 2way directions the valid components are:

```properties
players.1.directions.0.negative=KEY_W
players.1.directions.0.positive=KEY_S
```

Commas separate alternatives. `+` joins simultaneous physical inputs.

The mapping is validated against the active Input Chip:

- player index must be within `players.count`;
- system button index must be within `system.buttons`;
- player button index must be within `players.controls.buttons`;
- direction index must exist;
- direction components must match the direction type.

## D. Runtime Semantics

`InputSystem` stores previous and current state for every mapped logical control.
The public API exposes:

- `pressed`: false -> true during the current frame;
- `down`: true while active;
- `released`: true -> false during the current frame.

2way directions expose `NEGATIVE` and `POSITIVE`.

4way directions expose `UP`, `RIGHT`, `DOWN` and `LEFT`. They can also be
queried as a 2way projection:

```text
POSITIVE = UP or RIGHT
NEGATIVE = DOWN or LEFT
```

Projection `pressed` and `released` are calculated from previous/current
projection state, not from raw direction value equality.

Simultaneous policy:

- `last`: newest active physical component wins;
- `neutral`: one active component wins, multiple active components return neutral;
- `first`: current component is kept while held. A newer component can be stored
  briefly according to `buffer` and promoted if the current component is released.

This buffer is a direction-selection helper. It is not a gameplay input buffer.

## E. JavaScript API

FLX exposes descriptors and functional queries:

```js
const MOVE = direction(0);
const FIRE = button(0);

function motion(ship) {
    if (input_down(ship, MOVE, UP)) {
        accelerate(ship);
    }

    if (input_down(ship, MOVE, LEFT)) {
        rotate(ship, LEFT);
    }

    if (input_pressed(ship, FIRE)) {
        spawn(ship, "laser");
    }
}
```

Functions:

| Function | Meaning |
| --- | --- |
| `button(index)` | Creates a logical button descriptor. |
| `direction(index)` | Creates a logical direction descriptor. |
| `player(index)` | Creates an explicit player subject. |
| `system()` | Creates the system subject. |
| `input_pressed(subject, control)` | Button pressed this frame. |
| `input_down(subject, control)` | Button held. |
| `input_released(subject, control)` | Button released this frame. |
| `input_pressed(subject, direction, component)` | Direction component pressed this frame. |
| `input_down(subject, direction, component)` | Direction component held. |
| `input_released(subject, direction, component)` | Direction component released this frame. |

`subject` can be:

- a `RuntimeObject` with `control.player` declared in JSON;
- `player(n)`;
- `system()`.

Example using an explicit subject:

```js
if (input_pressed(system(), button(0))) {
    exit();
}
```

## F. Object Control Binding

Objects can bind themselves to a player through JSON:

```json
{
  "control": {
    "player": 1
  }
}
```

That value is exposed to JS as read-only `object.controlPlayer`.

Objects without `control.player` are not player subjects. Calling
`input_down(object, ...)` on them returns false and logs a warning.

## G. Removed Surface

The following public surface is removed from the consolidated contract:

- `Input.player(...)`;
- `Input.system`;
- `Input.pointer`;
- accidental `__flx_input_*` globals;
- old `Key.*` examples.

## H. Pending Future Work

Deliberately pending:

- 8way directions;
- analog axes;
- pointer input;
- text input;
- mouse wheel;
- dead zones;
- per-action gameplay buffers;
- configurable device remapping UI;
- named gameplay actions.

These features require a separate design pass.
