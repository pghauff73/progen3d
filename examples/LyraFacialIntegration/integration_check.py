#!/usr/bin/env python3
"""Independently validate the native Lyra facial integration.

The sampler evaluates every requested native scene. This program does not pose,
deform, interpolate, or repair geometry. Its only generated grammars replace the
entry rule to isolate the delivered head, or change the declared refinement or
iteration budget in a diagnostic fixture. Python's standard library suffices.
"""
from __future__ import annotations

import argparse
import ast
from collections import Counter, defaultdict
import hashlib
import json
import math
import os
from pathlib import Path
import re
import statistics
import struct
import subprocess
import time


HERE = Path(__file__).resolve().parent
BOUNDARIES = (0., 1.25, 2.25, 2.9, 3.55, 4.45, 7.6, 8.45, 9.2, 9.8,
              10.05, 10.4, 10.7, 11.65, 12.2, 13.6, 14.1, 15.)
FULL_TIMES = tuple(sorted((*BOUNDARIES, *((a + b) / 2 for a, b in zip(BOUNDARIES, BOUNDARIES[1:])))))
SMOKE_TIMES = (0., 7.6, 15.)
TRANSFORM_TOLERANCE_M = 2e-6
REFINEMENT_CORNER_TARGET_M = 1e-4


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def json_digest(value: object) -> str:
    return hashlib.sha256(json.dumps(value, sort_keys=True, separators=(',', ':'),
                                     allow_nan=False).encode()).hexdigest()


def write_json(path: Path, value: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, allow_nan=False) + '\n')


def points(flat: list, dimensions: int = 3) -> list[tuple]:
    return [tuple(flat[i:i + dimensions]) for i in range(0, len(flat), dimensions)]


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def norm(a):
    return math.sqrt(dot(a, a))


def distance(a, b):
    return norm(sub(a, b))


def float32(value):
    return struct.unpack('f', struct.pack('f', value))[0]


def matrix_product(a, b):
    """Column-major matrices, matching the native exported convention."""
    return [sum(a[k * 4 + row] * b[column * 4 + k] for k in range(4))
            for column in range(4) for row in range(4)]


def identity_matrix():
    return [1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1., 0., 0., 0., 0., 1.]


def translation_matrix(x, y, z):
    result = identity_matrix()
    result[12:15] = [x, y, z]
    return result


def rotation_matrix(degrees, axis):
    angle = math.radians(degrees)
    c, s = math.cos(angle), math.sin(angle)
    if axis == 'x':
        return [1., 0., 0., 0., 0., c, s, 0., 0., -s, c, 0., 0., 0., 0., 1.]
    return [c, 0., -s, 0., 0., 1., 0., 0., s, 0., c, 0., 0., 0., 0., 1.]


def transform(matrix, point):
    return tuple(sum(matrix[column * 4 + row] * point[column] for column in range(3)) + matrix[12 + row]
                 for row in range(3))


def transform_distance(a, b):
    # Metre-scale probes include translation and orientation, with readable units.
    return max(distance(transform(a, point), transform(b, point))
               for point in ((0., 0., 0.), (.1, 0., 0.), (0., .1, 0.), (0., 0., .1)))


def inverse_rigid(matrix):
    result = identity_matrix()
    for row in range(3):
        for column in range(3):
            result[column * 4 + row] = matrix[row * 4 + column]
    result[12:15] = [-sum(result[column * 4 + row] * matrix[12 + column] for column in range(3))
                     for row in range(3)]
    return result


def face_instance(sample):
    faces = [item for item in sample['scene']['instances'] if 'facial' in item]
    if len(faces) != 1:
        raise ValueError(f"{sample['id']}: expected one native FacialSkin, found {len(faces)}")
    return faces[0]


def rule_identity(instance):
    source = instance['source']
    return (source['rule'], source['primitiveOrdinal'], source['declarationSha256'])


class ValidationLedger:
    """Named acceptance gates and evidence, without silently dropping failures."""

    def __init__(self):
        self.checks = []

    def check(self, name, passed, **evidence):
        self.checks.append({'name': name, 'passed': bool(passed), **evidence})
        if not passed:
            print('FAIL:', name, flush=True)

    @property
    def passed(self):
        return all(check['passed'] for check in self.checks)


class SourceClockOracle:
    """Evaluate only the source's arithmetic choreography, independently of C++."""

    def __init__(self, source_text, manifest):
        self.manifest = manifest
        self.expressions = re.search(r'^Progress\([^\n]+\)\s*->\s*Pose\(([^\n]+)\)',
                                     source_text, re.MULTILINE).group(1).split()
        self.parameters = re.search(r'^Pose\(([^)]+)\)', source_text, re.MULTILINE).group(1).split()
        if len(self.expressions) != len(self.parameters):
            raise ValueError('Source Pose arguments no longer match the approved choreography oracle.')
        pose = re.search(r'^Pose\([^\n]+\)\s*->([\s\S]*?)(?=^\w[^\n]*->)', source_text, re.MULTILINE).group(1)
        required = (r'T\(0\s+1\.24038\s+0\)', r'A\(by\s+1\)',
                    r'T\(0\s+0?\.23962\s+0\)', r'A\(hy\s+1\)', r'A\(hp\s+0\)')
        if any(not re.search(pattern, pose) for pattern in required):
            raise ValueError('Source head transform changed; the independent transform oracle needs review.')

    @staticmethod
    def smoother(value):
        value = min(1., max(0., value))
        return value * value * value * (value * (value * 6. - 15.) + 10.)

    @classmethod
    def evaluate(cls, expression, variables):
        def visit(node):
            if isinstance(node, ast.Constant) and isinstance(node.value, (int, float)):
                return float(node.value)
            if isinstance(node, ast.Name) and node.id in variables:
                return variables[node.id]
            if isinstance(node, ast.UnaryOp) and isinstance(node.op, (ast.UAdd, ast.USub)):
                return visit(node.operand) * (-1 if isinstance(node.op, ast.USub) else 1)
            if isinstance(node, ast.BinOp):
                first, second = visit(node.left), visit(node.right)
                if isinstance(node.op, ast.Add): return first + second
                if isinstance(node.op, ast.Sub): return first - second
                if isinstance(node.op, ast.Mult): return first * second
                if isinstance(node.op, ast.Div): return first / second
            if (isinstance(node, ast.Call) and isinstance(node.func, ast.Name)
                    and node.func.id == 'smoother' and len(node.args) == 1 and not node.keywords):
                return cls.smoother(visit(node.args[0]))
            raise ValueError('Unsupported choreography expression: ' + expression)
        return visit(ast.parse(expression, mode='eval').body)

    def head_transform(self, seconds):
        seconds = min(15., max(0., float32(seconds)))
        variables = {'u' + str(index): float32(self.smoother((seconds - a) / (b - a)))
                     for index, (a, b) in enumerate(zip(BOUNDARIES, BOUNDARIES[1:]))}
        arguments = dict(zip(self.parameters, self.expressions))
        by, hy, hp = [self.evaluate(arguments[name], variables) for name in ('by', 'hy', 'hp')]
        result = translation_matrix(0., 1.24038, 0.)
        for matrix in (rotation_matrix(by, 'y'), translation_matrix(0., .23962, 0.),
                       rotation_matrix(hy, 'y'), rotation_matrix(hp, 'x')):
            result = matrix_product(result, matrix)
        return result

    def activations(self, seconds):
        variables = {'tm': min(15., max(0., float32(seconds)))}
        clock = self.manifest['clock']
        return tuple(self.evaluate(clock[key], variables) for key in ('leftActivation', 'rightActivation'))


