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

Generate and verify on Windows after building:

```powershell
build/Release/flx-format12-portability-tests.exe --write-fixture tests/fixtures/format12-windows.flxc
build/Release/flx-format12-portability-tests.exe --read-fixture tests/fixtures/format12-windows.flxc
Get-FileHash tests/fixtures/format12-windows.flxc -Algorithm SHA256
```

Adjust the executable path for a single-configuration Ninja build. Updating
the fixture also requires updating its hash and recording the new provenance.
The checked-in file must remain binary in Git.

The Linux GCC and Clang jobs export their own generated fixtures with the
diagnostic artifacts. After downloading an artifact, verify the reverse
direction on Windows with the same executable:

```powershell
build/Release/flx-format12-portability-tests.exe --read-fixture format12-linux-GCC.flxc
build/Release/flx-format12-portability-tests.exe --read-fixture format12-linux-Clang.flxc
```

The reverse direction is prepared and requires actual Linux artifacts.
