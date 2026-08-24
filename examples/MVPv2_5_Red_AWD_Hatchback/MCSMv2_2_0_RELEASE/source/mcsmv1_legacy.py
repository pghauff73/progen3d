#!/usr/bin/env python3
"""MCSMv1 / MVPv2.5 parametric modern-car generator.

Creates four research-informed modern-car variants as GLB/OBJ/STL meshes,
curve/section data, engineering previews, a Progen3D target grammar, and
validation reports.

The geometry is an original mathematical construction. It is not a replica of
any cited production vehicle. Manufacturer data are used only as dimensional
and architectural reference points.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import os
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any, Iterable, Sequence

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.interpolate import PchipInterpolator
from skimage.measure import marching_cubes
import trimesh
from trimesh.visual.material import PBRMaterial

if os.environ.get("MCSM_DISABLE_VTK") == "1":
    vtk = None
    numpy_to_vtk = None
    numpy_to_vtkIdTypeArray = None
else:
    try:
        import vtk  # type: ignore
        from vtk.util.numpy_support import numpy_to_vtk, numpy_to_vtkIdTypeArray  # type: ignore
    except Exception:  # pragma: no cover - preview is optional
        vtk = None
        numpy_to_vtk = None
        numpy_to_vtkIdTypeArray = None


# ---------------------------------------------------------------------------
# Research data, gathered from primary/official sources.
# ---------------------------------------------------------------------------
RESEARCH_SOURCES: list[dict[str, Any]] = [
    {
        "id": "vw_golf_r_2025",
        "name": "2025 Volkswagen Golf R",
        "source_title": "2025 Golf R Press Kit",
        "source_url": "https://media.vw.com/press-kits/298",
        "source_type": "official manufacturer press kit",
        "length_mm": 169.1 * 25.4,
        "width_mm": 70.4 * 25.4,
        "height_mm": 57.8 * 25.4,
        "wheelbase_mm": 103.5 * 25.4,
        "front_track_mm": None,
        "rear_track_mm": None,
        "tire": "235/35 R19",
        "drag_coefficient": 0.33,
        "frontal_area_m2": None,
        "architecture": "MQB, unitary construction, AWD hot hatch",
    },
    {
        "id": "audi_s3_sportback_2025",
        "name": "2025 Audi S3 Sportback",
        "source_title": "Audi S3 Sportback TFSI technical data, 29 Sep 2025",
        "source_url": "https://uploads.audi-mediacenter.com/system/production/car_motorizations/1380/file_en/e1d0cf6e340f19f870b1a87fb551d6882fc51db7/eTD-Audi-S3-Sportback-TFSI_250929.pdf",
        "source_type": "official manufacturer technical data PDF",
        "length_mm": (4352 + 4354) / 2,
        "width_mm": 1816,
        "height_mm": (1415 + 1466) / 2,
        "wheelbase_mm": (2618 + 2630) / 2,
        "front_track_mm": 1549,
        "rear_track_mm": 1518,
        "tire": "225/40 R18",
        "drag_coefficient": 0.34,
        "frontal_area_m2": 2.17,
        "architecture": "unitary steel, five-door, quattro AWD, McPherson front / 4-link rear",
    },
    {
        "id": "toyota_grmn_corolla_2026",
        "name": "2026 Toyota GRMN Corolla",
        "source_title": "2026 Toyota GRMN Corolla: Built for Drivers, Inspired by Motorsports",
        "source_url": "https://pressroom.toyota.com/2026-toyota-grmn-corolla-built-for-drivers-inspired-by-motorsports/",
        "source_type": "official manufacturer press release",
        "length_mm": 173.6 * 25.4,
        "width_mm": 72.8 * 25.4,
        "height_mm": 58.0 * 25.4,
        "wheelbase_mm": 103.9 * 25.4,
        "front_track_mm": 62.5 * 25.4,
        "rear_track_mm": 63.7 * 25.4,
        "tire": "245/40 ZR18",
        "drag_coefficient": None,
        "frontal_area_m2": None,
        "architecture": "GR-FOUR AWD, McPherson front / double-wishbone-type multilink rear",
    },
    {
        "id": "toyota_chr_bev_2026",
        "name": "2026 Toyota C-HR BEV",
        "source_title": "Toyota Debuts Stylish, Powerful 2026 C-HR Battery Electric Vehicle",
        "source_url": "https://pressroom.toyota.com/toyota-debuts-stylish-powerful-2026-c-hr-battery-electric-vehicle/",
        "source_type": "official manufacturer press release",
        "length_mm": 177.9 * 25.4,
        "width_mm": 73.6 * 25.4,
        "height_mm": 63.8 * 25.4,
        "wheelbase_mm": 108.3 * 25.4,
        "front_track_mm": None,
        "rear_track_mm": None,
        "tire": None,
        "drag_coefficient": None,
        "frontal_area_m2": None,
        "architecture": "dedicated BEV platform, AWD front/rear eAxles, underfloor battery",
    },
]

METHOD_SOURCES = [
    {
        "title": "SAE J1100 Motor Vehicle Dimensions",
        "url": "https://saemobilus.sae.org/standards/j1100_200509-motor-vehicle-dimensions",
        "use": "dimension conventions tied to the SAE three-dimensional reference system",
    },
    {
        "title": "SAE J182 Motor Vehicle Fiducial Marks and Three-Dimensional Reference System",
        "url": "https://saemobilus.sae.org/standards/j182_202011-motor-vehicle-fiducial-marks-three-dimensional-reference-system",
        "use": "vehicle reference frame and fiducials",
    },
    {
        "title": "Autodesk Alias NURBS / Class-A continuity documentation",
        "url": "https://help.autodesk.com/view/ALIAS/2024/ENU/?guid=GUID-366304CB-16FF-46F9-9F64-D7385358D855",
        "use": "G0/G1/G2/G3 continuity framing for future Class-A patch conversion",
    },
    {
        "title": "TUM DrivAer open automotive model",
        "url": "https://www.epc.ed.tum.de/en/aer/research-groups/automotive/drivaer/",
        "use": "open, modular automotive geometry and validation precedent",
    },
    {
        "title": "ApolloCar3D",
        "url": "https://arxiv.org/abs/1811.12222",
        "use": "semantic keypoints and deformable CAD-model fitting precedent",
    },
]

BASE_H = 1.46812


@dataclass(frozen=True)
class Variant:
    key: str
    label: str
    description: str
    package_basis: str
    length: float
    width: float
    height: float
    wheelbase: float
    track_front: float
    track_rear: float
    ground_clearance: float
    wheel_radius: float
    wheel_width: float
    front_overhang: float
    rear_overhang: float
    roof_scale: float = 1.0
    body_flare: float = 0.025
    cabin_front_factor: float = 1.0
    cabin_rear_factor: float = 1.0
    tail_taper: float = 1.0
    nose_taper: float = 1.0
    spoiler_scale: float = 1.0
    splitter_scale: float = 1.0
    grille_scale: float = 1.0
    crossover_cladding: bool = False
    battery_pack: bool = False
    exhaust_count: int = 4
    body_rgb: tuple[int, int, int] = (184, 10, 7)


def _median(values: Sequence[float]) -> float:
    return float(np.median(np.asarray(values, dtype=float)))


def derive_variants() -> dict[str, Variant]:
    hot = RESEARCH_SOURCES[:3]
    L = _median([float(r["length_mm"]) for r in hot]) / 1000.0
    W = _median([float(r["width_mm"]) for r in hot]) / 1000.0
    H = _median([float(r["height_mm"]) for r in hot]) / 1000.0
    WB = _median([float(r["wheelbase_mm"]) for r in hot]) / 1000.0
    tracks_f = [float(r["front_track_mm"]) for r in hot if r["front_track_mm"]]
    tracks_r = [float(r["rear_track_mm"]) for r in hot if r["rear_track_mm"]]
    TF = float(np.mean(tracks_f)) / 1000.0
    TR = float(np.mean(tracks_r)) / 1000.0
    remaining = L - WB
    base_front = remaining * 0.50
    base_rear = remaining - base_front

    gr = RESEARCH_SOURCES[2]
    s3 = RESEARCH_SOURCES[1]
    chr_ = RESEARCH_SOURCES[3]

    return {
        "reference": Variant(
            key="reference",
            label="Research Median AWD Hot Hatch",
            description="Median package of Golf R, S3 Sportback and GRMN Corolla with a balanced fastback hatch envelope.",
            package_basis="median of three official hot-hatch packages",
            length=L,
            width=W,
            height=H,
            wheelbase=WB,
            track_front=TF,
            track_rear=TR,
            ground_clearance=0.130,
            wheel_radius=0.340,
            wheel_width=0.235,
            front_overhang=base_front,
            rear_overhang=base_rear,
            roof_scale=1.0,
            body_flare=0.026,
            body_rgb=(190, 12, 8),
        ),
        "track": Variant(
            key="track",
            label="Track Widebody AWD",
            description="GRMN-proportioned wide-track package with stronger fender volumes, splitter and rear wing.",
            package_basis="GRMN Corolla dimensional envelope with original body-surface parameters",
            length=float(gr["length_mm"]) / 1000.0,
            width=float(gr["width_mm"]) / 1000.0,
            height=float(gr["height_mm"]) / 1000.0,
            wheelbase=float(gr["wheelbase_mm"]) / 1000.0,
            track_front=float(gr["front_track_mm"]) / 1000.0,
            track_rear=float(gr["rear_track_mm"]) / 1000.0,
            ground_clearance=0.105,
            wheel_radius=0.342,
            wheel_width=0.245,
            front_overhang=(float(gr["length_mm"]) - float(gr["wheelbase_mm"])) / 2000.0,
            rear_overhang=(float(gr["length_mm"]) - float(gr["wheelbase_mm"])) / 2000.0,
            roof_scale=0.98,
            body_flare=0.055,
            tail_taper=0.98,
            spoiler_scale=1.45,
            splitter_scale=1.35,
            grille_scale=1.12,
            body_rgb=(170, 5, 4),
        ),
        "aero": Variant(
            key="aero",
            label="Aero Fastback AWD",
            description="S3-sized low-drag-oriented variation with a lower greenhouse, longer taper and reduced frontal openings.",
            package_basis="Audi S3 Sportback package, original aerodynamic variation",
            length=float(s3["length_mm"]) / 1000.0,
            width=float(s3["width_mm"]) / 1000.0,
            height=float(s3["height_mm"]) / 1000.0,
            wheelbase=float(s3["wheelbase_mm"]) / 1000.0,
            track_front=float(s3["front_track_mm"]) / 1000.0,
            track_rear=float(s3["rear_track_mm"]) / 1000.0,
            ground_clearance=0.120,
            wheel_radius=0.333,
            wheel_width=0.225,
            front_overhang=(float(s3["length_mm"]) - float(s3["wheelbase_mm"])) / 2000.0,
            rear_overhang=(float(s3["length_mm"]) - float(s3["wheelbase_mm"])) / 2000.0,
            roof_scale=0.955,
            body_flare=0.012,
            cabin_front_factor=1.04,
            cabin_rear_factor=1.16,
            tail_taper=0.84,
            spoiler_scale=0.72,
            splitter_scale=0.88,
            grille_scale=0.72,
            body_rgb=(154, 14, 12),
        ),
        "crossover": Variant(
            key="crossover",
            label="Urban Crossover EV AWD",
            description="C-HR-sized battery-electric variation with a long wheelbase, tall cabin, raised ride height and underfloor battery.",
            package_basis="2026 Toyota C-HR BEV dimensions, original surface construction",
            length=float(chr_["length_mm"]) / 1000.0,
            width=float(chr_["width_mm"]) / 1000.0,
            height=float(chr_["height_mm"]) / 1000.0,
            wheelbase=float(chr_["wheelbase_mm"]) / 1000.0,
            track_front=1.600,
            track_rear=1.600,
            ground_clearance=0.185,
            wheel_radius=0.365,
            wheel_width=0.245,
            front_overhang=(float(chr_["length_mm"]) - float(chr_["wheelbase_mm"])) * 0.48 / 1000.0,
            rear_overhang=(float(chr_["length_mm"]) - float(chr_["wheelbase_mm"])) * 0.52 / 1000.0,
            roof_scale=1.12,
            body_flare=0.060,
            cabin_front_factor=1.02,
            cabin_rear_factor=1.08,
            tail_taper=0.93,
            spoiler_scale=0.90,
            splitter_scale=0.72,
            grille_scale=0.45,
            crossover_cladding=True,
            battery_pack=True,
            exhaust_count=0,
            body_rgb=(176, 35, 18),
        ),
    }


# ---------------------------------------------------------------------------
# Materials and mesh helpers.
# ---------------------------------------------------------------------------
def pbr(name: str, rgb: tuple[int, int, int], metallic: float, rough: float,
        alpha: int = 255) -> PBRMaterial:
    return PBRMaterial(
        name=name,
        baseColorFactor=[*rgb, alpha],
        metallicFactor=float(metallic),
        roughnessFactor=float(rough),
        alphaMode="BLEND" if alpha < 255 else "OPAQUE",
        doubleSided=alpha < 255,
    )


MAT_GLASS = pbr("TintedGlass", (12, 20, 28), 0.15, 0.10, 185)
MAT_BLACK = pbr("BlackTrim", (8, 9, 11), 0.25, 0.30)
MAT_TYRE = pbr("Tyre", (18, 18, 19), 0.0, 0.90)
MAT_RIM = pbr("ForgedWheel", (32, 35, 40), 0.80, 0.22)
MAT_METAL = pbr("MachinedMetal", (116, 120, 125), 0.85, 0.22)
MAT_BRAKE = pbr("BrakeCaliper", (210, 18, 8), 0.55, 0.32)
MAT_LIGHT = pbr("Headlamp", (210, 230, 245), 0.05, 0.06)
MAT_REARLIGHT = pbr("TailLamp", (225, 8, 8), 0.15, 0.08)
MAT_BATTERY = pbr("BatteryPack", (46, 48, 52), 0.35, 0.50)
MAT_CURVE_BLUE = pbr("CurveBlue", (20, 80, 235), 0.15, 0.35)
MAT_SECTION_YELLOW = pbr("SectionYellow", (245, 210, 20), 0.10, 0.40)


def set_preview(mesh: trimesh.Trimesh, rgb: tuple[float, float, float],
                opacity: float = 1.0, metallic: float = 0.0,
                roughness: float = 0.5) -> trimesh.Trimesh:
    mesh.metadata["preview_color"] = list(rgb)
    mesh.metadata["preview_opacity"] = float(opacity)
    mesh.metadata["preview_metallic"] = float(metallic)
    mesh.metadata["preview_roughness"] = float(roughness)
    return mesh


def make_box(extents: Sequence[float], center: Sequence[float], material: PBRMaterial,
             preview_rgb: tuple[float, float, float], rotation: np.ndarray | None = None,
             opacity: float = 1.0) -> trimesh.Trimesh:
    mesh = trimesh.creation.box(extents=np.asarray(extents, dtype=float))
    if rotation is not None:
        mesh.apply_transform(rotation)
    mesh.apply_translation(np.asarray(center, dtype=float))
    mesh.visual.material = material
    return set_preview(mesh, preview_rgb, opacity, 0.3, 0.35)


def make_ellipsoid(scale: Sequence[float], center: Sequence[float],
                   material: PBRMaterial, preview_rgb: tuple[float, float, float],
                   opacity: float = 1.0) -> trimesh.Trimesh:
    mesh = trimesh.creation.icosphere(subdivisions=2, radius=1.0)
    mesh.apply_scale(np.asarray(scale, dtype=float))
    mesh.apply_translation(np.asarray(center, dtype=float))
    mesh.visual.material = material
    return set_preview(mesh, preview_rgb, opacity, 0.25, 0.25)


def segment_cylinder(a: np.ndarray, b: np.ndarray, radius: float,
                     material: PBRMaterial, preview_rgb: tuple[float, float, float],
                     sections: int = 10) -> trimesh.Trimesh:
    direction = np.asarray(b, dtype=float) - np.asarray(a, dtype=float)
    length = float(np.linalg.norm(direction))
    if not np.isfinite(length) or length <= 1e-9:
        raise ValueError("zero-length tube segment")
    transform = trimesh.geometry.align_vectors([0.0, 0.0, 1.0], direction / length)
    transform[:3, 3] = (np.asarray(a, dtype=float) + np.asarray(b, dtype=float)) * 0.5
    mesh = trimesh.creation.cylinder(radius=radius, height=length, sections=sections,
                                     transform=transform)
    mesh.visual.material = material
    return set_preview(mesh, preview_rgb, 1.0, 0.2, 0.4)


def polyline_tube(points: Sequence[Sequence[float]], radius: float,
                  material: PBRMaterial, preview_rgb: tuple[float, float, float],
                  sections: int = 8) -> trimesh.Trimesh:
    pts = np.asarray(points, dtype=float)
    parts: list[trimesh.Trimesh] = []
    for a, b in zip(pts[:-1], pts[1:]):
        if np.linalg.norm(b - a) > 1e-8:
            parts.append(segment_cylinder(a, b, radius, material, preview_rgb, sections))
    if not parts:
        raise ValueError("polyline must contain at least two distinct points")
    mesh = trimesh.util.concatenate(parts)
    mesh.visual.material = material
    return set_preview(mesh, preview_rgb, 1.0, 0.2, 0.4)


def smooth_min(a: np.ndarray, b: np.ndarray, k: float = 7.0) -> np.ndarray:
    return -np.logaddexp(-k * a, -k * b) / k


# ---------------------------------------------------------------------------
# Core detailed mathematical body model.
# ---------------------------------------------------------------------------
class ModernCarModel:
    """Original implicit section-field model resolved to a triangle mesh."""

    def __init__(self, variant: Variant):
        self.v = variant
        self.front = variant.wheelbase / 2.0 + variant.front_overhang
        self.rear = -(variant.wheelbase / 2.0 + variant.rear_overhang)
        self.mid = (self.front + self.rear) * 0.5 + 0.03
        self.vertical_scale = variant.height / BASE_H

        # Rear-to-front station fields. PCHIP avoids overshoot while retaining
        # a smooth first derivative.
        self.station_s = np.asarray([0.00, 0.06, 0.18, 0.32, 0.50, 0.68, 0.82, 0.94, 1.00])
        tail = variant.tail_taper
        nose = variant.nose_taper
        width_ratio = np.asarray([
            0.52 * tail, 0.88 * tail, 1.04, 1.00, 0.98,
            1.00, 1.04, 0.90 * nose, 0.52 * nose,
        ])
        body_center = np.asarray([0.35, 0.44, 0.57, 0.61, 0.62, 0.61, 0.59, 0.52, 0.45]) * self.vertical_scale
        body_halfheight = np.asarray([0.22, 0.33, 0.45, 0.49, 0.50, 0.49, 0.47, 0.40, 0.30]) * self.vertical_scale

        self._body_width_ratio = PchipInterpolator(self.station_s, width_ratio)
        self._body_center = PchipInterpolator(self.station_s, body_center)
        self._body_halfheight = PchipInterpolator(self.station_s, body_halfheight)

        # Greenhouse envelope.
        self.cabin_rear = self.rear + 0.50 * variant.cabin_rear_factor
        self.cabin_front = self.front - 1.26 * variant.cabin_front_factor
        self.cabin_mid = (self.cabin_rear + self.cabin_front) * 0.52
        self.cabin_s = np.asarray([0.00, 0.12, 0.35, 0.62, 0.82, 1.00])
        cabin_width_ratio = np.asarray([0.34, 0.61, 0.75, 0.78, 0.67, 0.31])
        cabin_center = np.asarray([1.02, 1.07, 1.10, 1.11, 1.06, 0.98]) * self.vertical_scale
        cabin_halfheight = (
            np.asarray([0.20, 0.31, 0.37, 0.38, 0.31, 0.17])
            * self.vertical_scale
            * variant.roof_scale
        )
        self._cabin_width_ratio = PchipInterpolator(self.cabin_s, cabin_width_ratio)
        self._cabin_center = PchipInterpolator(self.cabin_s, cabin_center)
        self._cabin_halfheight = PchipInterpolator(self.cabin_s, cabin_halfheight)

    @staticmethod
    def _asym_norm(x: np.ndarray, rear: float, mid: float, front: float) -> np.ndarray:
        return np.where(x >= mid, (x - mid) / (front - mid), (mid - x) / (mid - rear))

    def s(self, x: np.ndarray | float) -> np.ndarray:
        return np.clip((np.asarray(x) - self.rear) / (self.front - self.rear), 0.0, 1.0)

    def sc(self, x: np.ndarray | float) -> np.ndarray:
        return np.clip((np.asarray(x) - self.cabin_rear) / (self.cabin_front - self.cabin_rear), 0.0, 1.0)

    def body_halfwidth(self, x: np.ndarray | float) -> np.ndarray:
        return self._body_width_ratio(self.s(x)) * (self.v.width / 2.0) * 0.96

    def body_center_z(self, x: np.ndarray | float) -> np.ndarray:
        return self._body_center(self.s(x))

    def body_halfheight(self, x: np.ndarray | float) -> np.ndarray:
        return self._body_halfheight(self.s(x))

    def roof_halfwidth(self, x: np.ndarray | float) -> np.ndarray:
        return self._cabin_width_ratio(self.sc(x)) * (self.v.width / 2.0) * 0.94

    def roof_center_z(self, x: np.ndarray | float) -> np.ndarray:
        return self._cabin_center(self.sc(x))

    def roof_halfheight(self, x: np.ndarray | float) -> np.ndarray:
        return self._cabin_halfheight(self.sc(x))

    def roof_top(self, x: np.ndarray | float) -> np.ndarray:
        x_arr = np.asarray(x)
        xi = np.abs(self._asym_norm(x_arr, self.cabin_rear, self.cabin_mid, self.cabin_front))
        crown = np.maximum(0.0, 1.0 - xi ** 3.2) ** (1.0 / 3.0)
        return self.roof_center_z(x_arr) + self.roof_halfheight(x_arr) * crown

    def belt_z(self, x: np.ndarray | float) -> np.ndarray:
        s = self.sc(x)
        return (0.82 + 0.13 * (1.0 - s) + 0.03 * np.sin(np.pi * s)) * self.vertical_scale

    def implicit_cabin(self, X: np.ndarray, Y: np.ndarray, Z: np.ndarray) -> np.ndarray:
        xic = self._asym_norm(X, self.cabin_rear, self.cabin_mid, self.cabin_front)
        sc = np.clip((X - self.cabin_rear) / (self.cabin_front - self.cabin_rear), 0.0, 1.0)
        a = self._cabin_width_ratio(sc) * (self.v.width / 2.0) * 0.94
        c = self._cabin_center(sc)
        b = self._cabin_halfheight(sc)
        field = (
            np.abs(xic) ** 3.6
            + (np.abs(Y) / (a + 1e-6)) ** 4.5
            + (np.abs((Z - c) / (b + 1e-6))) ** 3.0
            - 1.0
        )
        return np.where((X >= self.cabin_rear) & (X <= self.cabin_front), field, 10.0)

    def implicit_body(self, X: np.ndarray, Y: np.ndarray, Z: np.ndarray) -> np.ndarray:
        v = self.v
        xi = self._asym_norm(X, self.rear, self.mid, self.front)
        s = np.clip((X - self.rear) / (self.front - self.rear), 0.0, 1.0)
        a = self._body_width_ratio(s) * (v.width / 2.0) * 0.96
        c = self._body_center(s)
        b = self._body_halfheight(s)
        lower = (
            np.abs(xi) ** 5.2
            + (np.abs(Y) / (a + 1e-6)) ** 6.0
            + (np.abs((Z - c) / (b + 1e-6))) ** 4.2
            - 1.0
        )
        cabin = self.implicit_cabin(X, Y, Z)
        field = smooth_min(lower, cabin, 6.5)

        def fender(xw: float, rear_axle: bool) -> np.ndarray:
            longitudinal = 0.52 if rear_axle else 0.50
            vertical = 0.43 if rear_axle else 0.42
            halfwidth = (v.width / 2.0) * (1.0 + v.body_flare)
            return (
                ((X - xw) / longitudinal) ** 4
                + (np.abs(Y) / halfwidth) ** 8
                + ((Z - 0.49 * self.vertical_scale) / (vertical * self.vertical_scale)) ** 4
                - 1.0
            )

        field = smooth_min(field, fender(v.wheelbase / 2.0, False), 7.0)
        field = smooth_min(field, fender(-v.wheelbase / 2.0, True), 7.0)

        # Cylinder differences form open wheel houses.
        well_radius = v.wheel_radius + 0.055
        for xw in (v.wheelbase / 2.0, -v.wheelbase / 2.0):
            well = np.sqrt((X - xw) ** 2 + (Z - v.wheel_radius) ** 2) - well_radius
            field = np.maximum(field, -well)
        return field

    def _mesh_from_field(self, field: np.ndarray, axes: tuple[np.ndarray, np.ndarray, np.ndarray]) -> trimesh.Trimesh:
        xs, ys, zs = axes
        vertices, faces, _, _ = marching_cubes(
            field.astype(np.float32),
            level=0.0,
            spacing=(xs[1] - xs[0], ys[1] - ys[0], zs[1] - zs[0]),
        )
        vertices += np.asarray([xs[0], ys[0], zs[0]])
        return trimesh.Trimesh(vertices=vertices, faces=faces, process=True)

    def build_body(self, resolution: tuple[int, int, int]) -> trimesh.Trimesh:
        xs = np.linspace(self.rear - 0.10, self.front + 0.10, resolution[0])
        ys = np.linspace(-self.v.width * 0.60, self.v.width * 0.60, resolution[1])
        zs = np.linspace(-0.12, self.v.height + 0.08, resolution[2])
        field = self.implicit_body(xs[:, None, None], ys[None, :, None], zs[None, None, :])
        mesh = self._mesh_from_field(field, (xs, ys, zs))
        trimesh.smoothing.filter_taubin(mesh, lamb=0.45, nu=-0.48, iterations=3)

        # Enforce exact research package extents. This is a controlled affine
        # normalization after the mathematical surface is resolved.
        bounds = mesh.bounds
        extent = bounds[1] - bounds[0]
        scale = np.asarray([self.v.length, self.v.width, self.v.height]) / extent
        target_center = np.asarray([(self.front + self.rear) / 2.0, 0.0, self.v.height / 2.0])
        mesh.vertices = (mesh.vertices - bounds.mean(axis=0)) * scale + target_center

        body_material = pbr(f"Body_{self.v.key}", self.v.body_rgb, 0.68, 0.26)
        mesh.visual.material = body_material
        rgb = tuple(c / 255.0 for c in self.v.body_rgb)
        return set_preview(mesh, rgb, 1.0, 0.68, 0.26)

    def build_greenhouse_glass(self, resolution: tuple[int, int, int] = (120, 62, 62)) -> trimesh.Trimesh:
        xs = np.linspace(self.cabin_rear - 0.03, self.cabin_front + 0.03, resolution[0])
        ys = np.linspace(-self.v.width * 0.50, self.v.width * 0.50, resolution[1])
        zs = np.linspace(0.62 * self.vertical_scale, self.v.height + 0.03, resolution[2])
        field = self.implicit_cabin(xs[:, None, None], ys[None, :, None], zs[None, None, :])
        mesh = self._mesh_from_field(field, (xs, ys, zs))
        centers = mesh.triangles_center
        normals = mesh.face_normals
        belt = self.belt_z(centers[:, 0])
        roof = self.roof_top(centers[:, 0])
        mask = (
            (centers[:, 2] > belt + 0.010)
            & (centers[:, 2] < roof - 0.045)
            & (np.abs(normals[:, 2]) < 0.78)
        )
        mesh.update_faces(mask)
        mesh.remove_unreferenced_vertices()
        mesh.process(validate=True)
        mesh.vertices += mesh.vertex_normals * 0.006
        mesh.visual.material = MAT_GLASS
        return set_preview(mesh, (0.035, 0.055, 0.075), 0.58, 0.15, 0.10)

    def station_curves(self, count: int = 15) -> dict[str, list[list[float]]]:
        """Analytic longitudinal curves, independent of the tessellation."""
        xs = np.linspace(self.rear + 0.03, self.front - 0.03, count * 4)
        body_w = self.body_halfwidth(xs)
        body_c = self.body_center_z(xs)
        body_h = self.body_halfheight(xs)
        shoulder_z = body_c + body_h * 0.47
        rocker_z = np.full_like(xs, 0.20 * self.vertical_scale)
        curves = {
            "centre_spine": np.column_stack([xs, np.zeros_like(xs), body_c]).tolist(),
            "roof_centre": np.column_stack([
                np.linspace(self.cabin_rear, self.cabin_front, count * 4),
                np.zeros(count * 4),
                self.roof_top(np.linspace(self.cabin_rear, self.cabin_front, count * 4)),
            ]).tolist(),
            "shoulder_left": np.column_stack([xs, -body_w, shoulder_z]).tolist(),
            "shoulder_right": np.column_stack([xs, body_w, shoulder_z]).tolist(),
            "rocker_left": np.column_stack([xs, -body_w * 0.94, rocker_z]).tolist(),
            "rocker_right": np.column_stack([xs, body_w * 0.94, rocker_z]).tolist(),
        }
        cab_x = np.linspace(self.cabin_rear, self.cabin_front, count * 4)
        belt = self.belt_z(cab_x)
        belt_w = self.body_halfwidth(cab_x) * 0.93
        curves["belt_left"] = np.column_stack([cab_x, -belt_w, belt]).tolist()
        curves["belt_right"] = np.column_stack([cab_x, belt_w, belt]).tolist()
        return curves

    def extract_sections(self, body_mesh: trimesh.Trimesh, count: int = 17) -> list[dict[str, Any]]:
        sections: list[dict[str, Any]] = []
        xs = np.linspace(self.rear + 0.05, self.front - 0.05, count)
        for index, x in enumerate(xs):
            path = body_mesh.section(plane_origin=[x, 0.0, 0.0], plane_normal=[1.0, 0.0, 0.0])
            if path is None or not path.discrete:
                continue
            line = max(path.discrete, key=lambda p: len(p))
            sections.append({
                "index": index,
                "x_m": float(x),
                "u": float((x - self.rear) / (self.front - self.rear)),
                "points_xyz_m": np.asarray(line).round(6).tolist(),
            })
        return sections

    def projected_frontal_area(self, samples_x: int = 160, samples_y: int = 300,
                               samples_z: int = 260) -> float:
        ys = np.linspace(-self.v.width * 0.55, self.v.width * 0.55, samples_y)
        zs = np.linspace(0.0, self.v.height, samples_z)
        min_field = np.full((samples_y, samples_z), np.inf, dtype=np.float32)
        xs = np.linspace(self.rear, self.front, samples_x)
        for chunk in np.array_split(xs, 8):
            field = self.implicit_body(
                chunk[:, None, None],
                ys[None, :, None],
                zs[None, None, :],
            )
            min_field = np.minimum(min_field, field.min(axis=0).astype(np.float32))
        dy = float(ys[1] - ys[0])
        dz = float(zs[1] - zs[0])
        return float(np.count_nonzero(min_field <= 0.0) * dy * dz)


# ---------------------------------------------------------------------------
# Vehicle assemblies.
# ---------------------------------------------------------------------------
def wheel_assembly(v: Variant, x: float, y: float, z: float, name: str) -> list[tuple[str, trimesh.Trimesh]]:
    meshes: list[tuple[str, trimesh.Trimesh]] = []
    tyre_minor = 0.055 if v.key != "crossover" else 0.065
    rot = trimesh.transformations.rotation_matrix(np.pi / 2.0, [1.0, 0.0, 0.0])

    tyre = trimesh.creation.torus(
        major_radius=v.wheel_radius - tyre_minor,
        minor_radius=tyre_minor,
        major_sections=44,
        minor_sections=16,
    )
    tyre.apply_transform(rot)
    tyre.apply_scale([1.0, v.wheel_width / tyre.extents[1], 1.0])
    tyre.apply_translation([x, y, z])
    tyre.visual.material = MAT_TYRE
    set_preview(tyre, (0.05, 0.05, 0.055), 1.0, 0.0, 0.9)
    meshes.append((f"{name}_tyre", tyre))

    rim_radius = v.wheel_radius * 0.68
    rim_parts: list[trimesh.Trimesh] = []
    barrel = trimesh.creation.cylinder(radius=rim_radius, height=v.wheel_width * 0.62, sections=40)
    barrel.apply_transform(rot)
    barrel.apply_translation([x, y, z])
    rim_parts.append(barrel)
    hub = trimesh.creation.cylinder(radius=v.wheel_radius * 0.16, height=v.wheel_width * 0.72, sections=24)
    hub.apply_transform(rot)
    hub.apply_translation([x, y, z])
    rim_parts.append(hub)
    face_y = y + math.copysign(v.wheel_width * 0.33, y)
    for k in range(10):
        angle = k * math.pi / 5.0
        length = rim_radius * 0.72
        spoke = trimesh.creation.box(extents=[length, 0.025, 0.045])
        spoke.apply_transform(trimesh.transformations.rotation_matrix(-angle, [0.0, 1.0, 0.0]))
        spoke.apply_translation([
            x + math.cos(angle) * length * 0.23,
            face_y,
            z + math.sin(angle) * length * 0.23,
        ])
        rim_parts.append(spoke)
    rim = trimesh.util.concatenate(rim_parts)
    rim.visual.material = MAT_RIM
    set_preview(rim, (0.12, 0.13, 0.15), 1.0, 0.8, 0.22)
    meshes.append((f"{name}_rim", rim))

    disc = trimesh.creation.cylinder(radius=v.wheel_radius * 0.48, height=v.wheel_width * 0.68, sections=36)
    disc.apply_transform(rot)
    disc.apply_translation([x, y, z])
    disc.visual.material = MAT_METAL
    set_preview(disc, (0.50, 0.52, 0.54), 1.0, 0.85, 0.22)
    meshes.append((f"{name}_brake_disc", disc))

    caliper = make_box(
        [0.08, v.wheel_width * 0.25, 0.18],
        [x - v.wheel_radius * 0.35, y, z + 0.02],
        MAT_BRAKE,
        (0.82, 0.05, 0.025),
    )
    meshes.append((f"{name}_caliper", caliper))
    return meshes


def add_geometry(scene: trimesh.Scene, name: str, mesh: trimesh.Trimesh,
                 transform: np.ndarray | None = None) -> None:
    scene.add_geometry(mesh, geom_name=name, node_name=name, transform=transform)


def build_scene(model: ModernCarModel, resolution: tuple[int, int, int]) -> tuple[trimesh.Scene, trimesh.Trimesh]:
    v = model.v
    scene = trimesh.Scene()
    body = model.build_body(resolution)
    add_geometry(scene, "body", body)
    add_geometry(scene, "greenhouse_glass", model.build_greenhouse_glass())

    # Door/pillar station positions are retained for handle placement and
    # curve data. The glass is a continuous clipped greenhouse surface;
    # explicit pillar solids are intentionally omitted from this concept LOD.
    x_cuts = [
        model.cabin_rear + (model.cabin_front - model.cabin_rear) * 0.34,
        model.cabin_rear + (model.cabin_front - model.cabin_rear) * 0.67,
    ]

    # Wheels and brakes.
    for axle_name, x, track in (
        ("front", v.wheelbase / 2.0, v.track_front),
        ("rear", -v.wheelbase / 2.0, v.track_rear),
    ):
        for side in (-1.0, 1.0):
            y = side * track / 2.0
            label = f"{axle_name}_{'R' if side > 0 else 'L'}"
            for name, mesh in wheel_assembly(v, x, y, v.wheel_radius, label):
                add_geometry(scene, name, mesh)

    # Front and rear exterior details.
    front = model.front
    rear = model.rear
    grille = make_box(
        [0.060, v.width * 0.52 * v.grille_scale, 0.27 * v.grille_scale],
        [front - 0.015, 0.0, 0.48 * model.vertical_scale],
        MAT_BLACK,
        (0.025, 0.025, 0.028),
    )
    add_geometry(scene, "front_grille", grille)

    for side in (-1.0, 1.0):
        headlamp = make_ellipsoid(
            [0.055, v.width * 0.16, 0.070 * model.vertical_scale],
            [front - 0.020, side * v.width * 0.28, 0.76 * model.vertical_scale],
            MAT_LIGHT,
            (0.78, 0.90, 1.00),
        )
        add_geometry(scene, f"headlamp_{'R' if side > 0 else 'L'}", headlamp)
        taillamp = make_ellipsoid(
            [0.050, v.width * 0.13, 0.060 * model.vertical_scale],
            [rear + 0.020, side * v.width * 0.29, 0.80 * model.vertical_scale],
            MAT_REARLIGHT,
            (0.90, 0.03, 0.03),
        )
        add_geometry(scene, f"taillamp_{'R' if side > 0 else 'L'}", taillamp)
        mirror = make_ellipsoid(
            [0.10, 0.12, 0.055],
            [model.cabin_front - 0.05, side * v.width * 0.53, 0.98 * model.vertical_scale],
            MAT_BLACK,
            (0.02, 0.022, 0.026),
        )
        add_geometry(scene, f"mirror_{'R' if side > 0 else 'L'}", mirror)

    splitter = make_box(
        [0.18 * v.splitter_scale, v.width * 0.85, 0.045],
        [front - 0.07, 0.0, 0.13],
        MAT_BLACK,
        (0.02, 0.02, 0.023),
    )
    add_geometry(scene, "front_splitter", splitter)

    for side in (-1.0, 1.0):
        skirt = make_box(
            [v.wheelbase * 0.72, 0.055 if not v.crossover_cladding else 0.085, 0.055 if not v.crossover_cladding else 0.095],
            [0.0, side * v.width * 0.495, 0.20 if not v.crossover_cladding else 0.26],
            MAT_BLACK,
            (0.02, 0.02, 0.024),
        )
        add_geometry(scene, f"side_skirt_{'R' if side > 0 else 'L'}", skirt)

    spoiler_x = model.cabin_rear + 0.10
    spoiler_z = min(v.height - 0.025, float(model.roof_top(spoiler_x)) + 0.018)
    spoiler = make_box(
        [0.28 * v.spoiler_scale, v.width * 0.70, 0.045 * max(0.8, v.spoiler_scale)],
        [spoiler_x, 0.0, spoiler_z],
        MAT_BLACK,
        (0.02, 0.02, 0.024),
    )
    add_geometry(scene, "rear_spoiler", spoiler)

    diffuser = make_box(
        [0.16, v.width * 0.66, 0.13],
        [rear + 0.03, 0.0, 0.19],
        MAT_BLACK,
        (0.02, 0.02, 0.024),
    )
    add_geometry(scene, "rear_diffuser", diffuser)

    # Underbody and optional EV battery.
    underbody = make_box(
        [v.wheelbase * 0.78, v.width * 0.65, 0.060],
        [0.0, 0.0, v.ground_clearance],
        MAT_BLACK,
        (0.03, 0.032, 0.038),
    )
    add_geometry(scene, "underbody", underbody)
    if v.battery_pack:
        battery = make_box(
            [v.wheelbase * 0.72, v.width * 0.66, 0.14],
            [0.0, 0.0, v.ground_clearance + 0.07],
            MAT_BATTERY,
            (0.15, 0.16, 0.18),
        )
        add_geometry(scene, "battery_pack", battery)

    # Exhaust tips for ICE/hybrid variants.
    if v.exhaust_count > 0:
        per_side = max(1, v.exhaust_count // 2)
        for side in (-1.0, 1.0):
            for i in range(per_side):
                y = side * (v.width * (0.27 + i * 0.075))
                tip = trimesh.creation.cylinder(radius=0.042, height=0.13, sections=20)
                tip.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2.0, [0.0, 1.0, 0.0]))
                tip.apply_translation([rear - 0.03, y, 0.25])
                tip.visual.material = MAT_METAL
                set_preview(tip, (0.45, 0.47, 0.50), 1.0, 0.85, 0.22)
                add_geometry(scene, f"exhaust_{'R' if side > 0 else 'L'}_{i}", tip)

    # Door handles and panel-line evidence.
    for side in (-1.0, 1.0):
        for idx, x in enumerate((x_cuts[0] + 0.30, x_cuts[1] + 0.20)):
            y = side * float(model.body_halfwidth(x)) * 1.002
            handle = make_ellipsoid(
                [0.07, 0.012, 0.018],
                [x, y, 0.79 * model.vertical_scale],
                MAT_BLACK,
                (0.03, 0.03, 0.035),
            )
            add_geometry(scene, f"door_handle_{idx}_{'R' if side > 0 else 'L'}", handle)

    return scene, body


# ---------------------------------------------------------------------------
# Curve-network scene and data.
# ---------------------------------------------------------------------------
def build_curve_scene(model: ModernCarModel, curves: dict[str, list[list[float]]],
                      sections: list[dict[str, Any]]) -> trimesh.Scene:
    scene = trimesh.Scene()
    for name, points in curves.items():
        tube = polyline_tube(points, 0.006, MAT_CURVE_BLUE, (0.04, 0.20, 0.90), sections=7)
        add_geometry(scene, f"curve_{name}", tube)
    # A manageable subset of cross-sections.
    for section in sections[::2]:
        points = section["points_xyz_m"]
        if len(points) >= 2:
            tube = polyline_tube(points[::max(1, len(points) // 80)], 0.0045,
                                 MAT_SECTION_YELLOW, (0.95, 0.75, 0.05), sections=6)
            add_geometry(scene, f"section_{section['index']:02d}", tube)
    return scene


# ---------------------------------------------------------------------------
# Preview rendering.
# ---------------------------------------------------------------------------
def _mesh_world_instances(scene: trimesh.Scene) -> Iterable[tuple[str, trimesh.Trimesh]]:
    for node_name in scene.graph.nodes_geometry:
        transform, geom_name = scene.graph[node_name]
        mesh = scene.geometry[geom_name].copy()
        mesh.apply_transform(transform)
        yield geom_name, mesh


def _vtk_actor(mesh: trimesh.Trimesh) -> Any:
    if vtk is None or numpy_to_vtk is None or numpy_to_vtkIdTypeArray is None:
        raise RuntimeError("VTK preview rendering is unavailable")
    vertices = np.asarray(mesh.vertices, dtype=np.float64)
    faces = np.asarray(mesh.faces, dtype=np.int64)
    points = vtk.vtkPoints()
    points.SetData(numpy_to_vtk(vertices, deep=True))
    cells = vtk.vtkCellArray()
    legacy = np.hstack([np.full((len(faces), 1), 3, dtype=np.int64), faces]).ravel()
    cells.SetCells(len(faces), numpy_to_vtkIdTypeArray(legacy, deep=True))
    poly = vtk.vtkPolyData()
    poly.SetPoints(points)
    poly.SetPolys(cells)
    normals = vtk.vtkPolyDataNormals()
    normals.SetInputData(poly)
    normals.SetFeatureAngle(55.0)
    normals.SplittingOff()
    normals.ConsistencyOn()
    normals.AutoOrientNormalsOn()
    mapper = vtk.vtkPolyDataMapper()
    mapper.SetInputConnection(normals.GetOutputPort())
    actor = vtk.vtkActor()
    actor.SetMapper(mapper)
    color = mesh.metadata.get("preview_color", [0.65, 0.65, 0.65])
    opacity = float(mesh.metadata.get("preview_opacity", 1.0))
    prop = actor.GetProperty()
    prop.SetColor(float(color[0]), float(color[1]), float(color[2]))
    prop.SetOpacity(opacity)
    if hasattr(prop, "SetInterpolationToPBR"):
        prop.SetInterpolationToPBR()
        prop.SetMetallic(float(mesh.metadata.get("preview_metallic", 0.0)))
        prop.SetRoughness(float(mesh.metadata.get("preview_roughness", 0.5)))
    else:
        prop.SetSpecular(0.35)
        prop.SetSpecularPower(30)
    return actor


def render_scene(scene: trimesh.Scene, variant: Variant, output: Path, view: str,
                 size: tuple[int, int] = (1100, 700)) -> None:
    if vtk is None:
        return
    renderer = vtk.vtkRenderer()
    renderer.SetBackground(0.965, 0.968, 0.973)
    for _, mesh in _mesh_world_instances(scene):
        renderer.AddActor(_vtk_actor(mesh))

    # Ground plate and studio lights.
    ground = vtk.vtkPlaneSource()
    ground.SetOrigin(-variant.length * 0.65, -variant.width * 1.15, 0.0)
    ground.SetPoint1(variant.length * 0.65, -variant.width * 1.15, 0.0)
    ground.SetPoint2(-variant.length * 0.65, variant.width * 1.15, 0.0)
    gmap = vtk.vtkPolyDataMapper()
    gmap.SetInputConnection(ground.GetOutputPort())
    gactor = vtk.vtkActor()
    gactor.SetMapper(gmap)
    gactor.GetProperty().SetColor(0.80, 0.82, 0.85)
    gactor.GetProperty().SetOpacity(0.38)
    renderer.AddActor(gactor)

    key = vtk.vtkLight()
    key.SetPosition(6, -6, 7)
    key.SetFocalPoint(0, 0, 0.7)
    key.SetIntensity(1.1)
    renderer.AddLight(key)
    fill = vtk.vtkLight()
    fill.SetPosition(-4, 5, 4)
    fill.SetFocalPoint(0, 0, 0.7)
    fill.SetIntensity(0.65)
    renderer.AddLight(fill)

    window = vtk.vtkRenderWindow()
    window.SetOffScreenRendering(1)
    window.SetMultiSamples(4)
    window.AddRenderer(renderer)
    window.SetSize(*size)

    camera = renderer.GetActiveCamera()
    target = ((variant.front_overhang - variant.rear_overhang) * 0.5, 0.0, variant.height * 0.47)
    aspect = size[0] / size[1]
    if view == "front":
        camera.SetPosition(7.0, 0.0, variant.height * 0.48)
        camera.SetViewUp(0.0, 0.0, 1.0)
        scale = max(variant.height * 0.60, variant.width / (2.0 * aspect) * 1.12)
    elif view == "side":
        camera.SetPosition(0.0, -7.0, variant.height * 0.48)
        camera.SetViewUp(0.0, 0.0, 1.0)
        scale = max(variant.height * 0.62, variant.length / (2.0 * aspect) * 1.12)
    elif view == "top":
        camera.SetPosition(0.0, 0.0, 8.0)
        camera.SetViewUp(1.0, 0.0, 0.0)
        scale = max(variant.length * 0.56, variant.width / (2.0 * aspect) * 1.12)
    else:
        camera.SetPosition(6.3, -6.3, 3.3)
        camera.SetViewUp(0.0, 0.0, 1.0)
        scale = variant.length / (2.0 * aspect) * 1.18
    camera.SetFocalPoint(*target)
    camera.ParallelProjectionOn()
    camera.SetParallelScale(scale)
    renderer.ResetCameraClippingRange()
    window.Render()

    capture = vtk.vtkWindowToImageFilter()
    capture.SetInput(window)
    capture.SetInputBufferTypeToRGBA()
    capture.ReadFrontBufferOff()
    capture.Update()
    writer = vtk.vtkPNGWriter()
    writer.SetFileName(str(output))
    writer.SetInputConnection(capture.GetOutputPort())
    writer.Write()


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    candidates = [
        "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        "/usr/share/fonts/truetype/liberation2/LiberationSans-Bold.ttf" if bold else "/usr/share/fonts/truetype/liberation2/LiberationSans-Regular.ttf",
    ]
    for candidate in candidates:
        if Path(candidate).exists():
            return ImageFont.truetype(candidate, size=size)
    return ImageFont.load_default()


def contact_sheet(label: str, variant: Variant, images: dict[str, Path], output: Path) -> None:
    cells = {key: Image.open(path).convert("RGB") for key, path in images.items() if path.exists()}
    if len(cells) < 4:
        return
    cw, ch = 1100, 700
    header = 150
    canvas = Image.new("RGB", (cw * 2, ch * 2 + header), "white")
    draw = ImageDraw.Draw(canvas)
    draw.text((40, 20), label, fill=(20, 24, 32), font=font(40, True))
    dims = (
        f"L {variant.length*1000:.0f} mm   W {variant.width*1000:.0f} mm   "
        f"H {variant.height*1000:.0f} mm   WB {variant.wheelbase*1000:.0f} mm   AWD"
    )
    draw.text((42, 78), dims, fill=(65, 72, 82), font=font(26))
    positions = {"iso": (0, header), "side": (cw, header), "front": (0, header + ch), "top": (cw, header + ch)}
    labels = {"iso": "ISOMETRIC", "side": "SIDE", "front": "FRONT", "top": "TOP"}
    for key, pos in positions.items():
        im = cells[key].resize((cw, ch), Image.Resampling.LANCZOS)
        canvas.paste(im, pos)
        draw.rectangle([pos[0], pos[1], pos[0] + cw - 1, pos[1] + ch - 1], outline=(190, 195, 203), width=2)
        draw.text((pos[0] + 24, pos[1] + 18), labels[key], fill=(28, 34, 44), font=font(25, True))
    canvas.save(output, quality=95)


def variants_poster(variant_images: list[tuple[Variant, Path]], output: Path) -> None:
    cell_w, cell_h = 1200, 780
    header = 170
    canvas = Image.new("RGB", (cell_w * 2, cell_h * 2 + header), "white")
    draw = ImageDraw.Draw(canvas)
    draw.text((45, 24), "MCSMv1 RESEARCH-INFORMED MODERN CAR FAMILY", fill=(18, 22, 30), font=font(42, True))
    draw.text((47, 90), "Original parametric geometry, four package and surface variations", fill=(72, 78, 88), font=font(27))
    for index, (variant, path) in enumerate(variant_images):
        image = Image.open(path).convert("RGB").resize((cell_w, cell_h), Image.Resampling.LANCZOS)
        x = (index % 2) * cell_w
        y = header + (index // 2) * cell_h
        canvas.paste(image, (x, y))
        draw.rectangle([x, y, x + cell_w - 1, y + cell_h - 1], outline=(188, 194, 202), width=2)
        draw.text((x + 30, y + 26), variant.label, fill=(22, 27, 35), font=font(31, True))
        draw.text(
            (x + 32, y + 72),
            f"{variant.length*1000:.0f} × {variant.width*1000:.0f} × {variant.height*1000:.0f} mm  |  WB {variant.wheelbase*1000:.0f} mm",
            fill=(65, 72, 82),
            font=font(22),
        )
    canvas.save(output, quality=95)


# ---------------------------------------------------------------------------
# Progen3D target grammar.
# ---------------------------------------------------------------------------
def write_progen3d_grammar(path: Path, variants: dict[str, Variant]) -> None:
    entries = []
    for key, v in variants.items():
        entries.append(f"""
