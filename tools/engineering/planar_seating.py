"""Signed gap and alignment of two authored nominal seating datums."""
from dataclasses import dataclass
import math

from .domain_inputs import nonnegative, positive, scalar
from .interface_geometry import (
    cross, dot, interface_offset, rigid_interface, transverse_offset, unit_vector, vector,
)
from .quantities import StudyError


@dataclass(frozen=True)
class PlanarDatum:
    origin: tuple
    normal: tuple
    tangent: tuple
    half_width_m: float
    half_height_m: float

    @classmethod
    def read(cls, reference, scene, interface_type, scale):
        anchor, label = rigid_interface(reference, scene, interface_type, "PlaneRectangle")
        frame, region = anchor["world_frame"], anchor["authored_region"]
        normal = unit_vector(frame["normal"], f"{label} normal")
        tangent = unit_vector(frame["tangent"], f"{label} tangent")
        if abs(dot(normal, tangent)) > 1e-5:
            raise StudyError(f"{label} requires an orthogonal frame")
        return cls(vector(frame["origin"], f"{label} origin"), normal, tangent,
                   positive(region, "width") * scale / 2, positive(region, "height") * scale / 2)

    def gap_half_range(self, reference_normal):
        # Signed distance to a plane is affine over the rectangle, so its extrema are at corners.
        bitangent = cross(self.normal, self.tangent)
        return (self.half_width_m * abs(dot(self.tangent, reference_normal))
                + self.half_height_m * abs(dot(bitangent, reference_normal)))


def planar_seating(data, scene=None):
    if scene is None:
        raise StudyError("planar seating requires a native scene manifest")
    geometry = data["geometry"]
    if geometry["revision"] != scene["revision"]:
        raise StudyError("planar seating geometry revision is stale")
    scale = positive(geometry, "metres_per_scene_unit")
    seat_ref, mate_ref = data["seat"], data["mate"]
    if seat_ref["object"] == mate_ref["object"]:
        raise StudyError("planar seating requires two distinct objects")
    seat = PlanarDatum.read(seat_ref, scene, "Seat", scale)
    mate = PlanarDatum.read(mate_ref, scene, "Mate", scale)
    offset = interface_offset(mate.origin, seat.origin, scale)
    center_gap = dot(offset, seat.normal)
    half_range = mate.gap_half_range(seat.normal)
    minimum_gap, maximum_gap = center_gap - half_range, center_gap + half_range
    lateral = transverse_offset(offset, seat.normal, center_gap)
    angle = math.atan2(math.hypot(*cross(seat.normal, mate.normal)), -dot(seat.normal, mate.normal))
    target_gap = scalar(data["target_gap_m"], "target_gap_m")
    gap_tolerance = nonnegative(data, "max_gap_deviation_m")
    offset_limit = nonnegative(data, "max_lateral_offset_m")
    angle_limit = nonnegative(data, "max_normal_deviation_rad")
    if angle_limit >= math.pi / 2:
        raise StudyError("seating normal deviation limit must be less than pi/2 rad")
    checks = {
        "gap": max(abs(minimum_gap - target_gap), abs(maximum_gap - target_gap)) <= gap_tolerance,
        "lateral": lateral <= offset_limit,
        "opposed_normals": angle <= angle_limit,
    }
    return {
        "geometry_binding": {
            "revision": scene["revision"], "metres_per_scene_unit": scale,
            "seat": {key: seat_ref[key] for key in ("object", "interface")},
            "mate": {key: mate_ref[key] for key in ("object", "interface")},
        },
        "center_gap_m": center_gap,
        "minimum_gap_m": minimum_gap,
        "maximum_gap_m": maximum_gap,
        "lateral_offset_m": lateral,
        "normal_deviation_rad": angle,
        "seating_checks": checks,
        "seating_within_limits": all(checks.values()),
    }
