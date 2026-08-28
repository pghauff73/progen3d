#!/usr/bin/env python3
"""MCSMv2.2 kinematic modern-car reference implementation.

MCSMv2.2 carries forward the MCSMv2.0.1 integrity kernel and implements the
semantic-geometry and concept-kinematics slices planned for MCSMv2.1/2.2:

* a persistent UV semantic surface projected to the implicit outer scaffold;
* exclusive panel and glass-aperture ownership in semantic surface space;
* explicit fixed-body and movable closure surface assemblies;
* front MacPherson and rear multi-link concept hardpoint graphs;
* deterministic wheel-pose solvers and actual tyre-mesh pose sweeps;
* revolute/rising closure joints for four doors, bonnet and rear hatch;
* parent-relative helical side-glass motion;
* independent triangle-intersection, surface-distance, cavity and sweep checks;
* V0-V3 concept-assurance reporting.

The generated geometry remains an original concept model.  Kinematics are
parametric engineering hypotheses, not production hardpoint or homologation
data.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import shutil
import subprocess
import sys
import tempfile
import zipfile
from collections import defaultdict, deque
from dataclasses import asdict, dataclass, field, replace
from pathlib import Path
from typing import Any, Iterable, Mapping, Sequence

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.spatial import cKDTree
import trimesh

# Avoid loading the large optional VTK stack for mesh-only workers.  This must
# happen before the compatibility module imports the legacy generator.
if "--no-preview" in sys.argv:
    os.environ.setdefault("MCSM_DISABLE_VTK", "1")

try:
    import mcsmv201_base as base
except ImportError:  # pragma: no cover
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import mcsmv201_base as base

legacy = base.legacy

MCSM_VERSION = "2.2.0"
MODEL_SCHEMA = "MCSMv2.2-uv-semantic-closure-suspension-kinematics"


# =============================================================================
# General geometry helpers
# =============================================================================


def _unit(vector: Sequence[float], fallback: Sequence[float] = (0.0, 0.0, 1.0)) -> np.ndarray:
    value = np.asarray(vector, dtype=float)
    length = float(np.linalg.norm(value))
    if not np.isfinite(length) or length <= 1e-12:
        return np.asarray(fallback, dtype=float)
    return value / length


def axis_rotation_matrix(point: Sequence[float], axis: Sequence[float], angle_rad: float) -> np.ndarray:
    """Return a homogeneous transform rotating around an arbitrary world axis."""
    origin = np.asarray(point, dtype=float)
    direction = _unit(axis)
    rotation = trimesh.transformations.rotation_matrix(float(angle_rad), direction, point=origin)
    return np.asarray(rotation, dtype=float)


def translation_matrix(offset: Sequence[float]) -> np.ndarray:
    return np.asarray(trimesh.transformations.translation_matrix(np.asarray(offset, dtype=float)), dtype=float)


def transformed_mesh(mesh: trimesh.Trimesh, transform: np.ndarray) -> trimesh.Trimesh:
    result = mesh.copy()
    result.apply_transform(np.asarray(transform, dtype=float))
    return result


def transformed_points(points: np.ndarray, transform: np.ndarray) -> np.ndarray:
    points = np.asarray(points, dtype=float)
    homogeneous = np.column_stack([points, np.ones(len(points))])
    return (np.asarray(transform, dtype=float) @ homogeneous.T).T[:, :3]


def set_mesh_color(
    mesh: trimesh.Trimesh,
    name: str,
    rgb: tuple[int, int, int],
    opacity: float = 1.0,
    metallic: float = 0.25,
    roughness: float = 0.35,
) -> trimesh.Trimesh:
    mesh.visual.material = legacy.pbr(name, rgb, metallic, roughness, int(round(opacity * 255)))
    legacy.set_preview(mesh, tuple(channel / 255.0 for channel in rgb), opacity, metallic, roughness)
    return mesh


def scene_add(scene: trimesh.Scene, name: str, mesh: trimesh.Trimesh, transform: np.ndarray | None = None) -> None:
    scene.add_geometry(mesh, geom_name=name, node_name=name, transform=transform)


def export_scene(scene: trimesh.Scene, glb: Path, obj: Path | None = None) -> None:
    glb.write_bytes(scene.export(file_type="glb"))
    if obj is not None:
        obj.write_text(scene.export(file_type="obj"), encoding="utf-8")


def open_surface_integrity(mesh: trimesh.Trimesh) -> dict[str, Any]:
    finite = bool(np.isfinite(mesh.vertices).all() and np.isfinite(mesh.faces).all())
    areas = np.asarray(mesh.area_faces, dtype=float)
    scale = max(float(np.max(mesh.extents)), 1.0)
    degenerate = int(np.count_nonzero(areas <= scale * scale * 1e-14))
    sorted_faces = np.sort(np.asarray(mesh.faces, dtype=np.int64), axis=1)
    duplicate = int(len(sorted_faces) - len(np.unique(sorted_faces, axis=0)))
    edge_counts = np.bincount(mesh.edges_unique_inverse, minlength=len(mesh.edges_unique))
    nonmanifold = int(np.count_nonzero(edge_counts > 2))
    boundary = int(np.count_nonzero(edge_counts == 1))
    components = trimesh.graph.connected_components(
        mesh.face_adjacency,
        min_len=1,
        nodes=np.arange(len(mesh.faces)),
        engine="scipy",
    ) if len(mesh.faces) else []
    return {
        "finite": finite,
        "winding_consistent": bool(mesh.is_winding_consistent),
        "degenerate_face_count": degenerate,
        "duplicate_face_count": duplicate,
        "boundary_edge_count": boundary,
        "nonmanifold_edge_count": nonmanifold,
        "connected_component_count": int(len(components)),
        "pass": bool(finite and mesh.is_winding_consistent and degenerate == 0 and duplicate == 0 and nonmanifold == 0),
    }


def mesh_hash(mesh: trimesh.Trimesh) -> str:
    digest = hashlib.sha256()
    digest.update(np.asarray(mesh.vertices, dtype=np.float64).round(8).tobytes())
    digest.update(np.asarray(mesh.faces, dtype=np.int64).tobytes())
    return digest.hexdigest()


# =============================================================================
# Persistent semantic UV surface and MCSMv2.1 partition carried into v2.2
# =============================================================================


@dataclass
class SemanticUVSurface:
    mesh: trimesh.Trimesh
    uv: np.ndarray
    samples_x: int
    ring_size: int
    projection_report: dict[str, Any]

    @staticmethod
    def build(
        model: base.ModernCarModelV2,
        samples_x: int = 96,
        samples_half: int = 40,
        end_inset_m: float = 0.008,
    ) -> "SemanticUVSurface":
        xs = np.linspace(model.rear + end_inset_m, model.front - end_inset_m, samples_x)
        rings = [model.sections.section_ring(float(x), samples_half=samples_half) for x in xs]
        ring_size = len(rings[0])
        initial = np.vstack(rings)
        uv = np.asarray([
            (i / (samples_x - 1), j / ring_size)
            for i in range(samples_x)
            for j in range(ring_size)
        ], dtype=float)

        faces: list[list[int]] = []
        for i in range(samples_x - 1):
            a0 = i * ring_size
            b0 = (i + 1) * ring_size
            for j in range(ring_size):
                jn = (j + 1) % ring_size
                faces.append([a0 + j, b0 + j, b0 + jn])
                faces.append([a0 + j, b0 + jn, a0 + jn])
        faces_array = np.asarray(faces, dtype=np.int64)

        # Project the complete registered surface in vectorized batches.  The
        # earlier point-at-a-time implementation made the exact same bisection
        # solve but paid Python-call overhead hundreds of thousands of times.
        # Keeping one lo/hi interval per vertex preserves determinism while
        # reducing generation time by roughly an order of magnitude.
        projected = initial.copy()
        x_values = initial[:, 0]
        lower = np.asarray(model.sections.value("underbody_z", x_values), dtype=float)
        upper = np.asarray(model.sections.value("roof_crown_z", x_values), dtype=float)
        centres = np.column_stack([x_values, np.zeros(len(initial)), 0.5 * (lower + upper)])
        directions = initial - centres
        direction_length = np.linalg.norm(directions, axis=1)
        valid = direction_length > 1e-12
        lo = np.zeros(len(initial), dtype=float)
        hi = np.ones(len(initial), dtype=float)

        f_lo = np.asarray(model.evaluate_field_points(centres, include_wheelhouses=False), dtype=float)
        f_hi = np.asarray(model.evaluate_field_points(centres + directions, include_wheelhouses=False), dtype=float)
        for _ in range(5):
            grow = valid & (f_hi < 0.0) & (hi < 4.0)
            if not np.any(grow):
                break
            hi[grow] *= 1.5
            f_hi[grow] = np.asarray(model.evaluate_field_points(
                centres[grow] + hi[grow, None] * directions[grow],
                include_wheelhouses=False,
            ), dtype=float)
        valid &= (f_lo <= 0.0) & (f_hi >= 0.0)

        for _ in range(44):
            mid = 0.5 * (lo + hi)
            active_points = centres[valid] + mid[valid, None] * directions[valid]
            values = np.asarray(model.evaluate_field_points(active_points, include_wheelhouses=False), dtype=float)
            active_ids = np.flatnonzero(valid)
            inside_ids = active_ids[values <= 0.0]
            outside_ids = active_ids[values > 0.0]
            lo[inside_ids] = mid[inside_ids]
            hi[outside_ids] = mid[outside_ids]
        projected[valid] = centres[valid] + (0.5 * (lo[valid] + hi[valid]))[:, None] * directions[valid]
        failure_ids = np.flatnonzero(~valid).astype(int).tolist()

        mesh = trimesh.Trimesh(vertices=projected, faces=faces_array, process=False)
        mesh.fix_normals(multibody=True)
        residual = np.abs(model.evaluate_field_points(projected, include_wheelhouses=False))
        report = {
            "schema": "MCSMv2.2.SemanticProjection.v1",
            "method": "bracketed radial solve to implicit outer scaffold",
            "vertex_count": int(len(projected)),
            "failure_count": int(len(failure_ids)),
            "failure_ids": failure_ids[:32],
            "mean_abs_field_residual_m": float(np.mean(residual)),
            "p95_abs_field_residual_m": float(np.quantile(residual, 0.95)),
            "maximum_abs_field_residual_m": float(np.max(residual)),
            "end_inset_m": float(end_inset_m),
            "pass": bool(len(failure_ids) == 0 and np.max(residual) <= 1e-6),
        }
        set_mesh_color(mesh, "MCSMv2_2_SemanticUVSurface", (36, 94, 215), 0.34, 0.20, 0.34)
        return SemanticUVSurface(mesh=mesh, uv=uv, samples_x=samples_x, ring_size=ring_size, projection_report=report)

    def face_uv(self) -> tuple[np.ndarray, np.ndarray, np.ndarray, np.ndarray]:
        face_uv = self.uv[self.mesh.faces]
        u = face_uv[:, :, 0].mean(axis=1)
        angle = face_uv[:, :, 1] * (2.0 * np.pi)
        v = np.mod(
            np.arctan2(np.sin(angle).mean(axis=1), np.cos(angle).mean(axis=1)),
            2.0 * np.pi,
        ) / (2.0 * np.pi)
        side = np.where(v < 0.5, 1, -1)  # +1 right, -1 left
        q = np.where(v < 0.5, 2.0 * v, 2.0 * (1.0 - v))  # 0 roof, 1 underbody
        return u, v, q, side

    def extract_faces(self, face_ids: np.ndarray, name: str, rgb: tuple[int, int, int], opacity: float = 0.82) -> trimesh.Trimesh:
        ids = np.asarray(face_ids, dtype=np.int64)
        if len(ids) == 0:
            raise ValueError(f"surface region {name} is empty")
        patch = self.mesh.submesh([ids], append=True, repair=False)
        if patch is None or len(patch.faces) == 0:
            raise ValueError(f"surface region {name} failed extraction")
        patch.remove_unreferenced_vertices()
        set_mesh_color(patch, name, rgb, opacity, 0.20, 0.35)
        return patch


@dataclass
class SurfacePartition:
    panel_owner: np.ndarray
    aperture_owner: np.ndarray
    resolved_owner: np.ndarray
    panel_meshes: dict[str, trimesh.Trimesh]
    aperture_meshes: dict[str, trimesh.Trimesh]
    closure_meshes: dict[str, trimesh.Trimesh]
    fixed_body_mesh: trimesh.Trimesh
    graph: dict[str, Any]


PANEL_COLORS: dict[str, tuple[int, int, int]] = {
    "hood": (230, 58, 42),
    "roof": (226, 174, 30),
    "front_door_left": (84, 184, 70),
    "front_door_right": (84, 184, 70),
    "rear_door_left": (34, 178, 165),
    "rear_door_right": (34, 178, 165),
    "front_fender_left": (240, 123, 28),
    "front_fender_right": (240, 123, 28),
    "rear_quarter_left": (135, 75, 190),
    "rear_quarter_right": (135, 75, 190),
    "rear_hatch": (85, 102, 205),
    "front_bumper": (57, 87, 176),
    "rear_bumper": (73, 82, 155),
    "rocker_left": (32, 151, 153),
    "rocker_right": (32, 151, 153),
    "underbody": (55, 59, 67),
    "fixed_body": (170, 174, 184),
}

APERTURE_COLORS: dict[str, tuple[int, int, int]] = {
    "windshield": (35, 85, 125),
    "rear_glass": (35, 85, 125),
    "front_side_glass_left": (29, 76, 113),
    "front_side_glass_right": (29, 76, 113),
    "rear_side_glass_left": (29, 76, 113),
    "rear_side_glass_right": (29, 76, 113),
}

CLOSURE_NAMES = {
    "front_door_left",
    "front_door_right",
    "rear_door_left",
    "rear_door_right",
    "hood",
    "rear_hatch",
}


def _face_components(mesh: trimesh.Trimesh, face_ids: np.ndarray) -> int:
    ids = np.asarray(face_ids, dtype=np.int64)
    if len(ids) == 0:
        return 0
    local_map = np.full(len(mesh.faces), -1, dtype=np.int64)
    local_map[ids] = np.arange(len(ids))
    adjacency = mesh.face_adjacency
    keep = (local_map[adjacency[:, 0]] >= 0) & (local_map[adjacency[:, 1]] >= 0)
    local_edges = local_map[adjacency[keep]]
    neighbours: list[list[int]] = [[] for _ in range(len(ids))]
    for a, b in local_edges:
        neighbours[int(a)].append(int(b))
        neighbours[int(b)].append(int(a))
    count = 0
    seen: set[int] = set()
    for start in range(len(ids)):
        if start in seen:
            continue
        count += 1
        stack = [start]
        seen.add(start)
        while stack:
            current = stack.pop()
            for nxt in neighbours[current]:
                if nxt not in seen:
                    seen.add(nxt)
                    stack.append(nxt)
    return count


def boundary_loops_for_faces(mesh: trimesh.Trimesh, face_ids: np.ndarray) -> list[list[int]]:
    """Return ordered boundary vertex loops/chains for a selected face region."""
    selected = np.asarray(face_ids, dtype=np.int64)
    if len(selected) == 0:
        return []
    counts: dict[tuple[int, int], int] = defaultdict(int)
    for face in np.asarray(mesh.faces[selected], dtype=np.int64):
        for a, b in ((face[0], face[1]), (face[1], face[2]), (face[2], face[0])):
            edge = (int(min(a, b)), int(max(a, b)))
            counts[edge] += 1
    boundary = [edge for edge, count in counts.items() if count == 1]
    adjacency: dict[int, list[int]] = defaultdict(list)
    for a, b in boundary:
        adjacency[a].append(b)
        adjacency[b].append(a)
    unused = {tuple(sorted(edge)) for edge in boundary}
    loops: list[list[int]] = []
    while unused:
        first = next(iter(unused))
        start_candidates = [vertex for vertex in first if len(adjacency[vertex]) == 1]
        start = start_candidates[0] if start_candidates else first[0]
        chain = [start]
        previous = None
        current = start
        while True:
            candidates = [
                neighbour for neighbour in adjacency[current]
                if tuple(sorted((current, neighbour))) in unused and neighbour != previous
            ]
            if not candidates:
                break
            nxt = candidates[0]
            unused.remove(tuple(sorted((current, nxt))))
            chain.append(nxt)
            previous, current = current, nxt
            if current == start:
                break
        loops.append(chain)
    return loops


def build_surface_partition(surface: SemanticUVSurface) -> SurfacePartition:
    u, _, q, side_sign = surface.face_uv()
    face_count = len(surface.mesh.faces)
    panel_owner = np.full(face_count, "fixed_body", dtype=object)
    aperture_owner = np.full(face_count, "", dtype=object)

    def aperture(name: str, u0: float, u1: float, q0: float, q1: float, side: int | None = None) -> None:
        mask = (u >= u0) & (u < u1) & (q >= q0) & (q < q1)
        if side is not None:
            mask &= side_sign == side
        aperture_owner[mask] = name

    # Glass support regions are inset from closure boundaries so metal window
    # frames remain connected. Windshield and hatch glass cross the roof seam.
    aperture("windshield", 0.665, 0.765, 0.000, 0.285)
    aperture("rear_glass", 0.050, 0.220, 0.000, 0.300)
    aperture("front_side_glass_left", 0.485, 0.660, 0.140, 0.310, -1)
    aperture("front_side_glass_right", 0.485, 0.660, 0.140, 0.310, 1)
    aperture("rear_side_glass_left", 0.325, 0.430, 0.140, 0.310, -1)
    aperture("rear_side_glass_right", 0.325, 0.430, 0.140, 0.310, 1)

    def assign(name: str, mask: np.ndarray) -> None:
        panel_owner[mask] = name

    # Priority is deliberate: global underbody/bumper caps first, then styled
    # upper and side regions, then rockers. Every face retains one panel owner.
    assign("underbody", q >= 0.920)
    assign("front_bumper", (u >= 0.930) & (q < 0.920))
    assign("rear_bumper", (u < 0.060) & (q < 0.920))
    assign("hood", (u >= 0.720) & (u < 0.930) & (q < 0.320))
    assign("roof", (u >= 0.220) & (u < 0.720) & (q < 0.100))
    for sign, label in ((-1, "left"), (1, "right")):
        side = side_sign == sign
        assign(f"front_door_{label}", side & (u >= 0.455) & (u < 0.690) & (q >= 0.080) & (q < 0.840))
        assign(f"rear_door_{label}", side & (u >= 0.300) & (u < 0.455) & (q >= 0.080) & (q < 0.840))
        assign(f"front_fender_{label}", side & (u >= 0.690) & (u < 0.930) & (q >= 0.250) & (q < 0.880))
        assign(f"rear_quarter_{label}", side & (u >= 0.060) & (u < 0.300) & (q >= 0.200) & (q < 0.880))
        assign(f"rocker_{label}", side & (u >= 0.250) & (u < 0.720) & (q >= 0.840) & (q < 0.920))

    # The hatch is a closure spanning the rear centre and upper side surface.
    # Assign it after quarter panels so the tailgate is not accidentally
    # consumed by the surrounding fixed quarter domains.
    assign("rear_hatch", (u >= 0.060) & (u < 0.220) & (q < 0.620))

    resolved_owner = np.where(aperture_owner != "", aperture_owner, panel_owner)
    panel_meshes: dict[str, trimesh.Trimesh] = {}
    aperture_meshes: dict[str, trimesh.Trimesh] = {}
    closure_meshes: dict[str, trimesh.Trimesh] = {}
    panel_records: list[dict[str, Any]] = []
    aperture_records: list[dict[str, Any]] = []

    for name in sorted(set(panel_owner.tolist())):
        # Panel visuals exclude aperture support faces, producing actual open
        # glass regions in the closure/fixed-body surface model.
        ids = np.flatnonzero((panel_owner == name) & (aperture_owner == ""))
        if len(ids) == 0:
            continue
        mesh = surface.extract_faces(ids, f"Panel_{name}", PANEL_COLORS.get(name, (165, 165, 175)))
        panel_meshes[name] = mesh
        components = _face_components(surface.mesh, ids)
        loops = boundary_loops_for_faces(surface.mesh, ids)
        record = {
            "name": name,
            "face_count": int(len(ids)),
            "connected_component_count": int(components),
            "boundary_loop_count": int(sum(1 for loop in loops if len(loop) > 2 and loop[0] == loop[-1])),
            "boundary_chain_count": int(len(loops)),
            "closure": name in CLOSURE_NAMES,
        }
        panel_records.append(record)
        if name in CLOSURE_NAMES:
            closure_meshes[name] = mesh.copy()

    for name in sorted(set(aperture_owner.tolist()) - {""}):
        ids = np.flatnonzero(aperture_owner == name)
        mesh = surface.extract_faces(ids, f"Aperture_{name}", APERTURE_COLORS[name], opacity=0.54)
        aperture_meshes[name] = mesh
        components = _face_components(surface.mesh, ids)
        loops = boundary_loops_for_faces(surface.mesh, ids)
        aperture_records.append({
            "name": name,
            "face_count": int(len(ids)),
            "connected_component_count": int(components),
            "boundary_chain_count": int(len(loops)),
            "closed_boundary_loop_count": int(sum(1 for loop in loops if len(loop) > 2 and loop[0] == loop[-1])),
        })

    fixed_ids = np.flatnonzero(~np.isin(panel_owner, list(CLOSURE_NAMES)) & (aperture_owner == ""))
    fixed_body_mesh = surface.extract_faces(fixed_ids, "FixedBodyWithApertures", (152, 158, 168), opacity=0.48)

    owner_counts = {str(name): int(np.count_nonzero(resolved_owner == name)) for name in sorted(set(resolved_owner.tolist()))}
    graph = {
        "schema": "MCSMv2.2.SurfacePartition.v2",
        "surface_face_count": int(face_count),
        "coverage_fraction": float(np.mean(resolved_owner != "")),
        "unassigned_face_count": int(np.count_nonzero(resolved_owner == "")),
        "resolved_multiple_owner_face_count": 0,
        "exclusive_owner_count": int(len(set(resolved_owner.tolist()))),
        "owner_face_counts": owner_counts,
        "panels": panel_records,
        "apertures": aperture_records,
        "closure_names": sorted(CLOSURE_NAMES),
        "fixed_body_face_count": int(len(fixed_ids)),
        "compound_closure_notes": {
            "rear_hatch": "outer semantic skin may contain left/right components; a production inner frame is deferred",
        },
        "pass": bool(
            np.all(resolved_owner != "")
            and len(aperture_records) == 6
            and CLOSURE_NAMES.issubset(panel_meshes.keys())
            and all(
                item["connected_component_count"] <= (2 if item["name"] == "rear_hatch" else 1)
                for item in panel_records
                if item["closure"]
            )
            and all(item["connected_component_count"] == 1 for item in aperture_records)
        ),
    }
    return SurfacePartition(
        panel_owner=panel_owner,
        aperture_owner=aperture_owner,
        resolved_owner=resolved_owner,
        panel_meshes=panel_meshes,
        aperture_meshes=aperture_meshes,
        closure_meshes=closure_meshes,
        fixed_body_mesh=fixed_body_mesh,
        graph=graph,
    )


# =============================================================================
# Independent semantic/implicit surface alignment
# =============================================================================


def build_implicit_outer_scaffold(model: base.ModernCarModelV2, resolution: tuple[int, int, int] = (143, 79, 97)) -> trimesh.Trimesh:
    nx, ny, nz = resolution
    xs = np.linspace(model.rear - 0.018, model.front + 0.018, nx)
    ys = np.linspace(-model.v.width / 2.0 - 0.025, model.v.width / 2.0 + 0.025, ny)
    zs = np.linspace(0.0, model.v.height + 0.025, nz)
    field = model.field_on_axes(xs, ys, zs, include_wheelhouses=False)
    mesh = base.ModernCarModelV2._mesh_from_field(field, (xs, ys, zs))
    mesh.process(validate=True)
    mesh.fix_normals(multibody=True)
    set_mesh_color(mesh, "ImplicitOuterScaffold", (196, 94, 30), 0.28, 0.12, 0.42)
    return mesh


def surface_alignment_report(
    semantic: trimesh.Trimesh,
    scaffold: trimesh.Trimesh,
    sample_limit: int = 6000,
) -> dict[str, Any]:
    """Compare independently tessellated surfaces using nearest triangles.

    Vertex-to-vertex distance is dominated by unrelated sampling grids.  This
    gate instead measures each sampled point against the other mesh's actual
    triangles and compares the corresponding final-mesh normals.
    """
    rng = np.random.default_rng(220)
    semantic_all = np.asarray(semantic.vertices, dtype=float)
    scaffold_all = np.asarray(scaffold.vertices, dtype=float)
    semantic_ids = np.arange(len(semantic_all), dtype=np.int64)
    scaffold_ids = np.arange(len(scaffold_all), dtype=np.int64)
    if len(semantic_ids) > sample_limit:
        semantic_ids = np.sort(rng.choice(semantic_ids, sample_limit, replace=False))
    if len(scaffold_ids) > sample_limit:
        scaffold_ids = np.sort(rng.choice(scaffold_ids, sample_limit, replace=False))
    semantic_points = semantic_all[semantic_ids]
    scaffold_points = scaffold_all[scaffold_ids]

    scaffold_index = SurfaceDistanceIndex(scaffold)
    semantic_index = SurfaceDistanceIndex(semantic)
    d_si, face_si, _ = scaffold_index.closest(semantic_points, nearest_vertices=18)
    d_is, face_is, _ = semantic_index.closest(scaffold_points, nearest_vertices=18)

    semantic_vertex_normals = np.asarray(semantic.vertex_normals, dtype=float)[semantic_ids]
    scaffold_face_normals = np.asarray(scaffold.face_normals, dtype=float)[face_si]
    scaffold_vertex_normals = np.asarray(scaffold.vertex_normals, dtype=float)[scaffold_ids]
    semantic_face_normals = np.asarray(semantic.face_normals, dtype=float)[face_is]
    dot_si = np.clip(np.abs(np.einsum("ij,ij->i", semantic_vertex_normals, scaffold_face_normals)), 0.0, 1.0)
    dot_is = np.clip(np.abs(np.einsum("ij,ij->i", scaffold_vertex_normals, semantic_face_normals)), 0.0, 1.0)
    angle_si = np.degrees(np.arccos(dot_si))
    angle_is = np.degrees(np.arccos(dot_is))
    combined = np.concatenate([d_si, d_is])
    angles = np.concatenate([angle_si, angle_is])
    distance_tolerance = 0.012
    normal_tolerance = 28.0
    return {
        "schema": "MCSMv2.2.SemanticImplicitAlignment.v3",
        "basis": "bidirectional nearest-triangle distance between independently tessellated final surfaces",
        "semantic_sample_count": int(len(semantic_points)),
        "implicit_sample_count": int(len(scaffold_points)),
        "semantic_to_implicit_mean_m": float(np.mean(d_si)),
        "semantic_to_implicit_p95_m": float(np.quantile(d_si, 0.95)),
        "implicit_to_semantic_mean_m": float(np.mean(d_is)),
        "implicit_to_semantic_p95_m": float(np.quantile(d_is, 0.95)),
        "combined_p95_m": float(np.quantile(combined, 0.95)),
        "combined_maximum_m": float(np.max(combined)),
        "normal_angle_mean_deg": float(np.mean(angles)),
        "normal_angle_p95_deg": float(np.quantile(angles, 0.95)),
        "distance_tolerance_m": distance_tolerance,
        "normal_tolerance_deg": normal_tolerance,
        "pass": bool(
            np.quantile(combined, 0.95) <= distance_tolerance
            and np.quantile(angles, 0.95) <= normal_tolerance
        ),
    }


# =============================================================================
# Suspension hardpoints and wheel-pose solver
# =============================================================================


@dataclass(frozen=True)
class SuspensionHardpoint:
    name: str
    axle: str
    side: str
    role: str
    position: tuple[float, float, float]
    evidence_status: str = "ConceptDerived"
    confidence: float = 0.55


@dataclass(frozen=True)
class WheelPose:
    axle: str
    side: str
    steer_deg: float
    travel_m: float
    camber_deg: float
    toe_deg: float
    centre: tuple[float, float, float]
    transform: tuple[tuple[float, ...], ...]


class SuspensionKinematicModel:
    """Concept-level hardpoint graph and deterministic wheel-pose solver."""

    def __init__(self, model: base.ModernCarModelV2):
        self.model = model
        self.v = model.v
        self.hardpoints = self._build_hardpoints()

    def _build_hardpoints(self) -> list[SuspensionHardpoint]:
        hp: list[SuspensionHardpoint] = []
        for axle, x, track in (
            ("front", self.model.front_axle, self.v.track_front),
            ("rear", self.model.rear_axle, self.v.track_rear),
        ):
            for sign, side in ((-1.0, "left"), (1.0, "right")):
                hub_y = sign * track / 2.0
                hub_z = self.v.wheel_radius
                if axle == "front":
                    values = {
                        "hub_centre": (x, hub_y, hub_z),
                        "strut_top": (x - 0.035, sign * (track / 2.0 - 0.165), self.v.platform.floor_height + 0.72),
                        "lower_arm_front_inner": (x + 0.20, sign * 0.36, self.v.platform.floor_height + 0.035),
                        "lower_arm_rear_inner": (x - 0.20, sign * 0.38, self.v.platform.floor_height + 0.025),
                        "ball_joint": (x + 0.015, sign * (track / 2.0 - 0.035), hub_z - 0.045),
                        "tie_rod_inner": (x - 0.085, sign * 0.31, hub_z + 0.010),
                        "tie_rod_outer": (x - 0.025, sign * (track / 2.0 - 0.025), hub_z + 0.005),
                    }
                else:
                    values = {
                        "hub_centre": (x, hub_y, hub_z),
                        "upper_link_inner": (x + 0.04, sign * 0.43, hub_z + 0.22),
                        "upper_link_outer": (x + 0.02, sign * (track / 2.0 - 0.040), hub_z + 0.18),
                        "lower_link_front_inner": (x + 0.17, sign * 0.38, self.v.platform.floor_height + 0.020),
                        "lower_link_rear_inner": (x - 0.17, sign * 0.40, self.v.platform.floor_height + 0.015),
                        "lower_link_outer": (x, sign * (track / 2.0 - 0.035), hub_z - 0.050),
                        "toe_link_inner": (x - 0.12, sign * 0.34, hub_z + 0.015),
                        "toe_link_outer": (x - 0.05, sign * (track / 2.0 - 0.025), hub_z + 0.005),
                        "damper_top": (x + 0.03, sign * (track / 2.0 - 0.16), self.v.platform.floor_height + 0.65),
                    }
                for role, position in values.items():
                    hp.append(SuspensionHardpoint(
                        name=f"{axle}_{side}_{role}",
                        axle=axle,
                        side=side,
                        role=role,
                        position=tuple(float(value) for value in position),
                    ))
        return hp

    def wheel_pose(self, axle: str, side: str, steer_deg: float, travel_m: float) -> WheelPose:
        if axle not in {"front", "rear"}:
            raise ValueError(f"invalid axle: {axle}")
        if side not in {"left", "right"}:
            raise ValueError(f"invalid side: {side}")
        sign = -1.0 if side == "left" else 1.0
        x0 = self.model.front_axle if axle == "front" else self.model.rear_axle
        track = self.v.track_front if axle == "front" else self.v.track_rear
        steer = float(steer_deg if axle == "front" else 0.0)
        travel = float(travel_m)
        static_camber = -1.0 if axle == "front" else -1.3
        camber_gain_deg_per_m = -11.0 if axle == "front" else -14.0
        toe_gain_deg_per_m = 1.5 if axle == "front" else 2.0
        camber = static_camber + camber_gain_deg_per_m * travel
        toe = toe_gain_deg_per_m * travel
        centre = np.asarray([
            x0 + (-0.025 if axle == "front" else 0.010) * travel,
            sign * (track / 2.0 + 0.018 * travel),
            self.v.wheel_radius + travel,
        ], dtype=float)
        transform = translation_matrix(centre)
        transform = transform @ trimesh.transformations.rotation_matrix(math.radians(steer + toe), [0, 0, 1])
        transform = transform @ trimesh.transformations.rotation_matrix(math.radians(sign * camber), [1, 0, 0])
        return WheelPose(
            axle=axle,
            side=side,
            steer_deg=steer,
            travel_m=travel,
            camber_deg=float(camber),
            toe_deg=float(toe),
            centre=tuple(float(value) for value in centre),
            transform=tuple(tuple(float(value) for value in row) for row in transform),
        )

    def pose_grid(
        self,
        axle: str,
        side: str,
        steer_samples: int = 5,
        travel_samples: int = 3,
    ) -> list[WheelPose]:
        """Return a deterministic concept-level wheel-pose grid.

        Five front steering samples and three jounce samples include both
        extrema and the neutral pose.  The rear uses the same travel samples
        with zero steer.  Higher-density studies can request larger counts
        without changing the kinematic model.
        """
        steer = (
            np.linspace(
                self.v.wheel_suspension.steer_min_deg,
                self.v.wheel_suspension.steer_max_deg,
                max(3, int(steer_samples)),
            )
            if axle == "front" else np.asarray([0.0])
        )
        travel = np.linspace(
            self.v.wheel_suspension.travel_min,
            self.v.wheel_suspension.travel_max,
            max(3, int(travel_samples)),
        )
        return [self.wheel_pose(axle, side, float(delta), float(jounce)) for delta in steer for jounce in travel]

    def validate(self) -> dict[str, Any]:
        positions = np.asarray([item.position for item in self.hardpoints], dtype=float)
        finite = bool(np.isfinite(positions).all())
        names = {item.name for item in self.hardpoints}
        mirror_residuals: list[float] = []
        for item in self.hardpoints:
            if item.side != "left":
                continue
            opposite = item.name.replace("_left_", "_right_")
            if opposite in names:
                other = next(candidate for candidate in self.hardpoints if candidate.name == opposite)
                expected = np.asarray(item.position) * np.asarray([1.0, -1.0, 1.0])
                mirror_residuals.append(float(np.linalg.norm(expected - np.asarray(other.position))))
        pose_determinants: list[float] = []
        for axle in ("front", "rear"):
            for side in ("left", "right"):
                for pose in self.pose_grid(axle, side):
                    pose_determinants.append(float(np.linalg.det(np.asarray(pose.transform)[:3, :3])))
        return {
            "schema": "MCSMv2.2.SuspensionHardpointValidation.v1",
            "hardpoint_count": int(len(self.hardpoints)),
            "finite": finite,
            "maximum_mirror_residual_m": float(max(mirror_residuals, default=0.0)),
            "minimum_pose_rotation_determinant": float(min(pose_determinants, default=1.0)),
            "maximum_pose_rotation_determinant": float(max(pose_determinants, default=1.0)),
            "pass": bool(finite and max(mirror_residuals, default=0.0) <= 1e-9 and all(abs(value - 1.0) <= 1e-8 for value in pose_determinants)),
            "assurance_note": "Concept hardpoints and gain-based pose solver; not measured production suspension geometry.",
        }


# =============================================================================
# Actual tyre geometry and pose sweep
# =============================================================================


def create_tyre_mesh(wheel_radius: float, wheel_width: float, major_sections: int = 40, minor_sections: int = 14) -> trimesh.Trimesh:
    rim_radius = max(wheel_radius * 0.69, wheel_radius - 0.105)
    radial_radius = 0.5 * (wheel_radius - rim_radius)
    major_radius = rim_radius + radial_radius
    lateral_radius = 0.5 * wheel_width
    vertices: list[list[float]] = []
    faces: list[list[int]] = []
    for i in range(major_sections):
        theta = 2.0 * math.pi * i / major_sections
        ct, st = math.cos(theta), math.sin(theta)
        for j in range(minor_sections):
            phi = 2.0 * math.pi * j / minor_sections
            cp, sp = math.cos(phi), math.sin(phi)
            radius = major_radius + radial_radius * cp
            vertices.append([radius * ct, lateral_radius * sp, radius * st])
    for i in range(major_sections):
        ni = (i + 1) % major_sections
        for j in range(minor_sections):
            nj = (j + 1) % minor_sections
            a = i * minor_sections + j
            b = ni * minor_sections + j
            c = ni * minor_sections + nj
            d = i * minor_sections + nj
            faces.extend([[a, b, c], [a, c, d]])
    mesh = trimesh.Trimesh(vertices=np.asarray(vertices), faces=np.asarray(faces), process=True)
    mesh.fix_normals(multibody=True)
    set_mesh_color(mesh, "Tyre", (28, 29, 32), 1.0, 0.05, 0.72)
    return mesh


@dataclass
class TyreSweepResult:
    pose_records: list[dict[str, Any]]
    sweep_hulls: dict[str, trimesh.Trimesh]
    pose_meshes: dict[str, list[trimesh.Trimesh]]
    validation: dict[str, Any]


def build_tyre_sweeps(
    model: base.ModernCarModelV2,
    suspension: SuspensionKinematicModel,
    body: trimesh.Trimesh,
    steer_samples: int = 5,
    travel_samples: int = 3,
) -> TyreSweepResult:
    """Build transformed tyre meshes and independently sample final-body clearance.

    The sweep hull is generated from every vertex of every sampled tyre pose.
    Clearance is measured against the exported final triangle mesh.  Ray-parity
    containment is evaluated on a smaller deterministic subset because it is
    substantially more expensive than nearest-triangle distance without an
    optional acceleration library.
    """
    base_tyre = create_tyre_mesh(model.v.wheel_radius, model.v.wheel_width)
    parity_index = base.MeshSpatialIndex(body)
    distance_index = SurfaceDistanceIndex(body)
    pose_records: list[dict[str, Any]] = []
    hulls: dict[str, trimesh.Trimesh] = {}
    pose_meshes: dict[str, list[trimesh.Trimesh]] = {}
    wheel_records: list[dict[str, Any]] = []
    all_pass = True
    global_minimum = float("inf")

    for axle in ("front", "rear"):
        for side in ("left", "right"):
            name = f"{axle}_{side}"
            poses = suspension.pose_grid(
                axle, side,
                steer_samples=steer_samples,
                travel_samples=travel_samples,
            )
            transformed: list[trimesh.Trimesh] = []
            clouds: list[np.ndarray] = []
            distance_points: list[np.ndarray] = []
            parity_points: list[np.ndarray] = []
            for pose in poses:
                mesh = transformed_mesh(base_tyre, np.asarray(pose.transform))
                transformed.append(mesh)
                vertices = np.asarray(mesh.vertices, dtype=float)
                clouds.append(vertices)
                distance_ids = np.linspace(0, len(vertices) - 1, min(96, len(vertices))).astype(int)
                parity_ids = np.linspace(0, len(vertices) - 1, min(28, len(vertices))).astype(int)
                distance_points.append(vertices[distance_ids])
                parity_points.append(vertices[parity_ids])
                pose_records.append({
                    "wheel": name,
                    "steer_deg": pose.steer_deg,
                    "travel_m": pose.travel_m,
                    "camber_deg": pose.camber_deg,
                    "toe_deg": pose.toe_deg,
                    "centre_m": list(pose.centre),
                    "transform": [list(row) for row in pose.transform],
                })

            points = np.vstack(clouds)
            hull = trimesh.convex.convex_hull(points)
            set_mesh_color(hull, f"TyreSweep_{name}", (220, 32, 176), 0.24, 0.05, 0.50)
            hulls[name] = hull
            pose_meshes[name] = transformed

            distance_query = np.vstack(distance_points)
            parity_query = np.vstack(parity_points)
            distances = distance_index.distance(distance_query, nearest_vertices=12)
            inside = parity_index.contains_points(parity_query)
            minimum = float(np.min(distances))
            p01 = float(np.quantile(distances, 0.01))
            outside_fraction = float(np.mean(~inside))
            target = float(model.v.wheel_suspension.wheelhouse_clearance)
            tolerance = 0.004
            passed = bool(outside_fraction >= 0.99 and minimum >= target - tolerance)
            all_pass &= passed
            global_minimum = min(global_minimum, minimum)
            wheel_records.append({
                "wheel": name,
                "pose_count": int(len(poses)),
                "distance_sample_count": int(len(distance_query)),
                "parity_sample_count": int(len(parity_query)),
                "minimum_clearance_m": minimum,
                "p01_clearance_m": p01,
                "outside_fraction": outside_fraction,
                "declared_clearance_m": target,
                "tessellation_tolerance_m": tolerance,
                "pass": passed,
            })

    validation = {
        "schema": "MCSMv2.2.ActualTyrePoseSweepValidation.v2",
        "wheel_count": 4,
        "pose_count": int(len(pose_records)),
        "front_steer_samples": int(max(3, steer_samples)),
        "travel_samples": int(max(3, travel_samples)),
        "tyre_geometry": "elliptical-section torus mesh",
        "sweep_geometry": "convex hull of all transformed actual tyre vertices",
        "clearance_basis": "nearest final-body triangles plus sparse final-mesh ray parity",
        "minimum_clearance_m": float(global_minimum),
        "declared_clearance_m": float(model.v.wheel_suspension.wheelhouse_clearance),
        "wheels": wheel_records,
        "pass": bool(all_pass),
    }
    return TyreSweepResult(pose_records, hulls, pose_meshes, validation)


# =============================================================================
# Closure and helical glass kinematics
# =============================================================================


@dataclass(frozen=True)
class HingeSystem:
    name: str
    closure: str
    point: tuple[float, float, float]
    axis: tuple[float, float, float]
    maximum_angle_deg: float
    direction: float
    rise_m: float = 0.0
    translation_axis: tuple[float, float, float] = (0.0, 0.0, 1.0)
    joint_type: str = "revolute"

    def transform(self, state: float) -> np.ndarray:
        q = float(np.clip(state, 0.0, 1.0))
        angle = math.radians(self.direction * self.maximum_angle_deg * q)
        rotation = axis_rotation_matrix(self.point, self.axis, angle)
        if self.rise_m != 0.0:
            amount = self.rise_m * math.sin(0.5 * math.pi * q)
            return translation_matrix(np.asarray(self.translation_axis) * amount) @ rotation
        return rotation


@dataclass(frozen=True)
class HelicalGlassSystem:
    name: str
    parent_closure: str
    side: str
    travel_m: float
    inward_m: float
    longitudinal_m: float
    rotation_deg: float
    pivot: tuple[float, float, float]

    def local_transform(self, state: float) -> np.ndarray:
        q = float(np.clip(state, 0.0, 1.0))
        sign = -1.0 if self.side == "left" else 1.0
        # Inward means toward the vehicle centre. The rotation about X follows
        # the side-glass barrel and is deliberately small at concept level.
        offset = np.asarray([
            self.longitudinal_m * q,
            -sign * self.inward_m * q,
            -self.travel_m * q,
        ], dtype=float)
        rotation = trimesh.transformations.rotation_matrix(
            math.radians(sign * self.rotation_deg * q),
            [1.0, 0.0, 0.0],
            point=np.asarray(self.pivot, dtype=float),
        )
        return translation_matrix(offset) @ rotation


@dataclass
class ClosureSystemModel:
    hinges: dict[str, HingeSystem]
    glass_systems: dict[str, HelicalGlassSystem]
    closure_meshes: dict[str, trimesh.Trimesh]
    aperture_meshes: dict[str, trimesh.Trimesh]
    fixed_body_mesh: trimesh.Trimesh

    def closure_transform(self, name: str, state: float) -> np.ndarray:
        return self.hinges[name].transform(state)

    def glass_transform(self, glass_name: str, closure_state: float, glass_state: float) -> np.ndarray:
        system = self.glass_systems[glass_name]
        return self.closure_transform(system.parent_closure, closure_state) @ system.local_transform(glass_state)


def _hinge_point_from_mesh(mesh: trimesh.Trimesh, front_edge: bool, side: str | None, upper: bool = False) -> tuple[np.ndarray, np.ndarray]:
    vertices = np.asarray(mesh.vertices, dtype=float)
    x_target = np.max(vertices[:, 0]) if front_edge else np.min(vertices[:, 0])
    band = np.abs(vertices[:, 0] - x_target) <= max(0.02, 0.035 * np.ptp(vertices[:, 0]))
    selected = vertices[band]
    if len(selected) == 0:
        selected = vertices
    y = float(np.median(selected[:, 1]))
    z_min, z_max = float(np.min(selected[:, 2])), float(np.max(selected[:, 2]))
    point = np.asarray([x_target, y, 0.5 * (z_min + z_max)])
    axis = np.asarray([0.0, 0.0, 1.0])
    if upper:
        point[2] = z_max
    return point, axis


def build_closure_system(model: base.ModernCarModelV2, partition: SurfacePartition) -> ClosureSystemModel:
    meshes = partition.closure_meshes
    hinges: dict[str, HingeSystem] = {}
    for name, side, maximum in (
        ("front_door_left", "left", model.v.closures.front_door_max_deg),
        ("front_door_right", "right", model.v.closures.front_door_max_deg),
        ("rear_door_left", "left", model.v.closures.rear_door_max_deg),
        ("rear_door_right", "right", model.v.closures.rear_door_max_deg),
    ):
        point, axis = _hinge_point_from_mesh(meshes[name], front_edge=True, side=side)
        outward = -1.0 if side == "left" else 1.0
        point[1] += outward * 0.006
        hinges[name] = HingeSystem(
            name=f"hinge_{name}", closure=name,
            point=tuple(point), axis=tuple(axis), maximum_angle_deg=float(maximum),
            direction=1.0 if side == "left" else -1.0,
            joint_type="paired_vertical_revolute",
        )

    hood = meshes["hood"]
    hverts = np.asarray(hood.vertices)
    rear_x = float(np.min(hverts[:, 0]))
    top_band = hverts[np.abs(hverts[:, 0] - rear_x) <= max(0.02, 0.04 * np.ptp(hverts[:, 0]))]
    hood_point = np.asarray([rear_x, 0.0, float(np.mean(top_band[:, 2]))])
    hinges["hood"] = HingeSystem(
        name="hinge_hood", closure="hood", point=tuple(hood_point), axis=(0.0, 1.0, 0.0),
        maximum_angle_deg=float(model.v.closures.bonnet_max_deg), direction=-1.0,
        rise_m=0.035, translation_axis=(0.0, 0.0, 1.0), joint_type="rising_revolute_fourbar_approximation",
    )

    hatch = meshes["rear_hatch"]
    hv = np.asarray(hatch.vertices)
    front_x = float(np.max(hv[:, 0]))
    high = hv[(np.abs(hv[:, 0] - front_x) <= max(0.02, 0.05 * np.ptp(hv[:, 0])))]
    hatch_point = np.asarray([front_x, 0.0, float(np.max(high[:, 2]))])
    hinges["rear_hatch"] = HingeSystem(
        name="hinge_rear_hatch", closure="rear_hatch", point=tuple(hatch_point), axis=(0.0, 1.0, 0.0),
        maximum_angle_deg=float(model.v.closures.hatch_max_deg), direction=1.0,
        rise_m=0.020, translation_axis=(-1.0, 0.0, 1.0), joint_type="roof_header_revolute_with_strut_rise",
    )

    def glass_pivot(name: str) -> tuple[float, float, float]:
        bounds = np.asarray(partition.aperture_meshes[name].bounds, dtype=float)
        centre = 0.5 * (bounds[0] + bounds[1])
        # Use a barrel axis near the lower half of the raised glass rather
        # than rotating around the global origin.
        centre[2] = bounds[0, 2] + 0.30 * (bounds[1, 2] - bounds[0, 2])
        return tuple(float(value) for value in centre)

    glass_systems = {
        "front_side_glass_left": HelicalGlassSystem(
            "front_side_glass_left", "front_door_left", "left",
            model.v.closures.side_glass_travel, 0.018, -0.012, 2.2,
            glass_pivot("front_side_glass_left"),
        ),
        "front_side_glass_right": HelicalGlassSystem(
            "front_side_glass_right", "front_door_right", "right",
            model.v.closures.side_glass_travel, 0.018, -0.012, 2.2,
            glass_pivot("front_side_glass_right"),
        ),
        "rear_side_glass_left": HelicalGlassSystem(
            "rear_side_glass_left", "rear_door_left", "left",
            model.v.closures.side_glass_travel * 0.90, 0.015, 0.010, 1.8,
            glass_pivot("rear_side_glass_left"),
        ),
        "rear_side_glass_right": HelicalGlassSystem(
            "rear_side_glass_right", "rear_door_right", "right",
            model.v.closures.side_glass_travel * 0.90, 0.015, 0.010, 1.8,
            glass_pivot("rear_side_glass_right"),
        ),
    }

    return ClosureSystemModel(
        hinges=hinges,
        glass_systems=glass_systems,
        closure_meshes={name: mesh.copy() for name, mesh in meshes.items()},
        aperture_meshes={name: mesh.copy() for name, mesh in partition.aperture_meshes.items()},
        fixed_body_mesh=partition.fixed_body_mesh.copy(),
    )


# =============================================================================
# Independent surface collision and distance checks
# =============================================================================


class SurfaceDistanceIndex:
    def __init__(self, mesh: trimesh.Trimesh):
        self.mesh = mesh
        self.triangles = np.asarray(mesh.triangles, dtype=float)
        self.vertex_tree = cKDTree(np.asarray(mesh.vertices, dtype=float))
        self.vertex_faces = np.asarray(mesh.vertex_faces, dtype=np.int64)

    def closest(
        self,
        points: np.ndarray,
        nearest_vertices: int = 14,
    ) -> tuple[np.ndarray, np.ndarray, np.ndarray]:
        query = np.asarray(points, dtype=float)
        k = min(nearest_vertices, len(self.mesh.vertices))
        _, vertex_ids = self.vertex_tree.query(query, k=k)
        if k == 1:
            vertex_ids = vertex_ids[:, None]
        distances = np.empty(len(query), dtype=float)
        nearest_faces = np.empty(len(query), dtype=np.int64)
        nearest_points = np.empty_like(query)
        for index, point in enumerate(query):
            face_ids = self.vertex_faces[np.asarray(vertex_ids[index])].reshape(-1)
            face_ids = np.unique(face_ids[face_ids >= 0])
            if len(face_ids) == 0:
                distances[index] = float("inf")
                nearest_faces[index] = -1
                nearest_points[index] = point
                continue
            triangles = self.triangles[face_ids]
            repeated = np.broadcast_to(point, (len(triangles), 3))
            closest = trimesh.triangles.closest_point(triangles, repeated)
            candidate_distance = np.linalg.norm(closest - point, axis=1)
            local_id = int(np.argmin(candidate_distance))
            distances[index] = float(candidate_distance[local_id])
            nearest_faces[index] = int(face_ids[local_id])
            nearest_points[index] = closest[local_id]
        return distances, nearest_faces, nearest_points

    def distance(self, points: np.ndarray, nearest_vertices: int = 14) -> np.ndarray:
        return self.closest(points, nearest_vertices=nearest_vertices)[0]


def mesh_pair_intersection_screen(
    moving: trimesh.Trimesh,
    fixed: trimesh.Trimesh,
    maximum_candidate_pairs: int = 180_000,
) -> dict[str, Any]:
    triangles_a = np.asarray(moving.triangles, dtype=float)
    triangles_b = np.asarray(fixed.triangles, dtype=float)
    if len(triangles_a) == 0 or len(triangles_b) == 0:
        return {"candidate_pair_count": 0, "checked_pair_count": 0, "intersection_count": 0, "complete": True, "pass": True}
    cent_a = triangles_a.mean(axis=1)
    cent_b = triangles_b.mean(axis=1)
    rad_a = np.linalg.norm(triangles_a - cent_a[:, None, :], axis=2).max(axis=1)
    rad_b = np.linalg.norm(triangles_b - cent_b[:, None, :], axis=2).max(axis=1)
    max_b = float(rad_b.max())
    tree = cKDTree(cent_b)
    left: list[int] = []
    right: list[int] = []
    for i, centre in enumerate(cent_a):
        candidates = tree.query_ball_point(centre, float(rad_a[i] + max_b + 1e-12))
        if not candidates:
            continue
        candidate_array = np.asarray(candidates, dtype=np.int64)
        separation = np.linalg.norm(cent_b[candidate_array] - centre, axis=1)
        candidate_array = candidate_array[separation <= rad_a[i] + rad_b[candidate_array] + 1e-12]
        if len(candidate_array):
            left.extend([i] * len(candidate_array))
            right.extend(candidate_array.tolist())
    if not left:
        return {"candidate_pair_count": 0, "checked_pair_count": 0, "intersection_count": 0, "complete": True, "pass": True}
    left_a = np.asarray(left, dtype=np.int64)
    right_b = np.asarray(right, dtype=np.int64)
    amin, amax = triangles_a.min(axis=1), triangles_a.max(axis=1)
    bmin, bmax = triangles_b.min(axis=1), triangles_b.max(axis=1)
    overlap = np.all((amax[left_a] >= bmin[right_b] - 1e-12) & (bmax[right_b] >= amin[left_a] - 1e-12), axis=1)
    left_a, right_b = left_a[overlap], right_b[overlap]
    candidate_count = int(len(left_a))
    complete = candidate_count <= maximum_candidate_pairs
    if not complete:
        select = np.linspace(0, candidate_count - 1, maximum_candidate_pairs).astype(int)
        left_a, right_b = left_a[select], right_b[select]
    characteristic = max(float(np.max(moving.extents)), float(np.max(fixed.extents)), 1.0)
    hit = base._triangle_pair_intersections(triangles_a[left_a], triangles_b[right_b], characteristic * 1e-10)
    intersections = np.flatnonzero(hit)
    return {
        "candidate_pair_count": candidate_count,
        "checked_pair_count": int(len(left_a)),
        "intersection_count": int(len(intersections)),
        "complete": bool(complete),
        "pairs": [[int(left_a[i]), int(right_b[i])] for i in intersections[:24]],
        "pass": bool(complete and len(intersections) == 0),
    }


def distance_to_axis(points: np.ndarray, axis_point: Sequence[float], axis: Sequence[float]) -> np.ndarray:
    p = np.asarray(points, dtype=float)
    a = np.asarray(axis_point, dtype=float)
    d = _unit(axis)
    return np.linalg.norm(np.cross(p - a, d), axis=1)


def _mesh_without_axis_zone(
    mesh: trimesh.Trimesh,
    axis_point: Sequence[float],
    axis: Sequence[float],
    radius: float,
) -> trimesh.Trimesh:
    centroids = np.asarray(mesh.triangles_center, dtype=float)
    keep = distance_to_axis(centroids, axis_point, axis) >= float(radius)
    if not np.any(keep):
        return trimesh.Trimesh(vertices=np.empty((0, 3)), faces=np.empty((0, 3), dtype=np.int64), process=False)
    result = mesh.submesh([np.flatnonzero(keep)], append=True, repair=False)
    result.remove_unreferenced_vertices()
    return result


def validate_closure_sweeps(system: ClosureSystemModel) -> dict[str, Any]:
    fixed_index = SurfaceDistanceIndex(system.fixed_body_mesh)
    records: list[dict[str, Any]] = []
    all_pass = True
    for name, hinge in system.hinges.items():
        source = system.closure_meshes[name]
        states = np.linspace(0.10, 1.0, 7)
        state_records: list[dict[str, Any]] = []
        for state in states:
            posed = transformed_mesh(source, hinge.transform(float(state)))
            vertices = np.asarray(posed.vertices, dtype=float)
            hinge_zone = 0.120 if "door" in name else 0.075
            keep = distance_to_axis(vertices, hinge.point, hinge.axis) >= hinge_zone
            sample_source = vertices[keep]
            sample = sample_source[::max(1, len(sample_source) // 450)]
            distances = fixed_index.distance(sample)
            posed_for_screen = _mesh_without_axis_zone(posed, hinge.point, hinge.axis, hinge_zone)
            fixed_for_screen = _mesh_without_axis_zone(system.fixed_body_mesh, hinge.point, hinge.axis, hinge_zone * 0.72)
            intersection = mesh_pair_intersection_screen(posed_for_screen, fixed_for_screen)
            min_distance = float(np.min(distances)) if len(distances) else float("inf")
            passed = bool(intersection["pass"] and min_distance >= -1e-9)
            all_pass &= passed
            state_records.append({
                "state": float(state),
                "angle_deg": float(state * hinge.maximum_angle_deg),
                "minimum_non_hinge_distance_m": min_distance,
                "intersection": intersection,
                "pass": passed,
            })
        records.append({
            "closure": name,
            "joint_type": hinge.joint_type,
            "axis_point_m": list(hinge.point),
            "axis": list(hinge.axis),
            "maximum_angle_deg": hinge.maximum_angle_deg,
            "states": state_records,
            "pass": bool(all(item["pass"] for item in state_records)),
        })
    return {
        "schema": "MCSMv2.2.IndependentClosureSweepValidation.v1",
        "closures": records,
        "pass": bool(all_pass),
        "basis": "transformed closure triangles versus independent fixed-body aperture surface",
    }


def _door_cavity_bounds(door: trimesh.Trimesh, side: str, depth: float = 0.135) -> tuple[np.ndarray, np.ndarray]:
    """Conservative door-local cavity envelope derived from the closure bounds.

    The semantic door patch is a surface, not a manufactured inner/outer shell,
    so the cavity is represented by the closure's complete longitudinal and
    vertical extent plus a modest centreward allowance.  This is explicitly a
    concept packaging gate, not a stamped inner-panel claim.
    """
    bounds = np.asarray(door.bounds, dtype=float)
    lower = bounds[0] - np.asarray([0.030, 0.025, 0.030])
    upper = bounds[1] + np.asarray([0.030, 0.025, 0.030])
    centreward_allowance = 0.085
    if side == "left":
        upper[1] += centreward_allowance
    else:
        lower[1] -= centreward_allowance
    return lower, upper


def validate_glass_motion(system: ClosureSystemModel) -> dict[str, Any]:
    fixed_index = SurfaceDistanceIndex(system.fixed_body_mesh)
    records: list[dict[str, Any]] = []
    all_pass = True
    for glass_name, glass_system in system.glass_systems.items():
        glass = system.aperture_meshes[glass_name]
        door = system.closure_meshes[glass_system.parent_closure]
        cavity_min, cavity_max = _door_cavity_bounds(door, glass_system.side)
        state_records: list[dict[str, Any]] = []
        for state in np.linspace(0.0, 1.0, 6):
            local = glass_system.local_transform(float(state))
            posed = transformed_mesh(glass, local)
            vertices = np.asarray(posed.vertices, dtype=float)
            if state <= 1e-9:
                support_error = float(np.max(np.linalg.norm(vertices - np.asarray(glass.vertices), axis=1)))
                cavity_fraction = 1.0
            else:
                inside = np.all((vertices >= cavity_min - 0.020) & (vertices <= cavity_max + 0.020), axis=1)
                cavity_fraction = float(np.mean(inside))
                support_error = 0.0
            sample = vertices[::max(1, len(vertices) // 320)]
            fixed_distance = fixed_index.distance(sample)
            intersection = mesh_pair_intersection_screen(posed, system.fixed_body_mesh)
            passed = bool(
                (state > 0.0 or support_error <= 1e-9)
                and cavity_fraction >= (0.90 if state >= 0.4 else 0.82)
                and intersection["pass"]
                and float(np.min(fixed_distance)) >= 0.0002
            )
            all_pass &= passed
            state_records.append({
                "state": float(state),
                "vertical_drop_m": float(glass_system.travel_m * state),
                "rotation_deg": float(glass_system.rotation_deg * state),
                "cavity_containment_fraction": cavity_fraction,
                "minimum_fixed_body_distance_m": float(np.min(fixed_distance)),
                "support_pose_error_m": support_error,
                "intersection": intersection,
                "pass": passed,
            })
        # Parent-child composition is tested at one nontrivial combined state.
        combined = system.glass_transform(glass_name, 0.55, 0.60)
        determinant = float(np.linalg.det(combined[:3, :3]))
        records.append({
            "glass": glass_name,
            "parent_closure": glass_system.parent_closure,
            "motion_type": "helical_barrel_approximation",
            "states": state_records,
            "combined_parent_child_rotation_determinant": determinant,
            "pass": bool(all(item["pass"] for item in state_records) and abs(determinant - 1.0) <= 1e-8),
        })
    return {
        "schema": "MCSMv2.2.HelicalGlassMotionValidation.v1",
        "glass_systems": records,
        "pass": bool(all_pass and all(item["pass"] for item in records)),
        "assurance_note": "Guide motion is parametric and cavity-based; no regulator hardware or production glass-drop study is claimed.",
    }


# =============================================================================
# Scenes for partitions, suspension and closure states
# =============================================================================


def build_partition_scene(surface: SemanticUVSurface, partition: SurfacePartition) -> trimesh.Scene:
    scene = trimesh.Scene()
    for name, mesh in partition.panel_meshes.items():
        scene_add(scene, f"panel_{name}", mesh)
    for name, mesh in partition.aperture_meshes.items():
        scene_add(scene, f"aperture_{name}", mesh)
    return scene


def sphere_at(point: Sequence[float], radius: float, rgb: tuple[int, int, int]) -> trimesh.Trimesh:
    mesh = trimesh.creation.icosphere(subdivisions=2, radius=radius)
    mesh.apply_translation(point)
    return set_mesh_color(mesh, "Hardpoint", rgb, 1.0, 0.35, 0.32)


def cylinder_between(a: Sequence[float], b: Sequence[float], radius: float, rgb: tuple[int, int, int]) -> trimesh.Trimesh:
    a = np.asarray(a, dtype=float)
    b = np.asarray(b, dtype=float)
    vector = b - a
    length = float(np.linalg.norm(vector))
    if length <= 1e-9:
        return sphere_at(a, radius, rgb)
    mesh = trimesh.creation.cylinder(radius=radius, height=length, sections=12)
    direction = vector / length
    transform = trimesh.geometry.align_vectors([0, 0, 1], direction)
    if transform is None:
        transform = np.eye(4)
    transform[:3, 3] = 0.5 * (a + b)
    mesh.apply_transform(transform)
    return set_mesh_color(mesh, "SuspensionLink", rgb, 1.0, 0.55, 0.30)


def build_suspension_scene(
    model: base.ModernCarModelV2,
    suspension: SuspensionKinematicModel,
    tyre_sweeps: TyreSweepResult,
) -> trimesh.Scene:
    scene = trimesh.Scene()
    role_map = {item.name: item for item in suspension.hardpoints}
    for item in suspension.hardpoints:
        scene_add(scene, f"hardpoint_{item.name}", sphere_at(item.position, 0.022, (245, 195, 25)))
    link_roles = {
        "front": [
            ("lower_arm_front_inner", "ball_joint"),
            ("lower_arm_rear_inner", "ball_joint"),
            ("strut_top", "ball_joint"),
            ("tie_rod_inner", "tie_rod_outer"),
        ],
        "rear": [
            ("upper_link_inner", "upper_link_outer"),
            ("lower_link_front_inner", "lower_link_outer"),
            ("lower_link_rear_inner", "lower_link_outer"),
            ("toe_link_inner", "toe_link_outer"),
            ("damper_top", "lower_link_outer"),
        ],
    }
    for axle in ("front", "rear"):
        for side in ("left", "right"):
            for role_a, role_b in link_roles[axle]:
                a = role_map[f"{axle}_{side}_{role_a}"].position
                b = role_map[f"{axle}_{side}_{role_b}"].position
                scene_add(scene, f"link_{axle}_{side}_{role_a}_{role_b}", cylinder_between(a, b, 0.012, (70, 78, 88)))
            scene_add(scene, f"tyre_sweep_{axle}_{side}", tyre_sweeps.sweep_hulls[f"{axle}_{side}"])
    return scene


def closure_pose_scene(
    body: trimesh.Trimesh,
    partition: SurfacePartition,
    system: ClosureSystemModel,
    closure_states: Mapping[str, float],
    glass_states: Mapping[str, float],
    include_fixed_body: bool = True,
) -> trimesh.Scene:
    scene = trimesh.Scene()
    if include_fixed_body:
        scene_add(scene, "fixed_body", partition.fixed_body_mesh)
    # Non-closure panels provide context around the open apertures.
    for name, mesh in partition.panel_meshes.items():
        if name not in CLOSURE_NAMES and name != "fixed_body":
            scene_add(scene, f"fixed_panel_{name}", mesh)
    for name, mesh in system.closure_meshes.items():
        transform = system.closure_transform(name, closure_states.get(name, 0.0))
        scene_add(scene, f"closure_{name}", mesh, transform)
    # Fixed glass.
    for name in ("windshield",):
        if name in system.aperture_meshes:
            scene_add(scene, f"glass_{name}", system.aperture_meshes[name])
    # Hatch glass is a child of the rear hatch.
    if "rear_glass" in system.aperture_meshes:
        scene_add(
            scene,
            "glass_rear_glass",
            system.aperture_meshes["rear_glass"],
            system.closure_transform("rear_hatch", closure_states.get("rear_hatch", 0.0)),
        )
    for name, glass_system in system.glass_systems.items():
        transform = system.glass_transform(
            name,
            closure_states.get(glass_system.parent_closure, 0.0),
            glass_states.get(name, 0.0),
        )
        scene_add(scene, f"glass_{name}", system.aperture_meshes[name], transform)
    return scene


# =============================================================================
# MCSMv2.2 validation and assurance
# =============================================================================


def _assurance_v3(base_validation: Mapping[str, Any], checks: Mapping[str, Any]) -> dict[str, Any]:
    v0 = bool(base_validation["assurance"]["levels"]["V0"]["pass"])
    v1 = bool(base_validation["assurance"]["levels"]["V1"]["pass"])
    # MCSMv2.2 deliberately uses an open, registered semantic exterior surface.
    # The v2.0.1 compatibility validator expects a closed semantic volume, so
    # its aggregate V2 flag is not reused here.  Instead, reconstruct the V2
    # concept-geometry gate from the independent package/section/curve/BIW
    # evidence plus the v2.1 projection, alignment and partition checks.
    v2 = bool(
        v1
        and base_validation.get("package_pass", False)
        and base_validation.get("semantic_section_continuous_order", {}).get("pass", False)
        and base_validation.get("curve_network_intersection", {}).get("pass", False)
        and base_validation.get("body_in_white_connected", False)
        and checks["surface_projection"]["pass"]
        and checks["surface_partition"]["pass"]
        and checks["surface_alignment"]["pass"]
    )
    v3 = bool(
        v2
        and checks["suspension_hardpoints"]["pass"]
        and checks["actual_tyre_sweeps"]["pass"]
        and checks["closure_sweeps"]["pass"]
        and checks["glass_motion"]["pass"]
    )
    achieved = "V3" if v3 else "V2" if v2 else "V1" if v1 else "V0" if v0 else "NONE"
    return {
        "schema": "MCSMv2.2.Assurance.v1",
        "achieved_level": achieved,
        "levels": {
            "V0": {"pass": v0, "meaning": "finite framed parameter model and acyclic DAG"},
            "V1": {"pass": v1, "meaning": "valid deterministic principal body mesh"},
            "V2": {"pass": v2, "meaning": "package, semantic surface, exclusive partition and scaffold alignment"},
            "V3": {"pass": v3, "meaning": "sampled concept suspension, tyre, closure and helical-glass kinematics"},
            "V4": {"pass": False, "meaning": "manufacturing and structural engineering not claimed"},
            "V5": {"pass": False, "meaning": "CFD, crash, homologation and physical validation not claimed"},
        },
        "boundary": "V3 is concept-level sampled kinematic assurance, not production mechanism approval.",
    }


def validate_mcsmv22(
    model: base.ModernCarModelV2,
    body: trimesh.Trimesh,
    semantic: SemanticUVSurface,
    scaffold: trimesh.Trimesh,
    partition: SurfacePartition,
    suspension: SuspensionKinematicModel,
    tyre_sweeps: TyreSweepResult,
    closure_system: ClosureSystemModel,
    body_evidence: Mapping[str, Any],
    vehicle_scene: trimesh.Scene,
    biw_graph: Mapping[str, Any],
) -> dict[str, Any]:
    # Reuse the independent v2.0.1 integrity core, but supply a compact panel
    # graph because v2.2 owns the authoritative semantic partition separately.
    compatibility_graph = {
        "nodes": [{"name": item["name"]} for item in partition.graph["panels"]],
        "relations": [],
    }
    base_validation = base.validate_model(
        model,
        body,
        semantic.mesh,
        body_evidence,
        vehicle_scene,
        compatibility_graph,
        biw_graph,
    )
    alignment = surface_alignment_report(semantic.mesh, scaffold)
    suspension_check = suspension.validate()
    closure_check = validate_closure_sweeps(closure_system)
    glass_check = validate_glass_motion(closure_system)
    checks = {
        "surface_projection": semantic.projection_report,
        "surface_alignment": alignment,
        "surface_partition": partition.graph,
        "suspension_hardpoints": suspension_check,
        "actual_tyre_sweeps": tyre_sweeps.validation,
        "closure_sweeps": closure_check,
        "glass_motion": glass_check,
    }
    assurance = _assurance_v3(base_validation, checks)
    release_gate = bool(assurance["levels"]["V3"]["pass"])
    result = dict(base_validation)
    result.update({
        "version": MCSM_VERSION,
        "model_schema": MODEL_SCHEMA,
        "semantic_uv_surface": {
            "vertex_count": int(len(semantic.mesh.vertices)),
            "face_count": int(len(semantic.mesh.faces)),
            "open_surface_integrity": open_surface_integrity(semantic.mesh),
            "projection": semantic.projection_report,
        },
        "implicit_outer_scaffold_integrity": base.mesh_integrity_report(scaffold, self_intersection=True),
        "semantic_surface_implicit_alignment": alignment,
        "surface_partition": partition.graph,
        "suspension_hardpoint_model": suspension_check,
        "actual_tyre_pose_sweeps": tyre_sweeps.validation,
        "closure_kinematics": closure_check,
        "helical_glass_kinematics": glass_check,
        "assurance": assurance,
        "release_gate_pass": release_gate,
        "pass": release_gate,
    })
    return result


# =============================================================================
# Preview generation
# =============================================================================


def _font(size: int, bold: bool = False) -> ImageFont.ImageFont:
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    ]
    for candidate in candidates:
        if Path(candidate).exists():
            return ImageFont.truetype(candidate, size=size)
    return ImageFont.load_default()


def make_dashboard(
    preview_paths: Mapping[str, Path],
    validation: Mapping[str, Any],
    output: Path,
) -> None:
    canvas = Image.new("RGB", (2400, 1600), "white")
    draw = ImageDraw.Draw(canvas)
    draw.rectangle((0, 0, 2400, 120), fill=(18, 30, 54))
    draw.text((55, 30), "MCSMv2.2 · SEMANTIC KINEMATICS DASHBOARD", fill="white", font=_font(44, True))
    positions = {
        "partition": (40, 160, 1160, 870),
        "open": (1240, 160, 2360, 870),
        "suspension": (40, 900, 1160, 1510),
    }
    for key, box in positions.items():
        if key not in preview_paths or not preview_paths[key].exists():
            continue
        image = Image.open(preview_paths[key]).convert("RGB")
        x0, y0, x1, y1 = box
        image.thumbnail((x1 - x0, y1 - y0), Image.Resampling.LANCZOS)
        canvas.paste(image, (x0 + (x1 - x0 - image.width) // 2, y0 + (y1 - y0 - image.height) // 2))
        draw.rectangle(box, outline=(82, 91, 108), width=3)
    x0, y0 = 1240, 920
    draw.rounded_rectangle((x0, y0, 2360, 1510), radius=22, fill=(244, 247, 252), outline=(70, 84, 110), width=3)
    lines = [
        f"Assurance achieved: {validation['assurance']['achieved_level']}",
        f"Release gate: {validation['release_gate_pass']}",
        f"UV coverage: {validation['surface_partition']['coverage_fraction']*100:.1f}%",
        f"Apertures: {len(validation['surface_partition']['apertures'])}",
        f"Hardpoints: {validation['suspension_hardpoint_model']['hardpoint_count']}",
        f"Tyre poses: {validation['actual_tyre_pose_sweeps']['pose_count']}",
        f"Closure sweep: {validation['closure_kinematics']['pass']}",
        f"Helical glass: {validation['helical_glass_kinematics']['pass']}",
        f"Alignment p95: {validation['semantic_surface_implicit_alignment']['combined_p95_m']*1000:.2f} mm",
    ]
    draw.text((x0 + 40, y0 + 32), "INTEGRITY & KINEMATIC EVIDENCE", fill=(25, 38, 62), font=_font(30, True))
    for index, line in enumerate(lines):
        draw.text((x0 + 48, y0 + 100 + index * 48), line, fill=(45, 53, 67), font=_font(25))
    draw.text((50, 1540), "Concept-level geometry and sampled kinematics only. No production suspension, tooling, CFD, crash or homologation claim.", fill=(78, 78, 84), font=_font(22))
    canvas.save(output)


# =============================================================================
# Grammar and documentation
# =============================================================================


def write_target_grammar(path: Path, variants: Mapping[str, base.VehicleVariantV2]) -> None:
    variant_rules = []
    for key, variant in variants.items():
        variant_rules.append(f"""
