#!/usr/bin/env python3
"""Build the fitted Lyra character from native procedural source and references.

Native indexed references and OBJ files are build-time measurements. The delivered
grammars contain only native procedural descriptors and import no runtime mesh.
"""
from __future__ import annotations

import argparse
import bisect
from collections import defaultdict
from dataclasses import dataclass
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import subprocess


HERE = Path(__file__).resolve().parent
Point = tuple[float, float, float]


def add(first: Point, second: Point) -> Point:
    return tuple(first[axis] + second[axis] for axis in range(3))


def subtract(first: Point, second: Point) -> Point:
    return tuple(first[axis] - second[axis] for axis in range(3))


def multiply(point: Point, scale: float) -> Point:
    return tuple(coordinate * scale for coordinate in point)


def dot(first: Point, second: Point) -> float:
    return sum(first[axis] * second[axis] for axis in range(3))


def cross(first: Point, second: Point) -> Point:
    return (first[1] * second[2] - first[2] * second[1],
            first[2] * second[0] - first[0] * second[2],
            first[0] * second[1] - first[1] * second[0])


def length(point: Point) -> float:
    return math.sqrt(dot(point, point))


def normalize(point: Point) -> Point:
    magnitude = length(point)
    if magnitude <= 1e-14:
        raise ValueError('Cannot normalize a collapsed construction tangent.')
    return multiply(point, 1 / magnitude)


def lerp(first: Point, second: Point, fraction: float) -> Point:
    return add(multiply(first, 1 - fraction), multiply(second, fraction))


def float32(value: float) -> float:
    return struct.unpack('f', struct.pack('f', value))[0]


def number(value: float) -> str:
    return format(0.0 if value == 0 else value, '.12g')


def coordinates(points: list[Point]) -> str:
    return ' '.join(number(coordinate) for point in points for coordinate in point)


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def resample_polyline(points: list[Point], segments: int) -> list[Point]:
    """Sample the native index parameter; preserve every knot at common multiples."""
    old_segments = len(points) - 1
    result = []
    for index in range(segments + 1):
        position = index * old_segments / segments
        first = min(int(position), old_segments - 1)
        result.append(lerp(points[first], points[first + 1], position - first))
    return result


@dataclass(frozen=True)
class LyraFacePlacement:
    translation: Point
    rotation_x_degrees: float

    def rotate(self, point: Point) -> Point:
        radians = math.radians(self.rotation_x_degrees)
        cosine, sine = math.cos(radians), math.sin(radians)
        return (point[0], cosine * point[1] - sine * point[2],
                sine * point[1] + cosine * point[2])

    def apply(self, point: Point) -> Point:
        return add(self.rotate(point), self.translation)


class NativeFacialReference:
    """Native positions, rest UVs, boundaries and eye definition from one sample."""

    def __init__(self, path: Path):
        self.path = path
        report = json.loads(path.read_text())
        if not report.get('passed') or len(report.get('samples', [])) != 1:
            raise ValueError('The native reference batch did not return one accepted state.')
        faces = [instance['facial'] for instance in report['samples'][0]['scene']['instances']
                 if 'facial' in instance]
        if len(faces) != 1:
            raise ValueError('The native reference must contain exactly one FacialSkin.')
        face = faces[0]
        mesh = face['nativeMesh']
        self.positions = [tuple(mesh['positions'][index:index + 3])
                          for index in range(0, len(mesh['positions']), 3)]
        self.normals = [tuple(mesh['normals'][index:index + 3])
                        for index in range(0, len(mesh['normals']), 3)]
        self.uvs = [tuple(mesh['materialUv'][index:index + 2])
                    for index in range(0, len(mesh['materialUv']), 2)]
        self.triangles = [tuple(mesh['triangles'][index:index + 3])
                         for index in range(0, len(mesh['triangles']), 3)]
        self.landmarks = {name: tuple(point) for name, point in face['localLandmarks'].items()}
        self.eyeball_radius = face['eyeballRadiusLocalM']
        self.mechanics = face['mechanics']
        self.fixed_vertex_ids = set(mesh['fixedVertexIds'])
        if not self.positions or len(self.positions) != len(self.uvs):
            raise ValueError('Native face reference lacks matching positions and rest UVs.')
        if any(abs(min(uv[axis] for uv in self.uvs)) > 1e-8 or
               abs(max(uv[axis] for uv in self.uvs) - 1) > 1e-8 for axis in (0, 1)):
            raise ValueError('Native reference must expose the original unit rest chart.')
        if self.mechanics is None or not self.mechanics['converged']:
            raise ValueError('Native neutral face reference is missing or did not converge.')
        if self.mechanics['iterations'] != 0:
            raise ValueError('Native neutral face has unexpected prestress.')
        if not self.landmarks or self.eyeball_radius is None:
            raise ValueError('Native facial landmarks or eyeball radius are missing.')
        self.chart = {}
        self.chart_ids = {}
        for vertex_id, (point, uv) in enumerate(zip(self.positions, self.uvs)):
            # Only the immutable outer collar is queried. Interior rim UVs can
            # intentionally have distinct front and back positions.
            if min(uv[0], uv[1], 1 - uv[0], 1 - uv[1]) < .025:
                key = (round(uv[0], 6), round(uv[1], 6))
                if key in self.chart and length(subtract(point, self.chart[key])) > 1e-8:
                    raise ValueError('Ambiguous native outer chart position.')
                self.chart[key] = point
                self.chart_ids[key] = vertex_id

    def at(self, u: float, v: float) -> Point:
        return self.chart[(round(u, 6), round(v, 6))]

    def geometry_fingerprint(self) -> str:
        payload = {'positions': self.positions, 'materialUv': self.uvs, 'triangles': self.triangles,
                   'landmarks': self.landmarks, 'eyeballRadius': self.eyeball_radius,
                   'fixedVertexIds': sorted(self.fixed_vertex_ids),
                   'shapeKey': self.mechanics['shapeKey']}
        return hashlib.sha256(json.dumps(payload, sort_keys=True, separators=(',', ':'),
                                         allow_nan=False).encode()).hexdigest()

    def boundary(self, horizontal: int, vertical: int) -> dict:
        side_uvs = {
            'Bottom': [(index / horizontal, 0.) for index in range(horizontal + 1)],
            'Right': [(1., index / vertical) for index in range(vertical + 1)],
            'Top': [(1 - index / horizontal, 1.) for index in range(horizontal + 1)],
            'Left': [(0., 1 - index / vertical) for index in range(vertical + 1)],
        }
        perimeter_uvs = [uv for side in side_uvs.values() for uv in side[:-1]]
        perimeter_tangents = {}
        for index, (u, v) in enumerate(perimeter_uvs):
            key = (round(u, 6), round(v, 6))
            previous = self.at(*perimeter_uvs[(index - 1) % len(perimeter_uvs)])
            following = self.at(*perimeter_uvs[(index + 1) % len(perimeter_uvs)])
            normal = self.normals[self.chart_ids[key]]
            perimeter_tangents[key] = normalize(cross(subtract(following, previous), normal))
        result = {}
        for name, uvs in side_uvs.items():
            points, tangents = [], []
            for u, v in uvs:
                point = self.at(u, v)
                points.append(point)
                tangents.append(perimeter_tangents[(round(u, 6), round(v, 6))])
            vertex_ids = [self.chart_ids[(round(u, 6), round(v, 6))] for u, v in uvs]
            if not set(vertex_ids).issubset(self.fixed_vertex_ids):
                raise ValueError('The collar requires an entirely fixed native outer face boundary.')
            result[name] = {'restUV': uvs, 'nativeVertexIds': vertex_ids,
                            'points': points, 'outwardTangents': tangents}
        return result

    def boundary_loops(self) -> list[dict]:
        counts = defaultdict(int)
        directions = {}
        for triangle in self.triangles:
            vertices = list(triangle)
            for first, second in zip(vertices, vertices[1:] + vertices[:1]):
                key = tuple(sorted((first, second)))
                counts[key] += 1
                directions[key] = (first, second)
        successors = {}
        for edge, count in counts.items():
            if count == 1:
                first, second = directions[edge]
                if first in successors:
                    raise ValueError('Native boundary is not a collection of simple loops.')
                successors[first] = second
            elif count != 2:
                raise ValueError('Native reference contains a nonmanifold edge.')
        loops = []
        while successors:
            start = min(successors)
            loop, vertex_ids, current = [], [], start
            while True:
                loop.append(self.positions[current])
                vertex_ids.append(current)
                current = successors.pop(current)
                if current == start:
                    break
            center = tuple(sum(point[axis] for point in loop) / len(loop) for axis in range(3))
            loops.append({'positionsFaceLocalM': loop, 'centroidFaceLocalM': center,
                          'nativeVertexIds': vertex_ids, 'vertexCount': len(loop)})
        if len(loops) != 4:
            raise ValueError(f'Expected the native face to have four boundaries, found {len(loops)}.')
        outer = max(loops, key=lambda loop: loop['vertexCount'])
        for loop in loops:
            if loop is outer:
                loop['name'] = 'outer'
            elif loop['centroidFaceLocalM'][1] < 0:
                loop['name'] = 'mouth'
            else:
                loop['name'] = 'left_eye' if loop['centroidFaceLocalM'][0] > 0 else 'right_eye'
        return sorted(loops, key=lambda loop: loop['name'])


