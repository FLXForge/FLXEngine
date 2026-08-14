# FLX Input Characterization

This document characterizes the current FLX input model without changing
production behavior. It covers the four active layers:

```text
Machine / Input Chip -> input mapping -> Runtime Input -> JavaScript API
```

It is based on the current code, public docs, `flx.d.ts`, examples and tests.
The future model discussed here is only a design hypothesis.

## A. Machine / Input Chip actual

Current C++ model: `InputChipDefinition`.

| Field | Type | Default | Validation | Current meaning |
| --- | --- | ---: | --- | --- |
| `players` | int | 16 | `>= 1` | Maximum valid player index in mapping. Player indexes start at 1. |
| `direction` | string | `analog` | `none`, `2way`, `4way`, `8way`, `analog` | Single shared direction capability for every player. |
| `buttons.player` | int | 16 | `>= 0` | Maximum number of per-player logical buttons. Valid indexes are `0..N-1`. |
| `buttons.system` | int | 16 | `>= 0` | Maximum number of system logical buttons. Valid indexes are `0..N-1`. |
| `pointer` | bool | true | bool parser | Whether the pointer uses real mouse coordinates/buttons. |
| `text` | bool | true | bool parser | Declared capability only; no runtime text API currently consumes it. |

Default Machine is permissive: 16 players, one shared `analog` direction,
16 player buttons, 16 system buttons, pointer and text enabled.

Input Chip is loaded in `MachineLoader`, stored in `FlxContext.machine.input`,
serialized into `.flxc`, and passed to `InputSystem::configure` during Engine
initialization. It currently constrains mapping parsing; it does not create a
per-frame input state object.

### Consumption

| Field | Runtime consumer |
| --- | --- |
| `players` | `InputSystem::validPlayer` rejects mapping outside range. |
| `direction` | `InputSystem::directionMappingAllowed` rejects all direction mapping only when `none`. It does not enforce 2way/4way/8way/analog semantics. |
| `buttons.player` | `InputSystem::validPlayerButton` rejects player button indexes outside range. |
| `buttons.system` | `InputSystem::validSystemButton` rejects system button indexes outside range. |
| `pointer` | `InputSystem::update`, `pointerX/Y`, `pointerDown`, `pointerPressed`. |
| `text` | Loaded and serialized, but no observed runtime consumer yet. |

### Limits and restrictions

- All players share the same direction model and same button count.
- There is one direction set per player: `up`, `down`, `left`, `right`.
- `analog` is accepted as a chip value but is not represented as x/y,
  magnitude, angle or deadzone in runtime.
- `2way`, `4way` and `8way` currently differ only as declared values; mapping
  accepts any of `up/down/left/right` unless direction is `none`.
- `buttons.player` and `buttons.system` are arbitrary non-negative counts in
  data, but JS has no generated `BUTTON_N` constants.

## B. Input mapping actual

The project manifest may declare:

```text
input.mapping=controllers.input
```

`ProjectCompiler` resolves this file relative to the manifest directory and
embeds its text in `FlxContext`. Runtime parses the embedded text.

Current syntax is properties-style:

```text
system.buttons.0=KEY_ESCAPE,KEY_LEFT_ALT+KEY_Q
players.1.direction.up=KEY_W,KEY_UP,JOY1_UP
players.1.buttons.0=KEY_SPACE,JOY1_A
```

Rules observed:

- Empty lines and lines starting with `#` are ignored.
- `key=value` is required.
- `,` separates alternatives.
- `+` joins simultaneous physical inputs into a combination.
- A combination is down only when all physical inputs in it are down.
- A combination is pressed when the full combination is down and at least one
  physical input in it is pressed this frame.
- Mapping keys currently accepted:
  - `system.buttons.<index>`
  - `players.<player>.direction.up`
  - `players.<player>.direction.down`
  - `players.<player>.direction.left`
  - `players.<player>.direction.right`
  - `players.<player>.buttons.<index>`

Mapping is deliberately physical-to-logical. It does not know gameplay names
such as `FIRE`, `JUMP`, `PAUSE` or `START_GAME`.

### Physical tokens

Keyboard tokens are a hardcoded subset:

