#!/usr/bin/env bash
# Package the already-tested GCC Release executable; never rebuild it here.
set -euo pipefail

if [ "$#" -ne 3 ]; then
    echo "Usage: bash PackageLinuxDiagnostic.sh <source-dir> <build-dir> <output-dir>" >&2
    exit 1
fi
if [ "$(uname -s)" != Linux ] || [ "$(uname -m)" != x86_64 ]; then
    echo "Packaging requires a Linux x86-64 environment." >&2
    exit 1
fi

source_dir=$(cd -- "$1" && pwd)
build_dir=$(cd -- "$2" && pwd)
mkdir -p -- "$3"
output_dir=$(cd -- "$3" && pwd)
version=$(tr -d '\r\n' < "$source_dir/VERSION")
if [[ ! "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Invalid VERSION." >&2
    exit 1
fi
grep -Eq '^CMAKE_BUILD_TYPE:[^=]+=Release$' "$build_dir/CMakeCache.txt"
grep -Eq '^BUILD_SHARED_LIBS:[^=]+=OFF$' "$build_dir/CMakeCache.txt"
binary="$build_dir/flx"
test -x "$binary"

name="flx-$version-linux-x86_64"
work_dir=$(mktemp -d "$output_dir/.flx-package.XXXXXXXX")
# Cleanup must remain inside a generated child of the explicit output directory.
case "$work_dir" in
    "$output_dir"/.flx-package.*) ;;
    *) echo "Unexpected packaging directory." >&2; exit 1 ;;
esac
trap 'rm -rf -- "$work_dir"' EXIT
mkdir -p "$work_dir/stage/$name/licenses" "$work_dir/extracted"
bundle="$work_dir/stage/$name"

check_dependencies() {
    local executable=$1
    local report=$2
    if ! env -u LD_LIBRARY_PATH ldd "$executable" > "$report" 2>&1; then
        cat "$report" >&2
        echo "ldd failed." >&2
        return 1
    fi
    cat "$report"
    if grep -q 'not found' "$report"; then
        echo "Unresolved shared libraries." >&2
        return 1
    fi
    if grep -Fq "$build_dir/" "$report"; then
        echo "A runtime library still depends on the build directory." >&2
        return 1
    fi
}

check_dependencies "$binary" "$bundle/DEPENDENCIES.txt"
install -m 755 "$binary" "$bundle/flx"
cmp "$binary" "$bundle/flx"
install -m 644 "$source_dir/LICENSE" "$bundle/LICENSE"
install -m 644 "$source_dir/cmake/LinuxDiagnosticREADME.txt" "$bundle/README.txt"

# License texts only; no dependency sources, headers or development libraries.
license_files=(
    "raylib-src/LICENSE:raylib.txt"
    "raylib-src/src/external/glfw/LICENSE.md:GLFW.txt"
    "json-src/LICENSE.MIT:nlohmann-json.txt"
    "yaml_cpp-src/LICENSE:yaml-cpp.txt"
    "quickjs-src/LICENSE:QuickJS-NG.txt"
)
for entry in "${license_files[@]}"; do
    install -m 644 "$build_dir/_deps/${entry%:*}" "$bundle/licenses/${entry##*:}"
done
awk '/^\/\* Copyright/ { copying=1 } copying { print } copying && /\*\// { exit }' "$build_dir/_deps/quickjs-src/xsum.c" > "$bundle/licenses/xsum.txt"
test -s "$bundle/licenses/xsum.txt"
if grep -q '^#include' "$bundle/licenses/xsum.txt"; then
    echo "Invalid xsum license extraction." >&2
    exit 1
fi

{
    printf 'FLX version: %s\n' "$version"
    printf 'Commit: %s\n' "$(git -C "$source_dir" rev-parse HEAD)"
    printf 'Target: Ubuntu 24.04 x86-64, X11\nBuild type: Release\nCompiler: '
    "${CXX:-g++}" --version | sed -n '1p'
    printf 'Executable SHA-256: '
    sha256sum "$bundle/flx" | cut -d ' ' -f 1
} > "$bundle/BUILD_INFO.txt"

tar -czf "$work_dir/$name.tar.gz" -C "$work_dir/stage" "$name"
tar -xzf "$work_dir/$name.tar.gz" -C "$work_dir/extracted"
extracted="$work_dir/extracted/$name"
test -x "$extracted/flx"
cmp "$binary" "$extracted/flx"
check_dependencies "$extracted/flx" "$work_dir/extracted-dependencies.txt"
actual_version=$(cd -- "$extracted" && env -u LD_LIBRARY_PATH ./flx --version)
if [ "$actual_version" != "$version" ]; then
    echo "Extracted executable version does not match VERSION." >&2
    exit 1
fi
mv -- "$work_dir/$name.tar.gz" "$output_dir/$name.tar.gz"
echo "Verified archive: $output_dir/$name.tar.gz"
echo "Extracted executable version: $actual_version"
if [ -n "${GITHUB_OUTPUT:-}" ]; then
    printf 'archive=%s\nversion=%s\n' "$output_dir/$name.tar.gz" "$version" >> "$GITHUB_OUTPUT"
fi
