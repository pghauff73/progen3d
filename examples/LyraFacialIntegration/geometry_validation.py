"""Read-only seam, aperture and proxy audits of actual native head buffers.

All geometry comes from native sampler JSON. The manifest identifies intended
correspondences; it is not used as a replacement surface. Aperture visibility
uses finite interior rays and does not certify general self-contact.
"""
from collections import Counter, defaultdict
from dataclasses import dataclass
from array import array
import hashlib
import math


def subtract(first, second):
    return tuple(a - b for a, b in zip(first, second))


def dot(first, second):
    return sum(a * b for a, b in zip(first, second))


def cross(first, second):
    return (first[1]*second[2] - first[2]*second[1],
            first[2]*second[0] - first[0]*second[2],
            first[0]*second[1] - first[1]*second[0])


def native_triangle_quality(coordinates, structured_quad_pairs=False):
    minimum_area, previous_normal, minimum_quad_dot = math.inf, None, 1.0
    for start in range(0, len(coordinates), 9):
        a, b, c = (tuple(coordinates[i:i+3]) for i in range(start, start+9, 3))
        normal = cross(subtract(b, a), subtract(c, a))
        length = math.sqrt(dot(normal, normal))
        minimum_area = min(minimum_area, length*0.5)
        unit = tuple(value/length for value in normal) if length else (0.0, 0.0, 0.0)
        if structured_quad_pairs and start//9 % 2:
            minimum_quad_dot = min(minimum_quad_dot, dot(previous_normal, unit))
        previous_normal = unit
    return minimum_area, minimum_quad_dot


class NativeFrameTransform:
    def __init__(self, column_major):
        if len(column_major) != 16 or not all(math.isfinite(v) for v in column_major):
            raise ValueError("A native transform must contain 16 finite values")
        self.matrix = column_major
        self.columns = [tuple(column_major[4*j+i] for i in range(3)) for j in range(3)]
        self.translation = tuple(column_major[12:15])
        determinant = dot(self.columns[0], cross(self.columns[1], self.columns[2]))
        if abs(determinant) < 1e-14:
            raise ValueError("The native face frame is singular")
        self.inverse_rows = [tuple(v / determinant for v in cross(self.columns[1], self.columns[2])),
                             tuple(v / determinant for v in cross(self.columns[2], self.columns[0])),
                             tuple(v / determinant for v in cross(self.columns[0], self.columns[1]))]
        self.rigidity_error = max(
            abs(dot(self.columns[i], self.columns[j]) - (1.0 if i == j else 0.0))
            for i in range(3) for j in range(3))
        self.determinant = determinant

    def point(self, local):
        return tuple(self.translation[i] + sum(self.columns[j][i]*local[j] for j in range(3))
                     for i in range(3))

    def inverse_point(self, world):
        relative = subtract(world, self.translation)
        return tuple(dot(row, relative) for row in self.inverse_rows)


class NativeBoundaryGraph:
    """Recover edges from actual triangle corners with a declared tiny weld."""

    def __init__(self, positions=None, triangles=None, weld_tolerance=1e-7):
        self.positions = list(positions or [])
        self.edge_counts = Counter()
        self.edge_directions = {}
        self.maximum_weld_distance = 0.0
        self.collapsed_triangles = 0
        self.weld_tolerance = weld_tolerance
        self.cells = defaultdict(list)
        if triangles is not None:
            for triangle in triangles:
                self.add_triangle(triangle)

    def add_triangle(self, triangle):
        if len(set(triangle)) != 3:
            self.collapsed_triangles += 1
            return
        for first, second in zip(triangle, triangle[1:] + triangle[:1]):
            edge = tuple(sorted((first, second)))
            self.edge_counts[edge] += 1
            self.edge_directions[edge] = (first, second)

    def weld_point(self, point):
        tolerance = self.weld_tolerance
        cell = tuple(math.floor(value / tolerance) for value in point)
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                for dz in (-1, 0, 1):
                    for index in self.cells.get((cell[0]+dx, cell[1]+dy, cell[2]+dz), ()):
                        distance = math.dist(point, self.positions[index])
                        if distance <= tolerance:
                            self.maximum_weld_distance = max(self.maximum_weld_distance, distance)
                            return index
        index = len(self.positions)
        self.positions.append(point)
        self.cells[cell].append(index)
        return index

    def add_native_soup(self, coordinates):
        if len(coordinates) % 9 or not all(math.isfinite(v) for v in coordinates):
            raise ValueError("An actual native triangle buffer is invalid")
        for start in range(0, len(coordinates), 9):
            triangle = tuple(self.weld_point(tuple(coordinates[i:i+3]))
                             for i in range(start, start+9, 3))
            self.add_triangle(triangle)

    def finish(self):
        self.boundary_edges = {edge for edge, count in self.edge_counts.items() if count == 1}
        self.boundary_vertices = {index for edge in self.boundary_edges for index in edge}
        self.nonmanifold_edges = sum(count > 2 for count in self.edge_counts.values())
        self.boundary_adjacency = defaultdict(set)
        for first, second in self.boundary_edges:
            self.boundary_adjacency[first].add(second)
            self.boundary_adjacency[second].add(first)
        return self

    def nearest_boundary_vertex(self, point):
        if not self.boundary_vertices:
            raise ValueError("Expected an actual native boundary, but none was found")
        index = min(self.boundary_vertices, key=lambda item: math.dist(point, self.positions[item]))
        return index, math.dist(point, self.positions[index])

    def directed_edge_sign(self, first, second):
        edge = tuple(sorted((first, second)))
        if edge not in self.boundary_edges:
            raise ValueError("A declared join interval is not an actual native boundary edge")
        return 1 if self.edge_directions[edge] == (first, second) else -1


@dataclass
class ProjectedNativeTriangle:
    points: tuple
    rule: str
    object_name: str
    triangle_index: int

    def __post_init__(self):
        self.minimum = tuple(min(point[k] for point in self.points) for k in (0, 1))
        self.maximum = tuple(max(point[k] for point in self.points) for k in (0, 1))
        self.center = tuple((a+b)*0.5 for a, b in zip(self.minimum, self.maximum))

    def intersect_parallel_ray(self, x, y):
        a, b, c = self.points
        denominator = (b[1]-c[1])*(a[0]-c[0]) + (c[0]-b[0])*(a[1]-c[1])
        if abs(denominator) <= 1e-20:
            return None
        first = ((b[1]-c[1])*(x-c[0]) + (c[0]-b[0])*(y-c[1])) / denominator
        second = ((c[1]-a[1])*(x-c[0]) + (a[0]-c[0])*(y-c[1])) / denominator
        third = 1.0-first-second
        if min(first, second, third) < -1e-10:
            return None
        return first*a[2] + second*b[2] + third*c[2]


class ParallelNativeRayIndex:
    """A 2D bounding hierarchy for exact barycentric +Z/-Z ray intersections."""

    def __init__(self, triangles):
        self.triangles = triangles
        self.root = self.build(list(range(len(triangles))))

    def build(self, indices):
        minimum = tuple(min(self.triangles[i].minimum[k] for i in indices) for k in (0, 1))
        maximum = tuple(max(self.triangles[i].maximum[k] for i in indices) for k in (0, 1))
        if len(indices) <= 24:
            return minimum, maximum, indices, None, None
        axis = max((0, 1), key=lambda k: maximum[k]-minimum[k])
        indices.sort(key=lambda i: self.triangles[i].center[axis])
        middle = len(indices)//2
        return minimum, maximum, None, self.build(indices[:middle]), self.build(indices[middle:])

    def intersections(self, x, y):
        stack, hits = [self.root], []
        while stack:
            minimum, maximum, indices, left, right = stack.pop()
            if x < minimum[0]-1e-12 or x > maximum[0]+1e-12 or y < minimum[1]-1e-12 or y > maximum[1]+1e-12:
                continue
            if indices is None:
                stack.extend((left, right))
                continue
            for index in indices:
                triangle = self.triangles[index]
                height = triangle.intersect_parallel_ray(x, y)
                if height is not None:
                    hits.append((height, triangle))
        return hits