```text
KEY_A..KEY_Z
KEY_0..KEY_9
KEY_SPACE KEY_ENTER KEY_ESCAPE KEY_TAB KEY_BACKSPACE
KEY_LEFT KEY_RIGHT KEY_UP KEY_DOWN
KEY_LEFT_SHIFT KEY_RIGHT_SHIFT
KEY_LEFT_CONTROL KEY_RIGHT_CONTROL
KEY_LEFT_ALT KEY_RIGHT_ALT
KEY_MINUS KEY_EQUAL KEY_LEFT_BRACKET KEY_RIGHT_BRACKET
KEY_BACKSLASH KEY_SEMICOLON KEY_APOSTROPHE KEY_GRAVE
KEY_COMMA KEY_PERIOD KEY_SLASH
KEY_F1..KEY_F12
```

Gamepad tokens use:

```text
JOY<player>_A
JOY<player>_B
JOY<player>_X
JOY<player>_Y
JOY<player>_START
JOY<player>_SELECT
JOY<player>_UP
JOY<player>_DOWN
JOY<player>_LEFT
JOY<player>_RIGHT
JOY<player>_L1
JOY<player>_R1
```

`JOY1_*` maps to Raylib gamepad device 0, `JOY2_*` to device 1, and so on.
There is no mouse token syntax in `.input` today.

## C. Runtime Input actual

Runtime class: `InputSystem`.

The system stores parsed `InputAction` mappings:

- `systemButtons: unordered_map<int, InputAction>`
- `players: unordered_map<int, PlayerMapping>`
- `PlayerMapping` contains `up/down/left/right` plus indexed `buttons`.

It does not store previous input snapshots. It asks Raylib directly:

- keys: `IsKeyDown`, `IsKeyPressed`
- gamepad buttons: `IsGamepadButtonDown`, `IsGamepadButtonPressed`
- mouse buttons: `IsMouseButtonDown`, `IsMouseButtonPressed`
- mouse position: `GetMouseX`, `GetMouseY`

`Engine` calls `inputSystem.update(delta, screenWidth, screenHeight, screenArea)`
once per frame before script update. This update only refreshes pointer
coordinates or virtual pointer coordinates.

## D. API JavaScript actual

Public facade installed by `InputBindings`:

| API | Return | Semantics |
| --- | --- | --- |
| `Input.system.down(index)` | boolean | Logical system button is held. |
| `Input.system.pressed(index)` | boolean | Logical system button was pressed this frame. |
| `Input.player(n)` | object | Builds a small JS facade for player `n`. |
| `Input.player(n).up()` | boolean | Mapped up direction is held. |
| `Input.player(n).down()` | boolean | Mapped down direction is held. |
| `Input.player(n).left()` | boolean | Mapped left direction is held. |
| `Input.player(n).right()` | boolean | Mapped right direction is held. |
| `Input.player(n).button(index)` | boolean | Logical player button is held. |
| `Input.player(n).pressed(index)` | boolean | Logical player button was pressed this frame. |
| `Input.pointer.x()` | number | Pointer X in logical FLX coordinates. |
| `Input.pointer.y()` | number | Pointer Y in logical FLX coordinates. |
| `Input.pointer.down(index)` | boolean | Pointer button is held. |
| `Input.pointer.pressed(index)` | boolean | Pointer button was pressed this frame. |

Implementation detail currently exposed globally:

```text
__flx_input_system_down
__flx_input_system_pressed
__flx_input_player_up
__flx_input_player_down
__flx_input_player_left
__flx_input_player_right
__flx_input_player_button
__flx_input_player_pressed
__flx_input_pointer_x
__flx_input_pointer_y
__flx_input_pointer_down
__flx_input_pointer_pressed
```

These are not declared in `flx.d.ts` and should be treated as accidental
infrastructure exposure.

## E. Buttons

Runtime representation is index based. There is no conceptual gameplay meaning
in C++ or mapping.

Current player button APIs:

- held: `Input.player(n).button(index)`
- pressed: `Input.player(n).pressed(index)`

Current system button APIs:

- held: `Input.system.down(index)`
- pressed: `Input.system.pressed(index)`

There is no `released` query.

The model can store high button indexes as long as the Machine declares enough
buttons. The public language does not currently expose `BUTTON_0` constants,
therefore a future `BUTTON_126` problem is not solved by the current API. Games
currently use local constants such as:

```js
const FIRE_BUTTON = 0;
```

Future options to evaluate, without deciding now:

- numeric indexes remain the public representation;
- `button(index)` returns a descriptor;
- dynamic constants are generated by the host;
- logical handles are created by a separate API.

Static finite constants such as `BUTTON_0..BUTTON_15` would conflict with a
Machine that allows arbitrary button counts.

## F. Digital directions