MCSM_{key}() ->
    ModernCarModel(
        name(MCSM_{key})
        package(
            length({v.length:.6f})
            width({v.width:.6f})
            height({v.height:.6f})
            wheelbase({v.wheelbase:.6f})
            trackFront({v.track_front:.6f})
            trackRear({v.track_rear:.6f})
            groundClearance({v.ground_clearance:.6f})
        )
        wheels(radius({v.wheel_radius:.6f}) width({v.wheel_width:.6f}))
        style(
            roofScale({v.roof_scale:.6f})
            flare({v.body_flare:.6f})
            cabinFrontFactor({v.cabin_front_factor:.6f})
            cabinRearFactor({v.cabin_rear_factor:.6f})
            tailTaper({v.tail_taper:.6f})
            spoilerScale({v.spoiler_scale:.6f})
            splitterScale({v.splitter_scale:.6f})
            grilleScale({v.grille_scale:.6f})
        )
        drivetrain(AWD)
        output(MCSM_{key})
    )
""")
    text = f"""# ============================================================================
# MCSMv1 / MVPv2.5 MODERN CAR FAMILY
# Research-informed target grammar generated by modern_car_parametric_model.py
#
# This is target syntax for FGKv1. It uses an implicit section field resolved
# to a zero-isosurface, separate greenhouse glass, wheel assemblies and
# analytic character curves. It is not compatible with the current parser.
# ============================================================================

