#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="${TMPDIR:-/tmp}/progen3d-mcsmv22-grammar"
generated_directory="$build_directory/generated"
example_directory="$repository_root/examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_NATIVE"
application_path="$repository_root/progen3d-editor-gui"
compiler=${CXX:-g++}

rm -rf "$build_directory"
mkdir -p "$generated_directory"

"$compiler" \
	-std=c++17 -O2 -g -Wall -Wextra -Wpedantic \
	-I"$repository_root/include" \
	-I"$repository_root" \
	-I"$repository_root/third_party/vendor/glad/include" \
	"$repository_root/src/vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.cpp" \
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.cpp" \
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicGrammarLoweringService.cpp" \
	"$repository_root/tools/generate_mcsmv22_grammars.cpp" \
	-o "$build_directory/generate-mcsmv22-grammars"

"$build_directory/generate-mcsmv22-grammars" "$generated_directory"

grammar_names=(
	MCSMv22_Closed_Family_Preview.p3d
	MCSMv22_Engineering_Family_Preview.p3d
	MCSMv22_reference_Closed_Preview.p3d
	MCSMv22_reference_Open_Preview.p3d
	MCSMv22_track_Closed_Preview.p3d
	MCSMv22_track_Open_Preview.p3d
	MCSMv22_aero_Closed_Preview.p3d
	MCSMv22_aero_Open_Preview.p3d
	MCSMv22_crossover_Closed_Preview.p3d
	MCSMv22_crossover_Open_Preview.p3d
)

for grammar_name in "${grammar_names[@]}"; do
	committed_path="$example_directory/$grammar_name"
	generated_path="$generated_directory/$grammar_name"
	cmp "$committed_path" "$generated_path"
	rg -q '^Start[[:space:]]*->' "$committed_path"
	rg -q 'class\(ModernCarKinematicVariantV22\)|class\(ModernCarKinematicFamilyV22\)' "$committed_path"
	rg -q 'taxonomy\(Vehicle MCSMv2_2' "$committed_path"
	rg -q 'GeneratedMeshReference\(' "$committed_path"
	rg -q 'topology\(surface\)' "$committed_path"
	rg -q 'meshKey\(MCSMv22' "$committed_path"
	rg -q 'material\(' "$committed_path"
	if rg -n '!I\([^)]*\)[[:space:]]*\)' "$committed_path"; then
		echo "FAIL: $grammar_name contains an instance without material syntax" >&2
		exit 1
	fi
done

if [[ ! -x "$application_path" ]] || ! make -C "$repository_root" -q progen3d-editor-gui; then
	make -C "$repository_root" -j2 progen3d-editor-gui
fi
if ! command -v xvfb-run >/dev/null 2>&1; then
	echo "FAIL: MCSMv2.2 grammar checks require xvfb-run" >&2
	exit 1
fi

for grammar_name in "${grammar_names[@]}"; do
	committed_path="$example_directory/$grammar_name"
	log_path="$build_directory/${grammar_name%.p3d}.log"
	timeout_seconds=180
	if [[ "$grammar_name" == *Family* ]]; then
		timeout_seconds=360
	fi
	timeout "${timeout_seconds}s" xvfb-run -a "$application_path" \
		--smoke-test --open "$committed_path" >"$log_path" 2>&1
	if rg -n 'No .Start. rule|Grammar error|Grammar execution error|Scene generation failed|Assertion|SIGABRT' "$log_path"; then
		echo "FAIL: $grammar_name reported parser or render errors" >&2
		exit 1
	fi
	for marker in \
		PROGEN3D_GUI_SMOKE_TEST_PASSED \
		PROGEN3D_GUI_RENDER_FRAME_PASSED \
		PROGEN3D_GUI_FOV_SMOKE_TEST_PASSED; do
		rg -q "$marker" "$log_path"
	done
done

echo "MCSMv2.2 generated grammar checks passed."
