# FLX state machine characterization

This document records the consolidated state-machine contract after the
runtime and compiler alignment work.

## A. Consolidated Rules

- `states` is optional. If it is absent, the object has no state machine.
- An object without a state machine has no implicit empty state:
  - `state_current(object)` returns `""`.
  - `state_active(object, "")` returns `false`.
  - `state_entered(object)` returns `false`.
  - `state_time(object)` returns `0`.
  - `state_to(object, name)` logs a warning and does not create states.
- If `states` exists, `initial` is required, must be a non-empty string, and
  must reference a declared state.
- Every declared state must be an object.
- `next` is optional. Missing `next` and `next: []` both mean terminal state.
- Every `next` value must be a non-empty string and must reference a declared
  state.
- Duplicate `next` entries are invalid.
- Single-state machines are valid.
- Cycles are valid.
- Self-transition is valid only when explicitly declared in `next`.
- Invalid state declarations fail during compilation.
- Runtime transitions are requested with `state_to(object, stateName)`.
- The legacy `state` transition API is not registered.
- `state_to` validates the transition against the current state's `next` list.
- Invalid runtime transitions are ignored and logged as warnings.
- `state_entered(object)` is true during the first full runtime frame after the
  object enters its current state.
- `state_entered(object)` is not true inside the same callback that called
  `state_to`.
- `state_time(object)` is reset to `0` when entering a state.
- During the first full frame of a state, `state_time(object)` reports `0`.
- State time starts accumulating after that first full frame completes.
- States do not define callbacks or actions. JSON only declares states and
  valid transitions; JavaScript decides behavior.

## B. Runtime Model

`ObjectDefinition` owns the immutable state-machine declaration:

- `initialState`
- `stateTransitions`

`RuntimeObject` owns only live state:

- `state`
- `stateTime`
- `stateEnteredFrame`
- `definitionId`

`RuntimeObject` no longer stores a copy of `stateTransitions`. State bindings
resolve the object's `definitionId` through the compiled `ResourceRegistry` and
validate transitions against the compiled definition.

This keeps the runtime instance focused on live state while the compiled
project remains the source of declarative truth.

## C. Validation

The loader/compiler reports state-machine declaration errors before runtime.
Implemented diagnostic identities:

- `FLX-RESOURCE-00015 InvalidStateMachineDeclaration`
- `FLX-RESOURCE-00016 MissingStateMachineInitialState`
- `FLX-RESOURCE-00017 MissingStateMachineState`
- `FLX-RESOURCE-00018 InvalidStateTransitionTarget`
- `FLX-COMP-00016 CompiledProjectInvalidStateMachine`

Runtime warnings remain logger warnings for invalid `state_to` calls. Runtime
diagnostic identities are reserved for host/runtime result flows:

- `FLX-RUNTIME-00016 RuntimeObjectMissingStateMachine`
- `FLX-RUNTIME-00017 RuntimeInvalidStateTransition`

## D. Confirmed Tests

The test suite confirms:

- Valid single terminal state compiles.
- Invalid `states` type fails.
- Missing `initial` fails.
- Empty `initial` fails.
- `initial` pointing to an undeclared state fails.
- Non-object state entry fails.
- Non-array `next` fails.
- Missing `next` target fails.
- Objects without `states` do not expose an active empty state.
- `state_to` transitions are visible through `state_entered` in the next full
  runtime frame.
- `state_time` is zero in the first full frame after transition.
- Declared self-transition is observable on the next frame.
- Terminal state rejects outgoing transitions not declared in `next`.
- Public scripting surface registers `state_to` and does not depend on
  `state`.

## E. Scripting Grammar Contrast

The consolidated scripting API favors explicit verbs:

```js
state_to(game, "playing");
state_current(game);
state_active(game, "playing");
state_entered(game);
state_time(game);
```

The old function named `state` is intentionally removed because it used a noun
as a command and obscured that the operation is a validated transition request.

## F. Future Pending Rules

These areas remain deliberately outside this consolidation:

- State transition callbacks.
- State-local `born/action/motion/draw/dead` hooks.
- Guards or conditions declared in JSON.
- Hierarchical states.
- Global scene state machines.
- Runtime diagnostics emitted from script callbacks as structured
  `EngineResult` diagnostics.
- Editor visualizations of the transition graph.
- Serialization/versioning migration policy for future public `.flxc` formats.

## G. Risks Removed

- No implicit empty-string state for objects without `states`.
- No silent acceptance of malformed state declarations.
- No duplicate declarative FSM metadata copied into each `RuntimeObject`.
- No public legacy `state` alias.
- No immediate `state_entered` inside the same callback that requested a
  transition.
- No accidental self-transition unless it is explicitly declared.

## H. Open Questions

- Should invalid `state_to` calls eventually produce structured runtime
  diagnostics instead of only logger warnings?
- Should `state_entered` be queryable for a named state, for example
  `state_entered(object, "playing")`, or should the current simple API remain?
- Should state machines become reusable resources with their own schema, or is
  object-local declaration enough for v0.3.x?
- Should future scene orchestration use object states, a separate program
  lifecycle script, or both?