Runtime has exactly four named direction actions per player:

```text
up, down, left, right
```

JS exposes them only as held-state methods:

```js
Input.player(1).up()
Input.player(1).down()
Input.player(1).left()
Input.player(1).right()
```

There is no current way to ask:

- up pressed;
- up released;
- direction component of `DIRECTION_0`;
- second direction control.

Because directions call `actionDown`, holding a direction remains true every
frame. This explains menu locks implemented manually: a menu that moves on
`Input.player(1).up()` will continue moving while the key is held unless script
code adds a lock/cooldown.

## G. Analog directions

`input.direction: analog` is accepted and is the default, but current runtime
does not expose analog values. There is no:

- x;
- y;
- magnitude;
- angle;
- deadzone;
- analog-to-digital transition policy.

The current `analog` value behaves like "direction mappings are allowed" rather
than a real analog control.

## H. System

System input currently means indexed system buttons only:

```js
Input.system.down(index)
Input.system.pressed(index)
```

It is not a callable selector. There is no `system()` subject object today.
System has no pointer, text or direction facade.

A future `system()` subject for:

```js
input_pressed(system(), BUTTON_0)
```

would need a JS descriptor or special handle that does not pretend to be a
`RuntimeObject`.

## I. Pointer

When `chip.pointer` is true:

- position comes from the real mouse;
- coordinates are converted into logical FLX space using the current screen
  area;
- values are clamped to `0..screenWidth` and `0..screenHeight`;
- buttons call Raylib mouse button APIs directly.

When `chip.pointer` is false:

- a virtual pointer starts at the logical screen center;
- player 1 direction moves it at a hardcoded 160 logical units per second;
- pointer button 0 maps to player 1 button 0;
- other pointer buttons are false.

No mapping file syntax exists for mouse buttons, pointer axes, wheel or pointer
delta. Wheel is not represented.

## J. Text

`input.text` is loaded, validated and serialized, but no JavaScript text input
API was found. Keyboard as physical buttons and text entry are therefore not
separated in runtime yet; only the design intent is present in Machine data.

## K. Temporality: pressed / down / released

Current runtime can answer:

- held/down for system buttons, player buttons, directions and pointer buttons;
- pressed for system buttons, player buttons and pointer buttons.

Current runtime cannot answer:

- released for any logical control;
- pressed/released for direction components.

`pressed` currently delegates to Raylib `IsKeyPressed` /
`IsGamepadButtonPressed` / `IsMouseButtonPressed`. For combinations, FLX
requires the full combination to be down and any physical input inside it to
have been pressed this frame.

A future `released` API probably needs explicit previous/current logical state
snapshots. Raylib has physical release queries, but logical release for
alternatives and combinations should be derived consistently at the FLX layer.

## L. RuntimeObject control

No `control` block exists in `ObjectDefinition` or `RuntimeObject` today.
Objects do not know which player controls them. Scripts directly select a
player:

```js
if (Input.player(1).pressed(0)) {
    spawn(ship, "laser");
}
```

Conceptual future block:

```json
"control": {
  "player": 1
}
```

Viability notes:

- It belongs to world/runtime relation, not to mapping.
- It should not declare gameplay meanings.
- It would be inherited by `like` and copied through spawn like other object
  declaration data unless explicitly designed otherwise.
- `RuntimeObject` would need either a local control source or a definition
  reference lookup.
- If player does not exist in Machine, compile-time diagnostic is preferable to
  silent runtime false.
- Mutability is an open decision. Static control is simpler; dynamic reassignment
  becomes a runtime relation API.

## M. `player()` / `system()` subjects

Current `Input.player(n)` returns a JS facade object. It is not a general input
subject and is not passed to generic functions.

The conceptual API:

```js
input_pressed(ship, BUTTON_0)
input_pressed(player(1), BUTTON_0)
input_pressed(system(), BUTTON_0)
```

would need one of:

- object subjects resolved through `object.control.player`;
- explicit player descriptors;
- explicit system descriptors;
- a common internal subject variant.

The developer should not need `runtimeId` for input.

## N. Scalability: `BUTTON_N` / `DIRECTION_N`

Buttons:

- Machine can declare any non-negative button count.
- Runtime maps integer button indexes.
- JS currently uses integer indexes.
- No finite `BUTTON_N` constants exist.

Directions:

- Machine currently declares one direction family per player.
- Runtime hardcodes `up/down/left/right`.
- No `DIRECTION_0`, `DIRECTION_1` descriptors exist.