MCSMv2_{key}() ->
    ModernCarV22(
        name(MCSMv2_{key})
        package(
            length({variant.length:.6f}) width({variant.width:.6f}) height({variant.height:.6f})
            wheelbase({variant.wheelbase:.6f})
            trackFront({variant.track_front:.6f}) trackRear({variant.track_rear:.6f})
            groundClearance({variant.ground_clearance:.6f})
        )
        wheelSystem(
            radius({variant.wheel_radius:.6f}) width({variant.wheel_width:.6f})
            steerRange({variant.wheel_suspension.steer_min_deg:.3f} {variant.wheel_suspension.steer_max_deg:.3f})
            travelRange({variant.wheel_suspension.travel_min:.6f} {variant.wheel_suspension.travel_max:.6f})
        )
        closureSystem(
            frontDoor({variant.closures.front_door_max_deg:.3f})
            rearDoor({variant.closures.rear_door_max_deg:.3f})
            bonnet({variant.closures.bonnet_max_deg:.3f})
            hatch({variant.closures.hatch_max_deg:.3f})
            glassTravel({variant.closures.side_glass_travel:.6f})
        )
        output(MCSMv2_{key})
    )
""")
    path.write_text(f"""# ============================================================================
# MCSMv2.2 MODERN-CAR FAMILY
# FGKv1/MVPv2.5 target grammar generated by the MCSMv2.2 Python reference.
#
# Implemented in the reference generator:
#   PersistentUVSemanticSurface
#   ImplicitOuterScaffoldProjection
#   ExclusiveSurfaceDomainPartition
#   ExplicitGlassApertureDomains
#   SuspensionHardpointGraph
#   WheelPoseSolver
#   ActualTyrePoseSweep
#   ClosureHingeSystem
#   HelicalSideGlassMotion
#   IndependentKinematicCollisionValidation
# ============================================================================

