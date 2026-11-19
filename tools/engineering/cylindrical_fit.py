"""Worst-case diameter fit and nominal axis alignment at authored references."""
import math

from .domain_inputs import nonnegative, positive, scalar
from .quantities import StudyError
from .study import require_scene_interface


def vector(values, label):
    if not isinstance(values, list) or len(values) != 3:
        raise StudyError(f"{label} requires three coordinates")
    return tuple(scalar(value, label) for value in values)


def dot(a, b):
    return math.fsum(x * y for x, y in zip(a, b))


def read_axis(part, scene, interface_type):
    object_id = part["object"]
    interface_id = part["interface"]
    if not isinstance(object_id, str) or not isinstance(interface_id, str):
        raise StudyError(f"{interface_type} requires object and interface IDs")
    obj = scene["objects"].get(object_id)
    if obj is None:
        raise StudyError(f"unresolved scene object {object_id}")
    anchor = require_scene_interface(object_id, obj, interface_id)
    label = f"{object_id}:{interface_id}"
    if anchor["type"] != interface_type or anchor["authored_region"]["kind"] != "AxisSegment":
        raise StudyError(f"{label} requires a {interface_type} AxisSegment interface")
    positive(anchor["authored_region"], "length")
    if anchor["rigid_placement"] is not True:
        raise StudyError(f"{label} requires rigid object placement")
    frame = anchor["world_frame"]
    center = vector(frame["origin"], f"{label} origin")
    axis = vector(frame["normal"], f"{label} axis")
    norm = math.hypot(*axis)
    if abs(norm - 1.0) > 1e-5:
        raise StudyError(f"{label} requires a unit axis")
    return center, tuple(value / norm for value in axis)


def diameter_limits(part):
    lower = positive(part, "min_diameter_m")
    upper = positive(part, "max_diameter_m")
    if lower > upper:
        raise StudyError("minimum diameter exceeds maximum diameter")
    return lower, upper


def cylindrical_fit(data, scene=None):
    if scene is None:
        raise StudyError("cylindrical fit requires a native scene manifest")
    geometry = data["geometry"]
    if geometry["revision"] != scene["revision"]:
        raise StudyError("cylindrical fit geometry revision is stale")
    scene_scale = positive(geometry, "metres_per_scene_unit")
    dimension_source = data["dimension_source"]
    if not isinstance(dimension_source, str) or not dimension_source.strip():
        raise StudyError("cylindrical fit requires a dimension source")
    shaft, bore = data["shaft"], data["bore"]
    if shaft["object"] == bore["object"]:
        raise StudyError("cylindrical fit requires two distinct objects")
    shaft_center, shaft_axis = read_axis(shaft, scene, "Shaft")
    bore_center, bore_axis = read_axis(bore, scene, "Socket")
    shaft_min, shaft_max = diameter_limits(shaft)
    bore_min, bore_max = diameter_limits(bore)
    clearance_min, clearance_max = bore_min - shaft_max, bore_max - shaft_min
    if clearance_min == clearance_max == 0:
        fit = "line_to_line"
    elif clearance_min >= 0:
        fit = "clearance"
    elif clearance_max <= 0:
        fit = "interference"
    else:
        fit = "transition"

    offset = tuple(scalar((shaft_point - bore_point) * scene_scale, "offset")
                   for shaft_point, bore_point in zip(shaft_center, bore_center))
    axial_offset = dot(offset, bore_axis)
    radial_offset = math.hypot(*(value - axial_offset * direction
                                 for value, direction in zip(offset, bore_axis)))
    a, b, c = shaft_axis
    x, y, z = bore_axis
    cross_norm = math.hypot(b * z - c * y, c * x - a * z, a * y - b * x)
    axis_angle = math.atan2(cross_norm, abs(dot(shaft_axis, bore_axis)))
    axial_target = scalar(data["target_axial_offset_m"], "target_axial_offset_m")
    axial_limit = nonnegative(data, "max_axial_deviation_m")
    radial_limit = nonnegative(data, "max_radial_offset_m")
    angular_limit = nonnegative(data, "max_axis_angle_rad")
    if angular_limit > math.pi / 2:
        raise StudyError("axis angle limit cannot exceed pi/2 rad")
    axial_deviation = abs(axial_offset - axial_target)
    alignment = {
        "axial": axial_deviation <= axial_limit,
        "radial": radial_offset <= radial_limit,
        "angular": axis_angle <= angular_limit,
    }

    return {
        "geometry_binding": {
            "revision": scene["revision"],
            "metres_per_scene_unit": scene_scale,
            "shaft": {key: shaft[key] for key in ("object", "interface")},
            "bore": {key: bore[key] for key in ("object", "interface")},
        },
        "dimension_source": dimension_source.strip(),
        "fit_class": fit,
        "diametral_clearance_min_m": clearance_min,
        "diametral_clearance_max_m": clearance_max,
        "line_to_line_possible": clearance_min <= 0 <= clearance_max,
        "radial_offset_at_shaft_origin_m": radial_offset,
        "axial_offset_m": axial_offset,
        "axial_deviation_m": axial_deviation,
        "axis_angle_rad": axis_angle,
        "alignment_checks": alignment,
        "alignment_within_limits": all(alignment.values()),
    }
