# FLX Input Mapping microexample

Minimal v0.3.0 language example for `examples/language/input`.

This is not a game. Its purpose is to demonstrate that Machine Input and Input
Mapping are separate concerns.

## Files

- `input-4way.flx` — uses the v0.3.0 Default Machine (`direction(0)` is 4way).
- `input-2way.flx` — uses `../../machines/pongMachine.yml` (`direction(0)` is 2way).
- `input-demo.json` — the same RuntimeObject definition for both projects.
- `input-demo.js` — the same JavaScript for both projects.
- `shared.input` — the same physical mapping for both projects.

## Run

From `examples/language/input`:

```text
flx input-4way.flx
flx input-2way.flx
```

Both projects use the same:

```text
input-demo.json
input-demo.js
shared.input
```

Only the Machine changes.

## Shared directional mapping

```properties
players.1.directions.0.up=KEY_W,KEY_UP,JOY1_UP
players.1.directions.0.down=KEY_S,KEY_DOWN,JOY1_DOWN
players.1.directions.0.left=KEY_A,KEY_LEFT,JOY1_LEFT
players.1.directions.0.right=KEY_D,KEY_RIGHT,JOY1_RIGHT
```

The public `.input` vocabulary always uses `up`, `right`, `down` and `left`.

## 4way

`input-4way.flx` uses the Default Machine.

The mapping keeps all four logical directions:

```text
UP    -> UP
RIGHT -> RIGHT
DOWN  -> DOWN
LEFT  -> LEFT
```

Therefore:

```js
input_direction(object, MOVE, HORIZONTAL)
```

reads LEFT/RIGHT, while:

```js
input_direction(object, MOVE, VERTICAL)
```

reads UP/DOWN.

## 2way

`input-2way.flx` changes only the Machine:

```text
machine=../../machines/pongMachine.yml
```

The same public mapping is projected onto the two logical poles:

```text
UP / RIGHT   -> POSITIVE / +1
DOWN / LEFT  -> NEGATIVE / -1
```

For a 2way direction, both HORIZONTAL and VERTICAL observe that same logical
axis.

No `.input` change is required.
No JSON change is required.
No JavaScript change is required.

## Contract demonstrated

```text
Machine:     changes 2way <-> 4way
Mapping:     unchanged
JSON:        unchanged
JavaScript:  unchanged
```

That independence is the point of the example.

The 2way project expects this directory to be placed at:

```text
examples/language/input
```

inside the FLXEngine repository so that the relative Machine reference resolves
to the existing canonical `examples/machines/pongMachine.yml`.