Start ->
    MCSMv2Family()

MCSMv2Family() ->
    MCSMv2_reference()
    MCSMv2_track()
    MCSMv2_aero()
    MCSMv2_crossover()

ModernCarV22(name package wheelSystem closureSystem output) ->
    VehicleReferenceFrame(x(Front) y(Right) z(Up) handedness(Right) units(m))
    ParameterDependencyGraph(package wheelSystem closureSystem)

    SemanticSectionField(output(name.Sections))
    CharacterCurveNetwork(source(name.Sections) output(name.Curves))

    PersistentUVSemanticSurface(
        sections(name.Sections)
        curves(name.Curves)
        coordinates(u(Rear Front) v(Circumference))
        output(name.UVSurface)
    )

    ImplicitOuterScaffold(
        source(name.Sections)
        wheelhouses(false)
        output(name.OuterScaffold)
    )

    ProjectSurfaceToScaffold(
        surface(name.UVSurface)
        scaffold(name.OuterScaffold)
        method(BracketedRadial)
        output(name.AuthoritativeSurface)
    )

    SurfaceDomainPartition(
        source(name.AuthoritativeSurface)
        panels(
            Hood Roof FrontDoors RearDoors FrontFenders RearQuarters
            RearHatch FrontBumper RearBumper Rockers Underbody FixedBody
        )
        apertures(
            Windshield RearGlass FrontSideGlassL FrontSideGlassR
            RearSideGlassL RearSideGlassR
        )
        ownership(Exclusive)
        output(name.Partition)
    )

    SuspensionHardpointGraph(
        package(package)
        front(MacPherson)
        rear(MultiLink)
        output(name.Suspension)
    )

    WheelPoseSolver(
        suspension(name.Suspension)
        wheelSystem(wheelSystem)
        output(name.WheelPoses)
    )

    ActualTyrePoseSweep(
        poses(name.WheelPoses)
        tyre(EllipticalSectionTorus)
        envelope(ConvexHull)
        output(name.TyreSweeps)
    )

    ClosureKinematicGraph(
        partition(name.Partition)
        doors(PairedVerticalRevolute)
        bonnet(RisingFourBarApproximation)
        hatch(RoofHeaderRevolute)
        output(name.Closures)
    )

    HelicalSideGlassMotion(
        closures(name.Closures)
        glassApertures(name.Partition)
        output(name.GlassMotion)
    )

    ValidateMCSMv22(name)

