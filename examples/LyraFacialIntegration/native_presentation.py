"""Read native OBJ geometry and construct a genuine, uncapped inspection section.

This module has no Blender dependency. It never invents facial geometry. The
ordinary and hologram renderers consume the same immutable native document.
"""
from array import array
from dataclasses import dataclass, field
import hashlib
import json
import math
from pathlib import Path


@dataclass
class NativeTriangleSurface:
    name: str
    material_name: str = ""
    positions: array = field(default_factory=lambda: array("d"))
    normals: array = field(default_factory=lambda: array("d"))
    source_metadata: dict = field(default_factory=dict)

    @property
    def triangle_count(self):
        return len(self.positions) // 9

    def geometry_digest(self):
        digest = hashlib.sha256()
        digest.update(self.positions.tobytes())
        digest.update(self.normals.tobytes())
        return digest.hexdigest()


@dataclass
class NativeLyraDocument:
    path: Path
    source_sha256: str
    geometry_sha256: str
    surfaces: list
    landmarks: dict
    mechanics: list
    primitive_metadata: list
    bounds: list

    @property
    def triangle_count(self):
        return sum(surface.triangle_count for surface in self.surfaces)

    def provenance(self):
        return {
            "sourceObj": str(self.path.resolve()),
            "sourceSha256": self.source_sha256,
            "nativeGeometrySha256": self.geometry_sha256,
            "nativeObjectCount": len(self.surfaces),
            "nativeTriangleCount": self.triangle_count,
            "nativeBoundsM": self.bounds,
            "nativeNormals": True,
            "nativePositionsModifiedForOrdinaryOrHologram": False,
        }


class NativeLyraObjReader:
    """Read native triangle corners, preserving their authored normal indices."""

    def read(self, path):
        path = Path(path)
        positions, normals = array("d"), array("d")
        surfaces, landmarks, mechanics, primitive_metadata = [], {}, [], []
        current = None
        source_digest, geometry_digest = hashlib.sha256(), hashlib.sha256()
        lower, upper = [math.inf] * 3, [-math.inf] * 3
        with path.open("rb") as source:
            for raw in source:
                source_digest.update(raw)
                fields = raw.split()
                if not fields:
                    continue
                tag = fields[0]
                if tag in (b"v", b"vt", b"vn", b"f"):
                    geometry_digest.update(raw)
                if tag == b"o":
                    current = NativeTriangleSurface(fields[1].decode("utf-8"))
                    surfaces.append(current)
                elif tag == b"usemtl" and current is not None:
                    current.material_name = fields[1].decode("utf-8")
                elif tag == b"v":
                    point = tuple(map(float, fields[1:4]))
                    if len(point) != 3 or not all(math.isfinite(v) for v in point):
                        raise ValueError("Native OBJ has an invalid position")
                    positions.extend(point)
                    for axis, value in enumerate(point):
                        lower[axis] = min(lower[axis], value)
                        upper[axis] = max(upper[axis], value)
                elif tag == b"vn":
                    normal = tuple(map(float, fields[1:4]))
                    if len(normal) != 3 or not all(math.isfinite(v) for v in normal):
                        raise ValueError("Native OBJ has an invalid normal")
                    normals.extend(normal)
                elif tag == b"f":
                    if current is None or len(fields) != 4:
                        raise ValueError("Only named, triangulated native OBJ is accepted")
                    for corner in fields[1:]:
                        index = corner.split(b"/")
                        if len(index) != 3 or not index[2]:
                            raise ValueError("Every native triangle corner needs a normal")
                        position_start = 3 * (int(index[0]) - 1)
                        normal_start = 3 * (int(index[2]) - 1)
                        if not (0 <= position_start < len(positions) - 2 and
                                0 <= normal_start < len(normals) - 2):
                            raise ValueError("Native OBJ has an out-of-range index")
                        current.positions.extend(positions[position_start:position_start + 3])
                        current.normals.extend(normals[normal_start:normal_start + 3])
                elif fields[:2] == [b"#", b"facial_landmark"]:
                    landmarks[fields[2].decode()] = list(map(float, fields[3:6]))
                elif raw.startswith(b"# facial_elastic "):
                    mechanics.append(json.loads(raw[len(b"# facial_elastic "):]))
                elif raw.startswith(b"# native_primitive "):
                    primitive_metadata.append(json.loads(raw[len(b"# native_primitive "):]))
        if not surfaces or not all(surface.triangle_count for surface in surfaces):
            raise ValueError("Native OBJ contains no geometry or an empty object")
        metadata_by_name = {item["objectName"]: item for item in primitive_metadata
                            if "objectName" in item}
        for surface in surfaces:
            surface.source_metadata = metadata_by_name.get(surface.name, {})
        return NativeLyraDocument(path, source_digest.hexdigest(),
                                  geometry_digest.hexdigest(), surfaces, landmarks,
                                  mechanics, primitive_metadata, [lower, upper])