class LyraHeadSurfaceChart:
    """The pinned axial head profiles with their original row/angle correspondence."""

    def __init__(self, source_lines: list[str]):
        self.rings: list[list[Point]] = []
        self.provenance: list[dict] = []
        self.authored_profiles = []
        self.authored_scales = []
        for line_number in (124, 125, 126):
            line = source_lines[line_number - 1]
            profiles = {}
            for name, text in re.findall(r'profile\(([^ ]+) polygon\(([^)]+)\)\)', line):
                values = list(map(lambda item: float32(float(item)), text.split()))
                profiles[name] = list(zip(values[::2], values[1::2]))
            levels = re.findall(r'(?:at|linear)\(([^ ]+) ([^ ]+) scale\(([^ ]+) ([^)]+)\)\)', line)
            for y, profile, sx, sz in levels:
                y, sx, sz = [float32(float(value)) for value in (y, sx, sz)]
                ring = [(float32(x * sx), y, float32(z * sz)) for x, z in profiles[profile]]
                if self.rings and abs(self.rings[-1][0][1] - y) < 1e-9:
                    if max(length(subtract(first, second)) for first, second in zip(self.rings[-1], ring)) > 1e-7:
                        raise ValueError('The declared axial bands do not meet at the shared row.')
                    continue
                self.rings.append(ring)
                self.authored_profiles.append(profiles[profile])
                self.authored_scales.append((sx, sz))
                self.provenance.append({'sourceLine': line_number, 'profile': profile})
        if len(self.rings) != 220 or any(len(ring) != 192 for ring in self.rings):
            raise ValueError('Pinned Lyra head chart dimensions changed.')

    def at(self, row: int, column: int) -> Point:
        return self.rings[row][column % 192]

    def window(self, lower: int, upper: int, left: int, right: int,
               corner_blend_fraction: float = .15) -> dict:
        # Counter-clockwise as viewed from the front: bottom, right, top, left.
        top_columns = list(range(right, 192)) + list(range(left + 1))
        specifications = {
            'Bottom': [(lower, column) for column in reversed(top_columns)],
            'Right': [(row, right) for row in range(lower, upper + 1)],
            'Top': [(upper, column) for column in top_columns],
            'Left': [(row, left) for row in range(upper, lower - 1, -1)],
        }
        result = {}
        def corner_weight(distance: float, span: float) -> float:
            fraction = min(1., max(0., distance / (span * corner_blend_fraction)))
            return 1 - fraction * fraction * (3 - 2 * fraction)

        for name, samples in specifications.items():
            points, tangents = [], []
            for row, column in samples:
                point = self.at(row, column)
                vertical = normalize(subtract(self.at(row + 1, column), self.at(row - 1, column)))
                circumferential = normalize(subtract(self.at(row, column + 1), self.at(row, column - 1)))
                height_weight = (corner_weight(upper - row, upper - lower) -
                                 corner_weight(row - lower, upper - lower))
                angular_span = 192 - right + left
                angular_weight = (corner_weight((left - column) % 192, angular_span) -
                                  corner_weight((column - right) % 192, angular_span))
                outward = add(multiply(vertical, height_weight),
                              multiply(circumferential, angular_weight))
                points.append(point)
                tangents.append(normalize(outward))
            result[name] = {'chartSamples': samples, 'points': points, 'outwardTangents': tangents}
        return result