{''.join(variant_rules)}

ValidateMCSMv22(name) ->
    RequireAcyclicParameterGraph(name)
    RequireWatertightBody(name)
    RequireSurfaceProjection(name tolerance(0.000001))
    RequireBidirectionalSurfaceAlignment(name tolerance(0.012))
    RequireExclusiveSurfaceOwnership(name coverage(1.0))
    RequireSuspensionHardpointSymmetry(name)
    RequireActualTyreSweepClearance(name)
    RequireNoCollisionDuringClosureSweeps(name)
    RequireHelicalGlassCavityContainment(name)
    RequireDeterministicMeshHash(name)
    CommitOnlyIfValid()
""", encoding="utf-8")


def write_math_model(path: Path) -> None:
    path.write_text(r"""# MCSMv2.2 Mathematical Model

## Representation

\[
\mathcal M_{2.2}=(P,G,\Sigma,C,S_{UV},S_I,D,H,W,K_g,V)
\]

- \(P\): package and platform parameters;
- \(G\): typed dependency graph;
- \(\Sigma\): ordered semantic sections;
- \(C\): character curves;
- \(S_{UV}\): persistent UV semantic surface;
- \(S_I\): independently tessellated implicit outer scaffold;
- \(D\): exclusive panel/aperture domain partition;
- \(H\): suspension hardpoints and wheel-pose solver;
- \(W\): actual tyre mesh sweeps;
- \(K_g\): closure and helical-glass kinematics;
- \(V\): independent validation evidence.