def point_triangle_distance(point, a, b, c):
    """Closest point on the entire facet, including edge and vertex regions."""
    ab, ac, ap = sub(b, a), sub(c, a), sub(point, a)
    d1, d2 = dot(ab, ap), dot(ac, ap)
    if d1 <= 0 and d2 <= 0: return norm(ap)
    bp = sub(point, b)
    d3, d4 = dot(ab, bp), dot(ac, bp)
    if d3 >= 0 and d4 <= d3: return norm(bp)
    vc = d1 * d4 - d3 * d2
    if vc <= 0 and d1 >= 0 and d3 <= 0:
        v = d1 / (d1 - d3)
        return distance(point, tuple(a[i] + v * ab[i] for i in range(3)))
    cp = sub(point, c)
    d5, d6 = dot(ab, cp), dot(ac, cp)
    if d6 >= 0 and d5 <= d6: return norm(cp)
    vb = d5 * d2 - d1 * d6
    if vb <= 0 and d2 >= 0 and d6 <= 0:
        w = d2 / (d2 - d6)
        return distance(point, tuple(a[i] + w * ac[i] for i in range(3)))
    va = d3 * d6 - d5 * d4
    if va <= 0 and d4 >= d3 and d5 >= d6:
        w = (d4 - d3) / ((d4 - d3) + (d5 - d6))
        return distance(point, tuple(b[i] + w * (c[i] - b[i]) for i in range(3)))
    denominator = va + vb + vc
    if denominator <= 0:
        raise ValueError('Collapsed facet in exact globe clearance audit.')
    v, w = vb / denominator, vc / denominator
    return distance(point, tuple(a[i] + v * ab[i] + w * ac[i] for i in range(3)))


class NativeFaceAudit:
    """Independent indexed geometry checks on the actual delivered native face."""

    def __init__(self, manifest, ledger):
        self.manifest, self.ledger = manifest, ledger
        self.topologies = {}

    def inspect(self, sample, expected_activation, expected_transform=None, refined=False,
                young_modulus_override=None, support_stiffness_override=None):
        item = face_instance(sample)
        facial = item['facial']
        mesh, mechanics = facial['nativeMesh'], facial['mechanics']
        current, rest, solved, normals = [points(mesh[name]) for name in
                                          ('positions', 'restPositions', 'doublePositions', 'normals')]
        triangles = points(mesh['triangles'])
        minimum_area = float('inf')
        minimum_hemisphere = float('inf')
        minimum_normal_dot = float('inf')
        minimum_globe = float('inf')
        eye_centers = [facial['localLandmarks'][side + '_eyeball_center'] for side in ('left', 'right')]
        radius = facial['eyeballRadiusLocalM']
        for ids in triangles:
            a, b, c = (current[index] for index in ids)
            current_cross = cross(sub(b, a), sub(c, a))
            area_twice = norm(current_cross)
            rest_cross = cross(sub(rest[ids[1]], rest[ids[0]]), sub(rest[ids[2]], rest[ids[0]]))
            rest_length = norm(rest_cross)
            if area_twice == 0 or rest_length == 0:
                minimum_area = minimum_hemisphere = 0.
                minimum_normal_dot = -1.
                continue
            minimum_area = min(minimum_area, area_twice / 2)
            minimum_hemisphere = min(minimum_hemisphere, dot(current_cross, rest_cross) / rest_length)
            mean_normal = tuple(sum(normals[index][axis] for index in ids) for axis in range(3))
            minimum_normal_dot = min(minimum_normal_dot, dot(current_cross, mean_normal) / (area_twice * max(norm(mean_normal), 1e-300)))
            for center in eye_centers:
                minimum_globe = min(minimum_globe, point_triangle_distance(center, a, b, c) - radius)
        fixed_error = max((distance(current[i], rest[i]) for i in mesh['fixedVertexIds']), default=float('inf'))
        solved_fixed_error = max((distance(solved[i], rest[i]) for i in mesh['fixedVertexIds']), default=float('inf'))
        topology_key = json_digest({'triangles': mesh['triangles'], 'uv': mesh['materialUv']})
        if topology_key not in self.topologies:
            self.topologies[topology_key] = self.topology(mesh)
        topology = self.topologies[topology_key]
        target_vertices, target_triangles = ((13094, 25408) if refined else (3410, 6432))
        prefix = sample['id']
        expected_material = dict(self.manifest['face']['material'])
        if young_modulus_override is not None:
            expected_material['youngModulusPa'] = young_modulus_override
        if support_stiffness_override is not None:
            expected_material['supportNPerM3'] = support_stiffness_override
        material_error = max(abs(mechanics['material'][name] - value) / max(abs(value), 1e-12)
                             for name, value in expected_material.items())
        actual_size = [float(value) for value in re.search(r'\bsize\(([^)]+)\)', facial['canonicalText']).group(1).split()]
        size_error = max(abs(actual - expected) for actual, expected in zip(actual_size, self.manifest['face']['sizeM']))
        cable_parameters_match = all(
            muscle['name'] == declared['name']
            and abs(muscle['maximumShortening'] - declared['maximumShortening']) <= 1e-7
            and abs(muscle['stiffnessNPerM'] - declared['stiffnessNPerM']) <= 1e-6
            for muscle, declared in zip(mechanics['muscles'], self.manifest['face']['muscles']))
        self.ledger.check(prefix + '.authoredPhysicalModel', material_error <= 1e-6 and size_error <= 1e-8
                          and cable_parameters_match and mechanics['mouthMeshMode'] == self.manifest['face']['mouthMesh']
                          and mechanics['bendingScale'] == self.manifest['face']['bendingScale'],
                          maximumMaterialRelativeError=material_error, maximumPhysicalSizeErrorM=size_error,
                          material=mechanics['material'], bendingScale=mechanics['bendingScale'])
        area_floor = 1e-10 * self.manifest['face']['sizeM'][0] ** 2
        required_globe_clearance = 10e-6 * self.manifest['face']['sizeM'][0] / .16
        self.ledger.check(prefix + '.nativeTopology', topology['manifold'] and topology['components'] == 1
                          and topology['boundaryLoops'] == 4 and len(current) == target_vertices
                          and len(triangles) == target_triangles, **topology)
        self.ledger.check(prefix + '.facetGeometry', minimum_area > 0 and minimum_hemisphere > area_floor
                          and minimum_normal_dot > 0 and minimum_globe >= required_globe_clearance,
                          minimumTriangleAreaM2=minimum_area, minimumRestNormalProjectionM2=minimum_hemisphere,
                          restNormalProjectionFloorM2=area_floor, minimumFaceNormalDot=minimum_normal_dot,
                          minimumExactFacetGlobeClearanceM=minimum_globe,
                          requiredScaledGlobeClearanceM=required_globe_clearance,
                          globeClearanceReference={'minimumM': 10e-6, 'faceWidthM': .16})
        self.ledger.check(prefix + '.fixedSupport', fixed_error == 0 and solved_fixed_error == 0,
                          floatMeshMaximumErrorM=fixed_error, doubleSolveMaximumErrorM=solved_fixed_error,
                          fixedVertices=len(mesh['fixedVertexIds']))
        actual_activation = tuple(muscle['activation'] for muscle in mechanics['muscles'])
        activation_error = max(abs(a - b) for a, b in zip(actual_activation, expected_activation))
        self.ledger.check(prefix + '.activation', len(actual_activation) == 2 and activation_error <= 2e-6
                          and all(-1e-8 <= a <= .5000001 for a in actual_activation),
                          actual=list(actual_activation), expected=list(expected_activation), maximumError=activation_error)
        energy_sum = sum(mechanics[name] for name in ('membraneEnergyJ', 'supportEnergyJ', 'muscleEnergyJ', 'bendingEnergyJ'))
        energy_error = abs(energy_sum - mechanics['finalEnergyJ'])
        tolerance = self.manifest['face']['solve']['relativeFreeForceTolerance']
        self.ledger.check(prefix + '.mechanicalEquilibrium', mechanics['converged']
                          and mechanics['relativeFreeForce'] <= tolerance
                          and mechanics['maximumGreenStrain'] <= .35
                          and mechanics['finalEnergyJ'] <= mechanics['initialEnergyJ'] + 1e-15
                          and energy_error <= 1e-14 and all(mechanics[k] >= -1e-20 for k in
                          ('membraneEnergyJ', 'supportEnergyJ', 'muscleEnergyJ', 'bendingEnergyJ')),
                          iterations=mechanics['iterations'], relativeForceResidualFloat64=mechanics['relativeFreeForce'],
                          forceResidualFloat64N=mechanics['freeForceL2N'], relativeForceResidualFloat32Mesh=mechanics['meshRelativeFreeForce'],
                          forceResidualFloat32MeshN=mechanics['meshFreeForceL2N'], maximumPositionQuantizationM=mechanics['maximumPositionQuantizationM'],
                          maximumGreenStrain=mechanics['maximumGreenStrain'], energyPartitionErrorJ=energy_error,
                          initialEnergyJ=mechanics['initialEnergyJ'], finalEnergyJ=mechanics['finalEnergyJ'])
        if expected_transform is not None:
            matrix_error = transform_distance(item['primaryTransform'], expected_transform)
            actual_world = points(item['geometry']['positions'])
            maximum_corner_error = max(distance(actual_world[n], transform(expected_transform, current[index]))
                                       for n, index in enumerate(mesh['triangles']))
            maximum_landmark_error = max(distance(facial['worldLandmarks'][name], transform(expected_transform, point))
                                        for name, point in facial['localLandmarks'].items())
            actual_normals = points(item['geometry']['normals'])
            maximum_normal_error = 0.
            for corner, index in enumerate(mesh['triangles']):
                expected = tuple(sum(expected_transform[column * 4 + row] * normals[index][column]
                                     for column in range(3)) for row in range(3))
                expected = tuple(value / norm(expected) for value in expected)
                maximum_normal_error = max(maximum_normal_error, distance(actual_normals[corner], expected))
            self.ledger.check(prefix + '.independentHeadTransform', max(matrix_error, maximum_corner_error,
                              maximum_landmark_error) <= TRANSFORM_TOLERANCE_M and maximum_normal_error <= 1e-5,
                              matrixProbeErrorM=matrix_error, actualNativeCornerErrorM=maximum_corner_error,
                              worldLandmarkErrorM=maximum_landmark_error, toleranceM=TRANSFORM_TOLERANCE_M,
                              maximumWorldNormalVectorError=maximum_normal_error, normalVectorTolerance=1e-5)
        # The stored solved landmarks must coincide with an actual delivered mesh point.
        margin_names = [name for name in facial['localLandmarks'] if 'eyeball_center' not in name]
        landmark_mesh_error = max(min(distance(facial['localLandmarks'][name], p) for p in current) for name in margin_names)
        self.ledger.check(prefix + '.solvedLandmarks', landmark_mesh_error <= 1e-8, maximumMeshVertexDistanceM=landmark_mesh_error)
        return {'id': prefix, 'activation': list(actual_activation), 'vertices': len(current), 'triangles': len(triangles),
                'nativeGeometrySha256': item['nativeGeometrySha256'], 'topologySha256': topology_key,
                'iterations': mechanics['iterations'], 'relativeForceResidualFloat64': mechanics['relativeFreeForce'],
                'relativeForceResidualFloat32Mesh': mechanics['meshRelativeFreeForce'],
                'forceResidualFloat32MeshN': mechanics['meshFreeForceL2N'],
                'minimumExactFacetGlobeClearanceM': minimum_globe,
                'requiredScaledGlobeClearanceM': required_globe_clearance,
                'maximumDisplacementM': max(distance(a, b) for a, b in zip(current, rest)),
                'maximumGreenStrain': mechanics['maximumGreenStrain'],
                'cableShorteningM': {muscle['name']: muscle['restLengthM'] - muscle['lengthM']
                                    for muscle in mechanics['muscles']},
                'landmarksFaceLocalM': facial['localLandmarks']}

    @staticmethod
    def topology(mesh):
        vertices = len(mesh['positions']) // 3
        triangles = points(mesh['triangles'])
        edge_count = Counter()
        adjacency = defaultdict(set)
        for a, b, c in triangles:
            for x, y in ((a, b), (b, c), (c, a)):
                edge_count[tuple(sorted((x, y)))] += 1
                adjacency[x].add(y)
                adjacency[y].add(x)
        def component_count(graph):
            remaining, count = set(graph), 0
            while remaining:
                stack = [remaining.pop()]
                count += 1
                while stack:
                    for neighbor in graph[stack.pop()]:
                        if neighbor in remaining:
                            remaining.remove(neighbor)
                            stack.append(neighbor)
            return count
        boundary = defaultdict(set)
        for (a, b), count in edge_count.items():
            if count == 1:
                boundary[a].add(b)
                boundary[b].add(a)
        return {'vertices': vertices, 'triangles': len(triangles), 'components': component_count(adjacency),
                'boundaryLoops': component_count(boundary), 'boundaryVertices': len(boundary),
                'manifold': len(adjacency) == vertices and all(count in (1, 2) for count in edge_count.values())
                and all(len(neighbors) == 2 for neighbors in boundary.values())}


