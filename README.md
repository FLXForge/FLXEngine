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
  "$schema": "../../../tools/schemas/drawable.schema.json",
  "name": "Ship",

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
function motion(ship) {
    advance(ship);
}
```

FLX keeps game structure simple, readable and easy to modify.

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

This file defines the project structure, scenes, scripts and engine configuration.

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