## Semantic projection

For semantic point \(p\), section interior centre \(c\), and ray \(d=p-c\),
MCSMv2.2 finds \(t\) with a bracketed solve:

\[
F_o(c+t d)=0.
\]

The projected point retains the original \((u,v)\) identity.

## Surface partition

Every face receives exactly one base panel owner and optionally one aperture
owner. Apertures override base panel ownership in the resolved ledger:

\[
O(f)=\begin{cases}
A(f),&A(f)\neq\varnothing\\
P(f),&\text{otherwise.}
\end{cases}
\]

## Wheel pose

For steer \(\delta\) and suspension travel \(j\), each wheel receives a finite
rigid transform with derived centre, camber and toe. A real elliptical-section
tyre mesh is transformed through the sampled pose grid. The sweep envelope is
the convex hull of all transformed tyre vertices.

## Closure transform

A revolute closure uses

\[
T(q)=T(p)R_{\hat a}(q\theta_{max})T(-p),\qquad q\in[0,1].
\]

Bonnet and hatch systems add a small evidence-labelled rise translation.

## Helical side glass

Door-local glass motion combines downward travel, inward motion, longitudinal
shift and barrel-following rotation:

\[
T_g(q)=T(\Delta xq,\Delta yq,-hq)R_x(\phi q).
\]