Start ->
    ModernCarFamily()

ModernCarFamily() ->
    MCSM_reference()
    MCSM_track()
    MCSM_aero()
    MCSM_crossover()

ModernCarModel(name package wheels style drivetrain output) ->
    SAEVehicleReferenceFrame()
    VehiclePackage(package)

    ImplicitSectionField(
        lowerBody(
            stationInterpolation(PCHIP)
            longitudinalExponent(5.2)
            lateralExponent(6.0)
            verticalExponent(4.2)
        )

        greenhouse(
            stationInterpolation(PCHIP)
            longitudinalExponent(3.6)
            lateralExponent(4.5)
            verticalExponent(3.0)
        )

        fenders(
            front(Superellipsoid exponent(4 8 4))
            rear(Superellipsoid exponent(4 8 4))
        )

        combination(SmoothMinimum k(7.0))

        wheelhouseDifference(
            front(wheels clearance(0.055))
            rear(wheels clearance(0.055))
        )

        output(name.BodyField)
    )

    IsoSurface(
        field(name.BodyField)
        isoValue(0)
        output(name.BodyMesh)
    )

    GreenhouseGlass(
        source(name.BodyField.Greenhouse)
        clipAbove(BeltCurve)
        clipBelow(RoofCurve)
        output(name.Glass)
    )

    CharacterCurveNetwork(
        centreSpine
        roofCentre
        beltLeft beltRight
        shoulderLeft shoulderRight
        rockerLeft rockerRight
    )

    WheelAssemblyArray(package wheels drivetrain)
    ExteriorDetailSystem(style)
    ValidateModernCar(name)
{''.join(entries)}

