#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
release_root="$repository_root/examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_RELEASE"
output_root="$repository_root/examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_NATIVE_EVIDENCE/partition_meshes"
temporary_root="${TMPDIR:-/tmp}/progen3d-mcsmv22-native-partitions"

rm -rf "$temporary_root"
mkdir -p "$temporary_root"
python "$repository_root/tools/generate_mcsmv22_native_partition_meshes.py" \
	--release-root "$release_root" \
	--output-root "$temporary_root"

diff -ru "$output_root" "$temporary_root"
echo "MCSMv2.2 native partition mesh derivatives are deterministic."
