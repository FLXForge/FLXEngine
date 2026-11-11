FLX Engine - Linux integration test runtime

Target: Ubuntu 24.04 x86-64, X11/XWayland and OpenGL.
See BUILD_INFO.txt for the engine version, commit, compiler and executable hash.
DEPENDENCIES.txt records the shared libraries resolved on the build runner.

Run from the extracted directory:

    ./flx --version
    ./flx --help
    ./flx run-compiled /absolute/path/to/game.flxc --save-path=/writable/save-directory

Provide the project separately. A compiled .flxc embeds its declarative
resources, input mapping and JavaScript. Running a source .flx requires its
JSON, JavaScript, Machine and input files in their original project layout.

The engine's default font, default Machine/input mapping and generated
audio are built in. No engine asset directory is required alongside flx.

The default save location is saves/ under the working directory at session
startup. Use --save-path when the working directory is not writable.

The host system supplies glibc, the C++ runtime, X11/XWayland, OpenGL and
their dependencies. Audio backends and graphics drivers may load libraries
dynamically that ldd does not list. Test those on the integration machine.
A successful --version verifies startup, not rendering or audible audio.

80's Like Collection continues to build and link flx-core for its embedded
runtime and supplies its own projects and Hub assets. This archive supports
CLI-level project checks. Building the Hub also requires its source build
and the existing flx-core CMake integration.

LICENSE and licenses/ contain the FLX and dependency license notices.
