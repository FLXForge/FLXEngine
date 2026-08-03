# FLX Engine

> Lightweight 2D game development, forged around simplicity and direct creation.

<img align="left" style="width:260px" src="flxlogo.png" width="260px">

FLX Engine is a lightweight 2D game engine focused on clear architecture, direct workflows and minimal friction.

The project is designed for developers who want to build games without depending on oversized editors, proprietary pipelines or unnecessarily complex architectures.

---

## Philosophy

FLX focuses on:

- Lightweight workflows
- Clear architecture
- Fast iteration
- Direct control
- Standard technologies
- Minimal overhead

The goal is simple:

> Build games without fighting the engine.

FLX is especially suited for:

- 2D games
- Arcade projects
- Prototypes
- Visual novels
- Experimental ideas
- Small and medium-sized productions

---

## Minimal Example

```json
{
  "$schema": "https://flxforge.github.io/FLXEngine/schemas/object.schema.json",

  "origin": {
    "x": 320,
    "y": 160
  },

  "shape": {
    "type": "triangle",
    "color": "WHITE",
    "size": {
      "width": 18,
      "height": 24
    }
  }
}
```

```js
/// <reference path="https://flxforge.github.io/FLXEngine/scripts/flx.d.ts" />

function motion(ship) {
    advance(ship);
}
```

FLX keeps game structure simple, readable and easy to modify.

JSON files describe FLX objects. Objects gain capabilities from the
properties they declare: `shape` makes them drawable, `collision` makes
them collide, `behavior` attaches scripts and `children` declares what
can exist below them.

Objects can also declare local sounds:

```json
{
  "sounds": {
    "beep": {
      "kind": {
        "source": {
          "type": "oscillator",
          "wave": "pulse",
          "duty": 0.35
        },
        "note": 880
      },
      "duration": 0.08,
      "volume": 0.7
    }
  }
}
```

Sounds can also use a musical note instead of a raw frequency:

```json
{
  "sounds": {
    "coin": {
      "kind": {
        "source": {
          "type": "oscillator",
          "wave": "square"
        },
        "note": "C5"
      },
      "duration": 0.08,
      "volume": 0.4
    }
  }
}
```

Objects can declare generated music:

```json
{
  "music": {
    "theme": {
      "tempo": 120,
      "loop": true,
      "channels": {
        "lead": {
          "instrument": {
            "source": {
              "type": "oscillator",
              "wave": "square"
            }
          },
          "volume": 0.35,
          "length": "1/8",
          "notes": ["C4", "E4", "G4", "C5"]
        }
      }
    }
  }
}
```

Scripts can play declared audio and trigger screen fades:

```js
play_sound(ship, "beep");
play_music(game, "theme");

fade_on();
fade_off("#000000");
```

---

## Built Around Standards

FLX avoids unnecessary proprietary systems.

Projects are built using familiar technologies:

- JSON for project and scene data
- JavaScript for gameplay scripting
- Standard asset formats
- No mandatory custom editor
- No proprietary scripting language

You can work using your preferred tools and workflows.

---

## The `.flx` Project File

Every FLX project starts with a single entry point:

```text
MyGame.flx
```

This file defines project metadata, runtime configuration, the root object and,
optionally, the Machine YAML used by the project.

```text
machine=machines/standard.yml
input.mapping=input/default.input
window.mode=window
debug.console=false
```

If no Machine is declared, FLX uses an internal default Machine compatible with
the current runtime behavior.

`window.mode` can be `window` or `fullscreen`. `debug.console` controls runtime
console output and defaults to `false`; `debug.logs` remains a separate switch
for internal debug traces. On Windows, a build without a physical console window
can be produced by configuring CMake with `FLX_WINDOWS_SUBSYSTEM=ON`.
Fullscreen keeps the video chip logical resolution and scales it to the physical
display while preserving aspect ratio.

The normalized JavaScript input API uses an explicit mapping file:

```text
system.buttons.0=KEY_ESCAPE
players.1.direction.left=KEY_A,JOY1_LEFT
players.1.direction.right=KEY_D,JOY1_RIGHT
players.1.buttons.0=KEY_SPACE,JOY1_A
```

Scripts read this through `Input.system` and `Input.player(index)`. FLX does not
create an implicit mapping when `input.mapping` is missing.

JSON files can reference reusable project resources with FLX-root paths. The
leading slash points to the manifest `path`, not to the operating system root:

```json
{
  "note": "/music/notes:a",
  "shape": "/ui/title_shape"
}
```

Machine YAML can define video, audio and input chips. The audio chip describes
machine sound capabilities such as voice budgets, overflow policy, synthesis
character, fidelity and external audio resource support. Runtime support applies
voice limits before generating waves. In `reserved` mode music and sound voices
stay separate; in `shared` mode they form a common pool and `steal_from_music`
can temporarily pause music so a sound effect can play. Synthesis/fidelity
settings shape generated oscillators. File audio resources are validated but not
played yet.

---

## Compiled Projects

FLX can produce a temporary compiled project file for testing the future build
pipeline:

```text
flx compile --output=pong.flxc examples/pong.flx
flx run-compiled pong.flxc
```

The compiled file is binary and versioned. It contains the effective project
context, Machine, object registry, declarative sounds/music and embedded
JavaScript source. It is not the final packaging system yet: there is no
compression, cache, bytecode, atlas, sprite compiler or standalone game
executable in this step.

## CLI

The executable is `flx.exe` and the command form is:

```text
flx [command] [options] [target]
```

The default command is `run` and the default target is the current directory, so
`flx`, `flx .` and `flx run .` are equivalent. Use `flx --help` for available
commands and `flx --version` for the semantic FLX version read from `VERSION`.

---

## Current Status

FLX Engine is currently under active development.

The project is evolving progressively with a strong focus on maintaining architectural clarity and simplicity from the very beginning.

Some systems may evolve during the early 0.x versions.

---

## FLX Forge

FLX Engine is part of the broader **FLX Forge** initiative.

The idea behind Forge is simple:
create lightweight and understandable tools focused on direct game creation.

Additional tooling may appear progressively as the project evolves.

---

## Why FLX?

Because small projects deserve solid tools too.
