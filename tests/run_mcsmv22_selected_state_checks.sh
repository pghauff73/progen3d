#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="${TMPDIR:-/tmp}/progen3d-mcsmv22-selected-state"
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
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicPoseEvaluationService.cpp" \
	"$repository_root/src/vehicle/service/VehicleSuspensionKinematicEvaluationService.cpp" \
	"$repository_root/src/vehicle/service/VehicleClosureMotionEvaluationService.cpp" \
	"$repository_root/src/vehicle/service/VehicleReferenceFrameTransformationService.cpp" \
	"$repository_root/tests/mcsmv22_selected_state_harness.cpp" \
	-o "$build_directory/mcsmv22-selected-state-harness"

"$build_directory/mcsmv22-selected-state-harness"