class HeadRadialEnvelope:
    """Select an inner or outer triangle intersection along a horizontal ray."""

    def __init__(self, triangles: list[tuple[Point, Point, Point]], center_z: float,
                 select_outermost: bool = True):
        self.center_z = center_z
        self.select_outermost = select_outermost
        self.bin_height = .002
        self.bins = defaultdict(list)
        self.triangle_bins = defaultdict(list)
        self.triangles = []
        for first, second, third in triangles:
            edge_one, edge_two = subtract(second, first), subtract(third, first)
            if length(cross(edge_one, edge_two)) <= 1e-12:
                continue
            record = (first, edge_one, edge_two)
            triangle_index = len(self.triangles)
            self.triangles.append((first, second, third))
            lower = math.floor(min(first[1], second[1], third[1]) / self.bin_height)
            upper = math.floor(max(first[1], second[1], third[1]) / self.bin_height)
            for index in range(lower, upper + 1):
                self.bins[index].append(record)
                self.triangle_bins[index].append(triangle_index)

    def radius(self, y: float, direction: Point) -> float | None:
        origin = (0., y, self.center_z)
        selected = None
        for first, edge_one, edge_two in self.bins.get(math.floor(y / self.bin_height), []):
            perpendicular = cross(direction, edge_two)
            determinant = dot(edge_one, perpendicular)
            if abs(determinant) < 1e-14:
                continue
            offset = subtract(origin, first)
            u = dot(offset, perpendicular) / determinant
            if u < -1e-8 or u > 1 + 1e-8:
                continue
            second_perpendicular = cross(offset, edge_one)
            v = dot(direction, second_perpendicular) / determinant
            if v < -1e-8 or u + v > 1 + 1e-8:
                continue
            radius = dot(edge_two, second_perpendicular) / determinant
            if radius > 0 and (selected is None or
                               (radius > selected if self.select_outermost else radius < selected)):
                selected = radius
        return selected

    @staticmethod
    def clip_polygon(points: list[Point], normal: Point, offset: float) -> list[Point]:
        """Clip a convex polygon to normal dot point + offset >= 0."""
        if not points:
            return []
        result = []
        previous = points[-1]
        previous_value = dot(normal, previous) + offset
        for point in points:
            value = dot(normal, point) + offset
            if (value >= 0) != (previous_value >= 0):
                fraction = previous_value / (previous_value - value)
                result.append(lerp(previous, point, fraction))
            if value >= 0:
                result.append(point)
            previous, previous_value = point, value
        return result

    def cell_radius_bound(self, lower_y: float, upper_y: float,
                          lower_angle: float, upper_angle: float) -> float | None:
        """Bound every skin point in a carrier cell's complete Y/angular wedge.

        Clipping uses linear half-planes, not sampled rays. A convex function
        (radial distance) reaches its maximum on a clipped polygon at a vertex.
        The angular convention is direction=(-sin(angle), 0, cos(angle)).
        """
        if not 0 <= upper_angle - lower_angle < math.pi:
            raise ValueError('A carrier clearance cell must span less than a half circle.')
        lower_normal = (-math.cos(lower_angle), 0., -math.sin(lower_angle))
        upper_normal = (math.cos(upper_angle), 0., math.sin(upper_angle))
        planes = [((0., 1., 0.), -lower_y), ((0., -1., 0.), upper_y),
                  (lower_normal, -lower_normal[2] * self.center_z),
                  (upper_normal, -upper_normal[2] * self.center_z)]
        candidates = set()
        for index in range(math.floor(lower_y / self.bin_height),
                           math.floor(upper_y / self.bin_height) + 1):
            candidates.update(self.triangle_bins.get(index, []))
        maximum = None
        for triangle_index in candidates:
            polygon = list(self.triangles[triangle_index])
            for normal, offset in planes:
                polygon = self.clip_polygon(polygon, normal, offset)
                if not polygon:
                    break
            for point in polygon:
                radius = math.hypot(point[0], point[2] - self.center_z)
                maximum = radius if maximum is None else max(maximum, radius)
        return maximum