Current code cannot represent `DIRECTION_12` because directions are named fields
inside `PlayerMapping`, not indexed controls. Button scalability is better than
direction scalability, but the public language still needs a deliberate decision.

## O. Real uses

Examples using current normalized `Input`:

- Pong: `Input.player(1).up()` / `down()` for paddle movement.
- Asteroids: `up()` for thrust, `left/right()` for rotation,
  `pressed(FIRE_BUTTON)` for laser.
- Arkanoid: `left/right()` for paddle movement, `pressed(FIRE_BUTTON)` for
  title/ball actions.

Examples still using obsolete API:

- `examples/invaders/player/player.js`: `Key.down(KEY_LEFT)`,
  `Key.down(KEY_RIGHT)`, `Key.pressed(KEY_SPACE)`.
- `examples/scripting/timer/timer-demo.js`: `Key.pressed(KEY_SPACE/P/R/S)`.

No current examples were found using:

- `Input.system.*`;
- `Input.pointer.*`;
- `Input.player(n).button(index)`;
- multiple players;
- analog values;
- text input;
- mouse wheel.

The search did not find a checked-in `80sLike` directory in this repository.

## P. Real debt

- `Input.direction` chip values beyond `none` are mostly declarative; 2way/4way/
  8way/analog are not enforced or represented differently.
- `analog` is not a real analog input model yet.
- `input.text` is declared but unused.
- Directions have held-state only; no transition API.
- Runtime does not maintain logical previous/current state.
- Pointer fallback to virtual pointer is an implicit behavior tied to player 1.
- Internal `__flx_input_*` functions are public globals by accident.
- Old `Key.*` examples remain and are not implemented in the current API.
- Mapping parser accepts hardcoded physical token sets.
- Mapping parsing warnings go through `Logger`, not structured Diagnostics.

## Q. Limitations of the current model

- No asymmetric players.
- No multiple direction controls per player.
- No indexed directions.
- No analog vector data.
- No release detection.
- No object-bound control source.
- No mouse mapping grammar.
- No text input API.
- No deterministic input tests for physical pressed/down without a device
  abstraction or Raylib input seam.

## R. Future design hypothesis

The conceptual layering is coherent:

```text
Chip: what logical controls can exist.
Mapping: which physical inputs feed those logical controls.
JSON: which runtime object is associated with which logical source.
JS: what gameplay meaning each logical control has.
```

A candidate Machine shape could separate system and players:

```yaml
input:
  system:
    buttons: 2
    pointer: false
    text: false

  players:
    count: 1
    controls:
      directions:
        - 4way
      buttons: 2
```

This would improve:

- system/player separation;
- future multiple directions;
- future asymmetric player capabilities;
- eventual `DIRECTION_N` descriptors.

It would require changes in:

- `InputChipDefinition`;
- `MachineLoader`;
- `.flxc` codec;
- `InputSystem::PlayerMapping`;
- mapping grammar;
- JS facade;
- documentation and schemas.

## S. Questions requiring design decision

1. Should direction controls become indexed descriptors like buttons?
2. Should button constants exist at all, or should numeric indexes remain valid
   and enough?
3. Should `down` remain the public word for held state, or should future grammar
   use `held`/`active`?
4. Should directions support `pressed/released` as digital components?
5. Should analog directions generate digital components automatically?
6. Where should deadzone live: Machine, mapping, runtime policy or JS helper?
7. Should `control.player` be immutable definition metadata or mutable runtime
   relation?
8. What should happen if an object references a player not available in Machine?
9. Should pointer fallback through player 1 remain, or should it be explicit?
10. Should text input be part of `Input` or a separate text subsystem?
11. Should internal `__flx_input_*` globals be hidden before expanding the API?
12. Should old `Key.*` examples be migrated now or deliberately left as legacy
    fixtures until Input consolidation?

## Evidence added

Characterization tests:

- `flx-input-characterization-tests`
- default Machine input is permissive;
- current Input Chip shape loads;
- invalid Input Chip values fall back with warnings;
- mapping accepts current keyboard/gamepad/combination syntax;
- mapping is constrained by Input Chip limits;
- mapping rejects gameplay names.

Existing evidence:

- `flx-runtime-lifecycle-tests` verifies the public `Input` facade is registered.
- `flx-compiler-project-tests` verifies `input.mapping` is embedded by the
  compiler.
- `docs/scripting-api-characterization.md` records the public scripting surface
  and accidental `__flx_input_*` globals.
