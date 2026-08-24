#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="${TMPDIR:-/tmp}/progen3d-mcsmv22-assurance"
compiler=${CXX:-g++}

rm -rf "$build_directory"
mkdir -p "$build_directory"

"$compiler" \
	-std=c++17 -O2 -g -Wall -Wextra -Wpedantic \
	-I"$repository_root/include" \
	-I"$repository_root" \
	-I"$repository_root/third_party/vendor/glad/include" \
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicAssuranceEvaluationService.cpp" \
	"$repository_root/tests/mcsmv22_assurance_harness.cpp" \
	-o "$build_directory/mcsmv22-assurance-harness"

"$build_directory/mcsmv22-assurance-harness"

"$compiler" \
	-std=c++17 -O2 -g -Wall -Wextra -Wpedantic \
	-I"$repository_root/include" \
	-I"$repository_root" \
	-I"$repository_root/third_party/vendor/glad/include" \
	-c "$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicGeometryGenerationService.cpp" \
	-o "$build_directory/McsMv22KinematicGeometryGenerationService.o"
