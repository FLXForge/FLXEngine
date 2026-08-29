# FLX Scripting Input Contract v0.3.0

This document defines the public scripting contract for digital Input in FLX
v0.3.0.

It is normative for scripting behavior. It does not define future input
capabilities, hardware profiles or gameplay action naming.

## A. Layers

FLX Input has five separate layers:

```text
Machine Input Chip
-> Input Mapping
-> Subject
-> Scripting Input API
-> Gameplay meaning
```

The Machine Input Chip defines available logical capabilities.

The Input Mapping connects physical sources to those logical controls.

The subject determines which logical context is queried:

- a `RuntimeObject` with `control.player`;
- `player(index)`;
- `system()`.

The Scripting Input API observes the logical controls.
Gameplay meaning belongs to script code:

```js
const FIRE = button(0);
const MOVE = direction(0);
```

`FIRE` and `MOVE` are script-level names. The Machine and mapping do not know
about gameplay concepts such as fire, jump, pause, accept or cancel.

## B. Public API

Descriptor functions:

```js
button(index);
direction(index);
player(index);
system();
```

Button queries:

```js
input_pressed(subject, control);
input_down(subject, control);
input_released(subject, control);
```

Direction queries:

```js
input_pressed(subject, direction, component);
input_down(subject, direction, component);
input_released(subject, direction, component);
```

Removed or unavailable surfaces:

- `Input.*`;
- `Key.*`;
- `__flx_input_*`;
- legacy aliases.

## C. Subjects

A subject identifies the logical input context being queried.

### RuntimeObject Subject

A `RuntimeObject` can be used as subject when it declares `control.player`:

```json
{
  "control": {
    "player": 1
  }
}
```

```js
const FIRE = button(0);

if (input_pressed(ship, FIRE)) {
    spawn(ship, "laser");
}
```

`control.player` associates the object with a player. It does not define
buttons, directions, mapping or gameplay names.

### Explicit Player Subject

`player(index)` creates an explicit player subject:

```js
input_pressed(player(1), button(0));
```

`control.player` is not required for this form. It is only required when a
script wants to pass the `RuntimeObject` itself as the subject.

### System Subject

`system()` creates the system subject:

```js
const EXIT = button(0);

if (input_pressed(system(), EXIT)) {
    exit();
}
```

System subjects support buttons, not directions.

## D. Buttons

`button(index)` creates a logical button descriptor.

There are no public constants such as `BUTTON_0`, `BUTTON_1` or `BUTTON_N`.
The Machine determines which button indexes exist.

Button state is temporal:

| Query | Meaning |
| --- | --- |
| `input_pressed(subject, button)` | The button entered the active state this frame. |
| `input_down(subject, button)` | The button is active in the current frame. |
| `input_released(subject, button)` | The button left the active state this frame. |

Queries do not consume events. They are idempotent during a frame and can be
called more than once.

## E. Directions

`direction(index)` creates a logical direction descriptor.

A Machine can declare multiple directions per player. In v0.3.0, the implemented
direction types are:

- `2way`;
- `4way`.

The following are not implemented in v0.3.0:

- `8way`;
- `analog`.

## F. 2way Directions

A `2way` direction has these logical components:

- `NEGATIVE`;
- `POSITIVE`;
- `NEUTRAL`.

`NEGATIVE` and `POSITIVE` do not inherently mean up, down, left or right. Their
meaning belongs to gameplay code.

Example:

```js
const THROTTLE = direction(0);

if (input_down(player(1), THROTTLE, POSITIVE)) {
    accelerate(ship);
}
```

## G. 4way Directions

A `4way` direction has these native logical components:

- `UP`;
- `RIGHT`;
- `DOWN`;
- `LEFT`;
- `NEUTRAL`.

There are no diagonals in a `4way` direction. Simultaneous physical inputs are
resolved before JavaScript observes the logical state.

## H. POSITIVE And NEGATIVE Projections

