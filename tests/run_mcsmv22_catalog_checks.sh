#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
build_directory="${TMPDIR:-/tmp}/progen3d-mcsmv22-catalog"
generated_directory="$build_directory/generated"
source_release="$repository_root/examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_RELEASE"
committed_header="$repository_root/include/vehicle/mcsmv2/generated/GeneratedMcsMv22Catalog.h"
committed_manifest="$repository_root/examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_NATIVE_CATALOG_MANIFEST.json"
compiler=${CXX:-g++}

rm -rf "$build_directory"
mkdir -p "$generated_directory"

PYTHONDONTWRITEBYTECODE=1 python3 -B \
	"$repository_root/tools/generate_mcsmv22_catalog.py" \
	--source-release "$source_release" \
	--header "$generated_directory/GeneratedMcsMv22Catalog.h" \
	--manifest "$generated_directory/native_catalog_manifest.json"

cmp "$committed_header" "$generated_directory/GeneratedMcsMv22Catalog.h"
cmp "$committed_manifest" "$generated_directory/native_catalog_manifest.json"

"$compiler" \
	-std=c++17 -O2 -g -Wall -Wextra -Wpedantic \
	-I"$repository_root/include" \
	-I"$repository_root" \
	-I"$repository_root/third_party/vendor/glad/include" \
	"$repository_root/src/vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.cpp" \
	"$repository_root/tests/mcsmv22_catalog_harness.cpp" \
	-o "$build_directory/mcsmv22-catalog-harness"

"$build_directory/mcsmv22-catalog-harness"
