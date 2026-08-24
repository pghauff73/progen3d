#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="${TMPDIR:-/tmp}/progen3d-mcsmv22-provider"
compiler=${CXX:-g++}
source "$repository_root/tests/mcsmv22_native_link_sources.sh"

rm -rf "$build_directory"
mkdir -p "$build_directory"

"$compiler" \
	-std=c++17 -O2 -g -Wall -Wextra -Wpedantic \
	-I"$repository_root/include" \
	-I"$repository_root" \
	-I"$repository_root/third_party/vendor/glad/include" \
	"$repository_root/src/AppPaths.cpp" \
	"$repository_root/src/Mesh.cpp" \
	"${mcsmv22_native_link_sources[@]}" \
	"$repository_root/tests/mcsmv22_generated_mesh_provider_harness.cpp" \
	-o "$build_directory/mcsmv22-provider-harness"

"$build_directory/mcsmv22-provider-harness"