ValidateModernCar(name) ->
    RequireFiniteGeometry(name)
    RequireSymmetry(name plane(Y 0))
    RequirePackageDimensions(name)
    RequireWheelbase(name)
    RequireTrack(name)
    RequireWheelhouseClearance(name)
    RequireWatertightBody(name)
    RequireDeterministicMeshHash(name)
    CommitOnlyIfValid()
"""
    path.write_text(text, encoding="utf-8")


# ---------------------------------------------------------------------------
# Documentation.
# ---------------------------------------------------------------------------
def write_report(path: Path, variants: dict[str, Variant], validations: dict[str, Any]) -> None:
    hot_table = []
    for r in RESEARCH_SOURCES:
        hot_table.append(
            f"| {r['name']} | {float(r['length_mm']):.0f} | {float(r['width_mm']):.0f} | "
            f"{float(r['height_mm']):.0f} | {float(r['wheelbase_mm']):.0f} | "
            f"{r['front_track_mm'] if r['front_track_mm'] is not None else 'not stated'} / "
            f"{r['rear_track_mm'] if r['rear_track_mm'] is not None else 'not stated'} | "
            f"{r['drag_coefficient'] if r['drag_coefficient'] is not None else 'not stated'} |"
        )
    variant_table = []
    for v in variants.values():
        result = validations[v.key]
        variant_table.append(
            f"| {v.label} | {v.length*1000:.0f} | {v.width*1000:.0f} | {v.height*1000:.0f} | "
            f"{v.wheelbase*1000:.0f} | {v.track_front*1000:.0f}/{v.track_rear*1000:.0f} | "
            f"{result['frontal_area_proxy_m2']:.3f} | {result['body_faces']} |"
        )
    source_notes = "\n".join(f"- [{s['title']}]({s['url']}): {s['use']}." for s in METHOD_SOURCES)
    report = f"""# MCSMv1: Research-Informed Mathematical Modern-Car Model