The world transform is the parent-door transform multiplied by \(T_g\).

## Assurance

V3 in this release means sampled concept kinematics passed against independent
triangle and cavity checks. It is not production suspension, closure, regulator
or homologation approval.
""", encoding="utf-8")


def write_docs(output: Path, validations: Mapping[str, Any]) -> None:
    rows = []
    for result in validations.values():
        rows.append(
            f"| {result['label']} | {result['assurance']['achieved_level']} | "
            f"{result['release_gate_pass']} | {result['surface_partition']['coverage_fraction']*100:.1f}% | "
            f"{result['actual_tyre_pose_sweeps']['pose_count']} | "
            f"{result['closure_kinematics']['pass']} | {result['helical_glass_kinematics']['pass']} |"
        )
    (output / "MCSMv2_IMPLEMENTATION_REPORT.md").write_text(f"""# MCSMv2.2 Implementation Report

MCSMv2.2 implements the concept-kinematics slice over the MCSMv2.0.1 integrity
kernel and carries forward the MCSMv2.1 semantic-surface architecture.

## Delivered

1. persistent UV semantic surface;
2. radial convergence to an implicit outer scaffold;
3. exclusive panel and glass-aperture ownership;
4. explicit fixed body and movable closure surfaces;
5. concept front MacPherson and rear multi-link hardpoints;
6. wheel pose transforms over steer and travel;
7. actual tyre mesh pose sweeps and convex-hull envelopes;
8. four door, bonnet and hatch hinge systems;
9. parent-relative helical side-glass motion;
10. independent triangle, distance and cavity checks;
11. V3 concept-kinematic assurance boundary.

