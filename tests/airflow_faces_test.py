from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from import_airflow_faces import FIELDS, surface_loads, grammar


def face(name, point, area, pressure, shear):
    return dict(zip(FIELDS, (name, *point, *area, pressure, *shear)))


# Uniform gauge pressure cancels over every face of a unit cube.
cube = []
for axis in range(3):
    for sign in (-1, 1):
        normal = [0, 0, 0]
        normal[axis] = sign
        cube.append(face("cube", [v / 2 for v in normal], normal, 100, [0, 0, 0]))
assert surface_loads(cube)["cube"] == [0] * 6
# A 2 m2 face, 5 Pa gauge and 3 Pa tangential traction at y=1 m.
patch = surface_loads([face("wall", [0, 1, 0], [2, 0, 0], 5, [0, 0, 3])])
assert patch["wall"] == [-10, 0, 6, 6, 0, 10]
assert list(grammar(patch))[-1] == "BodyTorque(6 0 10)"
print("Surface pressure, shear and moment checks passed.")

from export_airflow_patches import named_surfaces, stl_text
triangle = "o part_0\nv 0 0 0\nv 0 2 0\nv 0 0 3\nf 1 2 3\n"
exported = named_surfaces(triangle.splitlines(), {"part_0": "duct_inner"})
assert exported["duct_inner"][0][0] == (1, 0, 0)
assert "endsolid duct_inner" in stl_text(exported)
try:
    named_surfaces(triangle.splitlines(), {"part_1": "wrong"})
except ValueError:
    pass
else:
    raise AssertionError("unmatched patch names must be rejected")
print("Named surface export, winding and patch map checks passed.")
