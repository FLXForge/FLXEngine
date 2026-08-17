# FLX Digital Input Consolidation

This document records the post-consolidation audit of the FLX digital input
system for v0.3.0.

```text
Machine Input Chip -> input mapping -> InputSystem -> JavaScript API
```

## A. Verdict

INPUT DIGITAL CONSOLIDATED for v0.3.0.

The implemented system is internally coherent across Machine YAML, input
mapping, runtime state, JavaScript bindings, examples, documentation and
`.flxc` v4 persistence.

No production defect was found during the post-consolidation audit. The changes
made during this audit are additional characterization tests and this report.

## B. Consolidated Contract

- Input is digital only in this version.
- The Machine declares capabilities, not gameplay actions.
- The mapping connects physical controls to logical controls.
- JavaScript queries logical controls through functions.
- Gameplay names such as `fire`, `jump`, `pause` or `start` do not appear in
  C++, Machine YAML or mapping files.
- Pointer, text, analog and 8way input are deliberately pending.
- Unsupported capabilities are diagnostics. They are not silently downgraded.

## C. Input Chip

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
| `players.controls.directions[].buffer` | Direction-selection buffer used only by `first`. |
| `players.controls.buttons` | Number of logical player buttons per player. |

If no input chip is declared, FLX uses the internal default: 16 players, 16
system buttons, 16 player buttons and one `4way` direction using `last`.
The full Default Machine is characterized in `docs/default-machine-characterization.md`.

The legacy shapes `input.direction`, `input.buttons.player` and
`input.buttons.system` are not part of the v0.3.0 contract.

## D. Mapping

Current mapping syntax:

```properties
system.buttons.0=KEY_ESCAPE,KEY_LEFT_ALT+KEY_Q

players.1.directions.0.up=KEY_W,JOY1_UP
players.1.directions.0.down=KEY_S,JOY1_DOWN
players.1.directions.0.left=KEY_A,JOY1_LEFT
players.1.directions.0.right=KEY_D,JOY1_RIGHT
players.1.buttons.0=KEY_SPACE,JOY1_A
```

For 2way directions:

```properties
players.1.directions.0.negative=KEY_W
players.1.directions.0.positive=KEY_S
```

Rules:

- commas separate alternatives;
- `+` joins simultaneous physical inputs;
- keyboard and gamepad buttons can coexist in the same logical control;
- multiple physical sources keep the logical control active while at least one
  source remains active;
- mapping is checked against the active Input Chip;
- out-of-range mapping lines are ignored with warnings;
- duplicate mapping entries are currently last-wins. This is characterized
  behavior, not a recommended authoring style.

## E. Runtime Semantics

`InputSystem` stores previous and current state for every mapped logical
control.

Button and direction temporality:

| Query | Meaning |
| --- | --- |
| `pressed` | false -> true during the current frame. |
| `down` | true while active. |
| `released` | true -> false during the current frame. |

2way directions expose `NEGATIVE` and `POSITIVE`.

4way directions expose `UP`, `RIGHT`, `DOWN` and `LEFT`. They can also be
queried through 2way projections:

```text
POSITIVE = UP or RIGHT
NEGATIVE = DOWN or LEFT
```

Projection `pressed` and `released` are calculated from previous/current
projection state, so transitions such as `RIGHT -> DOWN` release `POSITIVE`
and press `NEGATIVE`.

Simultaneous policies:

- `last`: newest active physical component wins.
- `neutral`: one active component wins; multiple active components return
  neutral.
- `first`: current component is kept while held. A newer component can be
  stored in a direction buffer and promoted if the current component is
  released before the buffer expires.

When several components are pressed in the same update, the current
implementation resolves physical order through component scan order:

```text
2way: negative, positive
4way: up, right, down, left
```

The direction buffer is not a gameplay action buffer. It only helps `first`
direction arbitration. `neutral` and `last` clear pending direction state every
update.

## F. JavaScript API

Public functions:

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

Public constants:

```js
NEGATIVE
POSITIVE
UP
DOWN
LEFT
RIGHT
```

Descriptors returned by `button`, `direction`, `player` and `system` contain
internal marker fields. Scripts should treat those objects as opaque API
values.

Invalid calls return `false` and log a warning. They do not throw and they do
not produce structured Runtime Diagnostics yet.

Removed public surface:

- `Input.*`;
- `Key.*`;
- accidental `__flx_input_*` globals.

## G. RuntimeObject Subject

Objects can bind themselves to a player in JSON:

```json
{
  "control": {
    "player": 1
  }
}
```

This value is exposed to scripts as read-only `object.controlPlayer`.

