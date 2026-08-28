#!/usr/bin/env bash
set -euo pipefail

repository_root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
source_package="$repository_root/examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_RELEASE"
python_bin=${MCSMV220_PYTHON:-python3}

PYTHONDONTWRITEBYTECODE=1 "$python_bin" -B \
	"$repository_root/tools/verify_mcsmv220_source_contract.py" \
	"$source_package"

MCSM_DISABLE_VTK=1 PYTHONDONTWRITEBYTECODE=1 PYTHONPATH="$source_package/source" \
	"$python_bin" -B -m unittest discover \
	-s "$source_package/source/tests" -p 'test_mcsmv22.py' -v

echo "MCSMv2.2 source contract checks passed."