## Deliverable

This package contains four original parametric 3D vehicles. Manufacturer data are used as dimensional and architectural reference points only; none of the meshes is a copy of a production CAD model.

## Internet research data

| Reference vehicle | Length mm | Width mm | Height mm | Wheelbase mm | Track F/R mm | Cd |
|---|---:|---:|---:|---:|---:|---:|
{chr(10).join(hot_table)}

The **reference hot hatch** uses the median length, width, height and wheelbase of the Golf R, Audi S3 Sportback and GRMN Corolla. Track values are averaged where official track dimensions are available. The crossover package uses the C-HR BEV dimensions directly.

## Mathematical body field

The car is expressed in an SAE-style vehicle frame with longitudinal coordinate `x`, lateral coordinate `y`, and vertical coordinate `z`. The body is the zero-isosurface of an implicit field.

Let

```text
s(x) = (x - x_rear) / (x_front - x_rear)
```

and let `a_b(s)`, `c_b(s)`, and `b_b(s)` be PCHIP-interpolated lower-body half-width, vertical centre, and half-height fields. The lower body is

```text
F_b = |xi_b|^5.2 + |y/a_b|^6.0 + |(z-c_b)/b_b|^4.2 - 1
```

The greenhouse is a second generalized superellipsoid,

```text
F_c = |xi_c|^3.6 + |y/a_c|^4.5 + |(z-c_c)/b_c|^3.0 - 1
```

