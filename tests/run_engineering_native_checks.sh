#!/usr/bin/env bash
set -euo pipefail

root=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
manifest=${1:-./progen3d-scene-manifest}
temp=$(mktemp -d)
trap 'rm -rf "$temp"' EXIT
"${CXX:-g++}" -std=c++17 -Wall -Wextra -Wpedantic -O2 -Iinclude \
    tests/engineering_preview_harness.cpp src/editor/controller/PreviewTimelineController.cpp \
    src/vegetation/service/VegetationMeasurementUnitNormalizationService.cpp -o "$temp/preview"
"$temp/preview"

cat > "$temp/motion.p3d" <<'GRAMMAR'
Start -> G(9.81 0 -1 0) [ T(0 10+t 0) S(2 3 4) A(90 2) M(3) V(1 2 3) VR(10 20 30) I(Cube material(plaster)) ] [ T(50 10 0) S(2 3 4) P(2) I(Cube material(plaster)) ]
GRAMMAR
"$manifest" "$temp/motion.p3d" "$temp/motion.json" --time 2 --step 0.01

cat > "$temp/seed.p3d" <<'GRAMMAR'
Start ->
    R Width(2 4)
    S(Width 1 1)
    T(0 t 0)
    I(Cube material(plaster))
GRAMMAR
"$manifest" "$temp/seed.p3d" "$temp/seed-a.json" --time 0
"$manifest" "$temp/seed.p3d" "$temp/seed-b.json" --time 2

cat > "$temp/open.p3d" <<'GRAMMAR'
Start -> P(1) I(Cylinder(topology(surface) close(none)) material(plaster))
GRAMMAR
if "$manifest" "$temp/open.p3d" "$temp/open.json" >"$temp/open.log" 2>&1; then
    echo "Open geometry unexpectedly accepted for density-derived mass" >&2
    exit 1
fi
rg -q "Density-derived mass is undefined" "$temp/open.log"

"$manifest" examples/EngineeringStudies/coupon.p3d "$temp/coupon.json"
python3 -m tools.engineering run examples/EngineeringStudies/coupon.p3physics --scene "$temp/coupon.json" --output "$temp/result.json"

"$manifest" examples/EngineeringStudies/bonded-coupon.p3d "$temp/bonded.json"
"$manifest" examples/EngineeringStudies/bonded-coupon.p3d "$temp/bonded-repeat.json"
cmp "$temp/bonded.json" "$temp/bonded-repeat.json"
python3 -m tools.engineering analyse examples/EngineeringStudies/bonded-coupon.json \
    --scene "$temp/bonded.json" --output "$temp/bonded-result.json"

sed 's/T(10 0 0)/T(10.1 0 0)/' examples/EngineeringStudies/bonded-coupon.p3d > "$temp/edited.p3d"
"$manifest" "$temp/edited.p3d" "$temp/edited.json"
if python3 -m tools.engineering analyse examples/EngineeringStudies/bonded-coupon.json \
    --scene "$temp/edited.json" --output "$temp/stale-result.json" >"$temp/stale.log" 2>&1; then
    echo "Edited native geometry unexpectedly accepted by the pinned study" >&2
    exit 1
fi
rg -q "coupon geometry revision is stale" "$temp/stale.log"
test ! -e "$temp/stale-result.json"

python3 - "$temp" <<'PY'
from pathlib import Path
import json, math, sys
folder = Path(sys.argv[1])
def read(name):
    return json.loads((folder / (name + ".json")).read_text())
motion = read("motion")
first, second = (motion["objects"][f"primitive:{n}"] for n in range(2))
assert first["mass"] == 3 and second["mass"] == 48
assert first["initial_position"][1] == 12
assert first["initial_velocity"] == [1, 2, 3]
assert first["initial_rotational_velocity"] == [10, 20, 30]
assert first["initial_rotation_degrees"][-1] == 90
assert abs(motion["preview_step"][0]["velocity"][1] - (2 - 9.81*.01)) < 1e-6
assert abs(motion["preview_step"][0]["position"][1] - (12 + (2-9.81*.01)*.01)) < 3e-6
a, b = (read("seed-" + variant)["objects"]["primitive:0"] for variant in "ab")
assert a["extent_scale"] == b["extent_scale"] and 2 < a["extent_scale"][0] < 4
assert b["initial_position"][1] - a["initial_position"][1] == 2
trace = read("result")
assert trace["bindings"]["substrate"]["revision"] == read("coupon")["revision"]
assert abs(trace["events"][0]["time_s"] - 50*math.log(2)) < 1e-6
print("Native grammar, legacy state transitions, open-surface rejection, and bound coupon passed.")
bonded = read("bonded-result")
assert bonded["geometry_binding"]["revision"] == read("bonded")["revision"]
assert bonded["geometry_binding"]["initial_bond_gap_m"] == 0
assert bonded["convergence"]["converged"]
assert bonded["events"][0]["name"] == "damage_onset"
assert 120 < bonded["events"][0]["time_s"] < 122
assert abs(bonded["thermal_energy_residual_J"]) < 1e-7
print("Native bonded-coupon export, reproducibility, damage onset, and stale-revision rejection passed.")
PY