For compatibility, a `4way` direction can be queried with `POSITIVE` and
`NEGATIVE`.

In a `4way` direction:

```text
POSITIVE = UP or RIGHT
NEGATIVE = DOWN or LEFT
```

These are projections. They are not additional native values of a `4way`.

Projection temporality is based on previous and current projected state.

Example:

```text
previous = UP
current  = RIGHT
```

For `POSITIVE`:

```text
pressed  = false
down     = true
released = false
```

This allows behavior written for a `2way` using `POSITIVE` and `NEGATIVE` to
remain useful if the Machine later expands that direction to `4way`.

No equivalent projection contract is defined for `8way` in v0.3.0.

## I. Simultaneous Policy

Every direction has a simultaneous policy.

Default:

```text
last
```

Policies:

| Policy | Meaning |
| --- | --- |
| `first` | The first active logical component keeps priority while it remains active. |
| `neutral` | A conflict produces `NEUTRAL`. |
| `last` | The last activated logical component gets priority. |

JavaScript observes the resolved logical state, not individual physical
presses.

## J. Direction Buffer

`buffer` is a duration in seconds.

Default:

```text
0
```

The direction buffer participates only in `first` direction resolution. It can
briefly remember a newer blocked component and promote it if the current
component is released before the buffer expires.

The direction buffer is not a gameplay input buffer.

FLX v0.3.0 does not provide a gameplay action buffer.

## K. Physical To Logical Pipeline

Input flows through this pipeline:

```text
physical input
-> mapping
-> direction/button resolution
-> logical state
-> pressed/down/released
-> JavaScript
```

Example with `4way` and `last`:

```text
previous frame:
UP

current physical state:
UP remains held
RIGHT enters

current logical state:
RIGHT
```

Therefore:

```text
released(UP)  = true
pressed(RIGHT) = true
```

even though the physical UP source is still held.

## L. Multiple Physical Sources

A single logical control can be fed by multiple physical sources:

```properties
players.1.directions.0.up=KEY_W,KEY_UP,JOY1_UP
players.1.buttons.0=KEY_SPACE,JOY1_A
```

The script observes the logical result. It does not need to know which physical
source produced it.

## M. Default Machine Input

If a project does not declare an explicit Machine, FLX uses the Default Machine
of the current version.

For Input in v0.3.0, the Default Machine provides:

```text
system.buttons = 2
players.count = 16
players.controls.buttons = 4

direction(0):
type = 4way
simultaneous = last
buffer = 0
```

These values define the logical Input capacity of the Default Machine. They do
not define physical bindings.

This document is not the full Default Machine specification. A future
`spec/default-machine.md` should define the complete default behavior.

## N. Input Mapping

Input Mapping connects physical input sources to logical Input controls.

Machine Input and Input Mapping are independent scopes:

```text
Machine Input                    Input Mapping
-------------                    -------------
logical capacity                 physical bindings
what the game can consume        what is physically available
```

Neither scope is required to have the same capacity as the other.

A mapping may provide controls that the current Machine does not consume, and a
Machine may expose logical controls for which the current mapping provides no
physical binding.

These differences do not invalidate or trim either declaration.

### Mapping syntax

Input mapping files use a properties-like syntax.

A logical control is declared on the left side of `=` and one or more physical
sources are declared on the right side.

Example:

```properties
players.1.directions.0.up=KEY_W,KEY_UP
players.1.directions.0.down=KEY_S,KEY_DOWN
players.1.directions.0.left=KEY_A,KEY_LEFT
players.1.directions.0.right=KEY_D,KEY_RIGHT

players.1.buttons.0=KEY_SPACE

system.buttons.0=KEY_ESCAPE
```

Mapping files must not define gameplay names:

```properties
# Not part of v0.3.0 mapping
fire=KEY_SPACE
players.1.buttons.fire=KEY_SPACE
```

Gameplay meaning belongs to scripting.

### Direction vocabulary