with a shorter longitudinal domain. Front and rear fender volumes use fourth/eighth/fourth-order superellipsoids. Fields are blended with

```text
smin_k(a,b) = -log(exp(-k a) + exp(-k b)) / k
```

and cylindrical wheel-house fields are subtracted using constructive field difference. The body mesh is the level set

```text
F(x,y,z) = 0.
```

The greenhouse glass is reconstructed from the greenhouse field, clipped between belt and roof curves. Character curves and station sections are exported separately, allowing the implicit prototype to be replaced later by an FGKv1 Class-A patch graph.

## Variations

| Variation | Length mm | Width mm | Height mm | WB mm | Track F/R mm | Frontal area proxy m² | Body faces |
|---|---:|---:|---:|---:|---:|---:|---:|
{chr(10).join(variant_table)}

### Research Median AWD Hot Hatch
Balanced daily-performance package and the primary mathematical reference.

### Track Widebody AWD
Wider fender field, wider track, stronger splitter and wing, and reduced nominal underbody clearance.

### Aero Fastback AWD
Reduced greenhouse height, longer rear taper and smaller front opening. This is a geometric design hypothesis, not a CFD-validated drag result.

### Urban Crossover EV AWD
Long wheelbase, taller greenhouse, raised body, underfloor battery volume and no exhaust system.

