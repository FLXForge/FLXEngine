# FLX Default Machine Characterization

This document characterizes the current Default Machine used by FLX v0.3.0.
It is an internal characterization document, not a public specification.

## A. Rule

The absence of an explicit `machine=...` entry in a project manifest does not
mean absence of capabilities.

It means:

```text
use the Default Machine of the current FLX version
```

The Default Machine must expose a generic, reasonably permissive set of
capabilities using only features implemented in that version.

## B. Construction Path

When `project.flx` does not declare `machine`, `ProjectCompiler` builds the
compiled context with:

```cpp
MachineLoader::defaultMachine()
```

That effective Machine is then stored in `CompiledProject.context.machine`.
For `.flxc` v4, the Machine is serialized and read back as part of the compiled
project context.

## C. Effective Defaults

### Video

| Field | Value |
| --- | --- |
| `screen.width` | `640` |
| `screen.height` | `480` |
| `screen.color` | `black` |
| `output.scale` | `1` |
| `output.smoothing` | `false` |
| `color.alpha` | `true` |
| `planes.enabled` | `false` |
| `objects.sprites` | `false` |

No palette, color level reduction or tonal color mode is enabled by default.

### Input

| Field | Value |
| --- | --- |
| `system.buttons` | `16` |
| `players.count` | `16` |
| `players.controls.buttons` | `16` |
| `players.controls.directions[0].type` | `4way` |
| `players.controls.directions[0].simultaneous` | `last` |
| `players.controls.directions[0].buffer` | `0` |

This is the consolidated v0.3.0 digital input default.

`direction(0)` exists for player subjects. It is a 4way direction because 4way
is the highest-resolution digital direction implemented in v0.3.0.

The default does not declare `8way`, `analog`, `pointer` or `text`.

### Audio

| Field | Value |
| --- | --- |
| `voices.music` | `8` |
| `voices.sound` | `16` |
| `voices.mode` | `shared` |
| `voices.overflow` | `replace_oldest` |
| `synthesis.model` | `open` |
| `synthesis.texture` | `rich` |
| `synthesis.movement` | `expressive` |
| `synthesis.noise` | `rich` |
| `fidelity.resolution` | `high` |
| `fidelity.dynamics` | `expressive` |
| `fidelity.space` | `stereo` |
| `resources.generated` | `true` |
| `resources.samples` | `true` |
| `resources.streams` | `true` |
| `fileAudio.mode` | `all` |

Audio was inspected during this task but not redesigned.

## D. Input Behavior Without Explicit Machine

A project without explicit Machine can use:

```js
const MOVE = direction(0);

if (input_pressed(player(1), MOVE, UP)) {
    // works without control.player
}
```

`player(1)` is an explicit subject and does not require the runtime object to
declare:

```json
{
  "control": {
    "player": 1
  }
}
```

`control.player` is only required when the script uses a `RuntimeObject` itself
as the input subject:

```js
input_pressed(ship, MOVE, UP)
```

## E. Absence, Defaults And Disabled Capabilities

Current model:

- no `machine` in `project.flx`: use full Default Machine;
- explicit Machine with omitted chip fields: start from Machine defaults and
  override declared fields;
- `input.players.count` cannot be below `1`;
- `input.players.controls.buttons` and `input.system.buttons` can be `0`;
- `input.players.controls.directions` currently cannot express "no directions"
  cleanly. An empty list is treated as an error and a default direction is
  restored;
- audio voices can be set to `0`, which can explicitly remove that voice pool;
- unsupported future input capabilities produce diagnostics instead of being
  silently downgraded.

Open question for the future `default-machine.md` specification:

```text
How should FLX distinguish "not configured, use default" from
"explicitly disabled" for every capability?
```

## F. Regression Coverage

The following behavior is now covered:

- Default Machine input exposes `player(1)`;
- Default Machine input exposes `direction(0)`;
- `direction(0)` is `4way`;
- the simultaneous policy is `last`;
- the direction buffer is `0`;
- a project without `machine=...` can compile and run input mapping for
  `player(1), direction(0)`;
- `player(1)` works from JavaScript without `control.player`;
- `RuntimeObject` subjects still require `control.player`;
- `.flxc` v4 preserves the Default Machine input values.

## G. Audit Result

No production change was required in this branch: the effective Default Machine
already provides the required v0.3.0 digital direction.

The reported 80sLike symptom is therefore guarded here as a regression case:
if a future change removes or empties the default direction list, the new tests
will fail.