class NativePlaneSectionService:
    """Clip a copied presentation surface against n·x <= offset, without caps.

    Crossing triangle edges are linearly intersected with the declared plane;
    original retained corners are copied exactly. Interpolated normals are
    normalized. This is a derived view, not an additional native equilibrium.
    """

    def __init__(self, normal=(1.0, 0.0, 0.0), offset=0.0):
        length = math.sqrt(sum(value * value for value in normal))
        if not math.isfinite(length) or length <= 0 or not math.isfinite(offset):
            raise ValueError("A cutaway needs a finite, nonzero plane normal")
        self.normal = tuple(value / length for value in normal)
        self.offset = offset / length

    def distance(self, point):
        return sum(a * b for a, b in zip(self.normal, point)) - self.offset

    def intersect(self, first, second, first_distance, second_distance):
        fraction = first_distance / (first_distance - second_distance)
        point = tuple(a + fraction * (b - a) for a, b in zip(first[0], second[0]))
        normal = tuple(a + fraction * (b - a) for a, b in zip(first[1], second[1]))
        length = math.sqrt(sum(value * value for value in normal))
        if length <= 1e-15:
            normal = first[1]
            length = math.sqrt(sum(value * value for value in normal))
        return point, tuple(value / length for value in normal)

    def section(self, surface):
        result = NativeTriangleSurface(surface.name, surface.material_name)
        result.source_metadata = dict(surface.source_metadata)
        removed, crossed = 0, 0
        maximum_plane_error = 0.0
        for start in range(0, len(surface.positions), 9):
            triangle = [
                (tuple(surface.positions[i:i + 3]), tuple(surface.normals[i:i + 3]))
                for i in range(start, start + 9, 3)
            ]
            distances = [self.distance(corner[0]) for corner in triangle]
            if all(distance <= 0 for distance in distances):
                result.positions.extend(surface.positions[start:start + 9])
                result.normals.extend(surface.normals[start:start + 9])
                continue
            if all(distance > 0 for distance in distances):
                removed += 1
                continue
            crossed += 1
            polygon = []
            for index in range(3):
                first, second = triangle[index], triangle[(index + 1) % 3]
                first_distance, second_distance = distances[index], distances[(index + 1) % 3]
                if first_distance <= 0:
                    polygon.append(first)
                if (first_distance <= 0) != (second_distance <= 0):
                    intersection = self.intersect(first, second, first_distance, second_distance)
                    maximum_plane_error = max(maximum_plane_error,
                                              abs(self.distance(intersection[0])))
                    polygon.append(intersection)
            for index in range(1, len(polygon) - 1):
                corners = (polygon[0], polygon[index], polygon[index + 1])
                a, b, c = (corner[0] for corner in corners)
                u, v = tuple(b[k] - a[k] for k in range(3)), tuple(c[k] - a[k] for k in range(3))
                cross = (u[1]*v[2] - u[2]*v[1], u[2]*v[0] - u[0]*v[2], u[0]*v[1] - u[1]*v[0])
                if sum(value * value for value in cross) <= 1e-30:
                    continue
                for point, normal in corners:
                    result.positions.extend(point)
                    result.normals.extend(normal)
        evidence = {
            "sourceObject": surface.name,
            "sourceTriangles": surface.triangle_count,
            "sectionTriangles": result.triangle_count,
            "removedWholeTriangles": removed,
            "intersectedTriangles": crossed,
            "maximumIntersectionPlaneErrorM": maximum_plane_error,
        }
        return result, evidence
