"""Worst-case diameter fit and nominal axis alignment at authored references."""
import math

from .coating_stack import fit_coatings
from .domain_inputs import nonnegative, positive, scalar
from .interface_geometry import (
    cross, dot, interface_offset, rigid_interface, transverse_offset, unit_vector, vector,
)
from .quantities import StudyError


def read_axis(part, scene, interface_type):
    anchor, label = rigid_interface(part, scene, interface_type, "AxisSegment")
    positive(anchor["authored_region"], "length")
    frame = anchor["world_frame"]
    return vector(frame["origin"], f"{label} origin"), unit_vector(frame["normal"], f"{label} axis")


def diameter_limits(part):
    lower = positive(part, "min_diameter_m")
    upper = positive(part, "max_diameter_m")
    if lower > upper:
        raise StudyError("minimum diameter exceeds maximum diameter")
    nominal = None
    if "nominal_diameter_m" in part:
        nominal = positive(part, "nominal_diameter_m")
        if not lower <= nominal <= upper:
            raise StudyError("nominal diameter must be within its declared limits")
    return lower, upper, nominal


def subtract_coating(clearance, reduction, roundoff):
    value = clearance - reduction
    # A result within arithmetic precision of zero is treated as line-to-line.
    return 0.0 if reduction and abs(value) <= roundoff else value


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
    shaft_min, shaft_max, shaft_nominal = diameter_limits(shaft)
    bore_min, bore_max, bore_nominal = diameter_limits(bore)
    if (shaft_nominal is None) != (bore_nominal is None):
        raise StudyError("nominal clearance requires both nominal diameters")
    uncoated_min, uncoated_max = bore_min - shaft_max, bore_max - shaft_min
    coatings = fit_coatings(data)
    if coatings:
        shaft_growth = 2 * coatings.get("shaft", {}).get("fit_thickness_m", 0.0)
        bore_reduction = 2 * coatings.get("bore", {}).get("fit_thickness_m", 0.0)
        if bore_reduction >= bore_min:
            raise StudyError("coating closes the minimum bore diameter")
        reduction = scalar(shaft_growth + bore_reduction, "coating diameter change")
    else:
        reduction = 0.0
    roundoff = 2 * math.ulp(max(shaft_max, bore_max))
    clearance_min = subtract_coating(uncoated_min, reduction, roundoff)
    clearance_max = subtract_coating(uncoated_max, reduction, roundoff)
    if clearance_min == clearance_max == 0:
        fit = "line_to_line"
    elif clearance_min >= 0:
        fit = "clearance"
    elif clearance_max <= 0:
        fit = "interference"
    else:
        fit = "transition"

    offset = interface_offset(shaft_center, bore_center, scene_scale)
    axial_offset = dot(offset, bore_axis)
    radial_offset = transverse_offset(offset, bore_axis, axial_offset)
    axis_angle = math.atan2(math.hypot(*cross(shaft_axis, bore_axis)), abs(dot(shaft_axis, bore_axis)))
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

    result = {
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
    if coatings:
        result["coatings"] = coatings
        result["uncoated_clearance_min_m"] = uncoated_min
        result["uncoated_clearance_max_m"] = uncoated_max
    if shaft_nominal is not None:
        nominal = bore_nominal - shaft_nominal
        result["nominal_diametral_clearance_m"] = subtract_coating(nominal, reduction, roundoff)
        if coatings:
            result["uncoated_nominal_clearance_m"] = nominal
    return result
