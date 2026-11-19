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

sed '/Object(id(CouponLeft)/i\        [ T(80 0 0) I(Cube material(plaster)) ]' \
    examples/EngineeringStudies/bonded-coupon.p3d > "$temp/inserted.p3d"
"$manifest" "$temp/inserted.p3d" "$temp/inserted.json"

sed 's/id(CouponRight)/id(Replacement)/' examples/EngineeringStudies/bonded-coupon.p3d > "$temp/renamed.p3d"
"$manifest" "$temp/renamed.p3d" "$temp/renamed.json"

sed 's/T(-10 0 0) S(20 20 20) I(Cube material(plaster))/& T(-20 0 0) I(Cube material(plaster))/' \
    examples/EngineeringStudies/bonded-coupon.p3d > "$temp/split.p3d"
"$manifest" "$temp/split.p3d" "$temp/split.json"

sed 's/T(-10 0 0) S(20 20 20) I(/T(-10 0 0) S(20 20 20) D(0.5 1 1) I(/' \
    examples/EngineeringStudies/bonded-coupon.p3d > "$temp/deformed.p3d"
"$manifest" "$temp/deformed.p3d" "$temp/deformed.json"

sed 's/id(CouponRight)/id(CouponLeft)/' examples/EngineeringStudies/bonded-coupon.p3d > "$temp/duplicate.p3d"
if "$manifest" "$temp/duplicate.p3d" "$temp/duplicate.json" >"$temp/duplicate.log" 2>&1; then
    echo "Duplicate native object ID unexpectedly accepted" >&2
    exit 1
fi
rg -qi "duplicate.*CouponLeft|CouponLeft.*(already|duplicate)" "$temp/duplicate.log"
test ! -e "$temp/duplicate.json"

sed 's/T(10 0 0)/T(10.1 0 0)/' examples/EngineeringStudies/bonded-coupon.p3d > "$temp/edited.p3d"
"$manifest" "$temp/edited.p3d" "$temp/edited.json"
if python3 -m tools.engineering analyse examples/EngineeringStudies/bonded-coupon.json \
    --scene "$temp/edited.json" --output "$temp/stale-result.json" >"$temp/stale.log" 2>&1; then
    echo "Edited native geometry unexpectedly accepted by the pinned study" >&2
    exit 1
fi
rg -q "coupon geometry revision is stale" "$temp/stale.log"
test ! -e "$temp/stale-result.json"

fit=examples/EngineeringStudies/cylindrical-fit
"$manifest" "$fit.p3d" "$temp/fit.json"
python3 -m tools.engineering analyse "$fit.json" --scene "$temp/fit.json" --output "$temp/fit-result.json"
sed '/^        FitHousing$/ { N; s/FitHousing\n        FitShaft/FitShaft\n        FitHousing/; }' "$fit.p3d" \
    | sed 's/I(Cylinder material/I(Cylinder(segments(32 1)) material/' > "$temp/fit-reordered.p3d"
"$manifest" "$temp/fit-reordered.p3d" "$temp/fit-reordered.json"
sed 's/T(0 0 0)/T(0.05 2 0)/' "$fit.p3d" > "$temp/fit-offset.p3d"
"$manifest" "$temp/fit-offset.p3d" "$temp/fit-offset.json"
sed 's/T(0 0 0)/A(90 2)/' "$fit.p3d" > "$temp/fit-tilted.p3d"
"$manifest" "$temp/fit-tilted.p3d" "$temp/fit-tilted.json"
sed 's/T(0 0 0)/S(2 1 1)/' "$fit.p3d" > "$temp/fit-scaled.p3d"
"$manifest" "$temp/fit-scaled.p3d" "$temp/fit-scaled.json"
sed 's/^    Object(id(FitAssembly)/    T(100 200 300) A(35 2) Object(id(FitAssembly)/' \
    "$fit.p3d" > "$temp/fit-rigid.p3d"
"$manifest" "$temp/fit-rigid.p3d" "$temp/fit-rigid.json"

