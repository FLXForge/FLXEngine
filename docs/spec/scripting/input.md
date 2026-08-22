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
system.buttons = 16
players.count = 16
players.controls.buttons = 16

direction(0):
type = 4way
simultaneous = last
buffer = 0
```

This document is not the full Default Machine specification. A future
`spec/default-machine.md` should define the complete default behavior.

## N. Input Mapping

Input mapping files use a properties-like syntax.

Example for a `4way` direction:

```properties
players.1.directions.0.up=KEY_W,KEY_UP
players.1.directions.0.down=KEY_S,KEY_DOWN
players.1.directions.0.left=KEY_A,KEY_LEFT
players.1.directions.0.right=KEY_D,KEY_RIGHT

players.1.buttons.0=KEY_SPACE

system.buttons.0=KEY_ESCAPE
```

Example for a `2way` direction:

```properties
players.1.directions.0.negative=KEY_W,KEY_UP
players.1.directions.0.positive=KEY_S,KEY_DOWN
```

Mapping files must not define gameplay names:

```properties
# Not part of v0.3.0 mapping
fire=KEY_SPACE
players.1.buttons.fire=KEY_SPACE
```

The current implementation logs warnings and ignores invalid mapping lines.
Structured mapping diagnostics are not part of the v0.3.0 scripting Input
contract.

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