## Validation performed

- finite vertices and triangle indices
- watertight main body mesh
- exact post-normalized body package dimensions
- hard wheelbase and track parameters
- analytic left/right symmetry
- exported longitudinal character curves and station sections
- projected frontal-area proxy calculated from the implicit body field
- deterministic parameter and validation records

## Research and method references

{source_notes}

## Limitations

The meshes are concept-level procedural geometry. They have not undergone CFD, crash, ergonomic, Class-A highlight, stamping, suspension-sweep, homologation, or manufacturing validation. The reported frontal area is a rasterized projection proxy; no drag coefficient is claimed for the generated vehicles. Wheel houses are geometric cylinder differences rather than suspension-swept envelopes. Glass, lighting, underbody, trim and wheel hardware are visual/packaging representations.

## Next high-value tests

1. Replace the smooth implicit body with an MVPv2.5 semantic curve network and G2 patch graph.
2. Fit section and character curves against calibrated front, side and top reference imagery.
3. Replace circular wheel-house cuts with full steer/jounce swept volumes.
4. Run mesh-independent CFD comparisons against a DrivAer validation case.
5. Add occupant H-points, eye points, head envelopes and ingress/egress checks.
6. Resolve body-in-white load paths and closure apertures from shared datums.
"""
    path.write_text(report, encoding="utf-8")


def write_readme(path: Path) -> None:
    path.write_text(
        """# Modern Car MCSMv1 Package

## Included