## Results

| Variant | Assurance | Release gate | UV coverage | Tyre poses | Closures | Glass |
|---|---:|---:|---:|---:|---:|---:|
{chr(10).join(rows)}

## Boundary

This release does not claim measured suspension hardpoints, production closure
hinges, regulator hardware, Class-A approval, stamping, structural FEA, CFD,
crash, homologation or physical validation.
""", encoding="utf-8")

    (output / "KNOWN_LIMITATIONS.md").write_text("""# MCSMv2.2 Known Limitations

- Suspension hardpoints are concept-derived and use gain-based wheel poses.
- Tyre sweeps are discretely sampled; convex hulls are conservative.
- Door, bonnet and hatch meshes are semantic exterior surfaces without inner panels, hems or hardware.
- Bonnet four-bar behavior is a rising-revolute approximation.
- Glass motion is a helical/barrel approximation without regulator hardware.
- Fixed-body apertures are open surface regions, not thickness-controlled manufactured apertures.
- Surface alignment is tessellation-level, not Class-A NURBS approval.
- No manufacturing, CFD, crash, ergonomic, homologation or physical-test claim is made.
""", encoding="utf-8")

    (output / "README.md").write_text("""# Modern Car MCSMv2.2

Regenerate:

```bash
python modern_car_mcsmv2.py --output generated --resolution medium
```

Use `--no-preview` to skip VTK renders.