`input_down(object, ...)`, `input_pressed(object, ...)` and
`input_released(object, ...)` use `object.controlPlayer` as the subject.
Objects without `control.player` are not valid player subjects; those queries
return `false` and log a warning.

`control.player` is validated during compilation against
`input.players.count`.

## H. Persistence And Examples

`.flxc` v4 stores the consolidated Input Chip and `control.player`. Roundtrip
tests confirm:

- system button count;
- player count;
- player button count;
- direction list;
- direction type;
- simultaneous policy;
- direction buffer;
- `RuntimeObject.controlPlayer`.

The current examples use the consolidated API:

- Pong uses a 2way direction and `NEGATIVE`/`POSITIVE`.
- Asteroids uses a 4way direction and a button through object subjects.
- Arkanoid uses a 4way direction and button bindings.
- Invaders uses 4way horizontal queries and a fire button.

## I. Verification Matrix

| Rule | Implementation | Test coverage |
| --- | --- | --- |
| Default input is permissive and digital | `MachineLoader::defaultMachine` | `default machine input is digital and permissive` |
| Consolidated YAML shape loads | `MachineLoader` | `input chip loads consolidated shape` |
| Unsupported capabilities are diagnostics | `MachineLoader` | `unsupported capabilities produce diagnostics` |
| Mapping accepts keyboard/gamepad alternatives | `InputSystem::loadMappingContent` | `mapping accepts keyboard gamepad combination and directions` |
| Mapping obeys chip limits | `InputSystem` mapping validators | `mapping is constrained by input chip` |
| Buttons expose pressed/down/released | `InputSystem` button states | `button pressed down released are logical and idempotent` |
| 2way uses NEGATIVE/POSITIVE | `componentAllowed`, `resolveDirection` | `two way native components` |
| 4way `last` chooses newest component | `resolveLast` | `four way last resolves newest logical component` |
| 4way projections transition correctly | `componentMatches` | `four way last projection transitions across polarity` |
| 4way `neutral` neutralizes conflict | `resolveNeutral` | `four way neutral policy` |
| `first` buffer promotes a candidate | `resolveFirst` | `first buffer promotes pending candidate` |
| `first` stores only latest pending candidate | `resolveFirst` | `first buffer keeps only latest candidate` |
| zero buffer disables hold | `resolveFirst` | `first with zero buffer does not hold pending candidate` |
| expired buffer does not block remaining input | `resolveFirst` | `first buffer expires candidate` |
| buffer does not affect `neutral` or `last` | `resolveDirection` | `buffer does not change neutral or last policy` |
| multiple physical sources keep logical state | `actionDown` | `multiple physical sources keep logical button and direction active` |
| high indexes and subjects are isolated | `InputSystem` state maps | `high indexes players system and directions are independent` |
| invalid runtime queries are false | `InputSystem` validators | `invalid queries return false` |
| project without explicit Machine can use `player(1), direction(0)` | `ProjectCompiler`, `InputSystem`, `InputBindings` | `project without machine uses default direction from player subject` |
| `control.player` is compile-validated | `ProjectCompiler` validator | `control player outside input chip fails` |
| Input survives `.flxc` v4 | binary codec | `compiled project roundtrip input v4` |
| Default Machine Input survives `.flxc` v4 | binary codec | `compiled project roundtrip default machine input v4` |
| old JS globals are absent | `InputBindings` registration | runtime lifecycle scripting surface test |

## J. Diagnostics

Structured Diagnostics currently cover configuration/compilation errors:

- unsupported Machine input capabilities;
- invalid input chip values;
- invalid `control.player` relative to the Input Chip.

Runtime misuse from JavaScript currently uses `Logger::warning` and returns
`false`. Runtime Diagnostics for input calls are deliberately pending.

Mapping validation currently happens while the runtime loads the embedded
mapping text. Malformed lines, out-of-range entries and incompatible direction
components are ignored with `Logger::warning`. They are not structured
Diagnostics yet. This is real debt for the future mapping specification because
a badly formatted `.input` file can degrade controls without failing compile.

Practical evidence after consolidation: the Asteroids menu in 80sLike replaced
manual direction locks with `input_pressed(player(1), direction(0), UP/DOWN/LEFT/RIGHT)`
and behaves correctly.

## K. Future Work

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
- structured `.input` validation diagnostics.

Open design questions:

- whether descriptor marker fields should remain visible to JavaScript;
- whether duplicate mapping entries should become diagnostics instead of
  last-wins;
- whether `object.controlPlayer` should remain a public readable property or
  become an internal subject relation queried only by functional API;
- how Runtime Diagnostics should report invalid input API usage.
