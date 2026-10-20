# Code signing policy

## Status and project identity

FLX Engine is an open-source, declarative 2D game engine maintained by one
developer. The official repository is
[FLXForge/FLXEngine](https://github.com/FLXForge/FLXEngine). It is distributed
under the [MIT license](../LICENSE); upstream components retain their own licenses.
The license identifies Rafael S. Ocasar as the copyright holder. FLX Forge is the
project identity, not a claim that a separate legal company exists.

This document prepares a future SignPath Foundation application. The project
has not claimed SignPath approval or activated signing in these workflows.
This policy does not claim that existing binaries are signed. Windows version
metadata and SHA-256 hashes are not digital signatures.

## Official distribution channels

Official Windows release downloads are the assets attached to
[GitHub Releases](https://github.com/FLXForge/FLXEngine/releases) in the official
repository. Source is available from the same repository. Documentation is
published at [the project website](https://flxforge.github.io/FLXEngine/).
Development workflow artifacts are CI outputs, not official releases. Mirrors,
forks and third-party packages are not authenticated by this policy.

The Windows distribution keeps the existing standalone `flx.exe` and ZIP layout:
`flx.exe`, `examples/`, `schemas/`, `scripts/flx.d.ts`, README, LICENSE and the five
example launchers at the ZIP root. No installer or new runtime dependency is added.

## Governance and release authorization

One maintainer performs development, contribution review and release authorization.
There is no independent human review or separation of duties between these roles.
Automated tests are not a substitute for human review. Outside contributions must
be reviewed by the maintainer before inclusion in an authorized release.

Release authorization is the maintainer's decision to publish a versioned tag
whose commit is contained in `main`. GitHub account/repository permissions control
who can push tags and manually dispatch workflows; repository files alone cannot
prove those settings. Manual dispatch rebuilds a specified release tag and must
pass the same validations. Validation helpers are checked out separately from
the workflow commit, so rebuilding an older source tag does not require that tag
to contain the new helper files. Existing assets may be overwritten by a successful
manual rerun, as in the previous workflow; this is not an immutable-release policy.
The workflow does not currently have a separate manual signing-approval gate.

## Build, validation and publication controls

The release configuration in this repository implements the following sequence.
These controls apply to runs using this workflow revision after it is merged;
they are not evidence that older releases passed these checks.

1. Accept only a `vX.Y.Z` release tag and check out that tag.
2. Fetch `main` and verify commit ancestry, then require the tag version to match
   the canonical `VERSION` file. CMake generates the CLI version from that file.
3. Build all Release targets on a GitHub-hosted Windows runner with CMake.
4. Run CTest before packaging. An empty test collection or failed test fails the
   build job. CI uses `FLX_TEST_HEADLESS=1`: graphics-dependent checks can be skipped,
   so this is not a claim that every graphical test ran.
5. Run CLI smoke checks for version, help, Pong validation and compilation.
   Each native command's exit code is checked; compilation must create output.
6. Record SHA-256 of that executable. Package it without rebuilding and verify
   both the standalone executable and the root `flx.exe` entry in the ZIP.
7. Transfer the EXE and ZIP through an immutable GitHub Actions artifact, addressed
   by the artifact ID from this run. Only a dependent publication job has
   `contents: write`. It verifies the recorded EXE/ZIP hashes and the ZIP's EXE
   again before uploading the two existing release assets. It does not execute
   code from the downloaded artifact.

A failed validation blocks dependent publication. No signing service, certificate,
signing secret or signing request is configured. Hash comparisons detect changes
between stages; they do not establish reproducible builds, external attestation,
or protection against a fully compromised trusted workflow or maintainer account.

## Supply-chain controls

All directly referenced GitHub Actions are pinned to full commit SHAs, with the
previously selected tag/major recorded in comments. The pins were resolved against
the upstream repositories on 2026-10-20. Pins require deliberate review and
maintenance; they do not automatically receive security fixes. A pinned composite
action may still refer to mutable actions or packages internally.

CMake FetchContent uses the same versions as before, pinned to these commits:

| Dependency | Selected version | Commit |
| --- | --- | --- |
| raylib | 6.0 | `dbc56a87da87d973a9c5baa4e7438a9d20121d28` |
| nlohmann/json | v3.12.0 | `55f93686c01528224f448c19128836e7df245f72` |
| yaml-cpp | 0.8.0 | `f7320141120f720aecc4c32be25586e7da9eb978` |
| QuickJS-NG | v0.10.1 | `3c9afc9943323ee9c7dbd123c0cd991448f4b6c2` |

Commits were compared with the local selected dependency checkouts and upstream
tag references (including the peeled nlohmann/json tag). Dependencies were not
upgraded. Dependency pins apply to source tags containing this CMake change;
rebuilding older tags retains their historical dependency declarations. Hosted runner images, compilers, SDKs, action internals and package
registries remain part of the build trust boundary. Build and version-check jobs
explicitly use read-only repository tokens; Pages retains its deployment scopes.

## Windows executable metadata: inspected, not yet implemented

The inspected local `flx.exe` has no ProductName, FileDescription, FileVersion,
ProductVersion or CompanyName version-resource values. The CLI version exists,
but that alone does not provide Windows VERSIONINFO metadata.

The smallest proposed change is a Windows-only `cmake/FlxVersion.rc.in` configured
by CMake and added only to target `flx`. Use ProductName `FLX Engine`,
FileDescription `FLX Engine command-line executable`, OriginalFilename `flx.exe`,
and FileVersion/ProductVersion derived from `VERSION` (numeric `major,minor,patch,0`
and matching string values). Enable the RC language only on Windows. Omit CompanyName
unless the maintainer explicitly establishes the intended publisher identity.
Do not use SignPath Foundation as a company/publisher field or imply a certificate.
This preparation does not implement that proposal or change executable resources.

## Planned controls and remaining SignPath work

Before applying, review the current [Foundation terms](https://signpath.org/terms.html)
and [GitHub integration requirements](https://docs.signpath.io/trusted-build-systems/github).
Eligibility and approval are decisions for SignPath Foundation, not this repository.

Pending maintainer and service configuration:

- Confirm account MFA, repository ownership, protected release tags and branch/ruleset
  settings. These are recommendations, not verified existing settings.
- Confirm named author/reviewer/approver roles with SignPath, transparently stating
  that one person currently holds all roles. Obtain an explicit manual approval
  for each future production signing request.
- Add and verify the Windows metadata proposal; configure artifact identity/version
  restrictions and trusted build provenance in SignPath.
- Review upstream licensing and signing eligibility, establish a privacy policy
  based on an actual data-flow review, and verify any installation/uninstallation
  obligations. No unverified promise about telemetry or network behavior is made here.
- Link this policy from the public homepage and release/download pages, and add
  service attribution only after approval and actual integration. The README links
  this preparation policy now; no claim of donated signing is made in advance.
- Only after explicit authorization, configure service credentials, approval policy
  and workflow integration. Signing changes the executable bytes: validate the
  signature and signed executable, recompute hashes, and package and publish that
  same signed executable. Do not reuse the current unsigned hash across signing.

Additional recommendations not implemented here: audit transitive action inputs,
automate reviewed pin updates, restrict workflow edits and tag movement, consider
protected-environment approval and immutable releases, retain provenance/SBOMs and
public checksums, and evaluate repeatable builds/toolchain pinning. Local testing
cannot establish GitHub-hosted runner behavior or enforce repository account settings.

## Reporting security issues

Use the official repository's **Security / Report a vulnerability** facility if
GitHub offers it. Availability of private vulnerability reporting has not been
verified or enabled by this change. If it is unavailable, open a minimal
[GitHub issue](https://github.com/FLXForge/FLXEngine/issues) requesting a private
reporting channel. Do not post exploit details, secrets or private data publicly;
wait for the maintainer to establish a private channel. No security email address
or response-time commitment is asserted. Include the affected version, asset URL,
workflow/run reference and relevant hashes once a suitable channel is available.