python3 - "$temp" <<'PY'
from pathlib import Path
import json, math, sys
from tools.engineering.coupon_geometry import validate_coupon_geometry
from tools.engineering.domains import analyse
from tools.engineering.quantities import StudyError
from tools.engineering.study import parse_study
folder = Path(sys.argv[1])
examples = Path("examples/EngineeringStudies")
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
assert read("coupon") == json.loads((examples / "coupon.scene.json").read_text())
print("Native grammar, legacy state transitions, open-surface rejection, and bound coupon passed.")
bonded = read("bonded-result")
scene = read("bonded")
assert scene == json.loads((examples / "bonded-coupon.scene.json").read_text())
assert bonded["geometry_binding"]["revision"] == scene["revision"]
assert bonded["geometry_binding"]["initial_bond_gap_m"] == 0
assert bonded["convergence"]["converged"]
assert bonded["events"][0]["name"] == "damage_onset"
assert 120 < bonded["events"][0]["time_s"] < 122
assert abs(bonded["thermal_energy_residual_J"]) < 1e-7
print("Native bonded-coupon export, reproducibility, damage onset, and stale-revision rejection passed.")

inserted, edited = read("inserted"), read("edited")
assert inserted["revision"] != scene["revision"]
assert edited["objects"]["CouponRight"]["initial_position"][0] != 10
assert scene["objects"]["primitive:0"]["initial_position"][0] == -10
assert inserted["objects"]["primitive:0"]["initial_position"][0] == 80
for identifier in ("CouponLeft", "CouponRight"):
    part = scene["objects"][identifier]
    assert part["geometry_primitive_count"] == 1
    assert set(part["faces"]) == {"-X", "+X", "-Y", "+Y", "-Z", "+Z"}
    assert inserted["objects"][identifier]["transform_column_major"] == part["transform_column_major"]
    assert inserted["objects"][identifier]["source_span"]["start_line"] == part["source_span"]["start_line"] + 1
    for exported in (inserted, edited):
        assert exported["objects"][identifier]["faces"] == part["faces"]

case = json.loads((examples / "bonded-coupon.json").read_text())
case["geometry"]["revision"] = inserted["revision"]
binding = validate_coupon_geometry(case, inserted)
assert binding["layer_objects"] == ["CouponLeft", "CouponRight"]
assert binding["layer_faces"] == ["+X", "-X"]
assert binding["initial_bond_gap_m"] == 0

for scene_name, expected_error in (
    ("renamed", "unresolved scene object CouponRight"),
    ("split", "ambiguous face +X on CouponLeft"),
    ("deformed", "missing face +X on CouponLeft"),
    ("edited", "require aligned, adjoining faces"),
):
    changed = read(scene_name)
    case["geometry"]["revision"] = changed["revision"]
    try:
        validate_coupon_geometry(case, changed)
    except StudyError as error:
        assert expected_error in str(error), str(error)
    else:
        raise AssertionError(f"unexpected successful geometry binding for {scene_name}")

sidecar = folder / "named.p3physics"
sidecar.write_text((examples / "coupon.p3physics").read_text().replace(
    'object("primitive:0")', 'object("CouponLeft") face "+X"'))
for catalog in (scene, inserted):
    anchor = parse_study(sidecar, catalog).bindings["substrate"]
    assert anchor == {"object": "CouponLeft", "revision": catalog["revision"], "face": "+X"}
print("Stable object and face anchors passed insertion, edit, duplicate, rename, split, and deformation checks.")

fit_scene, reordered = read("fit"), read("fit-reordered")
fit_case = json.loads((examples / "cylindrical-fit.json").read_text())
fit_result = read("fit-result")
assert fit_result["geometry_binding"]["revision"] == fit_scene["revision"]
assert fit_result["fit_class"] == "clearance"
assert math.isclose(fit_result["diametral_clearance_min_m"], 20e-6)
assert math.isclose(fit_result["diametral_clearance_max_m"], 80e-6)
assert fit_result["alignment_within_limits"] and fit_result["axis_angle_rad"] == 0
assert fit_result["radial_offset_at_shaft_origin_m"] == 0
assert fit_result["axial_offset_m"] == 0.004
assert fit_scene["objects"]["Housing"]["geometry_primitive_count"] == 2
assert fit_scene["objects"]["Pin"]["type"] == "Cylinder"
assert fit_scene["objects"]["primitive:0"]["extent_scale"][0] == 32
assert math.isclose(reordered["objects"]["primitive:0"]["extent_scale"][0], 9.95, rel_tol=1e-7)
assert fit_scene["objects"]["Pin"]["topology_hash"] != reordered["objects"]["Pin"]["topology_hash"]
assert fit_scene["revision"] != reordered["revision"]
for identifier in ("Housing", "Pin"):
    assert fit_scene["objects"][identifier]["interface_details"] == reordered["objects"][identifier]["interface_details"]