class LyraHairFitter:
    """Refit only the source front hair carrier and its attached strand paths.

    A shared radial displacement field preserves each strand's source offset
    from the carrier. The envelope uses native neutral face triangles and the
    exact authored collar grid. Native triangle-level clearance is checked by
    the independent integration audit after generation.
    """

    def __init__(self, source_lines: list[str], head_triangles: list, settings: dict):
        self.source_lines, self.settings = source_lines, settings
        self.center_z = settings['radialAxisZM']
        line = source_lines[126]
        self.profiles = {}
        for name, text in re.findall(r'profile\(([^ ]+) polygon\(([^)]+)\)\)', line):
            values = [float32(float(value)) for value in text.split()]
            self.profiles[name] = list(zip(values[::2], values[1::2]))
        self.levels, self.rings = [], []
        for y, name, sx, sz in re.findall(r'(?:at|linear)\(([^ ]+) ([^ ]+) scale\(([^ ]+) ([^)]+)\)\)', line):
            y, sx, sz = [float32(float(value)) for value in (y, sx, sz)]
            self.levels.append((y, name, sx, sz))
            self.rings.append([(float32(x * sx), y, float32(z * sz)) for x, z in self.profiles[name]])
        if len(self.rings) != 72 or any(len(ring) != 128 for ring in self.rings):
            raise ValueError('Pinned hair carrier chart changed.')
        self.head = HeadRadialEnvelope(head_triangles, self.center_z)
        self.cache = {}
        self.pair_angles, self.pair_displacements = [], []
        self.level_heights = [level[0] for level in self.levels]
        self.maximum_pair_thickness_change = 0.
        for ring in self.rings:
            angles, displacements = [], []
            for index in range(64):
                outer, inner = ring[index], ring[127 - index]
                radial = (inner[0], 0., inner[2] - self.center_z)
                direction = normalize(radial)
                if dot(subtract(outer, inner), direction) <= 0:
                    raise ValueError('Pinned carrier outer/inner strip pairing changed orientation.')
                angle = -math.atan2(radial[0], radial[2])
                if angles:
                    while angle < angles[-1] - math.pi:
                        angle += 2 * math.pi
                    if angle <= angles[-1]:
                        raise ValueError('Pinned carrier material angles are not ordered.')
                angles.append(angle)
                head_radius = self.head.radius(inner[1], direction) if radial[2] > 0 else None
                deficit = (head_radius + self.settings['radialTargetClearanceM'] - length(radial)
                           if head_radius is not None else -math.inf)
                amount = self.smooth_displacement(deficit)
                displacement = multiply(direction, amount)
                displacements.append(displacement)
                fitted_outer, fitted_inner = add(outer, displacement), add(inner, displacement)
                self.cache[outer], self.cache[inner] = fitted_outer, fitted_inner
                thickness_error = abs(length(subtract(fitted_outer, fitted_inner)) - length(subtract(outer, inner)))
                self.maximum_pair_thickness_change = max(self.maximum_pair_thickness_change, thickness_error)
            self.pair_angles.append(angles)
            self.pair_displacements.append(displacements)
        self.apply_cell_clearance_guard()

    def apply_cell_clearance_guard(self) -> None:
        """Protect the facets between paired carrier controls, including tips.

        If every cell vertex has radius >= (skin_bound + margin)/cos(span/2),
        its projection on the angular bisector is >= skin_bound + margin.
        Every point of either native triangle inherits that linear bound. This
        prevents long closing-tip chords cutting the skin between clear nodes.
        """
        required = [[length((point[0], 0., point[2] - self.center_z)) + length(displacement)
                     for point, displacement in zip(reversed(ring[64:]), displacements)]
                    for ring, displacements in zip(self.rings, self.pair_displacements)]
        maximum_addition, guarded_cells, protected_transition_cells = 0., 0, 0
        for level in range(len(self.rings) - 1):
            # Each interval covers both inner/outer strip cells. Single-node
            # entries cover the two closing-tip faces of the thick profile.
            for columns in [(0,)] + [(index, index + 1) for index in range(63)] + [(63,)]:
                controls = [(row, index) for row in (level, level + 1) for index in columns]
                if all(self.rings[row][127 - index][2] <= self.center_z for row, index in controls):
                    continue
                if any(self.rings[row][127 - index][2] <= self.center_z for row, index in controls):
                    # Keep the original rear carrier exact. These transition
                    # facets receive no conservative clearance claim; the same
                    # complete native triangle-intersection gate still covers
                    # them along with every fitted and unchanged carrier cell.
                    protected_transition_cells += 1
                    continue
                angles = []
                for row, index in controls:
                    reference_angle = self.pair_angles[row][index]
                    for point in (self.rings[row][index], self.rings[row][127 - index]):
                        angle = -math.atan2(point[0], point[2] - self.center_z)
                        angle += 2 * math.pi * round((reference_angle - angle) / (2 * math.pi))
                        angles.append(angle)
                lower_angle, upper_angle = min(angles), max(angles)
                skin_bound = self.head.cell_radius_bound(self.level_heights[level],
                                                        self.level_heights[level + 1],
                                                        lower_angle, upper_angle)
                if skin_bound is None:
                    continue
                minimum_radius = ((skin_bound + self.settings['radialTargetClearanceM']) /
                                  math.cos(.5 * (upper_angle - lower_angle)))
                guarded_cells += 1
                for row, index in controls:
                    inner = self.rings[row][127 - index]
                    required[row][index] = max(required[row][index], minimum_radius)
        for row, ring in enumerate(self.rings):
            for index in range(64):
                outer, inner = ring[index], ring[127 - index]
                radial = (inner[0], 0., inner[2] - self.center_z)
                original_amount = length(self.pair_displacements[row][index])
                amount = required[row][index] - length(radial)
                maximum_addition = max(maximum_addition, amount - original_amount)
                displacement = multiply(normalize(radial), amount)
                self.pair_displacements[row][index] = displacement
                fitted_outer, fitted_inner = add(outer, displacement), add(inner, displacement)
                self.cache[outer], self.cache[inner] = fitted_outer, fitted_inner
                thickness_error = abs(length(subtract(fitted_outer, fitted_inner)) -
                                      length(subtract(outer, inner)))
                self.maximum_pair_thickness_change = max(self.maximum_pair_thickness_change, thickness_error)
        self.cell_guard = {'method': 'Exact triangle clipping to each complete carrier Y/angular cell; convex radial bound and angular-bisector chord clearance.',
                           'guardedCells': guarded_cells,
                           'protectedRearTransitionCells': protected_transition_cells,
                           'domain': 'Cells whose complete source inner control set is in the front half; all other native facets remain included in the independent intersection audit.',
                           'maximumAdditionalDisplacementM': maximum_addition,
                           'includesClosingTipCells': True}

    def smooth_displacement(self, deficit: float) -> float:
        band = self.settings['smoothTransitionBandM']
        if deficit <= -band:
            return 0.
        if deficit >= band:
            return deficit
        # A C1 upper approximation of max(0, deficit), with exact zero outside
        # the transition band and no reduction in the target clearance.
        return (deficit + band)**2 / (4 * band)

    def displacement_at_level(self, level: int, angle: float) -> Point:
        angles = self.pair_angles[level]
        middle = .5 * (angles[0] + angles[-1])
        angle += 2 * math.pi * round((middle - angle) / (2 * math.pi))
        if angle < angles[0] - .05 or angle > angles[-1] + .05:
            return (0., 0., 0.)
        angle = min(angles[-1], max(angles[0], angle))
        first = min(len(angles) - 2, max(0, bisect.bisect_right(angles, angle) - 1))
        fraction = (angle - angles[first]) / (angles[first + 1] - angles[first])
        return lerp(self.pair_displacements[level][first], self.pair_displacements[level][first + 1], fraction)

    def fit_point(self, point: Point) -> Point:
        if point in self.cache:
            return self.cache[point]
        radial = (point[0], 0., point[2] - self.center_z)
        radius = length(radial)
        # Rear hair, bun and ears remain outside this fitting domain.
        if radial[2] <= 0 or radius <= 1e-12:
            self.cache[point] = point
            return point
        if point[1] < self.level_heights[0] or point[1] > self.level_heights[-1]:
            self.cache[point] = point
            return point
        level = min(len(self.level_heights) - 2, max(0, bisect.bisect_right(self.level_heights, point[1]) - 1))
        fraction = (point[1] - self.level_heights[level]) / (self.level_heights[level + 1] - self.level_heights[level])
        angle = -math.atan2(radial[0], radial[2])
        displacement = lerp(self.displacement_at_level(level, angle),
                            self.displacement_at_level(level + 1, angle), fraction)
        result = add(point, displacement)
        self.cache[point] = result
        return result

    def build(self) -> tuple[str, dict[int, str], dict]:
        profiles, levels, displacements = [], [], []
        changed_vertices = 0
        for index, ((y, name, sx, sz), ring) in enumerate(zip(self.levels, self.rings)):
            fitted = [self.fit_point(point) for point in ring]
            distances = [length(subtract(new, old)) for old, new in zip(ring, fitted)]
            displacements.extend(distances)
            changed_vertices += sum(distance > 1e-10 for distance in distances)
            # Preserve exact original profile coordinates where they are not
            # changed, including the entire unaffected rear surface.
            coordinates_2d = [original if distance <= 1e-10 else (new[0] / sx, new[2] / sz)
                              for original, new, distance in zip(self.profiles[name], fitted, distances)]
            polygon = ' '.join(number(value) for point in coordinates_2d for value in point)
            profiles.append(f'profile({name} polygon({polygon}))')
            levels.append(f"{'at' if index == 0 else 'linear'}({number(y)} {name} scale({number(sx)} {number(sz)}))")
        descriptor = 'AxialProfile(axis(y) ' + ' '.join(profiles + levels) + ' cap(none))'
        changed_lines, line_reports = {}, []
        for line_number in self.settings['associatedSweepLines']:
            source = self.source_lines[line_number - 1]
            match = re.search(r'path\(([^)]*)\)', source)
            if match is None:
                raise ValueError('An associated hair strand is not a pinned SweepDisk path.')
            values = [float32(float(value)) for value in match[1].split()]
            points = [tuple(values[index:index + 3]) for index in range(0, len(values), 3)]
            fitted = [self.fit_point(point) for point in points]
            distances = [length(subtract(new, old)) for old, new in zip(points, fitted)]
            if max(distances, default=0.) <= 1e-10:
                continue
            changed_lines[line_number] = source[:match.start(1)] + coordinates(fitted) + source[match.end(1):]
            displacements.extend(distances)
            line_reports.append({'sourceLine': line_number, 'changedControlPoints': sum(value > 1e-10 for value in distances),
                                 'sourceRule': 'LyraHeadAccessories',
                                 'primitiveOrdinal': (list(range(128, 222)) + list(range(228, 233)) + list(range(239, 244))).index(line_number),
                                 'maximumDisplacementM': max(distances)})
        report = {'carrierRule': 'LyraHeadHairProfile', 'carrierSourceLine': 127,
                  'modifiedSourceLines': [127] + sorted(changed_lines),
                  'associatedSweepChanges': line_reports, 'changedCarrierVertices': changed_vertices,
                  'maximumDisplacementM': max(displacements, default=0.),
                  'rmsDisplacementM': math.sqrt(sum(value * value for value in displacements) / len(displacements)),
                  'radialTargetClearanceM': self.settings['radialTargetClearanceM'],
                  'smoothTransitionBandM': self.settings['smoothTransitionBandM'],
                  'radialAxisZM': self.center_z,
                  'method': 'One common vector per paired outer/inner carrier control, selected from the skin-facing inner point and new head triangle envelope; strand offsets follow the same material-angle/height field.',
                  'facetClearanceConstruction': self.cell_guard,
                  'carrierPairing': 'source outer vertex i paired with inner vertex 127-i, i=0..63',
                  'maximumConstructedPairThicknessChangeM': self.maximum_pair_thickness_change,
                  'clearanceClaim': 'Conservative paired-cell construction against the native neutral face and authored collars; final native carrier and strand triangles require the independent contact audit.',
                  'unmodifiedAccessorySourceLines': [line for line in list(range(128, 222)) +
                                                      list(range(228, 233)) + list(range(239, 244))
                                                      if line not in changed_lines],
                  'classification': 'Explicitly fitted hair carrier and associated front/temple decorative strands.'}
        return descriptor, changed_lines, report