def point_inside_polygon(point, polygon):
    x, y = point
    inside = False
    for first, second in zip(polygon, polygon[1:] + polygon[:1]):
        if (first[1] > y) != (second[1] > y):
            crossing = first[0] + (y-first[1])*(second[0]-first[0])/(second[1]-first[1])
            if x < crossing:
                inside = not inside
    return inside


def polygon_vertical_intervals(x, polygon):
    intersections = []
    for first, second in zip(polygon, polygon[1:] + polygon[:1]):
        if (first[0] > x) != (second[0] > x):
            intersections.append(first[1] + (x-first[0])*(second[1]-first[1])/(second[0]-first[0]))
    intersections.sort()
    if len(intersections) % 2:
        raise ValueError("An aperture projection has an invalid vertical crossing count")
    return list(zip(intersections[::2], intersections[1::2]))


class LyraHeadGeometryAudit:
    def __init__(self, head_sample, manifest, prepared_apertures=None):
        scene = head_sample.get("scene", head_sample)
        self.instances = scene["instances"]
        self.head_sample = head_sample
        self.prepared_apertures = prepared_apertures
        self.manifest = manifest
        self.assembly = manifest["generated"]["assembly"]
        self.by_rule = defaultdict(list)
        for instance in self.instances:
            self.by_rule[instance["source"]["rule"]].append(instance)
        self.checks, self.metrics, self.failures = {}, {}, []
        self.contact_seam_chains = defaultdict(list)
        self.gap_limit = manifest["collar"]["maximumGapM"]
        self.face = self.single(self.assembly["faceRule"])
        self.face_frame = NativeFrameTransform(self.face["primaryTransform"])
        self.head_frame = NativeFrameTransform(self.single(self.assembly["collars"][0]["rule"])["primaryTransform"])
        self.native_mesh = self.face["facial"]["nativeMesh"]
        self.face_graph = self.build_face_graph()

    def single(self, rule):
        instances = self.by_rule.get(rule, [])
        if len(instances) != 1:
            raise ValueError(f"Expected one native instance for {rule}, found {len(instances)}")
        if "geometry" not in instances[0]:
            raise ValueError(f"Actual native triangle positions were not captured for {rule}")
        return instances[0]

    def check(self, name, condition, failure):
        self.checks[name] = bool(condition)
        if not condition:
            self.failures.append(failure)

    def build_face_graph(self):
        local = self.native_mesh["positions"]
        ids = self.native_mesh["triangles"]
        world = self.face["geometry"]["positions"]
        if len(world) != len(ids)*3:
            raise ValueError("Native face indexed mesh and actual expanded buffer differ in length")
        points = [None] * (len(local)//3)
        maximum_duplicate_error, maximum_transform_error = 0.0, 0.0
        for corner, index in enumerate(ids):
            point = tuple(world[corner*3:corner*3+3])
            if points[index] is not None:
                maximum_duplicate_error = max(maximum_duplicate_error, math.dist(points[index], point))
            points[index] = point
            maximum_transform_error = max(maximum_transform_error,
                math.dist(point, self.face_frame.point(local[index*3:index*3+3])))
        if any(point is None for point in points):
            raise ValueError("A native face vertex has no actual world-buffer correspondence")
        self.metrics["maximumFaceIndexedToWorldCornerErrorM"] = maximum_transform_error
        self.check("faceBufferCorrespondence", maximum_duplicate_error <= 1e-10 and maximum_transform_error <= 2e-6,
                   "The native face's indexed vertices do not match its actual world triangle buffer")
        minimum_area, _ = native_triangle_quality(world)
        self.metrics["actualFaceMinimumTriangleAreaM2"] = minimum_area
        self.check("facePositiveTriangleArea", minimum_area > 1e-14,
                   f"The actual native face has a degenerate triangle (minimum area {minimum_area:.9g} m²)")
        return NativeBoundaryGraph(points, [tuple(ids[i:i+3]) for i in range(0, len(ids), 3)]).finish()

    def map_actual_boundary(self, graph, expected_points, name):
        mapped, maximum = [], 0.0
        for point in expected_points:
            index, distance = graph.nearest_boundary_vertex(point)
            mapped.append(index)
            maximum = max(maximum, distance)
        edges_exist = all(tuple(sorted(pair)) in graph.boundary_edges for pair in zip(mapped, mapped[1:]))
        self.check(name+"Exists", edges_exist and len(set(mapped)) == len(mapped),
                   name+" is not a complete ordered boundary in the actual native mesh")
        self.check(name+"Placement", maximum <= self.gap_limit,
                   f"{name} actual boundary differs from its declaration by {maximum:.9g} m")
        return mapped, maximum

    def radial_chain(self, graph, first, last, excluded):
        chain, previous, current = [first], None, first
        for _ in range(len(graph.boundary_vertices)+1):
            if current == last:
                return chain
            choices = [other for other in graph.boundary_adjacency[current]
                       if other != previous and tuple(sorted((current, other))) not in excluded]
            if len(choices) != 1:
                raise ValueError("A collar corner does not define one continuous radial boundary")
            previous, current = current, choices[0]
            chain.append(current)
        raise ValueError("A collar corner boundary did not reach the retained-head seam")

    def audit_seams(self):
        retained = NativeBoundaryGraph()
        for patch in self.assembly["retainedHeadPatches"]:
            retained.add_native_soup(self.single(patch["rule"])["geometry"]["positions"])
        retained.finish()
        collar_reports, radial_chains = [], []
        for collar in self.assembly["collars"]:
            rule = collar["rule"]
            graph = NativeBoundaryGraph()
            coordinates = self.single(rule)["geometry"]["positions"]
            graph.add_native_soup(coordinates)
            graph.finish()
            minimum_area, minimum_quad_dot = native_triangle_quality(coordinates, True)
            self.check(rule+"ActualTriangleQuality", minimum_area > 1e-14 and minimum_quad_dot > 0,
                       f"{rule}: actual area {minimum_area:.9g} m², minimum paired-quad normal dot {minimum_quad_dot:.9g}")
            face_ids = collar["faceNativeVertexIds"]
            actual_face = [self.face_graph.positions[index] for index in face_ids]
            face_edges = all(tuple(sorted(pair)) in self.face_graph.boundary_edges
                             for pair in zip(face_ids, face_ids[1:]))
            self.check(rule+"NativeFaceBoundary", face_edges,
                       rule+" names a face perimeter that is not an actual native boundary")
            expected_face = [self.head_frame.point(point) for point in collar["joinSamplesHeadLocalM"]["face"]]
            expected_head = [self.head_frame.point(point) for point in collar["joinSamplesHeadLocalM"]["head"]]
            face_map, face_declaration_gap = self.map_actual_boundary(graph, expected_face, rule+"FaceJoin")
            head_map, head_declaration_gap = self.map_actual_boundary(graph, expected_head, rule+"HeadJoin")
            retained_points = [self.head_frame.point(point) for point in collar["headBoundaryHeadLocalM"]]
            retained_map, retained_declaration_gap = self.map_actual_boundary(retained, retained_points, rule+"RetainedJoin")
            self.contact_seam_chains[frozenset((rule, self.assembly["faceRule"]))].append(actual_face)
            actual_head_join = [retained.positions[index] for index in retained_map]
            for patch in self.assembly["retainedHeadPatches"]:
                self.contact_seam_chains[frozenset((rule, patch["rule"]))].append(actual_head_join)
            factor = collar["faceKnotsEverySamples"]
            head_factor = collar["headKnotsEverySamples"]
            if len(face_map) != (len(face_ids)-1)*factor+1 or len(head_map) != (len(retained_map)-1)*head_factor+1:
                raise ValueError(rule+" join subdivision does not match declared native boundary knots")
            face_gap, head_gap, orientation_errors = 0.0, 0.0, 0
            for index, actual_id in enumerate(face_map):
                interval = min(index//factor, len(actual_face)-2)
                fraction = (index-interval*factor)/factor
                expected = tuple(a+(b-a)*fraction for a, b in zip(actual_face[interval], actual_face[interval+1]))
                face_gap = max(face_gap, math.dist(graph.positions[actual_id], expected))
            for index, actual_id in enumerate(head_map):
                interval = min(index//head_factor, len(retained_map)-2)
                fraction = (index-interval*head_factor)/head_factor
                first, second = (retained.positions[retained_map[interval+j]] for j in (0, 1))
                expected = tuple(a+(b-a)*fraction for a, b in zip(first, second))
                head_gap = max(head_gap, math.dist(graph.positions[actual_id], expected))
            if face_edges:
                for index in range(len(face_map)-1):
                    interval = index//factor
                    orientation_errors += graph.directed_edge_sign(face_map[index], face_map[index+1]) == self.face_graph.directed_edge_sign(face_ids[interval], face_ids[interval+1])
                for index in range(len(head_map)-1):
                    interval = index//head_factor
                    orientation_errors += graph.directed_edge_sign(head_map[index], head_map[index+1]) == retained.directed_edge_sign(retained_map[interval], retained_map[interval+1])
            self.check(rule+"ActualSeamGap", max(face_gap, head_gap) <= self.gap_limit,
                       f"{rule} actual face/head seam gap is {max(face_gap, head_gap):.9g} m")
            self.check(rule+"ConsistentSeamWinding", orientation_errors == 0,
                       f"{rule} has {orientation_errors} adjacent boundary intervals with inconsistent orientation")
            self.check(rule+"ManifoldBoundary", graph.nonmanifold_edges == 0 and graph.collapsed_triangles == 0,
                       rule+" contains nonmanifold edges or tolerance-collapsed triangles")
            excluded = {tuple(sorted(pair)) for mapping in (face_map, head_map) for pair in zip(mapping, mapping[1:])}
            radial_chains.append((rule, graph,
                self.radial_chain(graph, face_map[0], head_map[0], excluded),
                self.radial_chain(graph, face_map[-1], head_map[-1], excluded)))
            collar_reports.append({"rule": rule, "pairedFaceSamples": len(face_map), "pairedHeadSamples": len(head_map),
                "maximumFaceSeamGapM": face_gap, "maximumRetainedHeadSeamGapM": head_gap,
                "maximumManifestPlacementGapM": max(face_declaration_gap, head_declaration_gap, retained_declaration_gap),
                "minimumActualTriangleAreaM2": minimum_area, "minimumActualQuadNormalDot": minimum_quad_dot,
                "inconsistentBoundaryIntervals": orientation_errors, "maximumTopologyWeldM": graph.maximum_weld_distance})
        corner_reports = []
        for first, second in zip(radial_chains, radial_chains[1:]+radial_chains[:1]):
            first_rule, first_graph, _, first_chain = first
            second_rule, second_graph, second_chain, _ = second
            if len(first_chain) != len(second_chain):
                raise ValueError("Adjacent actual collar radial boundaries have different subdivisions")
            gap = max(math.dist(first_graph.positions[a], second_graph.positions[b])
                      for a, b in zip(first_chain, second_chain))
            orientation_errors = sum(first_graph.directed_edge_sign(a, b) == second_graph.directed_edge_sign(c, d)
                for (a, b), (c, d) in zip(zip(first_chain, first_chain[1:]), zip(second_chain, second_chain[1:])))
            name = first_rule+"To"+second_rule
            self.contact_seam_chains[frozenset((first_rule, second_rule))].append(
                [first_graph.positions[index] for index in first_chain])
            self.check(name, gap <= self.gap_limit and orientation_errors == 0,
                       f"{name}: corner gap {gap:.9g} m, orientation mismatches {orientation_errors}")
            corner_reports.append({"first": first_rule, "second": second_rule, "maximumGapM": gap,
                                   "pairedSamples": len(first_chain), "inconsistentBoundaryIntervals": orientation_errors})
        self.check("retainedHeadManifoldEdges", retained.nonmanifold_edges == 0,
                   "The combined actual retained-head patches contain nonmanifold edges")
        self.metrics.update({"seamLimitM": self.gap_limit, "topologyWeldToleranceM": retained.weld_tolerance,
            "retainedHeadMaximumWeldM": retained.maximum_weld_distance,
            "retainedHeadToleranceCollapsedTriangles": retained.collapsed_triangles,
            "collarSeams": collar_reports, "collarCorners": corner_reports})

    def audit_proxies(self):
        evidence = []
        radius = self.face["facial"]["eyeballRadiusLocalM"]
        landmarks = self.face["facial"]["localLandmarks"]
        for eye in self.assembly["eyes"]:
            center = landmarks[eye["side"]+"_eyeball_center"]
            reference_error = max(math.dist(center, eye["centerFaceLocalM"]), abs(radius-eye["scleraRadiusM"]))
            self.check(eye["side"]+"EyeSharedReference", reference_error <= 1e-7,
                       "The "+eye["side"]+" eye proxy differs from the native face's globe reference")
            for kind, offset in (("sclera", 0.0), ("iris", eye["irisRadialOffsetM"]), ("pupil", eye["pupilRadialOffsetM"])):
                rule = eye[kind+"Rule"]
                instance = self.single(rule)
                coordinates = instance["geometry"]["positions"]
                radial_error = max(abs(math.dist(self.face_frame.inverse_point(coordinates[i:i+3]), center)-(radius+offset))
                                   for i in range(0, len(coordinates), 3))
                actual_center = self.face_frame.inverse_point(instance["primaryTransform"][12:15])
                center_error = math.dist(actual_center, center)
                self.check(rule+"ActualProxy", max(radial_error, center_error) <= 1e-7,
                           f"{rule}: actual native center/radius differs from its declared reference by {max(radial_error, center_error):.9g} m")
                evidence.append({"rule": rule, "maximumRadialVertexErrorM": radial_error,
                                 "centerErrorM": center_error, "sharedSphereRadiusM": radius+offset})
        self.metrics["eyeProxies"] = evidence

    def audit_apertures(self):
        prepared = self.prepared_apertures or PreparedLyraApertureAudit(self.head_sample, self.manifest)
        if self.prepared_apertures is not None and not prepared.matches_static_head(self.head_sample):
            raise ValueError("The prepared aperture geometry does not match the actual static head buffers")
        result = prepared.audit(self.face)
        self.checks.update(result["checks"])
        self.metrics.update(result["metrics"])
        self.failures.extend(result["failures"])

    def run(self):
        self.check("rigidFaceAndHeadFrames", max(self.face_frame.rigidity_error, self.head_frame.rigidity_error) <= 1e-6
                   and self.face_frame.determinant > 0 and self.head_frame.determinant > 0,
                   "The fitted face or head has a non-rigid or reflected primary transform")
        self.audit_seams()
        self.audit_proxies()
        self.audit_apertures()
        return {"passed": all(self.checks.values()), "checks": self.checks,
                "metrics": self.metrics, "failures": self.failures}


class PreparedLyraApertureAudit:
    """Reuse measured static geometry while replacing the actual native face.

    The active face is transformed from its actual WORLD triangle soup into
    its own native face frame. Thus rigid body/head motion is removed without
    modifying, deriving, or interpolating the facial shape. The caller must
    independently prove that the remaining head remains the same rigid local
    geometry when using audit_pose_apertures on a different world pose.
    """

    def __init__(self, neutral_head_sample, manifest):
        self.assembly = manifest["generated"]["assembly"]
        self.face_rule = self.assembly["faceRule"]
        instances = neutral_head_sample.get("scene", neutral_head_sample)["instances"]
        faces = [instance for instance in instances if instance["source"]["rule"] == self.face_rule]
        if len(faces) != 1:
            raise ValueError("The aperture reference requires exactly one native face")
        reference = faces[0]
        frame = NativeFrameTransform(reference["primaryTransform"])
        reference_mesh = reference["facial"]["nativeMesh"]
        self.topology = tuple(reference_mesh["triangles"])
        self.surface_tags = tuple(reference_mesh["surfaceBoundaries"])
        self.vertex_count = len(reference_mesh["positions"])//3
        self.static_signature = self.static_head_signature(neutral_head_sample)
        records = []
        for instance in instances:
            if instance["source"]["rule"] != self.face_rule:
                records.extend(self.triangle_records(instance, frame))
        self.static_index = ParallelNativeRayIndex(records)
        self.target_rules = {eye["side"]+"_eye": {eye[kind+"Rule"] for kind in ("sclera", "iris", "pupil")}
                             for eye in self.assembly["eyes"]}
        self.target_rules["mouth"] = {self.assembly["oralInterior"]["rule"]}
        self.boundaries = self.front_aperture_boundaries()

    def front_aperture_boundaries(self):
        """Recover each front/rim interface; the open topological edge is at the back.

        A deformed rim can tilt, so its back boundary is not an appropriate
        projected sampling region for the opening seen from the front. The
        manifest identifies the back loop only; actual native triangle tags
        identify the corresponding rim and its connected front interface.
        """
        if len(self.surface_tags)*3 != len(self.topology):
            raise ValueError("Native facial surface tags do not match the triangles")
        edge_tags = defaultdict(list)
        for start in range(0, len(self.topology), 3):
            triangle = self.topology[start:start+3]
            for first, second in zip(triangle, triangle[1:]+triangle[:1]):
                edge_tags[tuple(sorted((first, second)))].append(self.surface_tags[start//3])
        boundaries = []
        for declared in self.assembly["boundaryLoops"]:
            if declared["name"] not in self.target_rules:
                continue
            back_ids = tuple(declared["nativeVertexIds"])
            rim_tags = set()
            for first, second in zip(back_ids, back_ids[1:]+back_ids[:1]):
                tags = edge_tags.get(tuple(sorted((first, second))), [])
                if len(tags) != 1 or tags[0] == 0:
                    raise ValueError(f"{declared['name']} does not identify an actual native back-rim boundary")
                rim_tags.add(tags[0])
            if len(rim_tags) != 1:
                raise ValueError(f"{declared['name']} spans more than one native rim surface")
            rim_tag = next(iter(rim_tags))
            adjacency = defaultdict(set)
            for (first, second), tags in edge_tags.items():
                if len(tags) == 2 and set(tags) == {0, rim_tag}:
                    adjacency[first].add(second)
                    adjacency[second].add(first)
            if len(adjacency) < 3 or any(len(neighbors) != 2 for neighbors in adjacency.values()):
                raise ValueError(f"{declared['name']} front/rim interface is not a closed native loop")
            first = min(adjacency)
            ordered, previous, current = [first], first, min(adjacency[first])
            while current != first:
                if current in ordered:
                    raise ValueError(f"{declared['name']} front/rim interface repeats a vertex")
                ordered.append(current)
                following = next(index for index in adjacency[current] if index != previous)
                previous, current = current, following
            if len(ordered) != len(adjacency):
                raise ValueError(f"{declared['name']} has disconnected native front/rim interfaces")
            boundaries.append({"name": declared["name"], "frontIds": tuple(ordered),
                               "backIds": back_ids, "rimTag": rim_tag})
        return boundaries

    def static_head_signature(self, sample):
        digest = hashlib.sha256()
        for instance in sample.get("scene", sample)["instances"]:
            if instance["source"]["rule"] == self.face_rule:
                continue
            digest.update(instance["source"]["rule"].encode("utf-8"))
            digest.update(b"\0")
            digest.update(instance["objectName"].encode("utf-8"))
            digest.update(array("f", instance["geometry"]["positions"]).tobytes())
        return digest.hexdigest()

    def matches_static_head(self, sample):
        return self.static_head_signature(sample) == self.static_signature

    @staticmethod
    def triangle_records(instance, frame):
        coordinates = instance["geometry"]["positions"]
        if len(coordinates) % 9 or not all(math.isfinite(v) for v in coordinates):
            raise ValueError("An actual native aperture-obstruction buffer is invalid")
        records = []
        for start in range(0, len(coordinates), 9):
            points = tuple(frame.inverse_point(coordinates[i:i+3]) for i in range(start, start+9, 3))
            records.append(ProjectedNativeTriangle(points, instance["source"]["rule"], instance["objectName"], start//9))
        return records

    def audit(self, active_face_instance):
        checks, failures, reports = {}, [], []
        mesh = active_face_instance["facial"]["nativeMesh"]
        if (tuple(mesh["triangles"]) != self.topology or
                tuple(mesh["surfaceBoundaries"]) != self.surface_tags or
                len(mesh["positions"]) != 3*self.vertex_count):
            raise ValueError("A prepared aperture index requires the same native facial topology")
        frame = NativeFrameTransform(active_face_instance["primaryTransform"])
        if frame.rigidity_error > 1e-6 or frame.determinant <= 0:
            raise ValueError("A pose aperture audit requires a rigid, non-reflected native face frame")
        records = self.triangle_records(active_face_instance, frame)
        face_index = ParallelNativeRayIndex(records)
        actual_points = [None]*self.vertex_count
        maximum_local_roundtrip = 0.0
        for triangle_index, triangle in enumerate(records):
            for corner, point in enumerate(triangle.points):
                vertex_id = self.topology[3*triangle_index+corner]
                actual_points[vertex_id] = point
                maximum_local_roundtrip = max(maximum_local_roundtrip,
                    math.dist(point, mesh["positions"][3*vertex_id:3*vertex_id+3]))
        checks["poseActualFaceFrameCorrespondence"] = maximum_local_roundtrip <= 2e-6
        if not checks["poseActualFaceFrameCorrespondence"]:
            failures.append("Actual world face corners do not recover the recorded native local surface")
        for boundary in self.boundaries:
            name = boundary["name"]
            polygon = [actual_points[index] for index in boundary["frontIds"]]
            back_polygon = [actual_points[index] for index in boundary["backIds"]]
            minimum = [min(point[k] for point in polygon) for k in (0, 1)]
            maximum = [max(point[k] for point in polygon) for k in (0, 1)]
            samples, blocked, missing_target, common_samples = [], [], [], []

            def trace_ray(x, y):
                hits = self.static_index.intersections(x, y) + face_index.intersections(x, y)
                interior = [height for height, triangle in hits if triangle.rule in self.target_rules[name]]
                result = {"pointFaceLocalXYM": [x, y], "interiorTargetPresent": bool(interior),
                          "blockers": []}
                if not interior:
                    return result
                target_height = max(interior)
                for height, triangle in hits:
                    if triangle.rule in self.target_rules[name] or height <= target_height+1e-7:
                        continue
                    surface_tag = self.surface_tags[triangle.triangle_index] if triangle.rule == self.face_rule else None
                    result["blockers"].append({"pointFaceLocalXYM": [x, y], "objectName": triangle.object_name,
                        "rule": triangle.rule, "triangle": triangle.triangle_index,
                        "nativeFacialSurfaceTag": surface_tag,
                        "blockerKind": ("inner rim" if surface_tag == boundary["rimTag"] else
                                        "front skin" if surface_tag == 0 else "other native head surface"),
                        "inFrontOfTargetByM": height-target_height})
                result["blockers"].sort(key=lambda item: item["inFrontOfTargetByM"], reverse=True)
                return result

            front_results = []
            for x_fraction in (-0.75, -0.5, -0.25, 0.0, 0.25, 0.5, 0.75):
                for y_fraction in (-0.5, 0.0, 0.5):
                    x = (minimum[0]+maximum[0])*0.5 + x_fraction*(maximum[0]-minimum[0])*0.5
                    y = (minimum[1]+maximum[1])*0.5 + y_fraction*(maximum[1]-minimum[1])*0.5
                    if not point_inside_polygon((x, y), polygon):
                        continue
                    samples.append([x, y])
                    result = trace_ray(x, y)
                    front_results.append(result)
                    if point_inside_polygon((x, y), back_polygon):
                        common_samples.append(result)
                    if not result["interiorTargetPresent"]:
                        missing_target.append([x, y])
                    if result["blockers"]:
                        blocked.append(result["blockers"][0])
            central_x = (minimum[0]+maximum[0])*0.5
            intervals = [(max(front[0], back[0]), min(front[1], back[1]))
                         for front in polygon_vertical_intervals(central_x, polygon)
                         for back in polygon_vertical_intervals(central_x, back_polygon)
                         if max(front[0], back[0]) < min(front[1], back[1])]
            central = None
            central_height = 0.0
            if intervals:
                lower, upper = max(intervals, key=lambda interval: interval[1]-interval[0])
                central_height = upper-lower
                central = trace_ray(central_x, (lower+upper)*0.5)
                if not any(math.dist(central["pointFaceLocalXYM"], ray["pointFaceLocalXYM"]) < 1e-10
                           for ray in common_samples):
                    common_samples.append(central)
            foreign_blocked = [ray for ray in front_results
                               if any(item["blockerKind"] != "inner rim" for item in ray["blockers"])]
            common_blocked = [ray for ray in common_samples if ray["blockers"]]
            common_missing = [ray for ray in common_samples if not ray["interiorTargetPresent"]]
            checks[name+"NoForeignApertureObstruction"] = len(samples) >= 9 and not foreign_blocked and not missing_target
            checks[name+"CommonProjectedAperturePassage"] = len(common_samples) >= 9 and not common_blocked and not common_missing
            checks[name+"CentralPassage"] = (central is not None and central_height > 1e-7
                                              and central["interiorTargetPresent"] and not central["blockers"])
            if not checks[name+"NoForeignApertureObstruction"]:
                failures.append(f"{name}: {len(foreign_blocked)} non-rim blocked rays and {len(missing_target)} missing targets out of {len(samples)} front-opening rays")
            if not checks[name+"CommonProjectedAperturePassage"]:
                failures.append(f"{name}: {len(common_blocked)} blocked rays and {len(common_missing)} missing targets in {len(common_samples)} front/back-common rays")
            if not checks[name+"CentralPassage"]:
                failures.append(f"{name}: no positive unobstructed central front/back-common passage")
            reports.append({"aperture": name, "sampleCount": len(samples), "samplePointsFaceLocalXYM": samples,
                            "sampleRegion": "actual native front/rim interface loop",
                            "frontInterfaceVertexIds": list(boundary["frontIds"]),
                            "backBoundaryVertexCount": len(boundary["backIds"]),
                            "rimSurfaceTag": boundary["rimTag"],
                            "blockedRays": blocked, "missingInteriorTargets": missing_target,
                            "innerRimOcclusionCount": sum(ray["blockerKind"] == "inner rim" for ray in blocked),
                            "frontProjectionRayResults": front_results,
                            "commonProjectionSampleCount": len(common_samples),
                            "commonProjectionRayResults": common_samples,
                            "commonProjectionBlockedRayCount": len(common_blocked),
                            "centralCommonOpeningHeightM": central_height,
                            "centralPassageRay": central,
                            "expectedVisibleRules": sorted(self.target_rules[name])})
        checks["allAperturesAudited"] = len(reports) == 3
        if not checks["allAperturesAudited"]:
            failures.append("Expected native left-eye, right-eye and mouth boundary loops")
        return {"passed": all(checks.values()), "checks": checks, "failures": failures,
                "metrics": {"apertureVisibility": reports,
                    "maximumActualWorldToFaceLocalErrorM": maximum_local_roundtrip,
                    "preparedStaticHeadPositionSha256": self.static_signature,
                    "apertureVisibilityScope": "Finite +Z face-frame rays against all actual native triangles. "
                        "Front-loop rays retain and classify rim occlusion; front/back-common rays and a positive "
                        "central passage must reach the intended interior without any blocker. "
                        "Active face corners are inverse-transformed from their actual world buffer; reuse assumes "
                        "the separately verified rigid/static head geometry. This is not a general self-intersection proof."}}


def prepare_aperture_audit(neutral_head_sample: dict, manifest: dict) -> PreparedLyraApertureAudit:
    return PreparedLyraApertureAudit(neutral_head_sample, manifest)


def audit_pose_apertures(prepared: PreparedLyraApertureAudit, active_face_instance: dict) -> dict:
    try:
        return prepared.audit(active_face_instance)
    except (KeyError, IndexError, TypeError, ValueError, ArithmeticError) as error:
        return {"passed": False, "checks": {"completePoseApertureAudit": False},
                "metrics": {}, "failures": [str(error)]}


@dataclass
class NativeContactTriangle:
    points: tuple
    rule: str
    primitive_ordinal: int
    triangle_index: int

    def __post_init__(self):
        self.minimum = tuple(min(point[k] for point in self.points) for k in range(3))
        self.maximum = tuple(max(point[k] for point in self.points) for k in range(3))
        self.center = tuple((a+b)*0.5 for a, b in zip(self.minimum, self.maximum))

    def identity(self):
        return {"rule": self.rule, "primitiveOrdinal": self.primitive_ordinal,
                "triangle": self.triangle_index}


class NativeTriangleContactIndex:
    """A 3D broad phase over actual exported triangle corners."""

    def __init__(self, triangles):
        if not triangles:
            raise ValueError("A native contact index needs actual triangles")
        self.triangles = triangles
        self.root = self.build(list(range(len(triangles))))

    def build(self, indices):
        minimum = tuple(min(self.triangles[i].minimum[k] for i in indices) for k in range(3))
        maximum = tuple(max(self.triangles[i].maximum[k] for i in indices) for k in range(3))
        if len(indices) <= 16:
            return minimum, maximum, indices, None, None
        axis = max(range(3), key=lambda k: maximum[k]-minimum[k])
        indices.sort(key=lambda i: self.triangles[i].center[axis])
        middle = len(indices)//2
        return minimum, maximum, None, self.build(indices[:middle]), self.build(indices[middle:])

    def candidates(self, triangle, tolerance):
        stack = [self.root]
        while stack:
            minimum, maximum, indices, left, right = stack.pop()
            if any(minimum[k] > triangle.maximum[k]+tolerance or maximum[k] < triangle.minimum[k]-tolerance
                   for k in range(3)):
                continue
            if indices is None:
                stack.extend((left, right))
            else:
                for index in indices:
                    other = self.triangles[index]
                    if all(other.minimum[k] <= triangle.maximum[k]+tolerance and
                           other.maximum[k] >= triangle.minimum[k]-tolerance for k in range(3)):
                        yield other


class NativeTriangleContactService:
    """Plane/segment intersection, including a projected coplanar case.

    This independent narrow phase does not infer clearance from vertices or
    radial rays. The finite 1 nm tolerance treats touching as contact; it does
    not establish a positive clearance distance or a general contact solve.
    """

    TOLERANCE_M = 1e-9

    @staticmethod
    def records(instance):
        coordinates = instance["geometry"]["positions"]
        if len(coordinates) % 9 or not all(math.isfinite(value) for value in coordinates):
            raise ValueError("A native hair contact buffer is invalid")
        return [NativeContactTriangle(tuple(tuple(coordinates[i:i+3]) for i in range(start, start+9, 3)),
                                      instance["source"]["rule"], instance["source"]["primitiveOrdinal"], start//9)
                for start in range(0, len(coordinates), 9)]

    @staticmethod
    def point_in_triangle(point, triangle, normal, tolerance):
        for first, second in zip(triangle, triangle[1:]+triangle[:1]):
            edge = subtract(second, first)
            signed = dot(cross(edge, subtract(point, first)), normal)
            if signed < -tolerance*math.sqrt(dot(edge, edge)):
                return False
        return True

    @staticmethod
    def coplanar_overlap(first, second, normal, tolerance):
        omitted = max(range(3), key=lambda k: abs(normal[k]))
        axes = [k for k in range(3) if k != omitted]
        a = [(point[axes[0]], point[axes[1]]) for point in first]
        b = [(point[axes[0]], point[axes[1]]) for point in second]
        for triangle in (a, b):
            for start, end in zip(triangle, triangle[1:]+triangle[:1]):
                axis = (start[1]-end[1], end[0]-start[0])
                projected_a = [dot(point, axis) for point in a]
                projected_b = [dot(point, axis) for point in b]
                allowance = tolerance*math.sqrt(dot(axis, axis))
                if min(projected_a) > max(projected_b)+allowance or min(projected_b) > max(projected_a)+allowance:
                    return False
        return True

    @classmethod
    def intersects(cls, first, second):
        triangles = (first.points, second.points)
        normals = []
        for triangle in triangles:
            normal = cross(subtract(triangle[1], triangle[0]), subtract(triangle[2], triangle[0]))
            length = math.sqrt(dot(normal, normal))
            if length <= 1e-15:
                raise ValueError("A hair contact triangle is degenerate at the audit's numeric scale")
            normals.append(tuple(value/length for value in normal))
        distances = [[dot(subtract(point, triangles[i][0]), normals[i]) for point in triangles[1-i]]
                     for i in range(2)]
        if any(min(values) > cls.TOLERANCE_M or max(values) < -cls.TOLERANCE_M for values in distances):
            return False
        normal_cross = cross(*normals)
        if dot(normal_cross, normal_cross) <= 1e-20:
            return cls.coplanar_overlap(*triangles, normals[0], cls.TOLERANCE_M)
        for plane_index in range(2):
            triangle, other = triangles[plane_index], triangles[1-plane_index]
            normal, values = normals[plane_index], distances[plane_index]
            for edge in range(3):
                first_point, second_point = other[edge], other[(edge+1)%3]
                first_distance, second_distance = values[edge], values[(edge+1)%3]
                if abs(first_distance) <= cls.TOLERANCE_M and cls.point_in_triangle(first_point, triangle, normal, cls.TOLERANCE_M):
                    return True
                if (first_distance > 0) == (second_distance > 0):
                    continue
                fraction = first_distance/(first_distance-second_distance)
                point = tuple(a+fraction*(b-a) for a, b in zip(first_point, second_point))
                if cls.point_in_triangle(point, triangle, normal, cls.TOLERANCE_M):
                    return True
        return False

    @classmethod
    def intersection_points(cls, first, second):
        """Return contact endpoints for classifying intended seam-only contact."""
        if not cls.intersects(first, second):
            return []
        triangles, normals = (first.points, second.points), []
        for triangle in triangles:
            normal = cross(subtract(triangle[1], triangle[0]), subtract(triangle[2], triangle[0]))
            length = math.sqrt(dot(normal, normal))
            normals.append(tuple(value/length for value in normal))
        points = []
        for plane_index in range(2):
            triangle, other = triangles[plane_index], triangles[1-plane_index]
            normal = normals[plane_index]
            distances = [dot(subtract(point, triangle[0]), normal) for point in other]
            for edge in range(3):
                a, b = other[edge], other[(edge+1)%3]
                da, db = distances[edge], distances[(edge+1)%3]
                if abs(da) <= cls.TOLERANCE_M and cls.point_in_triangle(a, triangle, normal, cls.TOLERANCE_M):
                    points.append(a)
                if (da > 0) != (db > 0):
                    fraction = da/(da-db)
                    point = tuple(x+fraction*(y-x) for x, y in zip(a, b))
                    if cls.point_in_triangle(point, triangle, normal, cls.TOLERANCE_M):
                        points.append(point)
        normal_cross = cross(*normals)
        if dot(normal_cross, normal_cross) <= 1e-20:
            omitted = max(range(3), key=lambda k: abs(normals[0][k]))
            axes = [k for k in range(3) if k != omitted]
            cross2 = lambda a, b: a[0]*b[1]-a[1]*b[0]
            for a, b in zip(triangles[0], triangles[0][1:]+triangles[0][:1]):
                direction_a = tuple(b[k]-a[k] for k in axes)
                for c, d in zip(triangles[1], triangles[1][1:]+triangles[1][:1]):
                    direction_b = tuple(d[k]-c[k] for k in axes)
                    denominator = cross2(direction_a, direction_b)
                    if abs(denominator) <= 1e-24:
                        continue
                    separation = tuple(c[k]-a[k] for k in axes)
                    first_fraction = cross2(separation, direction_b)/denominator
                    second_fraction = cross2(separation, direction_a)/denominator
                    if -1e-10 <= first_fraction <= 1+1e-10 and -1e-10 <= second_fraction <= 1+1e-10:
                        points.append(tuple(x+first_fraction*(y-x) for x, y in zip(a, b)))
        if not points:
            raise ValueError("A native triangle contact has no reliable intersection witness")
        return points

    @classmethod
    def audit(cls, hair_instances, skin_instances):
        skin = [triangle for instance in skin_instances for triangle in cls.records(instance)]
        index = NativeTriangleContactIndex(skin)
        groups, contacts, candidate_count = [], [], 0
        for instance in hair_instances:
            group_count, examples = 0, []
            triangles = cls.records(instance)
            for hair_triangle in triangles:
                for skin_triangle in index.candidates(hair_triangle, cls.TOLERANCE_M):
                    candidate_count += 1
                    if cls.intersects(hair_triangle, skin_triangle):
                        group_count += 1
                        if len(examples) < 8:
                            examples.append({"hair": hair_triangle.identity(), "skin": skin_triangle.identity(),
                                             "hairPointsWorldM": hair_triangle.points,
                                             "skinPointsWorldM": skin_triangle.points})
            groups.append({"source": instance["source"], "triangles": len(triangles),
                           "contactPairCount": group_count, "examples": examples})
            contacts.extend(examples)
        count = sum(group["contactPairCount"] for group in groups)
        return {"passed": count == 0, "checks": {"noFittedHairNewSkinTriangleContacts": count == 0},
                "failures": [] if count == 0 else [f"Fitted hair intersects or touches new facial/collar skin in {count} native triangle pairs"],
                "metrics": {"contactPairCount": count, "candidatePairCount": candidate_count,
                            "newSkinTriangleCount": len(skin), "hairGroups": groups,
                            "contactToleranceM": cls.TOLERANCE_M,
                            "scope": "All actual native triangles of the declared fitted hair components versus face and collars. "
                                     "Touching counts as contact; this is not a positive-clearance or whole-head self-collision proof."}}


def audit_fitted_hair_contacts(head_sample: dict, manifest: dict) -> dict:
    """Check native fitted hair against newly introduced skin, once per declared fit."""
    try:
        instances = head_sample.get("scene", head_sample)["instances"]
        assembly = manifest["generated"]["assembly"]
        fitting = assembly["fittedHair"]
        identities = {(fitting["carrierRule"], 0)}
        for strand in fitting["associatedSweepChanges"]:
            identities.add((strand["sourceRule"], strand["primitiveOrdinal"]))
        hair = [instance for instance in instances
                if (instance["source"]["rule"], instance["source"]["primitiveOrdinal"]) in identities]
        if len(hair) != len(identities):
            raise ValueError("Fitted hair identities do not resolve to unique actual native components")
        skin_rules = {assembly["faceRule"]} | {collar["rule"] for collar in assembly["collars"]}
        skin = [instance for instance in instances if instance["source"]["rule"] in skin_rules]
        if len(skin) != len(skin_rules):
            raise ValueError("Expected exactly one actual native mesh per new facial/collar skin rule")
        return NativeTriangleContactService.audit(hair, skin)
    except (KeyError, IndexError, TypeError, ValueError, ArithmeticError) as error:
        return {"passed": False, "checks": {"completeFittedHairContactAudit": False},
                "metrics": {}, "failures": [str(error)]}


class LyraNewSkinContactAudit:
    """Compare component pairs and exclude only measured intended seam contact."""

    SEAM_CLASSIFICATION_TOLERANCE_M = 1e-7

    def __init__(self, head_sample, manifest):
        self.geometry = LyraHeadGeometryAudit(head_sample, manifest)
        self.geometry.audit_seams()
        if self.geometry.failures:
            raise ValueError("Seams must validate before classifying contact: "+"; ".join(self.geometry.failures))
        self.assembly = self.geometry.assembly
        self.new_rules = {self.assembly["faceRule"]} | {collar["rule"] for collar in self.assembly["collars"]}
        self.retained_rules = {patch["rule"] for patch in self.assembly["retainedHeadPatches"]}

    @staticmethod
    def point_segment_distance(point, first, second):
        edge = subtract(second, first)
        length_squared = dot(edge, edge)
        fraction = max(0.0, min(1.0, dot(subtract(point, first), edge)/length_squared)) if length_squared else 0.0
        closest = tuple(a+fraction*b for a, b in zip(first, edge))
        return math.dist(point, closest)

    def intended_seam_contact(self, first, second, points):
        chains = self.geometry.contact_seam_chains.get(frozenset((first.rule, second.rule)), [])
        if not chains:
            return False
        return all(any(self.point_segment_distance(point, a, b) <= self.SEAM_CLASSIFICATION_TOLERANCE_M
                       for chain in chains for a, b in zip(chain, chain[1:])) for point in points)

    def run(self):
        records = [triangle for instance in self.geometry.instances
                   if instance["source"]["rule"] in self.new_rules | self.retained_rules
                   for triangle in NativeTriangleContactService.records(instance)]
        index = NativeTriangleContactIndex(records)
        reports, candidates = {}, 0
        for first in records:
            if first.rule not in self.new_rules:
                continue
            for second in index.candidates(first, NativeTriangleContactService.TOLERANCE_M):
                if first.rule == second.rule or (second.rule in self.new_rules and first.rule >= second.rule):
                    continue
                candidates += 1
                if not NativeTriangleContactService.intersects(first, second):
                    continue
                pair = tuple(sorted((first.rule, second.rule)))
                report = reports.setdefault(pair, {"firstRule": pair[0], "secondRule": pair[1],
                    "intendedSeamContactPairs": 0, "unexpectedContactPairs": 0, "examples": []})
                points = NativeTriangleContactService.intersection_points(first, second)
                intended = self.intended_seam_contact(first, second, points)
                report["intendedSeamContactPairs" if intended else "unexpectedContactPairs"] += 1
                if not intended and len(report["examples"]) < 8:
                    report["examples"].append({"first": first.identity(), "second": second.identity(),
                                               "intersectionPointsWorldM": points,
                                               "firstPointsWorldM": first.points, "secondPointsWorldM": second.points})
        unexpected = sum(report["unexpectedContactPairs"] for report in reports.values())
        intended = sum(report["intendedSeamContactPairs"] for report in reports.values())
        return {"passed": unexpected == 0, "checks": {"noUnexpectedNewSkinComponentContacts": unexpected == 0},
                "failures": [] if unexpected == 0 else [f"New facial/collar skin has {unexpected} native triangle contacts away from its intended shared seams"],
                "metrics": {"unexpectedContactPairCount": unexpected, "intendedSeamContactPairCount": intended,
                    "candidatePairCount": candidates, "componentPairs": list(reports.values()),
                    "contactToleranceM": NativeTriangleContactService.TOLERANCE_M,
                    "seamClassificationToleranceM": self.SEAM_CLASSIFICATION_TOLERANCE_M,
                    "scope": "Actual face/collar pairs and each new component versus retained head skin. "
                             "Only contact whose full intersection witnesses lie on its measured intended shared seam is excluded. "
                             "Intra-component self-intersection and unrelated accessory pairs are outside this check."}}


def audit_new_skin_contacts(head_sample: dict, manifest: dict) -> dict:
    try:
        return LyraNewSkinContactAudit(head_sample, manifest).run()
    except (KeyError, IndexError, TypeError, ValueError, ArithmeticError) as error:
        return {"passed": False, "checks": {"completeNewSkinContactAudit": False},
                "metrics": {}, "failures": [str(error)]}


def audit_inherited_accessory_contacts(source_head_sample: dict, head_sample: dict, manifest: dict) -> dict:
    """Inventory retained original accessory contacts after exact buffer identity.

    Existing contacts between unchanged original accessory components are
    explicit exceptions, not evidence that the new face/collars are clear.
    The latter must pass their separate contact checks.
    """
    try:
        assembly = manifest["generated"]["assembly"]
        original_lines = manifest["headClassification"]["accessorySourceLines"]
        changed_lines = set(assembly["fittedHair"]["modifiedSourceLines"])
        source_instances = source_head_sample.get("scene", source_head_sample)["instances"]
        actual_instances = head_sample.get("scene", head_sample)["instances"]
        selected, identities = [], []
        for ordinal, source_line in enumerate(original_lines):
            if source_line in changed_lines:
                continue
            original = [item for item in source_instances if item["source"]["rule"] == "LyraHead"
                        and item["source"]["startLine"] == source_line]
            actual = [item for item in actual_instances if item["source"]["rule"] == "LyraHeadAccessories"
                      and item["source"]["primitiveOrdinal"] == ordinal]
            if len(original) != 1 or len(actual) != 1:
                raise ValueError(f"Original accessory source line {source_line} has no unique native correspondence")
            original, actual = original[0], actual[0]
            for field in ("positions", "normals"):
                if original["geometry"][field] != actual["geometry"][field]:
                    raise ValueError(f"Accessory source line {source_line} has changed actual {field}; its old contacts cannot be inherited")
            selected.append(original)
            identities.append({"sourceLine": source_line, "original": original["source"],
                               "integrated": actual["source"], "actualGeometryIdentical": True})
        records = [triangle for instance in selected for triangle in NativeTriangleContactService.records(instance)]
        index = NativeTriangleContactIndex(records)
        source_lines = {item["source"]["primitiveOrdinal"]: item["source"]["startLine"] for item in selected}
        reports, candidates = {}, 0
        for first in records:
            for second in index.candidates(first, NativeTriangleContactService.TOLERANCE_M):
                if first.primitive_ordinal >= second.primitive_ordinal:
                    continue
                candidates += 1
                if NativeTriangleContactService.intersects(first, second):
                    pair = (source_lines[first.primitive_ordinal], source_lines[second.primitive_ordinal])
                    report = reports.setdefault(pair, {"originalSourceLines": list(pair), "contactPairCount": 0,
                                                        "classification": "unchanged original accessory pair", "examples": []})
                    report["contactPairCount"] += 1
                    if len(report["examples"]) < 2:
                        report["examples"].append({"first": first.identity(), "second": second.identity()})
        return {"passed": True, "checks": {"retainedAccessoryActualBuffersIdentical": True}, "failures": [],
                "metrics": {"unchangedAccessoryCount": len(selected), "candidatePairCount": candidates,
                            "inheritedContactPairCount": sum(report["contactPairCount"] for report in reports.values()),
                            "inheritedComponentPairs": list(reports.values()), "exactNativeCorrespondences": identities,
                            "contactToleranceM": NativeTriangleContactService.TOLERANCE_M,
                            "scope": "Contacts between unchanged original accessory components, retained only after exact "
                                     "actual position/normal equality. These listed historical contacts are intentional exceptions; "
                                     "they do not exempt any new face/collar or fitted-hair contact."}}
    except (KeyError, IndexError, TypeError, ValueError, ArithmeticError) as error:
        return {"passed": False, "checks": {"completeInheritedAccessoryContactAudit": False},
                "metrics": {}, "failures": [str(error)]}


def audit_new_skin_accessory_contacts(source_head_sample: dict, head_sample: dict, manifest: dict) -> dict:
    """Allow only the explicitly bounded, revised earpiece mounting interface.

    The old accessory buffer must be exact. Current native intersection
    witnesses must remain on the declared original mounting triangles and
    within the authored allowance of their original skin-intersection curves.
    All other accessory/new-skin contacts fail, including fitted strands.
    """
    try:
        original = source_head_sample.get("scene", source_head_sample)["instances"]
        actual = head_sample.get("scene", head_sample)["instances"]
        assembly = manifest["generated"]["assembly"]
        skin_rules = {assembly["faceRule"]} | {collar["rule"] for collar in assembly["collars"]}
        skin = [triangle for instance in actual if instance["source"]["rule"] in skin_rules
                for triangle in NativeTriangleContactService.records(instance)]
        skin_index = NativeTriangleContactIndex(skin)
        source_skin_lines = set(manifest["headClassification"]["reconstructedSourceLines"])
        original_skin = [triangle for instance in original if instance["source"]["rule"] == "LyraHead"
                         and instance["source"]["startLine"] in source_skin_lines
                         for triangle in NativeTriangleContactService.records(instance)]
        original_index = NativeTriangleContactIndex(original_skin)
        interfaces = []
        for declared in manifest.get("mountingInterfaces", []):
            selected_original = [item for item in original if item["source"]["rule"] == "LyraHead"
                                 and item["source"]["startLine"] == declared["originalSourceLine"]]
            selected_actual = [item for item in actual if item["source"]["rule"] == declared["accessoryRule"]
                               and item["source"]["primitiveOrdinal"] == declared["primitiveOrdinal"]]
            if len(selected_original) != 1 or len(selected_actual) != 1:
                raise ValueError("A declared mounting interface has no unique original/current native accessory")
            for field in ("positions", "normals"):
                if selected_original[0]["geometry"][field] != selected_actual[0]["geometry"][field]:
                    raise ValueError("The declared mounted accessory changed its actual native "+field)
            allowance = declared["maximumOriginalContactCurveShiftM"]
            if not math.isfinite(allowance) or allowance < 0:
                raise ValueError("A declared mounting allowance must be finite and nonnegative")
            original_triangles = NativeTriangleContactService.records(selected_original[0])
            curves = {}
            for triangle_id in declared["allowedAccessoryTriangles"]:
                triangle = original_triangles[triangle_id]
                segments = []
                for other in original_index.candidates(triangle, NativeTriangleContactService.TOLERANCE_M):
                    if NativeTriangleContactService.intersects(triangle, other):
                        points = NativeTriangleContactService.intersection_points(triangle, other)
                        segments.append(max(((a, b) for a in points for b in points), key=lambda pair: math.dist(*pair)))
                if not segments:
                    raise ValueError("An allowed mounting triangle has no original native skin contact")
                curves[triangle_id] = segments
            interfaces.append({"declaration": declared, "originalCurves": curves,
                               "currentContactPairs": [], "maximumMeasuredCurveShiftM": 0.0,
                               "originalAccessoryBufferEquality": True})
        unexpected, candidates = [], 0
        for instance in actual:
            if instance["source"]["rule"] != "LyraHeadAccessories":
                continue
            for accessory in NativeTriangleContactService.records(instance):
                for other in skin_index.candidates(accessory, NativeTriangleContactService.TOLERANCE_M):
                    candidates += 1
                    if not NativeTriangleContactService.intersects(accessory, other):
                        continue
                    points = NativeTriangleContactService.intersection_points(accessory, other)
                    matching = [interface for interface in interfaces
                        if interface["declaration"]["accessoryRule"] == accessory.rule
                        and interface["declaration"]["primitiveOrdinal"] == accessory.primitive_ordinal
                        and interface["declaration"]["skinRule"] == other.rule
                        and accessory.triangle_index in interface["originalCurves"]]
                    allowed, shift = False, None
                    if len(matching) == 1:
                        interface = matching[0]
                        curves = interface["originalCurves"][accessory.triangle_index]
                        shift = max(min(LyraNewSkinContactAudit.point_segment_distance(point, *segment)
                                        for segment in curves) for point in points)
                        allowed = shift <= interface["declaration"]["maximumOriginalContactCurveShiftM"]
                    contact = {"accessory": accessory.identity(), "skin": other.identity(),
                               "currentIntersectionWitnessesWorldM": points,
                               "maximumDistanceFromOriginalContactCurveM": shift}
                    if allowed:
                        interface["currentContactPairs"].append(contact)
                        interface["maximumMeasuredCurveShiftM"] = max(interface["maximumMeasuredCurveShiftM"], shift)
                    else:
                        unexpected.append(contact)
        missing = [interface["declaration"] for interface in interfaces if not interface["currentContactPairs"]]
        return {"passed": not unexpected and not missing,
                "checks": {"noUndeclaredAccessoryNewSkinContact": not unexpected,
                           "declaredRevisedMountsPresent": not missing},
                "failures": ([f"{len(unexpected)} accessory/new-skin triangle contacts lie outside the declared revised mounting interface"] if unexpected else [])
                            + (["A declared revised mounting interface has no actual native contact"] if missing else []),
                "metrics": {"candidatePairCount": candidates, "unexpectedContactPairs": unexpected,
                            "revisedMountingInterfaces": interfaces,
                            "contactToleranceM": NativeTriangleContactService.TOLERANCE_M,
                            "scope": "All native LyraHeadAccessories triangles versus the new face and collars. "
                                     "The declared revised earpiece mount is an assembly allowance, not unchanged contact geometry, "
                                     "a general contact exemption, or a mechanical constraint."}}
    except (KeyError, IndexError, TypeError, ValueError, ArithmeticError) as error:
        return {"passed": False, "checks": {"completeNewSkinAccessoryContactAudit": False},
                "metrics": {}, "failures": [str(error)]}


def audit_head_geometry(head_sample: dict, manifest: dict, prepared_apertures=None) -> dict:
    """Audit a neutral/active native head sample; never alter either input."""
    audit = None
    try:
        audit = LyraHeadGeometryAudit(head_sample, manifest, prepared_apertures)
        return audit.run()
    except (KeyError, IndexError, TypeError, ValueError, ArithmeticError) as error:
        result = {"passed": False, "checks": dict(audit.checks) if audit else {},
                  "metrics": dict(audit.metrics) if audit else {},
                  "failures": list(audit.failures) if audit else []}
        result["checks"]["completeNativeAudit"] = False
        result["failures"].append(str(error))
        return result
