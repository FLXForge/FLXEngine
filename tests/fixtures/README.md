# Format-12 Windows fixture

`format12-windows.flxc` was generated on Windows x86-64 using MSVC 19.51,
Release, FLX 0.4.0. Its writer is the production `flx-core` target.
The input model is defined in `tests/portability/Format12PortabilityTests.cpp`.
The engine baseline is commit `fc02ab8`; the generator is included alongside
this fixture in the working tree. Regeneration must be performed on Windows
to retain the stated provenance.

The companion SHA-256 file is checked by the Linux workflow before compilation.
The fixture has no external source-file requirements. Verification uses a
headless production session, checks format 12 and resource relationships,
executes the embedded JavaScript and checks deterministic boolean, numeric
and UTF-8 results through persistence in a temporary directory.

The original format-12 writer predates the current codec. Do not regenerate
this fixture with the current writer: it emits format 14. Verification remains
available with `flx-format12-portability-tests --read-fixture <file>`.
The checked-in file and its hash remain unchanged.

The Linux GCC and Clang jobs export their own generated fixtures with the
diagnostic artifacts. After downloading an artifact, verify the reverse
direction on Windows with the same executable:

```powershell
build/Release/flx-format12-portability-tests.exe --read-fixture format12-linux-GCC.flxc
build/Release/flx-format12-portability-tests.exe --read-fixture format12-linux-Clang.flxc
```

The reverse direction is prepared and requires actual Linux artifacts.

## Format-13 Windows fixtures

`format13-windows.flxc` was generated before this phase changed the writer,
using the pre-existing MSVC Release `v040-audit-release` portability executable
(producer FLX 0.4.0). It preserves the same exchange model as
the format-12 test. `format13-bitmap-windows.flxc` was generated with the same
pre-existing CLI and contains one bitmap row `[red, blank]`; it verifies the real
format-13 bitmap layout, not a format-14 file with a patched version byte.
Both are immutable legacy inputs. Their hashes are recorded alongside them.

## Animation declaration fixture

`animation/declaration.json` is a minimal parser/compiler fixture: sparse keys,
inactive color/bitmap key, local palette, empty representation and base persist.
It is not a playable Animation example. Playback, JS and animated rendering are
not implemented in phase 1. The JSON can later seed a didactic microexample.

## CPU temporal evaluation fixtures (Animation phase 2)

`animation/temporal-loop.json` uses the actual object declaration syntax from
phase 1: a loop+current animation with offset, rotation and scale. It has no JS
or playable project yet. `animation/temporal-loop.trace.json` is test data with
hand-calculated deltas and expected poses, not public FLX syntax. The standalone
CPU suite and the source/format-14 equivalence test share those expectations.
The fixture can later seed one didactic microexample after playback/rendering
integration; no current visual capability is claimed.

Architecture, numeric recovery and integration limits are documented in
[the evaluator implementation note](../../docs/ANIMATION_EVALUATOR.md).

## Animation renderer fixture (phase 3)

`animation/renderer-object.json` is a real object declaration with palette,
persistent elements, mixed replacement, color and combined visual transforms.
Graphics tests compile it, write/read format 14 and compare real rendered pixels
using only internal C++ playback calls. It is not advertised as an executable
JavaScript Animation example. The separate `microexamples/animation/persist-order`
project is executable now: it demonstrates the static grouping without playback.