Primary additions are persistent UV surface ownership, explicit aperture and
closure surfaces, suspension hardpoints, actual tyre pose sweeps, closure motion,
helical side-glass motion and independent sampled kinematic validation.
""", encoding="utf-8")


# =============================================================================
# Release generation
# =============================================================================


def resolution_tuple(name: str) -> tuple[int, int, int]:
    return {
        "low": (113, 65, 81),
        "medium": (143, 79, 97),
        "high": (177, 97, 121),
    }[name]


def write_station_csv(path: Path, stations: Sequence[base.SemanticStation]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(asdict(stations[0]).keys()))
        writer.writeheader()
        for station in stations:
            writer.writerow(asdict(station))


def generate(
    output: Path,
    resolution_name: str = "medium",
    previews: bool = True,
    selected: Sequence[str] | None = None,
) -> dict[str, Any]:
    variants = base.derive_v2_variants()
    keys = list(selected) if selected else list(variants)
    unknown = [key for key in keys if key not in variants]
    if unknown:
        raise ValueError(f"unknown variants: {', '.join(unknown)}")

    output.mkdir(parents=True, exist_ok=True)
    for directory in (
        "models", "partitions", "kinematics", "dependency_graphs", "curves",
        "semantic_stations", "data", "previews",
    ):
        (output / directory).mkdir(exist_ok=True)

    (output / "VERSION.json").write_text(json.dumps({
        "version": MCSM_VERSION,
        "model_schema": MODEL_SCHEMA,
        "reference_frame": base.VehicleReferenceFrame().as_dict(),
        "assurance_model": "MCSMv2.2.Assurance.v1",
    }, indent=2), encoding="utf-8")
    (output / "model_parameters.json").write_text(
        json.dumps({key: base.serialize_variant(value) for key, value in variants.items()}, indent=2),
        encoding="utf-8",
    )
    (output / "data" / "research_data.json").write_text(json.dumps({
        "vehicle_sources": legacy.RESEARCH_SOURCES,
        "method_sources": legacy.METHOD_SOURCES,
        "source_note": "Carried from MCSMv1/MCSMv2.0.1; v2.2 introduces no new external dimensional claims.",
    }, indent=2), encoding="utf-8")

    validations: dict[str, Any] = {}
    bodies: dict[str, trimesh.Trimesh] = {}
    resolution = resolution_tuple(resolution_name)
    reference_preview_paths: dict[str, Path] = {}

    for key in keys:
        variant = variants[key]
        model = base.ModernCarModelV2(variant)
        body, body_evidence = model.build_body(resolution)
        bodies[key] = body.copy()

        semantic = SemanticUVSurface.build(model)
        scaffold = build_implicit_outer_scaffold(model)
        partition = build_surface_partition(semantic)
        suspension = SuspensionKinematicModel(model)
        tyre_sweeps = build_tyre_sweeps(model, suspension, body)
        closures = build_closure_system(model, partition)

        # Base visible scene uses the robust v2.0.1 body/wheel/details path.
        vehicle_scene, _, _, _, _ = base.build_vehicle_scene(model, resolution)
        partition_scene = build_partition_scene(semantic, partition)
        suspension_scene = build_suspension_scene(model, suspension, tyre_sweeps)
        closed_scene = closure_pose_scene(
            body, partition, closures,
            closure_states={name: 0.0 for name in closures.hinges},
            glass_states={name: 0.0 for name in closures.glass_systems},
        )
        open_scene = closure_pose_scene(
            body, partition, closures,
            closure_states={
                "front_door_left": 0.78,
                "front_door_right": 0.42,
                "rear_door_left": 0.50,
                "rear_door_right": 0.72,
                "hood": 0.78,
                "rear_hatch": 0.82,
            },
            glass_states={name: 0.70 for name in closures.glass_systems},
        )

        # Engineering scene from base carries BIW and package envelopes.
        engineering_scene, biw_graph, engineering_manifest = base.build_engineering_scene(model, body, semantic.mesh)
        for name, hinge in closures.hinges.items():
            p0 = np.asarray(hinge.point) - 0.12 * np.asarray(hinge.axis)
            p1 = np.asarray(hinge.point) + 0.12 * np.asarray(hinge.axis)
            scene_add(engineering_scene, f"hinge_axis_{name}", cylinder_between(p0, p1, 0.007, (245, 205, 20)))
        for name, hull in tyre_sweeps.sweep_hulls.items():
            scene_add(engineering_scene, f"actual_tyre_sweep_{name}", hull)

        validation = validate_mcsmv22(
            model, body, semantic, scaffold, partition, suspension,
            tyre_sweeps, closures, body_evidence, vehicle_scene, biw_graph,
        )
        validations[key] = validation
        (output / "validation.json").write_text(json.dumps(validations, indent=2), encoding="utf-8")

        # Core model exports.
        export_scene(vehicle_scene, output / "models" / f"modern_car_v2_{key}.glb")
        export_scene(engineering_scene, output / "models" / f"modern_car_v2_{key}_engineering.glb")
        export_scene(partition_scene, output / "models" / f"modern_car_v2_{key}_surface_partition.glb")
        export_scene(suspension_scene, output / "models" / f"modern_car_v2_{key}_suspension_kinematics.glb")
        export_scene(closed_scene, output / "models" / f"modern_car_v2_{key}_closures_closed.glb")
        export_scene(open_scene, output / "models" / f"modern_car_v2_{key}_closures_open.glb")
        body.export(output / "models" / f"modern_car_v2_{key}_body.stl")
        semantic.mesh.export(output / "models" / f"modern_car_v2_{key}_semantic_uv_surface.stl")
        scaffold.export(output / "models" / f"modern_car_v2_{key}_implicit_scaffold.stl")

        # Structured evidence.
        write_station_csv(output / "semantic_stations" / f"{key}_semantic_stations.csv", model.sections.stations)
        (output / "dependency_graphs" / f"{key}_parameter_graph.json").write_text(
            json.dumps(model.parameter_graph.as_dict(), indent=2), encoding="utf-8"
        )
        (output / "curves" / f"{key}_curve_network.json").write_text(json.dumps({
            "schema": "MCSMv2.2.CurveNetwork.v1",
            "coordinate_system": model.reference_frame.as_dict(),
            "curves": model.sections.character_curves(samples=96),
            "sections": model.semantic_sections(count=21),
        }, indent=2), encoding="utf-8")
        (output / "partitions" / f"{key}_surface_partition.json").write_text(
            json.dumps(partition.graph, indent=2), encoding="utf-8"
        )
        (output / "kinematics" / f"{key}_suspension_hardpoints.json").write_text(json.dumps({
            "schema": "MCSMv2.2.SuspensionHardpoints.v1",
            "hardpoints": [asdict(item) for item in suspension.hardpoints],
            "validation": suspension.validate(),
        }, indent=2), encoding="utf-8")
        (output / "kinematics" / f"{key}_wheel_pose_sweeps.json").write_text(json.dumps({
            "schema": "MCSMv2.2.WheelPoseSweeps.v1",
            "poses": tyre_sweeps.pose_records,
            "validation": tyre_sweeps.validation,
        }, indent=2), encoding="utf-8")
        (output / "kinematics" / f"{key}_closure_system.json").write_text(json.dumps({
            "schema": "MCSMv2.2.ClosureKinematicGraph.v1",
            "hinges": {name: asdict(value) for name, value in closures.hinges.items()},
            "glass_systems": {name: asdict(value) for name, value in closures.glass_systems.items()},
            "closure_validation": validation["closure_kinematics"],
            "glass_validation": validation["helical_glass_kinematics"],
        }, indent=2), encoding="utf-8")
        (output / "data" / f"{key}_biw_graph.json").write_text(json.dumps(biw_graph, indent=2), encoding="utf-8")
        (output / "data" / f"{key}_engineering_manifest.json").write_text(json.dumps(engineering_manifest, indent=2), encoding="utf-8")

        if previews and legacy.vtk is not None:
            paths = {
                "partition": output / "previews" / f"mcsmv2_2_{key}_partition.png",
                "open": output / "previews" / f"mcsmv2_2_{key}_open.png",
                "suspension": output / "previews" / f"mcsmv2_2_{key}_suspension.png",
                "closed": output / "previews" / f"mcsmv2_2_{key}_closed.png",
            }
            legacy.render_scene(partition_scene, variant, paths["partition"], "iso")
            legacy.render_scene(open_scene, variant, paths["open"], "iso")
            legacy.render_scene(suspension_scene, variant, paths["suspension"], "iso")
            legacy.render_scene(closed_scene, variant, paths["closed"], "iso")
            if key == "reference":
                reference_preview_paths = paths
                make_dashboard(paths, validation, output / "previews" / "mcsmv2_2_kinematics_dashboard.png")

        # Reference-specific convenience artifacts.
        if key == "reference":
            export_scene(trimesh.Scene(semantic.mesh), output / "models" / "modern_car_v2_reference_semantic_uv_surface.glb")
            export_scene(trimesh.Scene(scaffold), output / "models" / "modern_car_v2_reference_implicit_scaffold.glb")

    # Body-only family comparison.
    comparison = trimesh.Scene()
    offsets = {
        "reference": np.asarray([0.0, -2.8, 0.0]),
        "track": np.asarray([0.0, 2.8, 0.0]),
        "aero": np.asarray([5.5, -2.8, 0.0]),
        "crossover": np.asarray([5.5, 2.8, 0.0]),
    }
    for key, mesh in bodies.items():
        scene_add(comparison, f"{key}_body", mesh, translation_matrix(offsets[key]))
    export_scene(comparison, output / "models" / "modern_car_v2_variations_comparison.glb")

    (output / "validation.json").write_text(json.dumps(validations, indent=2), encoding="utf-8")
    write_target_grammar(output / "MCSMv2_Modern_Car_Family.p3d", variants)
    write_math_model(output / "MCSMv2_MATHEMATICAL_MODEL.md")
    write_docs(output, validations)

    summary = [
        "# MCSMv2.2 Validation and Assurance Summary",
        "",
        "| Variant | Assurance | Gate | UV coverage | Tyre sweep | Closures | Glass |",
        "|---|---:|---:|---:|---:|---:|---:|",
    ]
    for value in validations.values():
        summary.append(
            f"| {value['label']} | {value['assurance']['achieved_level']} | "
            f"{value['release_gate_pass']} | {value['surface_partition']['coverage_fraction']*100:.1f}% | "
            f"{value['actual_tyre_pose_sweeps']['pass']} | {value['closure_kinematics']['pass']} | "
            f"{value['helical_glass_kinematics']['pass']} |"
        )
    summary.extend([
        "",
        "V3 denotes sampled concept kinematics, not production mechanism approval.",
        "V4 manufacturing/structural and V5 CFD/crash/physical assurance remain unclaimed.",
    ])
    (output / "VALIDATION_SUMMARY.md").write_text("\n".join(summary) + "\n", encoding="utf-8")

    return {
        "schema": "MCSMv2.2.GenerationSummary.v1",
        "version": MCSM_VERSION,
        "output": str(output),
        "resolution": resolution_name,
        "variants": keys,
        "validation": validations,
    }



def _merge_worker_tree(worker: Path, output: Path) -> None:
    """Merge one isolated variant worker into the release tree."""
    directory_names = (
        "models", "partitions", "kinematics", "dependency_graphs",
        "curves", "semantic_stations", "data", "previews",
    )
    for name in directory_names:
        source = worker / name
        if source.exists():
            shutil.copytree(source, output / name, dirs_exist_ok=True)
    for name in ("VERSION.json", "model_parameters.json"):
        source = worker / name
        target = output / name
        if source.exists() and not target.exists():
            shutil.copy2(source, target)


def merge_existing_workers(
    output: Path,
    workers_root: Path,
    resolution_name: str = "medium",
) -> dict[str, Any]:
    variants = base.derive_v2_variants()
    validations: dict[str, Any] = {}
    output.mkdir(parents=True, exist_ok=True)
    for key in variants:
        worker = workers_root / key
        if not (worker / "validation.json").exists():
            raise FileNotFoundError(f"missing completed worker output: {worker}")
        _merge_worker_tree(worker, output)
        record = json.loads((worker / "validation.json").read_text(encoding="utf-8"))
        validations[key] = record[key]

    comparison = trimesh.Scene()
    offsets = {
        "reference": np.asarray([0.0, -2.8, 0.0]),
        "track": np.asarray([0.0, 2.8, 0.0]),
        "aero": np.asarray([5.5, -2.8, 0.0]),
        "crossover": np.asarray([5.5, 2.8, 0.0]),
    }
    for key in variants:
        mesh = trimesh.load_mesh(output / "models" / f"modern_car_v2_{key}_body.stl", process=False)
        scene_add(comparison, f"{key}_body", mesh, translation_matrix(offsets[key]))
    export_scene(comparison, output / "models" / "modern_car_v2_variations_comparison.glb")

    (output / "validation.json").write_text(json.dumps(validations, indent=2), encoding="utf-8")
    write_target_grammar(output / "MCSMv2_Modern_Car_Family.p3d", variants)
    write_math_model(output / "MCSMv2_MATHEMATICAL_MODEL.md")
    write_docs(output, validations)

    summary_lines = [
        "# MCSMv2.2 Validation and Assurance Summary", "",
        "| Variant | Assurance | Gate | UV coverage | Tyre sweep | Closures | Glass |",
        "|---|---:|---:|---:|---:|---:|---:|",
    ]
    for value in validations.values():
        summary_lines.append(
            f"| {value['label']} | {value['assurance']['achieved_level']} | "
            f"{value['release_gate_pass']} | {value['surface_partition']['coverage_fraction']*100:.1f}% | "
            f"{value['actual_tyre_pose_sweeps']['pass']} | "
            f"{value['closure_kinematics']['pass']} | {value['helical_glass_kinematics']['pass']} |"
        )
    summary_lines.extend(["", "V3 is sampled concept-kinematic assurance. V4-V5 are not claimed."])
    (output / "VALIDATION_SUMMARY.md").write_text("\n".join(summary_lines) + "\n", encoding="utf-8")

    logs_out = output / "worker_logs"
    logs_out.mkdir(exist_ok=True)
    for log in workers_root.glob("*.log"):
        shutil.copy2(log, logs_out / log.name)
    shutil.rmtree(workers_root)
    return {
        "schema": "MCSMv2.2.GenerationSummary.v1",
        "version": MCSM_VERSION,
        "output": str(output),
        "resolution": resolution_name,
        "variants": list(variants),
        "isolated_workers": True,
        "validation": validations,
    }


def generate_isolated(
    output: Path,
    resolution_name: str = "medium",
    previews: bool = True,
) -> dict[str, Any]:
    """Generate each variant in a fresh interpreter and merge the release.

    SciPy/trimesh spatial caches can retain substantial working sets after a
    full final-mesh validation.  Process isolation keeps generation bounded and
    also makes each variant failure independently reproducible.
    """
    variants = base.derive_v2_variants()
    output.mkdir(parents=True, exist_ok=True)
    for directory in (
        "models", "partitions", "kinematics", "dependency_graphs", "curves",
        "semantic_stations", "data", "previews",
    ):
        (output / directory).mkdir(exist_ok=True)
    workers_root = output / ".variant_workers"
    if workers_root.exists():
        shutil.rmtree(workers_root)
    workers_root.mkdir(parents=True)
    validations: dict[str, Any] = {}
    source_file = Path(__file__).resolve()

    for index, key in enumerate(variants, start=1):
        worker = workers_root / key
        print(f"[MCSMv2.2] variant {index}/{len(variants)}: {key}", flush=True)
        command = [
            sys.executable,
            str(source_file),
            "--output", str(worker),
            "--resolution", resolution_name,
            "--variant", key,
            "--worker",
        ]
        if not previews:
            command.append("--no-preview")
        log_path = workers_root / f"{key}.log"
        with log_path.open("w", encoding="utf-8") as log:
            completed = subprocess.run(
                command,
                stdout=log,
                stderr=subprocess.STDOUT,
                check=False,
                env={**os.environ, "PYTHONPATH": str(source_file.parent)},
            )
        if completed.returncode != 0:
            tail = log_path.read_text(encoding="utf-8", errors="replace")[-4000:]
            raise RuntimeError(f"variant worker {key} failed ({completed.returncode})\n{tail}")
        _merge_worker_tree(worker, output)
        record = json.loads((worker / "validation.json").read_text(encoding="utf-8"))
        validations[key] = record[key]

    return merge_existing_workers(output, workers_root, resolution_name)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("MCSMv2_2_0_Generated"))
    parser.add_argument("--resolution", choices=["low", "medium", "high"], default="medium")
    parser.add_argument("--variant", action="append", choices=["reference", "track", "aero", "crossover"])
    parser.add_argument("--no-preview", action="store_true")
    parser.add_argument("--worker", action="store_true", help=argparse.SUPPRESS)
    parser.add_argument("--merge-workers", type=Path, help=argparse.SUPPRESS)
    parser.add_argument(
        "--in-process",
        action="store_true",
        help="Generate all variants in one interpreter instead of isolated workers.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    if args.merge_workers is not None:
        summary = merge_existing_workers(args.output, args.merge_workers, args.resolution)
    elif args.variant or args.worker or args.in_process:
        summary = generate(
            output=args.output,
            resolution_name=args.resolution,
            previews=not args.no_preview,
            selected=args.variant,
        )
    else:
        # Replace this already-heavy interpreter with the lightweight process
        # orchestrator.  That keeps each final-mesh validator in a fresh worker
        # and prevents retained SciPy/trimesh caches from accumulating.
        orchestrator = Path(__file__).resolve().parent / "generate_release.py"
        command = [
            sys.executable, str(orchestrator),
            "--output", str(args.output),
            "--resolution", args.resolution,
        ]
        if args.no_preview:
            command.append("--no-preview")
        os.execv(sys.executable, command)
        raise AssertionError("os.execv returned unexpectedly")
    # Avoid echoing hundreds of kilobytes of nested validation records.
    concise = {
        "schema": summary["schema"],
        "version": summary["version"],
        "output": summary["output"],
        "resolution": summary["resolution"],
        "variants": summary["variants"],
        "isolated_workers": summary.get("isolated_workers", False),
        "assurance": {
            key: value["assurance"]["achieved_level"]
            for key, value in summary["validation"].items()
        },
        "release_gates": {
            key: value["release_gate_pass"]
            for key, value in summary["validation"].items()
        },
    }
    print(json.dumps(concise, indent=2))


if __name__ == "__main__":
    main()