class NativeBatchEvidence:
    """Hash-keyed native evaluations and explicit receipts for reproducibility."""

    def __init__(self, sampler, output):
        self.sampler = sampler.resolve()
        self.output = output.resolve()
        self.binary_hash = sha256(self.sampler)
        self.receipts = []
        self.environment = dict(os.environ)
        repository = HERE.parents[1]
        library = repository / 'third_party/debs/glfw/usr/lib/x86_64-linux-gnu'
        self.environment['LD_LIBRARY_PATH'] = str(library) + ':' + self.environment.get('LD_LIBRARY_PATH', '')

    def run(self, label, source, samples, capture_rules=(), require_face=True, allow_failure=False):
        if sha256(self.sampler) != self.binary_hash:
            raise RuntimeError('The sampler executable changed during validation; start a new pinned validation run.')
        request = {'captureRules': list(capture_rules), 'samples': samples}
        if require_face:
            request['requireFacialSkin'] = True
        key = hashlib.sha256((self.binary_hash + sha256(source) + json.dumps(request, sort_keys=True)).encode()).hexdigest()[:20]
        directory = self.output / 'validation_native' / (label + '_' + key)
        directory.mkdir(parents=True, exist_ok=True)
        request_path, report_path, receipt_path = (directory / name for name in ('requests.json', 'report.json', 'receipt.json'))
        write_json(request_path, request)
        if receipt_path.exists() and report_path.exists():
            receipt = json.loads(receipt_path.read_text())
            if (receipt.get('samplerSha256') == self.binary_hash and receipt.get('sourceSha256') == sha256(source)
                    and receipt.get('requestsSha256') == sha256(request_path) and receipt.get('reportSha256') == sha256(report_path)):
                report = json.loads(report_path.read_text())
                if report.get('passed') or allow_failure:
                    self.receipts.append({**receipt, 'receipt': str(receipt_path), 'cacheReused': True})
                    return report
        print('Evaluating native batch:', label, len(samples), 'states', flush=True)
        receipt = {'binary': str(self.sampler), 'samplerSha256': self.binary_hash, 'source': str(source),
                   'sourceSha256': sha256(source), 'requestsSha256': sha256(request_path),
                   'copiedObjectsOptimization': 'Mixed native object optimization; timings are not a uniformly optimized release benchmark.',
                   'startedUnix': time.time()}
        write_json(receipt_path, receipt)
        with (directory / 'native.log').open('w') as log:
            result = subprocess.run([str(self.sampler), '--batch', str(source), str(request_path), str(report_path)],
                                    cwd=HERE.parents[1], env=self.environment, stdout=log, stderr=subprocess.STDOUT)
        if sha256(self.sampler) != self.binary_hash or sha256(source) != receipt['sourceSha256']:
            raise RuntimeError('The sampler or grammar changed during a native batch; its evidence cannot be admitted.')
        receipt.update(returncode=result.returncode, elapsedSeconds=time.time() - receipt['startedUnix'], finishedUnix=time.time())
        if report_path.exists():
            receipt['reportSha256'] = sha256(report_path)
        write_json(receipt_path, receipt)
        self.receipts.append({**receipt, 'receipt': str(receipt_path), 'cacheReused': False})
        if not report_path.exists():
            raise RuntimeError(f'Native batch {label} produced no report; see {directory / "native.log"}')
        report = json.loads(report_path.read_text())
        if (result.returncode or not report.get('passed')) and not allow_failure:
            errors = [sample.get('error', sample.get('diagnostics', '')) for sample in report.get('samples', []) if not sample.get('passed')]
            raise RuntimeError(f'Native batch {label} failed: {errors}; see {directory / "native.log"}')
        return report

    def explicit_source_baseline(self, receipt_path, source, samples):
        """Admit only the explicitly supplied, hashed pre-CNS source baseline.

        This exception is scoped to a source containing no CurveNetworkSurface.
        The candidate body must still equal every original native buffer exactly.
        Both executable hashes remain in the final report.
        """
        receipt = json.loads(receipt_path.read_text())
        report_path = receipt_path.parent / 'report.json'
        requests_path = receipt_path.parent / 'requests.json'
        request = json.loads(requests_path.read_text())
        if ('CurveNetworkSurface' in source.read_text() or receipt.get('sourceSha256') != sha256(source)
                or receipt.get('requestsSha256') != sha256(requests_path)
                or receipt.get('reportSha256') != sha256(report_path)
                or request != {'captureRules': [], 'samples': samples}):
            raise ValueError('Explicit pre-CNS source baseline does not match the exact source/request/report contract.')
        report = json.loads(report_path.read_text())
        if receipt.get('returncode') != 0 or not report.get('passed'):
            raise ValueError('Explicit source baseline was not successful.')
        optimization_path = self.output / 'optimization/validation.json'
        optimization = json.loads(optimization_path.read_text())
        if (not optimization.get('passed') or not optimization.get('nativeBuffersIdentical')
                or optimization.get('samplerSha256') != self.binary_hash
                or optimization.get('denseOracleMaximumPositionDifferenceM') != 0
                or optimization.get('denseOracleMaximumNormalDifference') != 0):
            raise ValueError('Cross-binary source baseline requires the exact verified CNS optimization receipt.')
        self.receipts.append({**receipt, 'receipt': str(receipt_path), 'cacheReused': True,
                              'crossBinaryScope': 'Explicit original source without CNS; every candidate body buffer must match bit-for-bit.',
                              'optimizationReceipt': str(optimization_path), 'optimizationReceiptSha256': sha256(optimization_path)})
        return report

    def reject_input(self, label, source, expected_diagnostic):
        """Exercise either parse-time or native construction-time rejection."""
        if sha256(self.sampler) != self.binary_hash:
            raise RuntimeError('The sampler executable changed during validation.')
        key = hashlib.sha256((self.binary_hash + sha256(source)).encode()).hexdigest()[:20]
        directory = self.output / 'validation_native' / (label + '_' + key)
        directory.mkdir(parents=True, exist_ok=True)
        request_path, report_path, log_path = (directory / name for name in ('requests.json', 'report.json', 'native.log'))
        write_json(request_path, {'requireFacialSkin': True, 'samples': [{'id': label, 'seconds': 0}]})
        with log_path.open('w') as log:
            result = subprocess.run([str(self.sampler), '--batch', str(source), str(request_path), str(report_path)],
                                    cwd=HERE.parents[1], env=self.environment, stdout=log, stderr=subprocess.STDOUT)
        log_text = log_path.read_text()
        report = json.loads(report_path.read_text()) if report_path.exists() else None
        no_scene = report is None or (not report.get('passed') and all('scene' not in sample for sample in report.get('samples', [])))
        receipt = {'samplerSha256': self.binary_hash, 'sourceSha256': sha256(source),
                   'requestsSha256': sha256(request_path), 'nativeLogSha256': sha256(log_path),
                   'returncode': result.returncode, 'expectedDiagnostic': expected_diagnostic,
                   'diagnosticObserved': expected_diagnostic in log_text, 'noPublishableScene': no_scene}
        if report_path.exists(): receipt['reportSha256'] = sha256(report_path)
        write_json(directory / 'receipt.json', receipt)
        self.receipts.append({**receipt, 'receipt': str(directory / 'receipt.json')})
        return {'passed': result.returncode != 0 and no_scene and expected_diagnostic in log_text,
                'returncode': result.returncode, 'diagnosticObserved': expected_diagnostic,
                'noPublishableScene': no_scene, 'log': str(log_path)}


