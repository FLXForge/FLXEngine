# FLX Input Mapping microexample

This microexample demonstrates the separation between Machine Input and Input
Mapping in FLX v0.3.0.

It is intentionally not a game.

## Files

- `shared.input`: one physical mapping used unchanged by both Machines.
- `machine-2way.yml`: a Machine with one `2way` direction.
- `machine-4way.yml`: the same logical capacity expanded to `4way`.
- `input.js`: one script used unchanged with either Machine.

## Shared mapping

The public `.input` vocabulary always declares physical directional intent as:

```text
up
right
down
left
```

It does not declare `positive` or `negative`.

The same `shared.input` file is used with both Machine definitions.

## With `machine-4way.yml`

The four mapped directions preserve their identity:

```text
UP    -> UP
RIGHT -> RIGHT
DOWN  -> DOWN
LEFT  -> LEFT
```

`input_direction(..., HORIZONTAL)` observes LEFT/RIGHT.

`input_direction(..., VERTICAL)` observes UP/DOWN.

## With `machine-2way.yml`

The same mapping is projected onto the two logical poles:

```text
UP / RIGHT   -> POSITIVE / +1
DOWN / LEFT  -> NEGATIVE / -1
```

The mapping file does not change.

The JavaScript file does not change.

For a `2way` direction, both `HORIZONTAL` and `VERTICAL` project the same
logical way. This is intentional: the Machine exposes only two logical poles.

## Buttons

The mapping also provides the v0.3.0 default four player buttons and two system
buttons:

```text
button(0): Space / Joy A
button(1): Left or Right Control / Joy B
button(2): Left or Right Shift / Joy X
button(3): Z / Joy Y

system button(0): Enter / Joy Start
system button(1): Escape / Joy Select
```

Machine capacity and mapping coverage are independent. A Machine may expose
more or fewer controls than this mapping. A coverage difference may produce a
warning, but it does not invalidate or trim the mapping.

## What this example demonstrates

```text
Machine changes: 2way <-> 4way
Mapping changes: no
JavaScript changes: no
```

That is the contract being demonstrated by this microexample.
