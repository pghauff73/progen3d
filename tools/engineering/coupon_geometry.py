"""Verify that a coupon's layer dimensions match an exported native scene."""
import math

from .domain_inputs import positive, scalar
from .quantities import StudyError


def validate_coupon_geometry(data, scene):
    binding = data.get("geometry")
    if binding is None:
        if scene is not None:
            raise StudyError("the coupon does not declare a geometry binding")
        return None
    if scene is None:
        raise StudyError("the coupon declares geometry and requires a native scene manifest")
    revision = scene["revision"]
    pinned_revision = binding.get("revision")
    if not isinstance(pinned_revision, str) or not pinned_revision.strip():
        raise StudyError("coupon geometry requires a pinned scene revision")
    if pinned_revision != revision:
        raise StudyError("coupon geometry revision is stale")
    scale = positive(binding, "metres_per_scene_unit")
    tolerance = positive(binding, "tolerance_m")
    axis_name = binding["axis"]
    if axis_name not in ("x", "y", "z"):
        raise StudyError("coupon axis must be x, y or z")
    axis = "xyz".index(axis_name)
    transverse = [i for i in range(3) if i != axis]
    ids = binding["layer_objects"]
    if (not isinstance(ids, list) or len(ids) != 2
            or any(not isinstance(name, str) for name in ids) or ids[0] == ids[1]):
        raise StudyError("coupon geometry requires two distinct object IDs")
    boxes = []
    for name, layer in zip(ids, data["layers"], strict=True):
        item = scene["objects"].get(name)
        if not item or item.get("type") != "Cube":
            raise StudyError(f"{name} must be a native Cube object")
        transform = item.get("transform_column_major")
        if not isinstance(transform, list) or len(transform) != 16:
            raise StudyError(f"missing 4 by 4 transform for {name}")
        t = [scalar(value, "transform coordinate") for value in transform]
        if (any(t[4*i+3] != 0 for i in range(3)) or t[15] != 1
                or any(abs(t[4*c+r])*scale > tolerance
                       for c in range(3) for r in range(3) if c != r)):
            raise StudyError(f"{name} must be axis-aligned with an affine transform")
        lengths = [abs(t[5*i])*scale for i in range(3)]
        center = [t[12+i]*scale for i in range(3)]
        if min(lengths) <= 0 or abs(lengths[axis]-positive(layer, "length_m")) > tolerance:
            raise StudyError(f"{name} length does not match its thermal layer")
        area = math.prod(lengths[i] for i in transverse)
        area_tolerance = tolerance * (sum(lengths[i] for i in transverse)+tolerance)
        if abs(area-positive(data, "area_m2")) > area_tolerance:
            raise StudyError(f"{name} cross-section does not match area_m2")
        boxes.append((center, lengths))
    (a_pos, a_size), (b_pos, b_size) = boxes
    gap = b_pos[axis]-a_pos[axis]-(a_size[axis]+b_size[axis])/2
    if abs(gap) > tolerance or any(
        abs(a_pos[i]-b_pos[i]) > tolerance or abs(a_size[i]-b_size[i]) > tolerance for i in transverse
    ):
        raise StudyError("coupon objects require aligned, adjoining faces in layer order")
    return {"revision": revision, "axis": axis_name, "layer_objects": ids, "initial_bond_gap_m": gap}