def replace_start(text, expression):
    result, count = re.subn(r'^Start\s*->[^\n]*', 'Start -> ' + expression, text, flags=re.MULTILINE)
    if count != 1:
        raise ValueError('Expected exactly one Start rule for an isolated native test fixture.')
    return result


def clean_source(text):
    return '\n'.join(line.strip() for line in text.splitlines() if line.strip() and not line.lstrip().startswith('#'))


def source_rules(text):
    matches = list(re.finditer(r'^(\w+)(?:\([^\n]*\))?\s*->', text, re.MULTILINE))
    return {match.group(1): clean_source(text[match.start():matches[index + 1].start() if index + 1 < len(matches) else len(text)])
            for index, match in enumerate(matches)}


def body_signature(sample, original):
    result = Counter()
    for item in sample['scene']['instances']:
        rule = item['source']['rule']
        is_head = (rule == 'LyraHead') if original else (
            rule.startswith(('LyraFace', 'LyraFacial', 'LyraRetainedHead', 'LyraHead')) or rule == 'LyraOralInterior')
        if is_head:
            continue
        result[(rule_identity(item), item['nativeBufferSha256'], item['material'], item['alpha'], item['texscale'])] += 1
    return result


class LyraIntegrationValidation:
    def __init__(self, arguments):
        self.arguments = arguments
        self.directory = arguments.integration_dir.resolve()
        self.manifest_path = self.directory / 'fit_manifest.json'
        self.manifest = json.loads(self.manifest_path.read_text())
        self.source = arguments.source.resolve()
        self.ordinary = self.directory / 'Lyra_Procedural_Facial_Elastic.p3d'
        self.hologram = self.directory / 'Lyra_Facial_Hologram.p3d'
        self.input_hashes = {str(path): sha256(path) for path in
                             (self.manifest_path, self.source, self.ordinary, self.hologram,
                              Path(__file__), HERE / 'geometry_validation.py', HERE / 'hologram_profile.py')}
        self.ledger = ValidationLedger()
        self.native = NativeBatchEvidence(arguments.sampler, self.directory)
        self.faces = NativeFaceAudit(self.manifest, self.ledger)
        self.clock = SourceClockOracle(self.source.read_text(), self.manifest)
        self.fixture_directory = self.directory / 'validation_native' / 'fixtures'
        self.fixture_directory.mkdir(parents=True, exist_ok=True)
        self.fit_matrix = matrix_product(translation_matrix(*self.manifest['placement']['translationM']),
                                         rotation_matrix(self.manifest['placement']['rotationXDegrees'], 'x'))
        self.case_metrics = []
        self.timeline_metrics = []
        self.geometry_metrics = []
        self.timeline_aperture_metrics = []
        self.contact_metrics = []

    def fixture(self, name, text):
        path = self.fixture_directory / (name + '.p3d')
        path.write_text(text)
        return path

    def provenance(self):
        generated = self.manifest['generated']
        self.ledger.check('pinnedSource', sha256(self.source) == self.manifest['source']['sha256'],
                          actualSha256=sha256(self.source), expectedSha256=self.manifest['source']['sha256'])
        self.ledger.check('builderOutputHashes', sha256(self.ordinary) == generated['ordinaryGrammarSha256']
                          and sha256(self.hologram) == generated['hologramGrammarSha256'],
                          ordinarySha256=sha256(self.ordinary), hologramSha256=sha256(self.hologram))
        original_rules, candidate_rules = source_rules(self.source.read_text()), source_rules(self.ordinary.read_text())
        allowed = {'Start', 'Clip', 'Progress', 'Pose', 'LyraHead'}
        changed = [name for name, text in original_rules.items() if name not in allowed and candidate_rules.get(name) != text]
        self.ledger.check('unchangedBodyRules', not changed, comparedRules=len(original_rules) - len(allowed), unexpectedChangedRules=changed)
        self.ledger.check('ordinaryHologramSameNativeProgram', clean_source(self.ordinary.read_text()) == clean_source(self.hologram.read_text()),
                          interpretation='Only renderer metadata/comments differ; native programs are identical.')
        self.ledger.check('nativeProceduralDelivery', not re.search(r'\b(?:LoadMesh|ImportMesh)\b', clean_source(self.ordinary.read_text()))
                          and not generated['runtimeMeshImports'], runtimeMeshImports=generated['runtimeMeshImports'])
        reference_path = self.directory / generated['nativeReference']['file']
        reference = face_instance(json.loads(reference_path.read_text())['samples'][0])['facial']
        mesh = reference['nativeMesh']
        payload = {'positions': points(mesh['positions']), 'materialUv': points(mesh['materialUv'], 2),
                   'triangles': points(mesh['triangles']), 'landmarks': reference['localLandmarks'],
                   'eyeballRadius': reference['eyeballRadiusLocalM'], 'fixedVertexIds': sorted(mesh['fixedVertexIds']),
                   'shapeKey': reference['mechanics']['shapeKey']}
        self.ledger.check('nativeFitReferenceFingerprint', json_digest(payload) == generated['nativeReference']['geometrySha256'],
                          actualGeometrySha256=json_digest(payload), expectedGeometrySha256=generated['nativeReference']['geometrySha256'])
        errors = []
        for anchor in self.manifest['fitAnchors']:
            name = anchor['nativeLandmark']
            if name.startswith('mean('):
                first, second = name[5:-1].split(',')
                local = tuple((a + b) / 2 for a, b in zip(reference['localLandmarks'][first], reference['localLandmarks'][second]))
            else:
                local = reference['localLandmarks'][name]
            errors.append(distance(transform(self.fit_matrix, local), anchor['headLocalM']))
        rms, maximum = math.sqrt(sum(error * error for error in errors) / len(errors)), max(errors)
        self.ledger.check('independentFitAnchors', rms <= self.manifest['fitAcceptance']['rmsLimitM']
                          and maximum <= self.manifest['fitAcceptance']['maximumLimitM'], rmsM=rms, maximumM=maximum,
                          anchorResidualsM=errors, rigidPlacementOnly=self.manifest['placement']['rigidOnly'])

    def head_cases(self):
        text = self.ordinary.read_text()
        neutral_source = self.fixture('head_neutral', replace_start(text, 'LyraHead(0 0)'))
        neutral_report = self.native.run('head_neutral', neutral_source, [{'id': 'head_neutral', 'seconds': 0,
                                          'obj': '../../samples/head_neutral.obj'}], ('*',))
        self.neutral_head = neutral_report['samples'][0]
        from geometry_validation import prepare_aperture_audit
        self.prepared_apertures = prepare_aperture_audit(self.neutral_head, self.manifest)
        static_by_identity = {rule_identity(item): item for item in self.neutral_head['scene']['instances'] if 'facial' not in item}
        self.head_samples = []
        grid = (0., .25, .5) if self.arguments.mode == 'full' else (0., .5)
        for left in grid:
            rights = (0., .25, .5, .25, 0.) if self.arguments.mode == 'full' else ((0.,) if left == 0 else (.5,))
            source = self.fixture(f'head_left_{left:g}', replace_start(text, f'LyraHead({left:g} t)'))
            requests = [{'id': f'activation_{left:g}_{right:g}_{index}', 'seconds': right}
                        for index, right in enumerate(rights)]
            if left == .5:
                requests[0]['obj'] = '../../samples/' + ('head_left_smile.obj' if self.arguments.mode == 'full' else 'head_bilateral_smile.obj')
                if self.arguments.mode == 'full': requests[2]['obj'] = '../../samples/head_bilateral_smile.obj'
            report = self.native.run(f'head_left_{left:g}', source, requests, ('LyraFacialSkin',))
            first_by_activation = {}
            for sample, right in zip(report['samples'], rights):
                self.audit_rigid_components(sample, static_by_identity, identity_matrix())
                metric = self.faces.inspect(sample, (left, right), self.fit_matrix)
                if right in first_by_activation:
                    old = face_instance(first_by_activation[right])
                    current = face_instance(sample)
                    self.ledger.check(sample['id'] + '.repeatSeekExact', old['nativeBufferSha256'] == current['nativeBufferSha256']
                                      and old['facial']['nativeMesh']['doublePositions'] == current['facial']['nativeMesh']['doublePositions'])
                else:
                    first_by_activation[right] = sample
                    self.case_metrics.append(metric)
                    self.head_samples.append(sample)
        self.static_by_identity = static_by_identity
        # The geometry module is intentionally separate from the builder and renderer.
        from geometry_validation import (audit_head_geometry, audit_new_skin_contacts,
                                         audit_fitted_hair_contacts, audit_new_skin_accessory_contacts)
        for sample in self.head_samples:
            combined = {**sample, 'scene': {**self.neutral_head['scene'], 'instances': [
                face_instance(sample) if 'facial' in item else item for item in self.neutral_head['scene']['instances']]}}
            audit = audit_head_geometry(combined, self.manifest, prepared_apertures=self.prepared_apertures)
            self.ledger.check(sample['id'] + '.headSeamsAndApertures', audit['passed'],
                              failures=audit.get('failures', []), metrics=audit.get('metrics', {}))
            self.geometry_metrics.append({'id': sample['id'], **audit})
            print('Auditing actual native contacts:', sample['id'], flush=True)
            skin_contacts = audit_new_skin_contacts(combined, self.manifest)
            hair_contacts = audit_fitted_hair_contacts(combined, self.manifest)
            accessory_contacts = audit_new_skin_accessory_contacts(self.source_head_sample, combined, self.manifest)
            self.ledger.check(sample['id'] + '.newSkinContacts', skin_contacts['passed'],
                              failures=skin_contacts.get('failures', []), metrics=skin_contacts.get('metrics', {}))
            self.ledger.check(sample['id'] + '.fittedHairContacts', hair_contacts['passed'],
                              failures=hair_contacts.get('failures', []), metrics=hair_contacts.get('metrics', {}))
            self.ledger.check(sample['id'] + '.newSkinAccessoryContacts', accessory_contacts['passed'],
                              failures=accessory_contacts.get('failures', []), metrics=accessory_contacts.get('metrics', {}))
            self.contact_metrics.append({'id': sample['id'], 'newSkin': skin_contacts, 'fittedHair': hair_contacts,
                                         'newSkinAccessories': accessory_contacts})

    def unilateral_response(self):
        if self.arguments.mode != 'full':
            return
        neutral = next(case for case in self.case_metrics if case['activation'] == [0., 0.])['landmarksFaceLocalM']
        anchor_by_name = {anchor['name']: anchor['headLocalM'] for anchor in self.manifest['fitAnchors']}
        self.ledger.check('anatomicalSideLabels', neutral['mouth_left_corner'][0] > 0
                          and neutral['mouth_right_corner'][0] < 0
                          and anchor_by_name['left_eye_center'][0] > 0 and anchor_by_name['right_eye_center'][0] < 0,
                          convention='+X is anatomical left in both the pinned source head and native face frame.')
        reports = []
        for side, opposite, activation_index in (('left', 'right', 0), ('right', 'left', 1)):
            for level in (.25, .5):
                activation = [0., 0.]
                activation[activation_index] = level
                case = next(case for case in self.case_metrics if case['activation'] == activation)
                sample = next(sample for sample in self.head_samples if sample['id'] == case['id'])
                item = face_instance(sample)
                muscle = next(muscle for muscle in item['facial']['mechanics']['muscles'] if muscle['name'] == side + '_smile')
                origin = transform(inverse_rigid(item['primaryTransform']), muscle['origin'])
                ipsilateral = sub(case['landmarksFaceLocalM']['mouth_' + side + '_corner'], neutral['mouth_' + side + '_corner'])
                contralateral = sub(case['landmarksFaceLocalM']['mouth_' + opposite + '_corner'], neutral['mouth_' + opposite + '_corner'])
                direction = sub(origin, neutral['mouth_' + side + '_corner'])
                toward_origin = dot(ipsilateral, direction) / norm(direction)
                ipsilateral_length, contralateral_length = norm(ipsilateral), norm(contralateral)
                evidence = {'side': side, 'activation': level, 'ipsilateralDisplacementFaceLocalM': ipsilateral,
                            'contralateralDisplacementFaceLocalM': contralateral,
                            'ipsilateralDisplacementM': ipsilateral_length, 'contralateralDisplacementM': contralateral_length,
                            'ipsilateralToContralateralRatio': ipsilateral_length / max(contralateral_length, 1e-12),
                            'ratioDenominatorFloorM': 1e-12, 'displacementTowardCableOriginM': toward_origin,
                            'cableOriginFaceLocalM': origin}
                self.ledger.check(case['id'] + '.unilateralAnatomicalResponse', ipsilateral_length > contralateral_length
                                  and toward_origin > 0, **evidence)
                reports.append(evidence)
        self.unilateral_metrics = reports

    def source_head_reference(self):
        source = self.fixture('source_head_reference', replace_start(self.source.read_text(), 'LyraHead'))
        report = self.native.run('source_head_reference', source, [{'id': 'source_head_neutral', 'seconds': 0}],
                                 ('LyraHead',), require_face=False)
        self.source_head_sample = report['samples'][0]

    def accessory_provenance(self):
        from geometry_validation import audit_inherited_accessory_contacts
        inventory = audit_inherited_accessory_contacts(self.source_head_sample, self.neutral_head, self.manifest)
        self.ledger.check('unchangedAccessoryContactProvenance', inventory['passed'],
                          failures=inventory.get('failures', []), metrics=inventory.get('metrics', {}))
        self.accessory_contact_inventory = inventory

    def audit_rigid_components(self, sample, reference, head_transform):
        mismatched, missing, missing_corner_evidence, maximum_matrix_error, maximum_actual_error = [], [], [], 0., 0.
        instances = {rule_identity(item): item for item in sample['scene']['instances'] if 'facial' not in item}
        for identity, old in reference.items():
            item = instances.get(identity)
            if item is None:
                missing.append(list(identity))
                continue
            if item.get('nativeGeometrySha256') != old.get('nativeGeometrySha256'):
                mismatched.append(list(identity))
            expected = matrix_product(head_transform, old['primaryTransform'])
            maximum_matrix_error = max(maximum_matrix_error, transform_distance(expected, item['primaryTransform']))
            if 'maximumNativePositionTransformErrorM' not in item:
                missing_corner_evidence.append(list(identity))
            else:
                maximum_actual_error = max(maximum_actual_error, item['maximumNativePositionTransformErrorM'])
        self.ledger.check(sample['id'] + '.rigidRetainedHead', not missing and not mismatched and not missing_corner_evidence
                          and maximum_matrix_error <= TRANSFORM_TOLERANCE_M and maximum_actual_error <= TRANSFORM_TOLERANCE_M,
                          missing=missing, changedLocalGeometry=mismatched, maximumTransformProbeErrorM=maximum_matrix_error,
                          missingActualCornerEvidence=missing_corner_evidence,
                          maximumActualNativeCornerTransformErrorM=maximum_actual_error, comparedInstances=len(reference))

    def timeline(self):
        from geometry_validation import audit_pose_apertures
        times = FULL_TIMES if self.arguments.mode == 'full' else SMOKE_TIMES
        requests = [{'id': f'timeline_{index:03d}', 'seconds': seconds} for index, seconds in enumerate(times)]
        if self.arguments.source_baseline:
            baseline = self.native.explicit_source_baseline(self.arguments.source_baseline.resolve(), self.source, requests)
        else:
            baseline = self.native.run('source_timeline', self.source, requests, require_face=False)
        integrated_requests = [dict(request) for request in requests]
        integrated_requests[0]['obj'] = '../../samples/character_neutral.obj'
        smile_index = times.index(7.6)
        integrated_requests[smile_index]['obj'] = '../../samples/character_bilateral_smile.obj'
        candidate = self.native.run('integrated_timeline', self.ordinary, integrated_requests, ('LyraFacialSkin',))
        self.timeline_report = candidate
        self.timeline_parse_seconds = candidate['parseSeconds']
        self.source_parse_seconds = baseline['parseSeconds']
        for original, sample in zip(baseline['samples'], candidate['samples']):
            seconds = sample['seconds']
            original_body, candidate_body = body_signature(original, True), body_signature(sample, False)
            self.ledger.check(sample['id'] + '.bodyNativeBuffersUnchanged', original_body == candidate_body,
                              comparedBodyObjects=sum(original_body.values()), sourceSeconds=original['seconds'], candidateSeconds=seconds,
                              missingSignatures=sum((original_body - candidate_body).values()),
                              unexpectedSignatures=sum((candidate_body - original_body).values()))
            head = self.clock.head_transform(seconds)
            self.audit_rigid_components(sample, self.static_by_identity, head)
            self.timeline_metrics.append(self.faces.inspect(sample, self.clock.activations(seconds), matrix_product(head, self.fit_matrix)))
            aperture = audit_pose_apertures(self.prepared_apertures, face_instance(sample))
            self.ledger.check(sample['id'] + '.actualNativeApertures', aperture['passed'],
                              failures=aperture.get('failures', []), metrics=aperture.get('metrics', {}))
            self.timeline_aperture_metrics.append({'id': sample['id'], **aperture})
            self.ledger.check(sample['id'] + '.finiteActualWholeCharacter', sample['scene']['allActualNativeBuffersFinite']
                              and original['scene']['allActualNativeBuffersFinite'],
                              originalObjects=original['scene']['objectCount'], integratedObjects=sample['scene']['objectCount'],
                              originalTriangles=original['scene']['triangleCount'], integratedTriangles=sample['scene']['triangleCount'])
        first, last = face_instance(candidate['samples'][0]), face_instance(candidate['samples'][-1])
        self.ledger.check('headLocalNeutralReturn', first['facial']['nativeMesh']['positions'] == last['facial']['nativeMesh']['positions']
                          and first['facial']['nativeMesh']['doublePositions'] == last['facial']['nativeMesh']['doublePositions'],
                          interpretation='Neutral return is compared in the face material frame; the body choreography need not loop.')
        replay_requests = [{'id': 'seek_bilateral', 'seconds': 7.6}, {'id': 'seek_neutral', 'seconds': 0},
                           {'id': 'seek_bilateral_again', 'seconds': 7.6}, {'id': 'clip_before', 'seconds': -1},
                           {'id': 'clip_after', 'seconds': 16}]
        replay = self.native.run('integrated_replay', self.ordinary, replay_requests, ('LyraFacialSkin',))
        reference_times = {sample['seconds']: sample for sample in candidate['samples']}
        for sample in replay['samples']:
            expected = reference_times[min(15., max(0., sample['seconds']))]
            signature = lambda s: [(rule_identity(i), i['nativeBufferSha256']) for i in s['scene']['instances']]
            self.ledger.check(sample['id'] + '.wholeCharacterDeterministic', signature(sample) == signature(expected),
                              referenceSeconds=expected['seconds'], actualSeconds=sample['seconds'])
        hologram_requests = [{'id': 'hologram_' + str(index), 'seconds': seconds,
                              'obj': '../../samples/hologram_' + ('neutral' if seconds == 0 else 'bilateral_smile' if seconds == 7.6 else 'end') + '.obj'}
                             for index, seconds in enumerate(SMOKE_TIMES)]
        hologram = self.native.run('hologram_counterparts', self.hologram, hologram_requests)
        self.hologram_states_count = len(hologram['samples'])
        for sample in hologram['samples']:
            expected = reference_times[sample['seconds']]
            signature = lambda s: [(rule_identity(i), i['nativeBufferSha256'], i['material'], i['alpha']) for i in s['scene']['instances']]
            self.ledger.check(sample['id'] + '.nativeGeometryEqualsOrdinary', signature(sample) == signature(expected), seconds=sample['seconds'])

    def refinement(self):
        if self.arguments.mode != 'full':
            return
        rule = source_rules(self.ordinary.read_text())['LyraFacialSkin']
        text = 'Start -> LyraFacialSkin(t 0)\n' + rule + '\n'
        source = self.fixture('refinement_face', text.replace('segments(48 64) response(elastic)', 'segments(96 128) response(elastic)'))
        report = self.native.run('face_refinement', source, [{'id': 'refined_neutral', 'seconds': 0},
                                                           {'id': 'refined_left_05', 'seconds': .5}], ('LyraFacialSkin',))
        refined = [self.faces.inspect(sample, (sample['seconds'], 0), identity_matrix(), refined=True) for sample in report['samples']]
        coarse_neutral = next(metric for metric in self.case_metrics if metric['activation'] == [0., 0.])
        coarse_pose = next(metric for metric in self.case_metrics if metric['activation'] == [.5, 0.])
        deltas = {}
        for name in ('mouth_left_corner', 'mouth_right_corner', 'mouth_upper_margin', 'mouth_lower_margin'):
            coarse = sub(coarse_pose['landmarksFaceLocalM'][name], coarse_neutral['landmarksFaceLocalM'][name])
            fine = sub(refined[1]['landmarksFaceLocalM'][name], refined[0]['landmarksFaceLocalM'][name])
            deltas[name] = distance(coarse, fine)
        self.ledger.check('refinementLandmarkDisplacement', max(deltas.values()) <= REFINEMENT_CORNER_TARGET_M,
                          displacementDifferencesM=deltas, proposedIntegrationTargetM=REFINEMENT_CORNER_TARGET_M,
                          rationale='0.1 mm is a declared integration target, not a convergence-order claim; compare displacement relative to each neutral discretization.')
        displacement_difference = abs(refined[1]['maximumDisplacementM'] - coarse_pose['maximumDisplacementM'])
        displacement_relative = displacement_difference / max(coarse_pose['maximumDisplacementM'], 1e-12)
        coarse_shortening = coarse_pose['cableShorteningM']['left_smile']
        fine_shortening = refined[1]['cableShorteningM']['left_smile']
        shortening_difference = abs(fine_shortening - coarse_shortening)
        shortening_relative = shortening_difference / max(abs(coarse_shortening), 1e-12)
        self.ledger.check('refinementMaximumDisplacement', displacement_relative <= .1,
                          coarseMaximumM=coarse_pose['maximumDisplacementM'], refinedMaximumM=refined[1]['maximumDisplacementM'],
                          absoluteDifferenceM=displacement_difference, relativeDifference=displacement_relative, relativeTarget=.1)
        self.ledger.check('refinementCableShortening', shortening_relative <= .05 if abs(coarse_shortening) >= 1e-5 else shortening_difference <= 1e-6,
                          coarseShorteningM=coarse_shortening, refinedShorteningM=fine_shortening,
                          absoluteDifferenceM=shortening_difference, relativeDifference=shortening_relative,
                          relativeTarget=.05, nearZeroCutoffM=1e-5, nearZeroAbsoluteTargetM=1e-6)
        self.refinement_metrics = refined

    def material_sensitivity(self):
        if self.arguments.mode != 'full':
            return
        rule = source_rules(self.ordinary.read_text())['LyraFacialSkin']
        text = 'Start -> LyraFacialSkin(.5 0)\n' + rule + '\n'
        coarse = next(case for case in self.case_metrics if case['activation'] == [.5, 0.])
        self.material_sensitivity_metrics = {'baseline': coarse}
        variants = (('young_modulus', 0, 100000, 'youngModulusPa', {'young_modulus_override': 100000}),
                    ('support_stiffness', 3, 2000000, 'supportNPerM3', {'support_stiffness_override': 2000000}))
        for name, argument_index, value, material_key, audit_options in variants:
            def replace_material(match):
                arguments = match.group(1).split()
                if len(arguments) != 4:
                    raise ValueError('The fitted material declaration must contain four physical parameters.')
                arguments[argument_index] = str(value)
                return 'skinMaterial(' + ' '.join(arguments) + ')'
            changed, replacements = re.subn(r'skinMaterial\(([^)]+)\)', replace_material, text)
            if replacements != 1:
                raise ValueError('Expected one fitted material declaration for the explicit sensitivity fixture.')
            label = 'fitted_' + name + '_x2'
            source = self.fixture(label, changed)
            report = self.native.run(label, source, [{'id': label, 'seconds': 0}], ('LyraFacialSkin',))
            metric = self.faces.inspect(report['samples'][0], (.5, 0), identity_matrix(), **audit_options)
            material = face_instance(report['samples'][0])['facial']['mechanics']['material']
            changed_displacement = abs(metric['maximumDisplacementM'] - coarse['maximumDisplacementM'])
            changed_shortening = abs(metric['cableShorteningM']['left_smile'] - coarse['cableShorteningM']['left_smile'])
            self.ledger.check(label + '.sensitivity', material[material_key] == value
                              and changed_displacement > 1e-9 and changed_shortening > 1e-9,
                              changedParameter=material_key, baselineValue=self.manifest['face']['material'][material_key],
                              perturbedValue=material[material_key], maximumDisplacementDifferenceM=changed_displacement,
                              cableShorteningDifferenceM=changed_shortening,
                              interpretation='The same fitted chart and cable model responds to a declared physical-parameter change; this is a sensitivity measurement, not anatomical calibration.')
            self.material_sensitivity_metrics[name + 'TimesTwo'] = metric

    def wrapper_clamps(self):
        if self.arguments.mode != 'full':
            return
        rule = source_rules(self.ordinary.read_text())['LyraFacialSkin']
        expression = '(-1+2.1*t 1.1-2.1*t)'
        for public_rule in ('LyraHead', 'LyraFacialSkin'):
            text = (replace_start(self.ordinary.read_text(), public_rule + expression) if public_rule == 'LyraHead'
                    else 'Start -> LyraFacialSkin' + expression + '\n' + rule + '\n')
            source = self.fixture('clamp_' + public_rule, text)
            report = self.native.run('clamp_' + public_rule, source,
                                     [{'id': public_rule + '_lower_upper', 'seconds': 0},
                                      {'id': public_rule + '_upper_lower', 'seconds': 1}], ('LyraFacialSkin',))
            for sample, activation in zip(report['samples'], ((0., .5), (.5, 0.))):
                expected = next(s for s in self.head_samples if tuple(m['activation'] for m in face_instance(s)['facial']['mechanics']['muscles']) == activation)
                old, actual = face_instance(expected), face_instance(sample)
                local_equal = (old['nativeGeometrySha256'] == actual['nativeGeometrySha256']
                               and old['facial']['nativeMesh']['doublePositions'] == actual['facial']['nativeMesh']['doublePositions'])
                actual_activation = tuple(muscle['activation'] for muscle in actual['facial']['mechanics']['muscles'])
                whole_head_equal = True
                if public_rule == 'LyraHead':
                    signature = lambda s: [(rule_identity(i), i['nativeBufferSha256']) for i in s['scene']['instances']]
                    whole_head_equal = signature(sample) == signature(expected)
                self.ledger.check(sample['id'] + '.boundedWrapperEquality', local_equal and whole_head_equal and actual_activation == activation,
                                  expectedActivation=list(activation), exactNativeLocalGeometry=local_equal,
                                  actualActivation=list(actual_activation),
                                  exactWholeHeadGeometry=whole_head_equal if public_rule == 'LyraHead' else None,
                                  directNativeRange='The reusable native FacialSkin primitive retains activation in [0,1].')

    def negative_cases(self):
        text = self.ordinary.read_text()
        failed = replace_start(text, 'LyraHead(.5*t 0)').replace('solve(2000 0.0001)', 'solve(8 0.0001)')
        source = self.fixture('intentional_solver_failure', failed)
        report = self.native.run('failure_recovery', source, [{'id': 'before_failure', 'seconds': 0},
                                                             {'id': 'rejected_face', 'seconds': 1, 'obj': '../../samples/rejected_face_must_not_exist.obj'},
                                                             {'id': 'after_failure', 'seconds': 0}], ('LyraFacialSkin',), allow_failure=True)
        before, rejected, after = report['samples']
        diagnostic = json.dumps(rejected)
        rejected_obj = self.directory / 'samples/rejected_face_must_not_exist.obj'
        self.ledger.check('nativeFailedFaceBlocksCharacter', not rejected.get('passed') and 'did not converge' in diagnostic
                          and 'scene' not in rejected and not rejected_obj.exists(),
                          diagnostic=rejected.get('diagnostics', rejected.get('error')), staleObjExists=rejected_obj.exists())
        self.ledger.check('nativeFailureRecovery', before['passed'] and after['passed']
                          and [item['nativeBufferSha256'] for item in before['scene']['instances']]
                          == [item['nativeBufferSha256'] for item in after['scene']['instances']])
        canonical = self.manifest['generated']['nativeReference']['canonicalText']
        def invalid_native_activation(match):
            arguments = match.group(1).split()
            arguments[7] = '1.1'
            return 'muscle(' + ' '.join(arguments) + ')'
        invalid_activation, count = re.subn(r'muscle\((left_smile [^)]*)\)', invalid_native_activation, canonical)
        if count != 1:
            raise ValueError('The direct native activation fixture needs exactly one named left muscle.')
        def direct_native(descriptor):
            return 'Start -> I(' + descriptor + ' material(silvercyanpolishedmetal) 1 1)\n'
        invalid_cases = (
            ('invalid_activation', direct_native(invalid_activation), 'activation in [0,1]'),
            ('invalid_analytic_elastic_mix', direct_native(canonical.replace('smile(0 0)', 'smile(.1 0)')),
             'requires smile, jawOpen, blink and browRaise controls to be zero'),
        )
        for name, grammar, diagnostic in invalid_cases:
            evidence = self.native.reject_input(name, self.fixture(name, grammar), diagnostic)
            self.ledger.check(name, evidence.pop('passed'), **evidence)

    def run(self):
        self.provenance()
        self.source_head_reference()
        self.head_cases()
        self.unilateral_response()
        self.accessory_provenance()
        if not self.ledger.passed:
            raise RuntimeError('The isolated integrated-head gates failed; the full character sweep remains withheld until those failures are resolved.')
        self.timeline()
        self.refinement()
        self.material_sensitivity()
        self.wrapper_clamps()
        self.negative_cases()
        changed = [path for path, expected in self.input_hashes.items() if sha256(Path(path)) != expected]
        self.ledger.check('immutableValidationInputs', not changed, changedFiles=changed)

    def report(self):
        all_timings = []
        if hasattr(self, 'timeline_report'):
            all_timings = [sample['timing'] for sample in self.timeline_report['samples']]
        timings = {key: {'minimumSeconds': min(values), 'medianSeconds': statistics.median(values), 'maximumSeconds': max(values)}
                   for key in ('expansionSeconds', 'generationSeconds', 'nativeBufferAuditSeconds')
                   if (values := [timing[key] for timing in all_timings if key in timing])}
        return {'schema': 'progen3d.lyra-facial-integration-validation.v1', 'passed': self.ledger.passed,
                'coverageMode': self.arguments.mode, 'fullTimelineValidated': self.arguments.mode == 'full'
                and len(self.timeline_metrics) == 35 and self.ledger.passed,
                'samplerSha256': self.native.binary_hash, 'sourceSha256': sha256(self.source),
                'manifestSha256': sha256(self.manifest_path), 'ordinaryGrammarSha256': sha256(self.ordinary),
                'hologramGrammarSha256': sha256(self.hologram), 'checks': self.ledger.checks,
                'validationCodeSha256': sha256(Path(__file__)),
                'geometryAuditCodeSha256': sha256(HERE / 'geometry_validation.py'),
                'activationCases': self.case_metrics, 'timelineCases': self.timeline_metrics,
                'headGeometry': self.geometry_metrics, 'refinement': getattr(self, 'refinement_metrics', []),
                'timelineApertures': self.timeline_aperture_metrics,
                'unilateralLandmarkResponse': getattr(self, 'unilateral_metrics', []),
                'nativeContacts': {'schema': 'progen3d.lyra-native-contact-validation.v1', 'cases': self.contact_metrics,
                                   'inheritedAccessoryInventory': getattr(self, 'accessory_contact_inventory', {})},
                'materialSensitivity': getattr(self, 'material_sensitivity_metrics', {}),
                'hologramNativeCounterpartStates': getattr(self, 'hologram_states_count', 0),
                'nativeEvaluationReceipts': self.native.receipts, 'timelineTimingComponents': timings,
                'initialParseIncludesExpansionSeconds': {'source': getattr(self, 'source_parse_seconds', None),
                                                         'integrated': getattr(self, 'timeline_parse_seconds', None)},
                'timingInterpretation': 'Offline native parse/expand/generate/audit timings; existing objects use mixed optimization. These are not editor playback FPS or uniformly release-optimized benchmarks.',
                'mechanicalPrecisionInterpretation': 'Equilibrium and the separately re-evaluated float mesh residual are measured in the native face material frame. World-buffer transform/quantization errors are audited separately; the renderer buffer is not claimed to meet the double-precision force gate.',
                'samplingInterpretation': 'Each state is a new native SceneGenerationContext and an independent equilibrium. Parsed rules are reused by the batch sampler; the actual editor service has separate evidence.',
                'scope': 'Bounded static elastic smiles with fixed eye collars, fixed jaw, and immutable outer face support. Collar components coincide geometrically; they are not a welded whole-head shell.',
                'limitations': self.manifest.get('limitations', []),
                'proposedTolerances': {'headTransformM': TRANSFORM_TOLERANCE_M,
                                       'collarGapM': self.manifest['collar']['maximumGapM'],
                                       'refinementLandmarkDisplacementM': REFINEMENT_CORNER_TARGET_M}}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sampler', required=True, type=Path)
    parser.add_argument('--integration-dir', required=True, type=Path)
    parser.add_argument('--mode', choices=('smoke', 'full'), default='full')
    parser.add_argument('--source', type=Path, default=HERE / 'reference/Lyra_Procedural_NoMesh.p3d')
    parser.add_argument('--source-baseline', type=Path,
                        help='Explicit hashed receipt for the authorized pre-CNS source baseline; only valid for the exact full source/time request.')
    arguments = parser.parse_args()
    validation = LyraIntegrationValidation(arguments)
    try:
        validation.run()
    except Exception as error:
        validation.ledger.check('validationExecution', False, error=str(error), exceptionType=type(error).__name__)
    report = validation.report()
    output = arguments.integration_dir / 'validation.json'
    write_json(output, report)
    print(json.dumps({'passed': report['passed'], 'coverageMode': report['coverageMode'],
                      'activationCases': len(report['activationCases']), 'timelineCases': len(report['timelineCases']),
                      'failedChecks': [check['name'] for check in report['checks'] if not check['passed']],
                      'report': str(output)}, indent=2), flush=True)
    raise SystemExit(0 if report['passed'] else 1)


if __name__ == '__main__':
    main()
