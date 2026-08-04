# Reference Resolution Characterization

This document records the current behavior of FLX reference resolution before
refactoring `JsonLoader`.

## Passing Current Behavior

- REF-001: compiled runtime data does not load JSON during normal object creation.
- REF-002: inline children and relative referenced children compile.
- REF-005: absolute logical JSON object references from `/` resolve from project `path`.
- REF-006: relative JSON references resolve from the JSON file that declares them.
- REF-007: inherited scripts and inherited children from `like` keep the base file as source.
- REF-009: valid first-level internal references with `:` resolve.
- REF-012: `validate` and `compile` share the compiler graph entry point.
- REF-014: two consecutive compilations with different `path` roots do not reuse the previous root cache in the characterized case.

## Failing Desired Contract

- REF-005: scripts declared as absolute logical paths, such as `/scripts/root`, are not resolved from project `path`.
- REF-008: scripts overwritten after `like` still resolve from the base source file, not the consumer file.
- REF-008: children overwritten after `like` still resolve from the base source file, not the consumer file.
- REF-009: missing internal members are logged and skipped instead of becoming compiler diagnostics.
- REF-010: cycles in `like` are not safely tested because the current recursive resolver has no explicit cycle guard at load time.
- REF-011: ResourceId collision diagnostics are not implemented; a deterministic collision was not reproduced with the current `sourcePath#logicalId` model.
- REF-015: missing child references are logged and skipped instead of failing validation/compilation.
- REF-015: diagnostics do not preserve declared reference, declaring file and effective searched path as structured data.

## Ambiguous Cases

- ResourceId collision behavior is ambiguous. `ResourceRegistry::addObject` can detect insertion failure, but `ProjectCompiler` currently ignores the return value.
- Internal reference errors are visible through `Logger`, but not through `Diagnostics`, so CLI consumers cannot reliably act on them.

## `like` Provenance

Inherited references currently keep base-file provenance, which matches REF-007.
Overridden references currently inherit the merged object's `__sourceFile`, which
means overwritten scripts and children can still resolve as if they had been
declared in the base file. That violates REF-008.

## Global State

`JsonLoader` currently owns process-global `projectJsonRoot` and `jsonCache`.
`resolveProjectPath` resets both when the normalized project root changes, so
sequential compilations with different paths work in the characterized case.
The state is still global to the process, not owned by a compilation session,
so it is not ready for parallel compilation or nested tooling.

## Recommended Production Changes

- Move project root, JSON cache and reference chain into a per-compilation resolver.
- Preserve provenance per referenced value instead of per merged object.
- Route loader errors into `Diagnostics`, not only `Logger`.
- Add explicit cycle detection for `like` and internal references.
- Implement stable concrete diagnostics for missing references, missing internal members, cycles, script misses and ResourceId collisions.
- Make `ProjectCompiler` check `ResourceRegistry::addObject` return values and diagnose collisions.