def facial_descriptor(manifest: dict, left: str, right: str, detail: int = 1) -> str:
    face = manifest['face']
    material, solve = face['material'], face['solve']
    options = [f"size({' '.join(map(number, face['sizeM']))})",
               f"segments({face['segments'][0] * detail} {face['segments'][1] * detail})",
               'response(elastic)', f"bending({number(face['bendingScale'])})", 'mouthMesh(quality)',
               f"skinMaterial({number(material['youngModulusPa'])} {number(material['poissonRatio'])} "
               f"{number(material['thicknessM'])} {number(material['supportNPerM3'])})",
               f"solve({solve['maximumIterations']} {number(solve['relativeFreeForceTolerance'])})",
               'smile(0 0) jawOpen(0) blink(0 0) browRaise(0 0)']
    for muscle, activation in zip(face['muscles'], (left, right)):
        fixed = muscle['originUV'] + muscle['insertionUV'] + muscle['radiiUV']
        options.append(f"muscle({muscle['name']} {' '.join(map(number, fixed))} {activation} "
                       f"{number(muscle['maximumShortening'])} {number(muscle['stiffnessNPerM'])})")
    return 'FacialSkin(' + ' '.join(options) + ')'


def axial_patch(rings: list[list[Point]], authored_profiles: list, authored_scales: list) -> str:
    profiles, levels = [], []
    for index, ring in enumerate(rings):
        # Retain the authored profile coordinate scale as well as its physical
        # product. AxialProfile's polygon validator uses profile-space tolerances.
        polygon = ' '.join(number(value) for point in authored_profiles[index] for value in point)
        profiles.append(f'profile(r{index} polygon({polygon}))')
        scale = ' '.join(map(number, authored_scales[index]))
        levels.append(f"{'at' if index == 0 else 'linear'}({number(ring[0][1])} r{index} scale({scale}))")
    return 'AxialProfile(axis(y) ' + ' '.join(profiles + levels) + ' cap(none))'


def network_patch(rows: list[list[Point]]) -> str:
    """Interpolating patch grid with exact row polylines and two boundary columns."""
    if len(rows) < 2 or len(rows[0]) < 2 or any(len(row) != len(rows[0]) for row in rows):
        raise ValueError('A procedural surface patch requires a rectangular nonempty grid.')
    options = ['uCurve(polyline ' + coordinates(row) + ')' for row in rows]
    for column in (0, len(rows[0]) - 1):
        options.append('vCurve(polyline ' + coordinates([row[column] for row in rows]) + ')')
    options.extend([f'samplesU({len(rows)})', f'samplesV({len(rows[0])})',
                    'tolerance(.0000001)', 'method(interpolatingPatchGrid)'])
    return 'CurveNetworkSurface(' + ' '.join(options) + ')'