- `models/*.glb`: full colored 3D scenes
- `models/*.obj`: broadly compatible mesh exports
- `models/*_body.stl`: watertight body-only meshes
- `curves/reference_curve_network.json`: analytic longitudinal curves and 3D sections
- `curves/reference_station_table.csv`: parameter stations
- `MCSMv1_Modern_Car_Family.p3d`: proposed Progen3D/FGKv1 grammar
- `research_data.json`: official-source dimensional data
- `model_parameters.json`: complete variant and station parameters
- `validation.json`: mesh and package checks
- `previews/*.png`: orthographic and isometric engineering previews
- `RESEARCH_REPORT.md`: methods, equations, sources and limitations

## Regenerate

```bash
python modern_car_parametric_model.py --output generated --resolution medium
```

Resolution presets:

- `low`: fastest, suitable for grammar iteration
- `medium`: package default
- `high`: denser body mesh

Dependencies: NumPy, SciPy, scikit-image, trimesh, Pillow. VTK is optional and used only for PNG previews.
""",
        encoding="utf-8",
    )


# ---------------------------------------------------------------------------
# Generation and validation.
# ---------------------------------------------------------------------------
def resolution_tuple(name: str) -> tuple[int, int, int]:
    return {
        "low": (108, 58, 58),
        "medium": (130, 68, 68),
        "high": (164, 86, 86),
    }[name]


def validate_scene(variant: Variant, body: trimesh.Trimesh, model: ModernCarModel,
                   scene: trimesh.Scene) -> dict[str, Any]:
    bounds = body.bounds
    extents = bounds[1] - bounds[0]
    finite = bool(np.isfinite(body.vertices).all() and np.isfinite(body.faces).all())
    dimensions_error = np.abs(extents - np.asarray([variant.length, variant.width, variant.height]))
    geometry_finite = True
    for _, mesh in _mesh_world_instances(scene):
        geometry_finite = geometry_finite and bool(np.isfinite(mesh.vertices).all() and np.isfinite(mesh.faces).all())
    result = {
        "variant": variant.key,
        "label": variant.label,
        "body_vertices": int(len(body.vertices)),
        "body_faces": int(len(body.faces)),
        "body_watertight": bool(body.is_watertight),
        "body_volume_m3": float(body.volume),
        "body_surface_area_m2": float(body.area),
        "body_bounds_m": bounds.round(6).tolist(),
        "body_extents_m": extents.round(6).tolist(),
        "dimension_error_m": dimensions_error.round(9).tolist(),
        "finite_body": finite,
        "finite_scene": geometry_finite,
        "wheelbase_m": variant.wheelbase,
        "track_front_m": variant.track_front,
        "track_rear_m": variant.track_rear,
        "ground_clearance_parameter_m": variant.ground_clearance,
        "frontal_area_proxy_m2": model.projected_frontal_area(),
        "symmetry_by_construction": True,
        "scene_geometry_count": int(len(scene.geometry)),
        "pass": bool(
            body.is_watertight
            and finite
            and geometry_finite
            and float(dimensions_error.max()) < 1e-6
        ),
    }
    return result


def export_scene(scene: trimesh.Scene, glb: Path, obj: Path) -> None:
    glb.write_bytes(scene.export(file_type="glb"))
    obj.write_text(scene.export(file_type="obj"), encoding="utf-8")


def write_station_csv(path: Path, model: ModernCarModel, count: int = 21) -> None:
    xs = np.linspace(model.rear, model.front, count)
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "station", "u_rear_to_front", "x_m", "body_halfwidth_m",
            "body_center_z_m", "body_halfheight_m", "belt_z_m",
            "roof_halfwidth_m", "roof_top_z_m",
        ])
        for i, x in enumerate(xs):
            writer.writerow([
                i,
                (x - model.rear) / (model.front - model.rear),
                x,
                float(model.body_halfwidth(x)),
                float(model.body_center_z(x)),
                float(model.body_halfheight(x)),
                float(model.belt_z(x)),
                float(model.roof_halfwidth(x)) if model.cabin_rear <= x <= model.cabin_front else 0.0,
                float(model.roof_top(x)) if model.cabin_rear <= x <= model.cabin_front else 0.0,
            ])


def generate(output: Path, resolution_name: str = "medium", previews: bool = True,
             selected: Sequence[str] | None = None) -> dict[str, Any]:
    variants = derive_variants()
    selected_keys = list(selected) if selected else list(variants.keys())
    unknown = [key for key in selected_keys if key not in variants]
    if unknown:
        raise ValueError(f"unknown variant(s): {', '.join(unknown)}")

    output.mkdir(parents=True, exist_ok=True)
    model_dir = output / "models"
    curve_dir = output / "curves"
    preview_dir = output / "previews"
    model_dir.mkdir(exist_ok=True)
    curve_dir.mkdir(exist_ok=True)
    preview_dir.mkdir(exist_ok=True)

    (output / "research_data.json").write_text(
        json.dumps({"vehicle_sources": RESEARCH_SOURCES, "method_sources": METHOD_SOURCES}, indent=2),
        encoding="utf-8",
    )
    (output / "model_parameters.json").write_text(
        json.dumps({key: asdict(value) for key, value in variants.items()}, indent=2),
        encoding="utf-8",
    )

    validations: dict[str, Any] = {}
    scenes: dict[str, trimesh.Scene] = {}
    iso_images: list[tuple[Variant, Path]] = []
    res = resolution_tuple(resolution_name)

    for key in selected_keys:
        variant = variants[key]
        model = ModernCarModel(variant)
        scene, body = build_scene(model, res)
        scenes[key] = scene
        export_scene(
            scene,
            model_dir / f"modern_car_{key}.glb",
            model_dir / f"modern_car_{key}.obj",
        )
        body.export(model_dir / f"modern_car_{key}_body.stl")
        validations[key] = validate_scene(variant, body, model, scene)

        if key == "reference":
            curves = model.station_curves()
            sections = model.extract_sections(body)
            (curve_dir / "reference_curve_network.json").write_text(
                json.dumps({
                    "coordinate_system": {"x": "front", "y": "right", "z": "up"},
                    "curves": curves,
                    "sections": sections,
                }, indent=2),
                encoding="utf-8",
            )
            write_station_csv(curve_dir / "reference_station_table.csv", model)
            curve_scene = build_curve_scene(model, curves, sections)
            (model_dir / "modern_car_reference_curve_network.glb").write_bytes(
                curve_scene.export(file_type="glb")
            )

        if previews and vtk is not None:
            view_paths: dict[str, Path] = {}
            for view in ("iso", "side", "front", "top"):
                view_path = preview_dir / f"modern_car_{key}_{view}.png"
                render_scene(scene, variant, view_path, view)
                view_paths[view] = view_path
            contact_sheet(variant.label, variant, view_paths, preview_dir / f"modern_car_{key}_views.png")
            iso_images.append((variant, view_paths["iso"]))

    # Combined comparison model.
    comparison = trimesh.Scene()
    offsets = {
        "reference": np.asarray([0.0, -2.7, 0.0]),
        "track": np.asarray([0.0, 2.7, 0.0]),
        "aero": np.asarray([5.4, -2.7, 0.0]),
        "crossover": np.asarray([5.4, 2.7, 0.0]),
    }
    for key, scene in scenes.items():
        translation = trimesh.transformations.translation_matrix(offsets[key])
        for node_name in scene.graph.nodes_geometry:
            transform, geom_name = scene.graph[node_name]
            comparison.add_geometry(
                scene.geometry[geom_name],
                geom_name=f"{key}_{geom_name}",
                node_name=f"{key}_{node_name}",
                transform=translation @ transform,
            )
    (model_dir / "modern_car_variations_comparison.glb").write_bytes(comparison.export(file_type="glb"))

    if previews and len(iso_images) == 4:
        variants_poster(iso_images, preview_dir / "modern_car_variations_overview.png")

    (output / "validation.json").write_text(json.dumps(validations, indent=2), encoding="utf-8")
    write_progen3d_grammar(output / "MCSMv1_Modern_Car_Family.p3d", variants)
    write_report(output / "RESEARCH_REPORT.md", variants, validations)
    write_readme(output / "README.md")

    return {
        "output": str(output),
        "variants": selected_keys,
        "resolution": resolution_name,
        "validation": validations,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("Modern_Car_MCSMv1"))
    parser.add_argument("--resolution", choices=["low", "medium", "high"], default="medium")
    parser.add_argument("--variant", action="append", choices=["reference", "track", "aero", "crossover"])
    parser.add_argument("--no-preview", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    summary = generate(
        output=args.output,
        resolution_name=args.resolution,
        previews=not args.no_preview,
        selected=args.variant,
    )
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
