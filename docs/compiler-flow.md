# FLX Project Compilation Flow

This document records the current source-to-runtime flow while FLX transitions
towards an in-memory compilation step.

## Current Flow

```text
flx CLI project.flx path
-> ProjectCompiler
-> FlxContextBuilder
-> MachineLoader
-> JsonLoader
-> CompiledProject + ResourceRegistry
-> Engine
-> RuntimeWorld
-> RuntimeObjectBuilder
```

Compiled execution uses the same runtime path:

```text
Compiled .flxc file
-> CompiledProjectReader
-> CompiledProject + ResourceRegistry
-> Engine
-> RuntimeWorld
-> RuntimeObjectBuilder
```

## Responsibilities

- CLI: parses `flx [command] [options] [target]`, resolves the project manifest
  deterministically and asks `ProjectCompiler` to compile it.
- ProjectCompiler: creates the effective in-memory project, discovers the descriptive resource graph and validates script paths. It does not run the game.
- FlxContextBuilder: reads project properties, resolves project-relative paths, loads the Machine and applies screen defaults/overrides.
- MachineLoader: loads YAML Machine and chip definitions, including external chip files.
- JsonLoader: loads JSON objects, resolves `like`, block references, FLX-root references and child definitions.
- ObjectDefinition: represents resolved source object declarations.
- CompiledProject: stores the effective `FlxContext`, root JSON path, root resource id and `ResourceRegistry`.
- ResourceRegistry: stores compiled `ObjectDefinition` entries by stable logical resource id. Children and manual spawns point to resource ids instead of asking the runtime to load JSON.
- ScriptResource: stores JavaScript source code by resource id. QuickJS bytecode is not generated yet.
- Engine: starts runtime systems from `CompiledProject`; it no longer opens `project.flx`.
- RuntimeWorld: owns live runtime objects and creates them from definitions already present in `ResourceRegistry`.
- RuntimeObjectBuilder: converts `ObjectDefinition` into `RuntimeObject`.

## Where Things Happen Today

- Loading: `FlxContextBuilder`, `MachineLoader` and `JsonLoader`.
- Path resolution: project paths in `FlxContextBuilder`; JSON and FLX references in `JsonLoader`.
- References: `JsonLoader`.
- `like` inheritance and object merge: `JsonLoader`.
- Resource graph discovery: `ProjectCompiler`.
- Runtime child/spawn/grid instantiation: `RuntimeWorld` asks `ResourceRegistry` for compiled definitions by resource id.
- Script path resolution: `ProjectCompiler`.
- Script source loading: `ProjectCompiler`; runtime evaluates embedded script source from `ResourceRegistry`.
- Input mapping loading: `ProjectCompiler`; runtime parses embedded mapping text when available.
- Defaults: Machine defaults in `MachineLoader`; project screen defaults/overrides in `FlxContextBuilder`; object defaults in `ObjectDefinition`.
- Validation: currently split between loaders and schemas; compiler diagnostics wrap fatal load failures.
- RuntimeObject creation: `RuntimeObjectBuilder` and `RuntimeWorld`.

## Boundary

`ProjectCompiler` is now the only project source entry point. Runtime code should
receive `CompiledProject` data and should not open or interpret `project.flx` or
load JSON object definitions. JSON remains a compiler input, not a runtime input.

After compilation:

- root creation uses `CompiledProject.rootId`;
- automatic children use compiled child resource ids;
- manual `spawn()` uses compiled child resource ids;
- `creation.grid` resolves pattern entries through compiled child resource ids;
- scripts are loaded from paths resolved during compilation.
- scripts are evaluated from source embedded in `CompiledProject`;
- input mapping is parsed from text embedded in `CompiledProject`.

## Compiled File

The temporary compiled format uses the `.flxc` extension. It is a binary,
versioned internal format with:

- FLX magic;
- format version;
- engine compatibility string;
- effective runtime context;
- effective Machine definition;
- root resource id;
- resource registry;
- resolved object definitions;
- embedded JavaScript source.

It does not contain unresolved `like` declarations, source JSON, source YAML or
live runtime state. Source paths are kept only as relative diagnostic names.

Current commands:

```text
flx compile --output=pong.flxc examples/pong.flx
flx run-compiled pong.flxc
flx run examples/pong.flx
flx examples/pong.flx
```

## Smoke Execution

The runner accepts an optional bounded-frame mode:

```text
flx run --frames=10 examples/pong.flx
```

This compiles the project, starts the normal runtime, executes a fixed number of
frames and shuts down cleanly. It is intended for smoke tests where a graphical
environment is available.