def measure_patch_grid(rows: list[list[Point]]) -> dict:
    """Reject collapsed or locally folded authored quads before native generation."""
    grid = [[tuple(map(float32, point)) for point in row] for row in rows]
    minimum_area, minimum_alignment = math.inf, 1.
    for radial in range(len(grid) - 1):
        for along in range(len(grid[0]) - 1):
            first, second = grid[radial][along], grid[radial + 1][along]
            third, fourth = grid[radial + 1][along + 1], grid[radial][along + 1]
            first_cross = cross(subtract(second, first), subtract(third, first))
            second_cross = cross(subtract(third, first), subtract(fourth, first))
            first_area, second_area = length(first_cross), length(second_cross)
            minimum_area = min(minimum_area, .5 * first_area, .5 * second_area)
            if minimum_area <= 1e-14:
                raise ValueError('Authored collar contains a collapsed triangle.')
            alignment = dot(first_cross, second_cross) / (first_area * second_area)
            minimum_alignment = min(minimum_alignment, alignment)
            if alignment <= 0:
                raise ValueError('Authored collar contains a folded quadrilateral.')
    return {'minimumTriangleAreaM2': minimum_area, 'minimumQuadNormalDot': minimum_alignment,
            'vertices': len(grid) * len(grid[0]), 'triangles': 2 * (len(grid) - 1) * (len(grid[0]) - 1)}


def collar_grid(inner: dict, outer: dict, placement: LyraFacePlacement,
                radial_segments: int, tangent_scale: float) -> tuple[list[list[Point]], dict]:
    inner_points = [placement.apply(point) for point in inner['points']]
    inner_tangents = [placement.rotate(point) for point in inner['outwardTangents']]
    segments = math.lcm(len(inner_points) - 1, len(outer['points']) - 1)
    if segments + 1 > 2048:
        raise ValueError('Exact boundary knot alignment exceeds the native sampling limit.')
    near = resample_polyline(inner_points, segments)
    far = resample_polyline(outer['points'], segments)
    first_tangents = list(map(normalize, resample_polyline(inner_tangents, segments)))
    last_tangents = list(map(normalize, resample_polyline(outer['outwardTangents'], segments)))
    rows = []
    radial_parameters = []
    for radial in range(radial_segments + 1):
        # Resolve endpoint tangency geometrically. Uniform coarse samples can
        # turn a smooth Hermite join into a visible 30-degree polygonal corner.
        fraction = .5 - .5 * math.cos(math.pi * radial / radial_segments)
        radial_parameters.append(fraction)
        h00, h10 = 2 * fraction**3 - 3 * fraction**2 + 1, fraction**3 - 2 * fraction**2 + fraction
        h01, h11 = -2 * fraction**3 + 3 * fraction**2, fraction**3 - fraction**2
        row = []
        for start, finish, first_tangent, last_tangent in zip(near, far, first_tangents, last_tangents):
            tangent_length = tangent_scale * length(subtract(finish, start))
            point = add(add(multiply(start, h00), multiply(finish, h01)),
                        add(multiply(first_tangent, h10 * tangent_length),
                            multiply(last_tangent, h11 * tangent_length)))
            row.append(point)
        rows.append(row)
    record = {'faceRestUV': inner['restUV'], 'faceNativeVertexIds': inner['nativeVertexIds'],
              'faceBoundaryFaceLocalM': inner['points'],
              'faceBoundaryHeadLocalM': inner_points, 'headBoundaryHeadLocalM': outer['points'],
              'headChartSamples': outer['chartSamples'], 'joinSamplesHeadLocalM': {'face': near, 'head': far},
              'alongSegments': segments, 'radialSegments': radial_segments,
              'radialHermiteParameters': radial_parameters,
              'faceKnotsEverySamples': segments // (len(inner_points) - 1),
              'headKnotsEverySamples': segments // (len(outer['points']) - 1),
              'pairing': 'Each sampled join interval lies on its declared native boundary segment; extra samples subdivide straight edges without changing them.'}
    return rows, record


def primitive_rule(name: str, descriptor: str, material: str = 'silvercyanpolishedmetal') -> str:
    return f'{name} -> I({descriptor} material({material}) 1 1)\n'


def eye_rules(reference: NativeFacialReference) -> tuple[str, list[dict]]:
    rules, records = [], []
    for side, landmark in [('Left', 'left_eyeball_center'), ('Right', 'right_eyeball_center')]:
        center = reference.landmarks[landmark]
        diameter = 2 * reference.eyeball_radius
        rules.append(f"LyraFaceEye{side} -> [ T({' '.join(map(number, center))}) "
                     f"LyraFace{side}Sclera LyraFace{side}Iris LyraFace{side}Pupil ]\n")
        rules.append(f'LyraFace{side}Sclera -> [ S({number(diameter)} {number(diameter)} {number(diameter)}) '
                     'I(Sphere(segments(32 24)) material(whitepolishedmetal) 1 1) ]\n')
        for suffix, degrees, inflation, material, latitude in [
            ('Iris', 27., .000004, 'tealbluemetal', 16),
            ('Pupil', 11.8125, .000008, 'blackmattepaint', 7)]:
            scale = number(diameter + inflation)
            rules.append(f'LyraFace{side}{suffix} -> [ A(90 0) S({scale} {scale} {scale}) '
                         f'I(Sphere(polar(0 {number(degrees)}) topology(surface) close(none) '
                         f'segments(32 {latitude})) material({material}) 1 1) ]\n')
        records.append({'side': side.lower(), 'centerFaceLocalM': center,
                        'scleraRadiusM': reference.eyeball_radius,
                        'scleraRule': f'LyraFace{side}Sclera', 'irisRule': f'LyraFace{side}Iris',
                        'pupilRule': f'LyraFace{side}Pupil',
                        'irisRadialOffsetM': .000002, 'pupilRadialOffsetM': .000004})
    return '\n'.join(rules), records


def append_rule_parameter(line: str, rule: str, parameter: str) -> str:
    pattern = re.compile(r'\b' + re.escape(rule) + r'\(')
    match = pattern.search(line)
    if match is None:
        raise ValueError(f'Pinned source rule call {rule} was not found.')
    depth = 1
    for index in range(match.end(), len(line)):
        depth += (line[index] == '(') - (line[index] == ')')
        if depth == 0:
            return line[:index] + ' ' + parameter + line[index:]
    raise ValueError('Unclosed pinned source rule call.')


