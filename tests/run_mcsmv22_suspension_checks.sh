#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="${TMPDIR:-/tmp}/progen3d-mcsmv22-suspension"
compiler=${CXX:-g++}

rm -rf "$build_directory"
mkdir -p "$build_directory"

"$compiler" \
	-std=c++17 -O2 -g -Wall -Wextra -Wpedantic \
	-I"$repository_root/include" \
	-I"$repository_root" \
	-I"$repository_root/third_party/vendor/glad/include" \
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.cpp" \
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22SuspensionKinematicEvaluationService.cpp" \
	"$repository_root/src/vehicle/service/VehicleSuspensionKinematicEvaluationService.cpp" \
	"$repository_root/src/vehicle/service/VehicleReferenceFrameTransformationService.cpp" \
	"$repository_root/src/vehicle/service/VehicleTyreMeshGenerationService.cpp" \
	"$repository_root/src/geometry/service/MeshTopologyAnalyzer.cpp" \
	"$repository_root/src/AppPaths.cpp" \
	"$repository_root/src/Mesh.cpp" \
	"$repository_root/tests/mcsmv22_suspension_harness.cpp" \
	-o "$build_directory/mcsmv22-suspension-harness"

"$build_directory/mcsmv22-suspension-harness"
