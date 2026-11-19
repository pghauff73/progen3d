"""Validate and measure resolved rigid interface frames from native manifests."""
import math

from .domain_inputs import scalar
from .quantities import StudyError
from .study import require_scene_interface


def rigid_interface(reference, scene, interface_type, region_kind):
    object_id = reference["object"]
    interface_id = reference["interface"]
    if not isinstance(object_id, str) or not isinstance(interface_id, str):
        raise StudyError(f"{interface_type} requires object and interface IDs")
    obj = scene["objects"].get(object_id)
    if obj is None:
        raise StudyError(f"unresolved scene object {object_id}")
    anchor = require_scene_interface(object_id, obj, interface_id)
    label = f"{object_id}:{interface_id}"
    if anchor["type"] != interface_type or anchor["authored_region"]["kind"] != region_kind:
        raise StudyError(f"{label} requires a {interface_type} {region_kind} interface")
    if anchor["rigid_placement"] is not True:
        raise StudyError(f"{label} requires rigid object placement")
    return anchor, label


def vector(values, label):
    if not isinstance(values, list) or len(values) != 3:
        raise StudyError(f"{label} requires three coordinates")
    return tuple(scalar(value, label) for value in values)


def unit_vector(values, label):
    direction = vector(values, label)
    norm = math.hypot(*direction)
    if abs(norm - 1.0) > 1e-5:
        raise StudyError(f"{label} requires a unit vector")
    return tuple(value / norm for value in direction)


def dot(a, b):
    return math.fsum(x * y for x, y in zip(a, b, strict=True))


def cross(u, v):
    a, b, c = u
    x, y, z = v
    return b * z - c * y, c * x - a * z, a * y - b * x


def interface_offset(moving, reference, scale):
    return tuple(scalar((point - origin) * scale, "interface offset")
                 for point, origin in zip(moving, reference, strict=True))


def transverse_offset(offset, normal, normal_offset):
    return math.hypot(*(value - normal_offset * direction
                        for value, direction in zip(offset, normal, strict=True)))