def render_character(source_lines: list[str], manifest: dict, head_rules: str) -> str:
    result = []
    for index, line in enumerate(source_lines, start=1):
        if 123 <= index <= 244:
            continue
        if line.startswith('Clip(tm) ->'):
            line = append_rule_parameter(line, 'Progress', 'tm')
        elif line.startswith('Progress('):
            line = append_rule_parameter(line, 'Progress', 'tm')
            line = append_rule_parameter(line, 'Pose', 'tm')
        elif line.startswith('Pose('):
            line = append_rule_parameter(line, 'Pose', 'tm')
        elif index == 17:
            if 'LyraHead ]' not in line:
                raise ValueError('Pinned head hierarchy changed.')
            line = line.replace('LyraHead ]', 'LyraFacialSchedule(tm) ]')
        result.append(line)
    header = ('# Lyra milestone D: native elastic face and procedural head collar.\n'
              '# The native face, shared eyes and fixed collar inherit the original head hierarchy once.\n'
              '# Fixed jaw/eyes; bounded smile controls; independent static equilibrium at each clamped time.\n')
    return header + '\n'.join(result) + '\n\n' + head_rules


class LyraHeadGrammarBuilder:
    def __init__(self, manifest: dict, source_lines: list[str], reference: NativeFacialReference):
        self.manifest, self.source_lines, self.reference = manifest, source_lines, reference
        self.chart = LyraHeadSurfaceChart(source_lines)
        placement = manifest['placement']
        self.placement = LyraFacePlacement(tuple(placement['translationM']), placement['rotationXDegrees'])

    def build(self) -> tuple[str, dict]:
        manifest, chart, reference = self.manifest, self.chart, self.reference
        window = manifest['window']
        lower, upper = window['lowerRing'], window['upperRing']
        left, right = window['leftCircumferentialIndex'], window['rightCircumferentialIndex']
        rules, head_calls, retained = [], [], []
        placed_face = [self.placement.apply(point) for point in reference.positions]
        head_envelope_triangles = [tuple(placed_face[index] for index in triangle) for triangle in reference.triangles]
        for name, first, last in [('Lower', 0, lower), ('Upper', upper, len(chart.rings) - 1)]:
            rule = 'LyraRetainedHead' + name
            rules.append(primitive_rule(rule, axial_patch(chart.rings[first:last + 1],
                                                          chart.authored_profiles[first:last + 1],
                                                          chart.authored_scales[first:last + 1])))
            head_calls.append(rule)
            retained.append({'rule': rule, 'sourceRows': [first, last], 'sourceColumns': [0, 192],
                             'method': 'unchanged axial ring geometry and original triangle diagonal'})
        for patch_index, first in enumerate(range(lower, upper, 8)):
            last = min(first + 8, upper)
            rule = f'LyraRetainedHeadBack{patch_index:02d}'
            rows = [[chart.at(row, column) for column in range(left, right + 1)]
                    for row in range(first, last + 1)]
            rules.append(primitive_rule(rule, network_patch(rows)))
            head_calls.append(rule)
            retained.append({'rule': rule, 'sourceRows': [first, last], 'sourceColumns': [left, right],
                             'method': 'native interpolating grid; original row knots and triangle diagonal'})
        boundaries = reference.boundary(*manifest['face']['segments'])
        outer = chart.window(lower, upper, left, right,
                             manifest['collar']['cornerTangentBlendFraction'])
        collars = []
        for side in ('Bottom', 'Right', 'Top', 'Left'):
            rule = 'LyraFaceCollar' + side
            rows, record = collar_grid(boundaries[side], outer[side], self.placement,
                                       manifest['collar']['radialSegments'],
                                       manifest['collar']['hermiteTangentLengthScale'])
            record['rule'], record['side'] = rule, side.lower()
            record['authoredGridGeometry'] = measure_patch_grid(rows)
            for radial in range(len(rows) - 1):
                for along in range(len(rows[0]) - 1):
                    first, second = rows[radial][along], rows[radial + 1][along]
                    third, fourth = rows[radial + 1][along + 1], rows[radial][along + 1]
                    head_envelope_triangles.extend([(first, second, third), (first, third, fourth)])
            collars.append(record)
            rules.append(primitive_rule(rule, network_patch(rows)))
            head_calls.append(rule)
        hair_descriptor, fitted_accessories, hair_report = LyraHairFitter(
            self.source_lines, head_envelope_triangles, manifest['hairFit']).build()
        rules.append(primitive_rule('LyraHeadHairProfile', hair_descriptor, 'blackbluemattepaint'))
        head_calls.append('LyraHeadHairProfile')
        accessory_lines = manifest['headClassification']['accessorySourceLines']
        rules.append('LyraHeadAccessories ->\n' + '\n'.join(fitted_accessories.get(index, self.source_lines[index - 1])
                                                            for index in accessory_lines) + '\n')
        head_calls.append('LyraHeadAccessories')
        face = manifest['face']
        rules.append(f"LyraFacialSkin(leftActivation rightActivation) -> I({facial_descriptor(manifest, 'clamp(leftActivation,0,.5)', 'clamp(rightActivation,0,.5)')} material({face['materialName']}) 1 1)\n")
        eye_text, eyes = eye_rules(reference)
        rules.append(eye_text)
        # This explicitly declared interior is a visual backing, not a contact surface.
        mouth = multiply(add(reference.landmarks['mouth_upper_margin'], reference.landmarks['mouth_lower_margin']), .5)
        oral_center = (mouth[0], mouth[1], mouth[2] - .022)
        oral_size = (face['sizeM'][0] * .47, face['sizeM'][1] * .18, .012)
        rules.append(f"LyraOralInterior -> [ T({' '.join(map(number, oral_center))}) "
                     f"S({' '.join(map(number, oral_size))}) I(Sphere(segments(28 18)) material(blackmattepaint) 1 1) ]\n")
        fit_scope = (f"[ T({' '.join(map(number, self.placement.translation))}) "
                     f"A({number(self.placement.rotation_x_degrees)} 0) LyraFacialSkin(leftActivation rightActivation) "
                     'LyraFaceEyeLeft LyraFaceEyeRight LyraOralInterior ]')
        head = 'LyraHead(leftActivation rightActivation) ->\n    ' + '\n    '.join(head_calls + [fit_scope]) + '\n'
        clock = manifest['clock']
        schedule = f"LyraFacialSchedule(tm) -> LyraHead({clock['leftActivation']} {clock['rightActivation']})\n"
        report = {'retainedHeadPatches': retained, 'collars': collars, 'eyes': eyes,
                  'fittedHair': hair_report,
                  'oralInterior': {'rule': 'LyraOralInterior', 'centerFaceLocalM': oral_center,
                                   'sizeM': oral_size, 'purpose': 'recessed visual oral backing'},
                  'faceRule': 'LyraFacialSkin', 'headRule': 'LyraHead',
                  'placementScope': fit_scope, 'boundaryLoops': reference.boundary_loops()}
        return schedule + '\n' + head + '\n'.join(rules), report