seat = reordered["objects"]["Housing"]["interface_details"]["seatingPlane"]
assert seat["authored_region"] == {"kind": "PlaneRectangle", "width": 28, "height": 28}
assert seat["world_frame"]["origin"] == [0, 16, 0]

def rejects(data, scene, message):
    try:
        analyse(data, scene)
    except StudyError as error:
        assert message in str(error), str(error)
    else:
        raise AssertionError(f"expected cylindrical fit rejection: {message}")

rejects(fit_case, reordered, "geometry revision is stale")
rejects(fit_case, None, "requires a native scene manifest")
sidecar.write_text((examples / "coupon.p3physics").read_text().replace(
    'object("primitive:0")', 'object("Housing") interface "seatingPlane"'))
for catalog in (fit_scene, reordered):
    anchor = parse_study(sidecar, catalog).bindings["substrate"]
    assert anchor["interface"] == "seatingPlane" and anchor["revision"] == catalog["revision"]

for scene_name in ("fit-reordered", "fit-offset", "fit-tilted", "fit-rigid"):
    catalog = read(scene_name)
    fit_case["geometry"]["revision"] = catalog["revision"]
    result = analyse(fit_case, catalog)
    assert result["fit_class"] == "clearance"
    if scene_name == "fit-offset":
        assert not result["alignment_within_limits"]
        assert result["alignment_checks"] == {"axial": False, "radial": False, "angular": True}
        assert math.isclose(result["radial_offset_at_shaft_origin_m"], 50e-6, rel_tol=1e-7)
        assert math.isclose(result["axial_offset_m"], 0.006)
    elif scene_name == "fit-tilted":
        assert not result["alignment_within_limits"]
        assert not result["alignment_checks"]["angular"]
        assert math.isclose(result["axis_angle_rad"], math.pi / 2, abs_tol=1e-6)
    else:
        assert result["alignment_within_limits"]
        for key in ("diametral_clearance_min_m", "diametral_clearance_max_m",
                    "radial_offset_at_shaft_origin_m", "axial_offset_m", "axis_angle_rad"):
            assert math.isclose(result[key], fit_result[key], abs_tol=1e-7)
scaled = read("fit-scaled")
fit_case["geometry"]["revision"] = scaled["revision"]
rejects(fit_case, scaled, "requires rigid object placement")

fit_case["geometry"]["revision"] = fit_scene["revision"]
fit_case["bore"]["interface"] = "missing"
rejects(fit_case, fit_scene, "missing interface missing on Housing")
fit_case["bore"]["interface"] = "seatingPlane"
rejects(fit_case, fit_scene, "requires a Socket AxisSegment interface")
fit_case["bore"]["interface"] = "boreAxis"
for lower, upper, fit_class, line_to_line in (
    (0.00990, 0.00993, "interference", False),
    (0.00995, 0.00997, "transition", True),
    (0.00996, 0.01003, "clearance", True),
):
    fit_case["bore"].update(min_diameter_m=lower, max_diameter_m=upper)
    result = analyse(fit_case, fit_scene)
    assert (result["fit_class"], result["line_to_line_possible"]) == (fit_class, line_to_line)
fit_case["shaft"]["min_diameter_m"] = fit_case["bore"]["max_diameter_m"] = 0.00996
assert analyse(fit_case, fit_scene)["fit_class"] == "line_to_line"
fit_case["shaft"]["min_diameter_m"] = 0.03
rejects(fit_case, fit_scene, "minimum diameter exceeds maximum")
print("Authored interface anchors and cylindrical fit passed native export, remeshing, alignment, and tolerance checks.")
PY