The public directional vocabulary of an Input Mapping is:

```text
up
right
down
left
```

The mapping format does not expose `positive` or `negative`.

The same directional mapping can be used with a `2way` or `4way` Machine
direction.

For `4way`, the components retain their native logical meaning:

```text
up    -> UP
right -> RIGHT
down  -> DOWN
left  -> LEFT
```

For `2way`, FLX projects the four public mapping components onto the two logical
poles:

```text
up    ┐
right ├-> POSITIVE

down  ┐
left  ├-> NEGATIVE
```

The physical sources of `up` and `right` become alternatives for the positive
pole. The physical sources of `down` and `left` become alternatives for the
negative pole.

This projection does not require several physical directions to be active at
the same time.

The loss of distinction between `UP` and `RIGHT`, or between `DOWN` and `LEFT`,
is inherent to the lower logical capacity of a `2way` direction.

### Alternatives and combinations

`,` declares alternative physical sources. Any alternative can activate the
logical control:

```properties
players.1.buttons.0=KEY_SPACE,JOY1_A
```

`+` declares a physical combination. Every member of that combination must be
active:

```properties
players.1.buttons.0=KEY_LEFT_CONTROL+KEY_A
```

Alternatives and combinations may coexist in the same binding.

### Default Input Mapping

If `input.mapping` is absent, FLX uses the Default Input Mapping of the current
version.

For v0.3.0:

```properties
players.1.directions.0.up=KEY_W,KEY_UP,JOY1_UP
players.1.directions.0.down=KEY_S,KEY_DOWN,JOY1_DOWN
players.1.directions.0.left=KEY_A,KEY_LEFT,JOY1_LEFT
players.1.directions.0.right=KEY_D,KEY_RIGHT,JOY1_RIGHT

players.1.buttons.0=KEY_SPACE,JOY1_A
players.1.buttons.1=KEY_LEFT_CONTROL,KEY_RIGHT_CONTROL,JOY1_B
players.1.buttons.2=KEY_LEFT_SHIFT,KEY_RIGHT_SHIFT,JOY1_X
players.1.buttons.3=KEY_Z,JOY1_Y

system.buttons.0=KEY_ENTER,JOY1_START
system.buttons.1=KEY_ESCAPE,JOY1_SELECT
```

The Default Input Mapping is the useful default for the current FLX version. It
is not a permanent hardware profile and may evolve as Machine Input gains new
capabilities in future versions.

If `input.mapping` is declared explicitly, only that mapping is used.

A missing, unreadable, empty or invalid explicit mapping is a compilation error.
FLX does not fall back to the Default Input Mapping when an explicitly selected
mapping fails.

### Mapping validation

Input Mapping validity is intrinsic to the mapping declaration.

Errors include:

- a mapping file that cannot be opened;
- malformed `key=value` syntax;
- unknown or structurally invalid mapping keys;
- unknown public direction components;
- unknown physical input tokens;
- empty or incomplete bindings;
- duplicate logical bindings;
- an explicit mapping with no effective declarations.

An invalid member of an alternative or combination makes the mapping invalid.
FLX does not silently preserve only the portions of an invalid declaration that
it can understand.

Mapping errors are reported through Diagnostics v2 and make the corresponding
operation fail.

### Machine coverage

Machine capacity does not determine whether an Input Mapping is valid.

When Machine Input and Input Mapping expose different coverage, FLX may report
Diagnostics v2 warnings to inform the developer.

Examples include:

```text
Machine exposes 2 player buttons
Mapping provides 4 player buttons
-> valid mapping
-> warning
```

and:

```text
Machine exposes 4 player buttons
Mapping provides 2 player buttons
-> valid mapping
-> warning
```

Coverage warnings do not make the operation fail. They do not remove mappings,
add bindings or otherwise modify either declaration.

The relevant v0.3.0 Input Mapping diagnostics are:

| Code | Identifier | Severity |
| --- | --- | --- |
| `FLX-INPUT-00010` | `InputMappingCouldNotBeOpened` | error |
| `FLX-INPUT-00011` | `InputMappingMalformedLine` | error |
| `FLX-INPUT-00012` | `InputMappingUnknownKey` | error |
| `FLX-INPUT-00013` | `InputMappingInvalidKey` | error |
| `FLX-INPUT-00014` | `InputMappingPlayerOutOfRange` | warning |
| `FLX-INPUT-00015` | `InputMappingButtonOutOfRange` | warning |
| `FLX-INPUT-00016` | `InputMappingDirectionOutOfRange` | warning |
| `FLX-INPUT-00017` | `InputMappingUnknownDirectionComponent` | error |
| `FLX-INPUT-00018` | `InputMappingUnknownPhysicalToken` | error |
| `FLX-INPUT-00019` | `InputMappingEmptyBinding` | error |
| `FLX-INPUT-00020` | `InputMappingDuplicateBinding` | error |
| `FLX-INPUT-00021` | `InputMappingEmpty` | error |
| `FLX-INPUT-00022` | `InputMappingMachineControlUnmapped` | warning |

The `OutOfRange` identifiers are coverage diagnostics. Their names do not imply
that the mapping is invalid.

### Compilation boundary

Input Mapping source is resolved before Runtime execution.

During source compilation, FLX loads and validates the effective mapping and
prepares the logical mapping used by the project.

Runtime does not reopen or reparse the `.input` source file. `InputSystem`
receives the already prepared mapping and is responsible only for runtime input
state and resolution.

Compiled `.flxc` execution uses the compiled mapping rather than returning to
the original `.input` source.

Source execution and compiled execution therefore share the same effective
Input Mapping contract.

## O. Constants

Input constants are logical component identifiers:

```js
UP
RIGHT
DOWN
LEFT
NEGATIVE
POSITIVE
```

Scripts must not depend on their numeric representation. They are not spatial
movement multipliers.

Use movement helpers such as `move_horizontal`, `move_vertical`, `rotate`,
`advance`, `accelerate` or explicit numeric values for spatial behavior.

For normalized directional intent, scripts SHOULD use:

```js
input_direction(subject, direction(index), HORIZONTAL);
input_direction(subject, direction(index), VERTICAL);
```

`input_direction()` returns `-1`, `0` or `1` for the digital input types
implemented in v0.3.0. In a `2way` direction, both axes project onto the same
logical way so a Machine can later expand to `4way` without forcing script
changes.

In a `4way` direction, axis queries use only their concrete components:
`HORIZONTAL` reads `LEFT` and `RIGHT`; `VERTICAL` reads `UP` and `DOWN`.
`UP` and `DOWN` never affect the horizontal axis, and `LEFT` and `RIGHT` never
affect the vertical axis.

The normalized convention is:

```text
RIGHT = +1
UP    = +1
LEFT  = -1
DOWN  = -1
```

This is input intent, not screen-space movement. A script that wants screen Y
to increase downward should decide that explicitly when applying the intent.

## P. Examples

Button:

```js
const ACTION = button(0);

function action(object) {
    if (input_pressed(player(1), ACTION)) {
        object.local["pressed"] = 1;
    }

    if (input_down(player(1), ACTION)) {
        object.local["down"] = 1;
    }

    if (input_released(player(1), ACTION)) {
        object.local["released"] = 1;
    }
}
```

Direction:

```js
const MOVE = direction(0);

function action(object) {
    if (input_pressed(player(1), MOVE, UP)) {
        object.local["upPressed"] = 1;
    }

    if (input_down(player(1), MOVE, LEFT)) {
        object.local["leftDown"] = 1;
    }
}
```

Object subject:

```js
const FIRE = button(0);

function action(ship) {
    if (input_pressed(ship, FIRE)) {
        spawn(ship, "laser");
    }
}
```

This last form requires `ship` to declare `control.player`.
