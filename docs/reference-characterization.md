# Reference Resolution Consolidation

This document records the consolidated behavior of FLX reference resolution
after correcting the gaps found during characterization.

## Passing Current Behavior

- REF-001: compiled runtime data does not load JSON during normal object creation.
- REF-002: inline children and relative referenced children compile.
- REF-005: absolute logical JSON object references from `/` resolve from project `path`.
- REF-005: absolute logical script references from `/` resolve from project `path`.
- REF-006: relative JSON references resolve from the JSON file that declares them.
- REF-007: inherited scripts and inherited children from `like` keep the base file as source.
- REF-008: overwritten scripts after `like` resolve from the consumer file.
- REF-008: overwritten children after `like` resolve from the consumer file.
- REF-009: valid first-level internal references with `:` resolve.
- REF-009: missing first-level internal members fail compilation with diagnostics.
- REF-010: cycles in `like` are detected deterministically.
- REF-012: `validate` and `compile` share the compiler graph entry point.
- REF-014: two consecutive compilations with different `path` roots do not reuse the previous root cache in the characterized case.
- REF-015: missing child references fail compilation with diagnostics.

## Remaining Gaps

- REF-011: ResourceId collision diagnostics are not implemented; a deterministic collision was not reproduced with the current `sourcePath#logicalId` model.
- Diagnostics include declared reference, declaring file, field and effective path in the message. Structured `details` are still pending because the `Diagnostic` model does not expose a details object yet.

## Ambiguous Cases

- ResourceId collision behavior is still hard to exercise through source files. `ProjectCompiler` now checks `addObject` and `addScript` return values, but a deterministic source-level collision fixture is still pending.

## `like` Provenance

Inherited references keep base-file provenance, which matches REF-007.
Overridden `behavior.scripts` and `children` keep explicit consumer-file
provenance, which matches REF-008 without requiring a generic metadata layer
for every JSON value.

## Global State

`JsonLoader` now owns `projectJsonRoot` and `jsonCache` in a per-load session.
`resolveProjectPath` only resolves paths and no longer mutates loader state.
Sequential compilations with different paths are covered by tests, and the
loader no longer relies on shared mutable project root/cache state.

## Recommended Production Changes

- Add structured diagnostic `details` once the Diagnostics model supports it.
- Add a deterministic source fixture for `ResourceIdCollision` if/when the ResourceId model can express one.
- Extend cycle detection to other future reference families if they gain recursive resolution beyond `like`.