def check_fit(manifest: dict, reference: NativeFacialReference) -> dict:
    placement = LyraFacePlacement(tuple(manifest['placement']['translationM']), manifest['placement']['rotationXDegrees'])
    residuals = []
    for anchor in manifest['fitAnchors']:
        if anchor['nativeLandmark'].startswith('mean('):
            point = multiply(add(reference.landmarks['mouth_upper_margin'], reference.landmarks['mouth_lower_margin']), .5)
        else:
            point = reference.landmarks[anchor['nativeLandmark']]
        actual = placement.apply(point)
        residuals.append({'name': anchor['name'], 'actualHeadLocalM': actual,
                          'residualM': length(subtract(actual, tuple(anchor['headLocalM'])))})
    rms = math.sqrt(sum(item['residualM']**2 for item in residuals) / len(residuals))
    maximum = max(item['residualM'] for item in residuals)
    accepted = rms <= manifest['fitAcceptance']['rmsLimitM'] and maximum <= manifest['fitAcceptance']['maximumLimitM']
    if not accepted:
        raise ValueError(f'Native fit failed: RMS={rms}m, maximum={maximum}m; authored targets unchanged.')
    return {'accepted': True, 'rmsM': rms, 'maximumM': maximum, 'anchors': residuals}


def run_native_reference(sampler: Path, manifest: dict, output: Path) -> NativeFacialReference:
    directory = output / 'references'
    directory.mkdir(parents=True, exist_ok=True)
    grammar = directory / 'native_face_neutral.p3d'
    request, report = directory / 'native_reference_request.json', directory / 'native_face_neutral.json'
    grammar.write_text(f"Start -> I({facial_descriptor(manifest, '0', '0')} material({manifest['face']['materialName']}) 1 1)\n")
    request.write_text(json.dumps({'requireFacialSkin': True, 'captureRules': ['*'],
                                  'samples': [{'id': 'neutral', 'seconds': 0, 'obj': 'native_face_neutral.obj'}]}, indent=2) + '\n')
    report.unlink(missing_ok=True)
    outcome = subprocess.run([str(sampler), '--batch', str(grammar), str(request), str(report)],
                             capture_output=True, text=True)
    if outcome.returncode or not report.exists():
        raise RuntimeError('Native reference generation failed.\n' + outcome.stdout + outcome.stderr)
    (directory / 'native_reference.log').write_text(outcome.stdout + outcome.stderr)
    return NativeFacialReference(report)


def build_character(sampler: Path, source: Path, manifest_path: Path, output: Path) -> dict:
    manifest = json.loads(manifest_path.read_text())
    if sha256(source) != manifest['source']['sha256']:
        raise ValueError('Source hash mismatch: a different Lyra source needs an explicitly reviewed fit.')
    output.mkdir(parents=True, exist_ok=True)
    reference = run_native_reference(sampler.resolve(), manifest, output)
    fit = check_fit(manifest, reference)
    lines = source.read_text().splitlines()
    head_rules, assembly = LyraHeadGrammarBuilder(manifest, lines, reference).build()
    grammar = render_character(lines, manifest, head_rules)
    ordinary = output / 'Lyra_Procedural_Facial_Elastic.p3d'
    hologram = output / 'Lyra_Facial_Hologram.p3d'
    ordinary.write_text(grammar)
    hologram_source = source.parent / 'Lyra_Active_Hologram.p3d'
    if sha256(hologram_source) != manifest['source']['hologramSha256']:
        raise ValueError('Hologram source hash mismatch; presentation profile is not the pinned source.')
    hologram_lines = hologram_source.read_text().splitlines()
    settings = [line for line in hologram_lines if line.startswith('# @hologram ')]
    if len(settings) != 1:
        raise ValueError('Pinned hologram source must contain exactly one presentation profile.')
    # The remainder is byte-identical grammar. Generic native viewers keep the
    # ordinary palette; only profile-aware presentation consumes this comment.
    hologram.write_text('# Lyra integrated facial hologram; identical native geometry.\n' + settings[0] + '\n' + grammar)
    (output / 'references' / 'head_neutral.p3d').write_text('Start -> LyraHead(0 0)\n\n' + head_rules)
    manifest['generated'] = {'fit': fit, 'assembly': assembly,
                             'nativeReference': {'file': 'references/native_face_neutral.json',
                                                 'geometrySha256': reference.geometry_fingerprint(),
                                                 'shapeKey': reference.mechanics['shapeKey'],
                                                 'canonicalText': reference.mechanics['canonicalText'],
                                                 'landmarksFaceLocalM': reference.landmarks,
                                                 'eyeballRadiusM': reference.eyeball_radius,
                                                 'restUVSource': 'direct native indexed materialUv; independent of render texture tiling',
                                                 'solverPrecision': reference.mechanics['solverPrecision']},
                             'ordinaryGrammarSha256': sha256(ordinary),
                             'hologramGrammarSha256': sha256(hologram),
                             'hologramSourceSha256': sha256(hologram_source),
                             'builderSha256': sha256(Path(__file__)),
                             'runtimeMeshImports': False}
    (output / 'fit_manifest.json').write_text(json.dumps(manifest, indent=2, sort_keys=True) + '\n')
    return manifest


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sampler', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--source', type=Path, default=HERE / 'reference/Lyra_Procedural_NoMesh.p3d')
    parser.add_argument('--manifest', type=Path, default=HERE / 'fit_manifest.json')
    args = parser.parse_args()
    result = build_character(args.sampler, args.source, args.manifest, args.output)
    print(json.dumps({'output': str(args.output), 'fit': result['generated']['fit'],
                      'retainedHeadPatches': len(result['generated']['assembly']['retainedHeadPatches']),
                      'collarPatches': len(result['generated']['assembly']['collars'])}, indent=2))


if __name__ == '__main__':
    main()
