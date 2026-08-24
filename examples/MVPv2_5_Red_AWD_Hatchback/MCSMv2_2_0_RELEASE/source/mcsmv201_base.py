#!/usr/bin/env python3
"""MCSMv2 hybrid parametric modern-car generator.

MCSMv2 enriches the MCSMv1 implicit vehicle scaffold with:

* typed package, platform, style, wheel/suspension, powertrain and closure data;
* an auditable parameter-dependency graph;
* semantic cross-sections with named automotive landmarks;
* a two-family 3D character-curve network;
* a hybrid semantic-surface + implicit-scaffold body representation;
* steer/jounce-aware wheel-envelope approximations;
* semantic panel extraction and closure metadata;
* occupant, functional-package and body-in-white engineering layers;
* evidence, uncertainty, deterministic hashes and richer validation;
* a continuous reference-to-crossover design-manifold demonstration.

The geometry is an original mathematical construction. Manufacturer data copied
from MCSMv1 are used only as package references, never as proprietary CAD.
"""
from __future__ import annotations

import argparse
import csv
import hashlib
import json
import math
import os
import shutil
import sys
import zipfile
from dataclasses import asdict, dataclass, field, replace
from pathlib import Path
from typing import Any, Callable, Iterable, Mapping, Sequence

import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.interpolate import PchipInterpolator
from scipy.optimize import least_squares
from scipy.spatial import cKDTree
from skimage.measure import marching_cubes
import trimesh
from trimesh.visual.material import PBRMaterial

# The package ships the MCSMv1 generator as a local helper module.  Only its
# research records, robust mesh helpers and VTK preview renderer are reused.
try:
    import mcsmv1_legacy as legacy
except ImportError:  # pragma: no cover
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import mcsmv1_legacy as legacy


MCSM_VERSION = "2.0.1"
MODEL_SCHEMA = "MCSMv2.0.1-hybrid-semantic-implicit-integrity"


# =============================================================================
# Evidence and dependency graph
# =============================================================================

@dataclass(frozen=True)
class EvidenceRecord:
    status: str
    source: str
    confidence: float
    uncertainty: float | None = None
    note: str = ""

    def __post_init__(self) -> None:
        if not (0.0 <= self.confidence <= 1.0):
            raise ValueError("confidence must be in [0, 1]")
        if self.uncertainty is not None and self.uncertainty < 0:
            raise ValueError("uncertainty must be non-negative")


ParameterScalar = float | int | bool | str


def _parameter_value_type(value: Any) -> str:
    if isinstance(value, bool):
        return "bool"
    if isinstance(value, int):
        return "int"
    if isinstance(value, str):
        return "enum"
    return "float"


@dataclass
class ParameterNode:
    name: str
    dependencies: tuple[str, ...]
    evaluator: Callable[[Mapping[str, ParameterScalar]], ParameterScalar] | None
    formula: str
    evidence: EvidenceRecord
    unit: str = "1"
    value_type: str = "float"
    explicit_value: ParameterScalar | None = None
    value: ParameterScalar | None = None


class ParameterGraph:
    """Deterministic, unit-aware dependency graph with cycle diagnostics."""

    def __init__(self) -> None:
        self.nodes: dict[str, ParameterNode] = {}
        self._evaluation_order: list[str] = []

    def add_explicit(
        self,
        name: str,
        value: ParameterScalar,
        evidence: EvidenceRecord,
        unit: str = "1",
        value_type: str | None = None,
    ) -> None:
        if name in self.nodes:
            raise ValueError(f"duplicate parameter: {name}")
        if isinstance(value, np.generic):
            value = value.item()
        self.nodes[name] = ParameterNode(
            name=name,
            dependencies=(),
            evaluator=None,
            formula="explicit",
            evidence=evidence,
            unit=unit,
            value_type=value_type or _parameter_value_type(value),
            explicit_value=value,
        )

    def add_derived(
        self,
        name: str,
        dependencies: Sequence[str],
        evaluator: Callable[[Mapping[str, ParameterScalar]], ParameterScalar],
        formula: str,
        evidence: EvidenceRecord,
        unit: str = "1",
        value_type: str = "float",
    ) -> None:
        if name in self.nodes:
            raise ValueError(f"duplicate parameter: {name}")
        self.nodes[name] = ParameterNode(
            name=name,
            dependencies=tuple(dependencies),
            evaluator=evaluator,
            formula=formula,
            evidence=evidence,
            unit=unit,
            value_type=value_type,
        )

    def evaluate(self) -> dict[str, ParameterScalar]:
        values: dict[str, ParameterScalar] = {}
        visiting: list[str] = []
        visited: set[str] = set()
        order: list[str] = []

        def visit(name: str) -> ParameterScalar:
            if name in values:
                return values[name]
            if name in visiting:
                first = visiting.index(name)
                cycle = " -> ".join([*visiting[first:], name])
                raise ValueError(f"parameter dependency cycle: {cycle}")
            if name not in self.nodes:
                raise KeyError(f"unknown dependency: {name}")
            visiting.append(name)
            node = self.nodes[name]
            if node.explicit_value is not None:
                value: ParameterScalar = node.explicit_value
            else:
                dep_values = {dep: visit(dep) for dep in node.dependencies}
                if node.evaluator is None:
                    raise RuntimeError(f"derived parameter lacks evaluator: {name}")
                value = node.evaluator(dep_values)
                if isinstance(value, np.generic):
                    value = value.item()
            if isinstance(value, (float, int)) and not isinstance(value, bool):
                if not np.isfinite(value):
                    raise ValueError(f"non-finite parameter: {name}")
            node.value = value
            values[name] = value
            visiting.pop()
            visited.add(name)
            order.append(name)
            return value

        for key in sorted(self.nodes):
            visit(key)
        self._evaluation_order = order
        return values

    @property
    def evaluation_order(self) -> list[str]:
        if not self._evaluation_order:
            self.evaluate()
        return list(self._evaluation_order)

    def as_dict(self) -> dict[str, Any]:
        values = self.evaluate()
        return {
            "schema": "MCSMv2.0.1.ParameterGraph.v2",
            "evaluation_order": self.evaluation_order,
            "nodes": {
                name: {
                    "value": values[name],
                    "unit": node.unit,
                    "value_type": node.value_type,
                    "dependencies": list(node.dependencies),
                    "formula": node.formula,
                    "evidence": asdict(node.evidence),
                }
                for name, node in sorted(self.nodes.items())
            },
        }


@dataclass(frozen=True)
class VehicleReferenceFrame:
    """Explicit MCSMv2 frame and its deterministic Progen3D adapter."""

    name: str = "MCSMv2VehicleFrame"
    origin: tuple[float, float, float] = (0.0, 0.0, 0.0)
    longitudinal_axis: tuple[float, float, float] = (1.0, 0.0, 0.0)
    lateral_axis: tuple[float, float, float] = (0.0, 1.0, 0.0)
    vertical_axis: tuple[float, float, float] = (0.0, 0.0, 1.0)
    handedness: str = "right"
    length_unit: str = "m"
    angle_unit: str = "deg"

    def validate(self, tolerance: float = 1e-10) -> dict[str, Any]:
        axes = np.asarray([
            self.longitudinal_axis,
            self.lateral_axis,
            self.vertical_axis,
        ], dtype=float)
        norms = np.linalg.norm(axes, axis=1)
        gram = axes @ axes.T
        cross = np.cross(axes[0], axes[1])
        handed = float(np.dot(cross, axes[2]))
        valid = bool(
            np.isfinite(axes).all()
            and np.all(np.abs(norms - 1.0) <= tolerance)
            and np.all(np.abs(gram - np.eye(3)) <= tolerance)
            and self.handedness == "right"
            and handed > 1.0 - tolerance
        )
        return {
            "valid": valid,
            "axis_norms": norms.tolist(),
            "gram_matrix": gram.tolist(),
            "handedness_scalar": handed,
            "tolerance": tolerance,
        }

    def transform_to_progen3d(self) -> np.ndarray:
        # MCSMv2: X=front, Y=right, Z=up.
        # Progen3D vehicle convention: X=right, Y=up, Z=front.
        return np.asarray([
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ])

    def as_dict(self) -> dict[str, Any]:
        return {
            "schema": "MCSMv2.0.1.VehicleReferenceFrame.v1",
            "name": self.name,
            "origin": list(self.origin),
            "axes": {
                "x": {"semantic": "front", "vector": list(self.longitudinal_axis)},
                "y": {"semantic": "right", "vector": list(self.lateral_axis)},
                "z": {"semantic": "up", "vector": list(self.vertical_axis)},
            },
            "handedness": self.handedness,
            "length_unit": self.length_unit,
            "angle_unit": self.angle_unit,
            "to_progen3d_matrix": self.transform_to_progen3d().tolist(),
            "validation": self.validate(),
        }


# =============================================================================
# Typed model parameters
# =============================================================================

@dataclass(frozen=True)
class VehiclePackageParameters:
    length: float
    width: float
    height: float
    wheelbase: float
    track_front: float
    track_rear: float
    ground_clearance: float
    front_overhang: float
    rear_overhang: float


@dataclass(frozen=True)
class PlatformParameters:
    floor_height: float
    front_hpoint_x: float
    rear_hpoint_x: float
    hpoint_z: float
    eye_z: float
    head_clearance: float
    seat_lateral: float


@dataclass(frozen=True)
class StyleParameters:
    roof_scale: float
    greenhouse_front_factor: float
    greenhouse_rear_factor: float
    nose_taper: float
    tail_taper: float
    hood_wedge: float
    roof_crown: float
    tumblehome: float
    belt_rise: float
    shoulder_strength: float
    front_fender_amplitude: float
    rear_haunch_amplitude: float
    rocker_tuck: float
    door_scallop: float
    spoiler_scale: float
    splitter_scale: float
    grille_scale: float


@dataclass(frozen=True)
class WheelSuspensionParameters:
    wheel_radius: float
    wheel_width: float
    steer_min_deg: float
    steer_max_deg: float
    travel_min: float
    travel_max: float
    wheelhouse_clearance: float


@dataclass(frozen=True)
class PowertrainParameters:
    architecture: str
    drive_layout: str
    battery_pack: bool
    battery_thickness: float
    engine_envelope_length: float
    engine_envelope_width: float
    engine_envelope_height: float
    exhaust_count: int


@dataclass(frozen=True)
class ClosureParameters:
    front_door_max_deg: float = 68.0
    rear_door_max_deg: float = 68.0
    bonnet_max_deg: float = 62.0
    hatch_max_deg: float = 72.0
    side_glass_travel: float = 0.55
    nominal_panel_gap: float = 0.004


@dataclass(frozen=True)
class VehicleVariantV2:
    key: str
    label: str
    description: str
    package_basis: str
    package: VehiclePackageParameters
    platform: PlatformParameters
    style: StyleParameters
    wheel_suspension: WheelSuspensionParameters
    powertrain: PowertrainParameters
    closures: ClosureParameters
    body_rgb: tuple[int, int, int]
    reference_frame: VehicleReferenceFrame = field(default_factory=VehicleReferenceFrame)
    crossover_cladding: bool = False
    provenance: Mapping[str, EvidenceRecord] = field(default_factory=dict)

    # Duck-typed properties used by the legacy preview and wheel helpers.
    @property
    def length(self) -> float:
        return self.package.length

    @property
    def width(self) -> float:
        return self.package.width

    @property
    def height(self) -> float:
        return self.package.height

    @property
    def wheelbase(self) -> float:
        return self.package.wheelbase

    @property
    def track_front(self) -> float:
        return self.package.track_front

    @property
    def track_rear(self) -> float:
        return self.package.track_rear

    @property
    def ground_clearance(self) -> float:
        return self.package.ground_clearance

    @property
    def front_overhang(self) -> float:
        return self.package.front_overhang

    @property
    def rear_overhang(self) -> float:
        return self.package.rear_overhang

    @property
    def wheel_radius(self) -> float:
        return self.wheel_suspension.wheel_radius

    @property
    def wheel_width(self) -> float:
        return self.wheel_suspension.wheel_width

    @property
    def roof_scale(self) -> float:
        return self.style.roof_scale

    @property
    def body_flare(self) -> float:
        return max(self.style.front_fender_amplitude, self.style.rear_haunch_amplitude)

    @property
    def cabin_front_factor(self) -> float:
        return self.style.greenhouse_front_factor

    @property
    def cabin_rear_factor(self) -> float:
        return self.style.greenhouse_rear_factor

    @property
    def tail_taper(self) -> float:
        return self.style.tail_taper

    @property
    def nose_taper(self) -> float:
        return self.style.nose_taper

    @property
    def spoiler_scale(self) -> float:
        return self.style.spoiler_scale

    @property
    def splitter_scale(self) -> float:
        return self.style.splitter_scale

    @property
    def grille_scale(self) -> float:
        return self.style.grille_scale

    @property
    def battery_pack(self) -> bool:
        return self.powertrain.battery_pack

    @property
    def exhaust_count(self) -> int:
        return self.powertrain.exhaust_count

    def to_legacy_variant(self) -> legacy.Variant:
        return legacy.Variant(
            key=self.key,
            label=self.label,
            description=self.description,
            package_basis=self.package_basis,
            length=self.length,
            width=self.width,
            height=self.height,
            wheelbase=self.wheelbase,
            track_front=self.track_front,
            track_rear=self.track_rear,
            ground_clearance=self.ground_clearance,
            wheel_radius=self.wheel_radius,
            wheel_width=self.wheel_width,
            front_overhang=self.front_overhang,
            rear_overhang=self.rear_overhang,
            roof_scale=self.style.roof_scale,
            body_flare=self.body_flare,
            cabin_front_factor=self.style.greenhouse_front_factor,
            cabin_rear_factor=self.style.greenhouse_rear_factor,
            tail_taper=self.style.tail_taper,
            nose_taper=self.style.nose_taper,
            spoiler_scale=self.style.spoiler_scale,
            splitter_scale=self.style.splitter_scale,
            grille_scale=self.style.grille_scale,
            crossover_cladding=self.crossover_cladding,
            battery_pack=self.powertrain.battery_pack,
            exhaust_count=self.powertrain.exhaust_count,
            body_rgb=self.body_rgb,
        )


@dataclass(frozen=True)
class SemanticStation:
    index: int
    role: str
    u: float
    x: float
    underbody_z: float
    underbody_halfwidth: float
    rocker_z: float
    rocker_halfwidth: float
    lower_z: float
    lower_halfwidth: float
    shoulder_z: float
    shoulder_halfwidth: float
    belt_z: float
    belt_halfwidth: float
    glass_shoulder_z: float
    glass_shoulder_halfwidth: float
    roof_rail_z: float
    roof_rail_halfwidth: float
    roof_crown_z: float
    confidence: float

    def landmark_pairs(self) -> list[tuple[str, float, float]]:
        """Return right-half section landmarks as (name, halfwidth, z)."""
        return [
            ("underbody", self.underbody_halfwidth, self.underbody_z),
            ("rocker", self.rocker_halfwidth, self.rocker_z),
            ("lower_body", self.lower_halfwidth, self.lower_z),
            ("shoulder", self.shoulder_halfwidth, self.shoulder_z),
            ("belt", self.belt_halfwidth, self.belt_z),
            ("glass_shoulder", self.glass_shoulder_halfwidth, self.glass_shoulder_z),
            ("roof_rail", self.roof_rail_halfwidth, self.roof_rail_z),
            ("roof_crown", 0.0, self.roof_crown_z),
        ]


# =============================================================================
# Variant derivation and continuous design manifold
# =============================================================================

PACKAGE_EVIDENCE = EvidenceRecord(
    status="PublishedPackage",
    source="MCSMv1 official manufacturer package research",
    confidence=0.95,
    uncertainty=0.002,
    note="Manufacturer-derived package reference; not proprietary CAD.",
)
PLATFORM_EVIDENCE = EvidenceRecord(
    status="Derived",
    source="MCSMv2 platform derivation",
    confidence=0.82,
    uncertainty=0.010,
)
STYLE_EVIDENCE = EvidenceRecord(
    status="DesignChoice",
    source="MCSMv2 style field",
    confidence=0.60,
    uncertainty=0.015,
)
WHEEL_PACKAGE_EVIDENCE = EvidenceRecord(
    status="PackageReference",
    source="MCSMv1 tyre and wheel package research",
    confidence=0.82,
    uncertainty=0.006,
)
ENGINEERING_ASSUMPTION_EVIDENCE = EvidenceRecord(
    status="EngineeringAssumption",
    source="MCSMv2 concept-level envelope policy",
    confidence=0.58,
    uncertainty=0.015,
)
POWERTRAIN_EVIDENCE = EvidenceRecord(
    status="ArchitectureReference",
    source="MCSMv2 variant architecture",
    confidence=0.70,
    uncertainty=None,
)
CLOSURE_EVIDENCE = EvidenceRecord(
    status="DesignChoice",
    source="MVPv2.5 closure design ranges",
    confidence=0.55,
    uncertainty=2.0,
    note="Angle uncertainty is expressed in degrees.",
)
DERIVED_EVIDENCE = EvidenceRecord(
    status="Derived",
    source="MCSMv2.0.1 dependency graph",
    confidence=0.88,
    uncertainty=0.005,
)
# Compatibility alias used by earlier tests and consumers.
PRIMARY_EVIDENCE = PACKAGE_EVIDENCE

_PACKAGE_FIELDS = set(VehiclePackageParameters.__dataclass_fields__)
_PLATFORM_FIELDS = set(PlatformParameters.__dataclass_fields__)
_STYLE_FIELDS = set(StyleParameters.__dataclass_fields__)
_WHEEL_FIELDS = set(WheelSuspensionParameters.__dataclass_fields__)
_POWERTRAIN_FIELDS = set(PowertrainParameters.__dataclass_fields__)
_CLOSURE_FIELDS = set(ClosureParameters.__dataclass_fields__)


def parameter_evidence_for_variant(variant: VehicleVariantV2, name: str) -> EvidenceRecord:
    """Return evidence for the exact parameter, never a blanket package record."""
    override = variant.provenance.get(f"parameter:{name}")
    if override is not None:
        return override
    if name in _PACKAGE_FIELDS:
        return PACKAGE_EVIDENCE
    if name in _PLATFORM_FIELDS:
        return PLATFORM_EVIDENCE
    if name in _STYLE_FIELDS or name == "crossover_cladding":
        return STYLE_EVIDENCE
    if name in {"wheel_radius", "wheel_width"}:
        return WHEEL_PACKAGE_EVIDENCE
    if name in _WHEEL_FIELDS:
        return ENGINEERING_ASSUMPTION_EVIDENCE
    if name in _POWERTRAIN_FIELDS or name in {
        "powertrain_architecture", "drive_layout"
    }:
        basis = variant.provenance.get("powertrain")
        return basis or POWERTRAIN_EVIDENCE
    if name in _CLOSURE_FIELDS:
        return CLOSURE_EVIDENCE
    return DERIVED_EVIDENCE


def _platform_for(v: legacy.Variant) -> PlatformParameters:
    front_h = v.wheelbase * 0.06
    rear_h = -v.wheelbase * 0.29
    floor_height = max(v.ground_clearance + 0.13, 0.24)
    hpoint_z = floor_height + (0.30 if not v.battery_pack else 0.35)
    return PlatformParameters(
        floor_height=floor_height,
        front_hpoint_x=front_h,
        rear_hpoint_x=rear_h,
        hpoint_z=hpoint_z,
        eye_z=min(v.height - 0.15, hpoint_z + 0.62),
        head_clearance=0.085,
        seat_lateral=v.width * 0.14,
    )


def derive_v2_variants() -> dict[str, VehicleVariantV2]:
    legacy_variants = legacy.derive_variants()
    output: dict[str, VehicleVariantV2] = {}

    style_overrides: dict[str, dict[str, float]] = {
        "reference": dict(
            hood_wedge=0.52, roof_crown=0.50, tumblehome=0.30,
            belt_rise=0.045, shoulder_strength=0.032,
            front_fender_amplitude=0.030, rear_haunch_amplitude=0.040,
            rocker_tuck=0.050, door_scallop=0.024,
        ),
        "track": dict(
            hood_wedge=0.58, roof_crown=0.44, tumblehome=0.27,
            belt_rise=0.055, shoulder_strength=0.055,
            front_fender_amplitude=0.060, rear_haunch_amplitude=0.075,
            rocker_tuck=0.055, door_scallop=0.030,
        ),
        "aero": dict(
            hood_wedge=0.46, roof_crown=0.40, tumblehome=0.35,
            belt_rise=0.038, shoulder_strength=0.024,
            front_fender_amplitude=0.020, rear_haunch_amplitude=0.028,
            rocker_tuck=0.060, door_scallop=0.018,
        ),
        "crossover": dict(
            hood_wedge=0.48, roof_crown=0.56, tumblehome=0.26,
            belt_rise=0.060, shoulder_strength=0.060,
            front_fender_amplitude=0.065, rear_haunch_amplitude=0.070,
            rocker_tuck=0.035, door_scallop=0.020,
        ),
    }

    for key, lv in legacy_variants.items():
        package = VehiclePackageParameters(
            length=lv.length,
            width=lv.width,
            height=lv.height,
            wheelbase=lv.wheelbase,
            track_front=lv.track_front,
            track_rear=lv.track_rear,
            ground_clearance=lv.ground_clearance,
            front_overhang=lv.front_overhang,
            rear_overhang=lv.rear_overhang,
        )
        ov = style_overrides[key]
        style = StyleParameters(
            roof_scale=lv.roof_scale,
            greenhouse_front_factor=lv.cabin_front_factor,
            greenhouse_rear_factor=lv.cabin_rear_factor,
            nose_taper=lv.nose_taper,
            tail_taper=lv.tail_taper,
            hood_wedge=ov["hood_wedge"],
            roof_crown=ov["roof_crown"],
            tumblehome=ov["tumblehome"],
            belt_rise=ov["belt_rise"],
            shoulder_strength=ov["shoulder_strength"],
            front_fender_amplitude=ov["front_fender_amplitude"],
            rear_haunch_amplitude=ov["rear_haunch_amplitude"],
            rocker_tuck=ov["rocker_tuck"],
            door_scallop=ov["door_scallop"],
            spoiler_scale=lv.spoiler_scale,
            splitter_scale=lv.splitter_scale,
            grille_scale=lv.grille_scale,
        )
        wheel = WheelSuspensionParameters(
            wheel_radius=lv.wheel_radius,
            wheel_width=lv.wheel_width,
            steer_min_deg=-32.0,
            steer_max_deg=32.0,
            travel_min=-0.070 if key != "crossover" else -0.085,
            travel_max=0.060 if key != "crossover" else 0.075,
            wheelhouse_clearance=0.028 if key != "track" else 0.024,
        )
        powertrain = PowertrainParameters(
            architecture="BEV" if lv.battery_pack else "ICE_AWD",
            drive_layout="AWD",
            battery_pack=lv.battery_pack,
            battery_thickness=0.145 if lv.battery_pack else 0.0,
            engine_envelope_length=0.78 if not lv.battery_pack else 0.48,
            engine_envelope_width=1.18,
            engine_envelope_height=0.56 if not lv.battery_pack else 0.34,
            exhaust_count=lv.exhaust_count,
        )
        provenance = {
            "package": PACKAGE_EVIDENCE,
            "platform": PLATFORM_EVIDENCE,
            "style": STYLE_EVIDENCE,
            "wheel_suspension": ENGINEERING_ASSUMPTION_EVIDENCE,
            "powertrain": EvidenceRecord(
                "ArchitectureReference", lv.package_basis, 0.72, None
            ),
            "closures": CLOSURE_EVIDENCE,
        }
        output[key] = VehicleVariantV2(
            key=key,
            label=lv.label,
            description=lv.description,
            package_basis=lv.package_basis,
            package=package,
            platform=_platform_for(lv),
            style=style,
            wheel_suspension=wheel,
            powertrain=powertrain,
            closures=ClosureParameters(),
            body_rgb=lv.body_rgb,
            crossover_cladding=lv.crossover_cladding,
            provenance=provenance,
        )
    return output


def interpolate_variant(
    a: VehicleVariantV2,
    b: VehicleVariantV2,
    t: float,
    key: str,
    label: str,
) -> VehicleVariantV2:
    """Continuous geometric interpolation for manifold visualization."""
    t = float(np.clip(t, 0.0, 1.0))

    def mix(x: float, y: float) -> float:
        return (1.0 - t) * x + t * y

    package = VehiclePackageParameters(**{
        name: mix(getattr(a.package, name), getattr(b.package, name))
        for name in a.package.__dataclass_fields__
    })
    platform = PlatformParameters(**{
        name: mix(getattr(a.platform, name), getattr(b.platform, name))
        for name in a.platform.__dataclass_fields__
    })
    style = StyleParameters(**{
        name: mix(getattr(a.style, name), getattr(b.style, name))
        for name in a.style.__dataclass_fields__
    })
    wheel = WheelSuspensionParameters(**{
        name: mix(getattr(a.wheel_suspension, name), getattr(b.wheel_suspension, name))
        for name in a.wheel_suspension.__dataclass_fields__
    })
    architecture = a.powertrain.architecture if t < 0.5 else b.powertrain.architecture
    powertrain = PowertrainParameters(
        architecture=architecture,
        drive_layout="AWD",
        battery_pack=t >= 0.5 and b.powertrain.battery_pack,
        battery_thickness=mix(a.powertrain.battery_thickness, b.powertrain.battery_thickness),
        engine_envelope_length=mix(a.powertrain.engine_envelope_length, b.powertrain.engine_envelope_length),
        engine_envelope_width=mix(a.powertrain.engine_envelope_width, b.powertrain.engine_envelope_width),
        engine_envelope_height=mix(a.powertrain.engine_envelope_height, b.powertrain.engine_envelope_height),
        exhaust_count=a.powertrain.exhaust_count if t < 0.5 else b.powertrain.exhaust_count,
    )
    rgb = tuple(int(round(mix(x, y))) for x, y in zip(a.body_rgb, b.body_rgb))
    return VehicleVariantV2(
        key=key,
        label=label,
        description=f"Continuous manifold sample t={t:.2f} from {a.key} to {b.key}",
        package_basis="MCSMv2 continuous interpolation",
        package=package,
        platform=platform,
        style=style,
        wheel_suspension=wheel,
        powertrain=powertrain,
        closures=a.closures,
        body_rgb=rgb,
        crossover_cladding=t >= 0.5 and b.crossover_cladding,
        provenance={"manifold": EvidenceRecord("Interpolated", "MCSMv2 manifold", 0.75, 0.01)},
    )


# =============================================================================
# Semantic section field
# =============================================================================

class SemanticSectionModel:
    """Named section landmarks derived from package, style and occupant data."""

    LANDMARK_NAMES = (
        "underbody_z", "underbody_halfwidth",
        "rocker_z", "rocker_halfwidth",
        "lower_z", "lower_halfwidth",
        "shoulder_z", "shoulder_halfwidth",
        "belt_z", "belt_halfwidth",
        "glass_shoulder_z", "glass_shoulder_halfwidth",
        "roof_rail_z", "roof_rail_halfwidth",
        "roof_crown_z",
    )

    Z_FIELDS = (
        "underbody_z", "rocker_z", "lower_z", "shoulder_z",
        "belt_z", "glass_shoulder_z", "roof_rail_z", "roof_crown_z",
    )
    WIDTH_FIELDS = (
        "underbody_halfwidth", "rocker_halfwidth", "lower_halfwidth",
        "shoulder_halfwidth", "belt_halfwidth",
        "glass_shoulder_halfwidth", "roof_rail_halfwidth",
    )

    def __init__(self, variant: VehicleVariantV2):
        self.v = variant
        self.legacy_model = legacy.ModernCarModel(variant.to_legacy_variant())
        self.front = self.legacy_model.front
        self.rear = self.legacy_model.rear
        self.length = self.front - self.rear
        self.front_axle = variant.wheelbase / 2.0
        self.rear_axle = -variant.wheelbase / 2.0
        self.cabin_front = self.legacy_model.cabin_front
        self.cabin_rear = self.legacy_model.cabin_rear
        self.stations = self._build_stations()
        self._interpolators = self._build_interpolators()

    @staticmethod
    def _gaussian4(x: float | np.ndarray, centre: float, width: float) -> np.ndarray:
        return np.exp(-((np.asarray(x) - centre) / max(width, 1e-6)) ** 4)

    def _station_roles(self) -> list[tuple[str, float]]:
        u_rear_axle = (self.rear_axle - self.rear) / self.length
        u_front_axle = (self.front_axle - self.rear) / self.length
        return [
            ("tail_face", 0.000),
            ("rear_bumper", 0.060),
            ("rear_axle", u_rear_axle),
            ("rear_door", 0.330),
            ("b_pillar", 0.455),
            ("front_door", 0.585),
            ("a_pillar", 0.690),
            ("hood_rear", 0.770),
            ("front_axle", u_front_axle),
            ("nose", 0.945),
            ("front_face", 1.000),
        ]

    def _build_stations(self) -> list[SemanticStation]:
        v = self.v
        style = v.style
        raw: list[dict[str, float | str]] = []
        for index, (role, u) in enumerate(self._station_roles()):
            x = self.rear + u * self.length
            base_w = float(self.legacy_model.body_halfwidth(x))
            centre = float(self.legacy_model.body_center_z(x))
            halfheight = float(self.legacy_model.body_halfheight(x))
            body_bottom = max(v.ground_clearance * 0.82, centre - halfheight)
            body_top = centre + halfheight

            front_g = float(self._gaussian4(x, self.front_axle, 0.56))
            rear_g = float(self._gaussian4(x, self.rear_axle, 0.58))
            door_mid = 0.5 * (self.cabin_front + self.cabin_rear)
            door_g = float(self._gaussian4(x, door_mid, max(0.9, (self.cabin_front - self.cabin_rear) * 0.65)))

            shoulder_y = base_w * (1.0 + style.shoulder_strength)
            shoulder_y += style.front_fender_amplitude * front_g
            shoulder_y += style.rear_haunch_amplitude * rear_g

            lower_y = shoulder_y * 0.985 - style.door_scallop * door_g
            rocker_y = lower_y * 0.965 - style.rocker_tuck * door_g
            under_y = rocker_y * (0.72 if not v.crossover_cladding else 0.76)

            lower_z = max(body_bottom + 0.08, centre - halfheight * 0.28)
            rocker_z = max(body_bottom + 0.035, v.ground_clearance + 0.025)
            shoulder_z = centre + halfheight * 0.43

            in_cabin = self.cabin_rear <= x <= self.cabin_front
            if in_cabin:
                belt_z = float(self.legacy_model.belt_z(x))
                roof_top = float(self.legacy_model.roof_top(x))
                roof_halfwidth = max(0.03, float(self.legacy_model.roof_halfwidth(x)))
                greenhouse_blend = min(
                    1.0,
                    max(0.0, (x - self.cabin_rear) / 0.28),
                    max(0.0, (self.cabin_front - x) / 0.28),
                )
                greenhouse_blend = max(0.08, greenhouse_blend)
                roof_halfwidth *= (1.0 - 0.28 * style.tumblehome) * greenhouse_blend
                glass_y = max(0.035, shoulder_y * (0.68 - 0.10 * style.tumblehome) * greenhouse_blend)
                roof_rail_y = max(0.025, roof_halfwidth)
            else:
                # Hood and tail are still represented by the same semantic
                # section schema; greenhouse landmarks collapse toward centre.
                belt_z = centre + halfheight * 0.52
                roof_top = body_top
                hood_zone = float(self._gaussian4(x, self.front - 0.45, 0.75))
                roof_top += style.hood_wedge * 0.035 * hood_zone
                glass_y = max(0.015, shoulder_y * 0.10)
                roof_rail_y = max(0.010, glass_y * 0.65)

            # Roof-crown mode changes broad upper curvature while preserving H.
            roof_top += (style.roof_crown - 0.5) * 0.035 * (1.0 if in_cabin else 0.25)
            belt_z += style.belt_rise * (0.5 - u)

            glass_z = min(roof_top - 0.050, belt_z + 0.022)
            roof_rail_z = max(glass_z + 0.020, roof_top - 0.045)

            # Keep every section strictly ordered in z.
            zvals = np.asarray([
                body_bottom,
                rocker_z,
                lower_z,
                shoulder_z,
                belt_z,
                glass_z,
                roof_rail_z,
                roof_top,
            ], dtype=float)
            zvals = np.maximum.accumulate(zvals + np.arange(len(zvals)) * 1e-5)

            raw.append({
                "index": float(index),
                "role": role,
                "u": float(u),
                "x": float(x),
                "underbody_z": float(zvals[0]),
                "underbody_halfwidth": float(max(0.03, under_y)),
                "rocker_z": float(zvals[1]),
                "rocker_halfwidth": float(max(0.04, rocker_y)),
                "lower_z": float(zvals[2]),
                "lower_halfwidth": float(max(0.05, lower_y)),
                "shoulder_z": float(zvals[3]),
                "shoulder_halfwidth": float(max(0.06, shoulder_y)),
                "belt_z": float(zvals[4]),
                "belt_halfwidth": float(max(0.04, shoulder_y * 0.93)),
                "glass_shoulder_z": float(zvals[5]),
                "glass_shoulder_halfwidth": float(glass_y),
                "roof_rail_z": float(zvals[6]),
                "roof_rail_halfwidth": float(roof_rail_y),
                "roof_crown_z": float(zvals[7]),
                "confidence": 0.80 if role in {"front_axle", "rear_axle", "b_pillar"} else 0.68,
            })

        # Enforce package width and height before meshing.  This replaces the
        # large post-hoc affine normalization used by MCSMv1.
        max_width = 2.0 * max(float(item["shoulder_halfwidth"]) for item in raw)
        y_scale = v.width / max_width
        for item in raw:
            for key in (
                "underbody_halfwidth", "rocker_halfwidth", "lower_halfwidth",
                "shoulder_halfwidth", "belt_halfwidth",
                "glass_shoulder_halfwidth", "roof_rail_halfwidth",
            ):
                item[key] = float(item[key]) * y_scale

        max_roof = max(float(item["roof_crown_z"]) for item in raw)
        z_scale = v.height / max_roof
        for item in raw:
            for key in (
                "underbody_z", "rocker_z", "lower_z", "shoulder_z",
                "belt_z", "glass_shoulder_z", "roof_rail_z", "roof_crown_z",
            ):
                item[key] = float(item[key]) * z_scale

        stations: list[SemanticStation] = []
        for item in raw:
            stations.append(SemanticStation(
                index=int(item["index"]),
                role=str(item["role"]),
                u=float(item["u"]),
                x=float(item["x"]),
                underbody_z=float(item["underbody_z"]),
                underbody_halfwidth=float(item["underbody_halfwidth"]),
                rocker_z=float(item["rocker_z"]),
                rocker_halfwidth=float(item["rocker_halfwidth"]),
                lower_z=float(item["lower_z"]),
                lower_halfwidth=float(item["lower_halfwidth"]),
                shoulder_z=float(item["shoulder_z"]),
                shoulder_halfwidth=float(item["shoulder_halfwidth"]),
                belt_z=float(item["belt_z"]),
                belt_halfwidth=float(item["belt_halfwidth"]),
                glass_shoulder_z=float(item["glass_shoulder_z"]),
                glass_shoulder_halfwidth=float(item["glass_shoulder_halfwidth"]),
                roof_rail_z=float(item["roof_rail_z"]),
                roof_rail_halfwidth=float(item["roof_rail_halfwidth"]),
                roof_crown_z=float(item["roof_crown_z"]),
                confidence=float(item["confidence"]),
            ))
        return stations

    def _build_interpolators(self) -> dict[str, PchipInterpolator]:
        """Build non-crossing vertical fields and independent width fields.

        Absolute vertical PCHIP curves can cross between valid stations.  v2.0.1
        instead interpolates the underbody base and the logarithm of each
        positive adjacent vertical gap.  Reconstructing cumulative gaps makes
        z-ordering an invariant over the complete longitudinal domain.
        """
        x = np.asarray([s.x for s in self.stations], dtype=float)
        self._z_base_interpolator = PchipInterpolator(
            x,
            np.asarray([getattr(s, self.Z_FIELDS[0]) for s in self.stations], dtype=float),
        )
        self._z_gap_interpolators: list[PchipInterpolator] = []
        for lower, upper in zip(self.Z_FIELDS, self.Z_FIELDS[1:]):
            gaps = np.asarray([
                getattr(s, upper) - getattr(s, lower)
                for s in self.stations
            ], dtype=float)
            gaps = np.maximum(gaps, 1e-7)
            self._z_gap_interpolators.append(PchipInterpolator(x, np.log(gaps)))
        return {
            name: PchipInterpolator(
                x,
                np.asarray([getattr(s, name) for s in self.stations], dtype=float),
            )
            for name in self.WIDTH_FIELDS
        }

    def value(self, name: str, x: np.ndarray | float) -> np.ndarray:
        query = np.asarray(x, dtype=float)
        if name in self.Z_FIELDS:
            index = self.Z_FIELDS.index(name)
            value = np.asarray(self._z_base_interpolator(query), dtype=float)
            for gap_interp in self._z_gap_interpolators[:index]:
                value = value + np.exp(np.asarray(gap_interp(query), dtype=float))
            return value
        return np.asarray(self._interpolators[name](query), dtype=float)

    def continuous_order_diagnostics(self, samples: int = 4001) -> dict[str, Any]:
        xs = np.linspace(self.rear, self.front, samples)
        z = np.vstack([self.value(name, xs) for name in self.Z_FIELDS])
        gaps = np.diff(z, axis=0)
        width = np.vstack([self.value(name, xs) for name in self.WIDTH_FIELDS])
        negative = np.argwhere(gaps < -1e-10)
        minimum_gap_index = np.unravel_index(int(np.argmin(gaps)), gaps.shape)
        minimum_width_index = np.unravel_index(int(np.argmin(width)), width.shape)
        return {
            "schema": "MCSMv2.0.1.ContinuousSectionOrder.v1",
            "sample_count": int(samples),
            "negative_z_gap_count": int(len(negative)),
            "minimum_z_gap_m": float(gaps[minimum_gap_index]),
            "minimum_z_gap_lower_field": self.Z_FIELDS[minimum_gap_index[0]],
            "minimum_z_gap_upper_field": self.Z_FIELDS[minimum_gap_index[0] + 1],
            "minimum_z_gap_x_m": float(xs[minimum_gap_index[1]]),
            "negative_width_count": int(np.count_nonzero(width < -1e-10)),
            "minimum_width_m": float(width[minimum_width_index]),
            "minimum_width_field": self.WIDTH_FIELDS[minimum_width_index[0]],
            "minimum_width_x_m": float(xs[minimum_width_index[1]]),
            "pass": bool(np.all(gaps >= -1e-10) and np.all(width >= -1e-10)),
        }

    def curve_network_intersection_diagnostics(
        self,
        samples_x: int = 33,
        samples_half: int = 192,
        tolerance_m: float = 0.012,
    ) -> dict[str, Any]:
        """Measure curve-to-section agreement instead of assigning zero.

        Character-curve points are independently compared with the nearest
        point on a densely tessellated transverse section ring at the same x.
        This tests the two exported curve families as realized geometry.
        """
        curve_specs: list[tuple[str, str | None, str, float]] = [
            ("roof_centre", None, "roof_crown_z", 0.0),
            ("roof_rail_left", "roof_rail_halfwidth", "roof_rail_z", -1.0),
            ("roof_rail_right", "roof_rail_halfwidth", "roof_rail_z", 1.0),
            ("glass_shoulder_left", "glass_shoulder_halfwidth", "glass_shoulder_z", -1.0),
            ("glass_shoulder_right", "glass_shoulder_halfwidth", "glass_shoulder_z", 1.0),
            ("belt_left", "belt_halfwidth", "belt_z", -1.0),
            ("belt_right", "belt_halfwidth", "belt_z", 1.0),
            ("shoulder_left", "shoulder_halfwidth", "shoulder_z", -1.0),
            ("shoulder_right", "shoulder_halfwidth", "shoulder_z", 1.0),
            ("rocker_left", "rocker_halfwidth", "rocker_z", -1.0),
            ("rocker_right", "rocker_halfwidth", "rocker_z", 1.0),
            ("underbody_edge_left", "underbody_halfwidth", "underbody_z", -1.0),
            ("underbody_edge_right", "underbody_halfwidth", "underbody_z", 1.0),
        ]
        residuals: list[float] = []
        worst: dict[str, Any] | None = None
        for x in np.linspace(self.rear, self.front, samples_x):
            ring = self.section_ring(float(x), samples_half=samples_half)
            for curve_name, width_name, z_name, sign in curve_specs:
                y = 0.0 if width_name is None else sign * float(self.value(width_name, x))
                point = np.asarray([x, y, float(self.value(z_name, x))], dtype=float)
                distance = float(np.min(np.linalg.norm(ring - point, axis=1)))
                residuals.append(distance)
                if worst is None or distance > worst["residual_m"]:
                    worst = {
                        "curve": curve_name,
                        "x_m": float(x),
                        "residual_m": distance,
                    }
        values = np.asarray(residuals, dtype=float)
        return {
            "schema": "MCSMv2.0.1.CurveNetworkResidual.v1",
            "sample_count": int(len(values)),
            "section_count": int(samples_x),
            "curve_count": int(len(curve_specs)),
            "mean_residual_m": float(values.mean()),
            "rms_residual_m": float(np.sqrt(np.mean(values ** 2))),
            "p95_residual_m": float(np.quantile(values, 0.95)),
            "maximum_residual_m": float(values.max()),
            "tolerance_m": float(tolerance_m),
            "worst": worst,
            "pass": bool(values.max() <= tolerance_m),
        }

    def profile_landmarks(self, x: float) -> list[tuple[str, float, float]]:
        return [
            ("underbody", float(self.value("underbody_halfwidth", x)), float(self.value("underbody_z", x))),
            ("rocker", float(self.value("rocker_halfwidth", x)), float(self.value("rocker_z", x))),
            ("lower_body", float(self.value("lower_halfwidth", x)), float(self.value("lower_z", x))),
            ("shoulder", float(self.value("shoulder_halfwidth", x)), float(self.value("shoulder_z", x))),
            ("belt", float(self.value("belt_halfwidth", x)), float(self.value("belt_z", x))),
            ("glass_shoulder", float(self.value("glass_shoulder_halfwidth", x)), float(self.value("glass_shoulder_z", x))),
            ("roof_rail", float(self.value("roof_rail_halfwidth", x)), float(self.value("roof_rail_z", x))),
            ("roof_crown", 0.0, float(self.value("roof_crown_z", x))),
        ]

    def width_table(self, xs: np.ndarray, zs: np.ndarray) -> np.ndarray:
        table = np.zeros((len(xs), len(zs)), dtype=np.float64)
        for i, x in enumerate(xs):
            pairs = self.profile_landmarks(float(x))
            z_pts = np.asarray([p[2] for p in pairs], dtype=float)
            w_pts = np.asarray([p[1] for p in pairs], dtype=float)
            z_pts = np.maximum.accumulate(z_pts + np.arange(len(z_pts)) * 1e-8)
            table[i] = np.interp(zs, z_pts, w_pts, left=0.0, right=0.0)
        return table

    def character_curves(self, samples: int = 96) -> dict[str, list[list[float]]]:
        xs = np.linspace(self.rear, self.front, samples)
        curves: dict[str, list[list[float]]] = {
            "centre_spine": np.column_stack([
                xs,
                np.zeros_like(xs),
                0.5 * (self.value("underbody_z", xs) + self.value("roof_crown_z", xs)),
            ]).tolist(),
            "roof_centre": np.column_stack([xs, np.zeros_like(xs), self.value("roof_crown_z", xs)]).tolist(),
        }
        for name, width_field, z_field in (
            ("roof_rail", "roof_rail_halfwidth", "roof_rail_z"),
            ("glass_shoulder", "glass_shoulder_halfwidth", "glass_shoulder_z"),
            ("belt", "belt_halfwidth", "belt_z"),
            ("shoulder", "shoulder_halfwidth", "shoulder_z"),
            ("rocker", "rocker_halfwidth", "rocker_z"),
            ("underbody_edge", "underbody_halfwidth", "underbody_z"),
        ):
            widths = self.value(width_field, xs)
            heights = self.value(z_field, xs)
            curves[f"{name}_left"] = np.column_stack([xs, -widths, heights]).tolist()
            curves[f"{name}_right"] = np.column_stack([xs, widths, heights]).tolist()
        return curves

    def section_ring(self, x: float, samples_half: int = 34) -> np.ndarray:
        # Top centre -> right side -> underbody centre.
        pairs = list(reversed(self.profile_landmarks(x)))
        # Preserve the semantic underbody edge and append an explicit centre
        # point.  MCSMv2.0.0 replaced the edge width with zero, so the
        # transverse family could not intersect the exported underbody-edge
        # character curves.
        underbody_z = pairs[-1][2]
        pairs.append(("underbody_centre", 0.0, underbody_z))
        y = np.asarray([p[1] for p in pairs], dtype=float)
        z = np.asarray([p[2] for p in pairs], dtype=float)
        chord = np.sqrt(np.diff(y) ** 2 + np.diff(z) ** 2)
        t = np.concatenate([[0.0], np.cumsum(chord)])
        if t[-1] <= 1e-9:
            raise ValueError("collapsed semantic section")
        t /= t[-1]
        yi = PchipInterpolator(t, y)(np.linspace(0.0, 1.0, samples_half))
        zi = PchipInterpolator(t, z)(np.linspace(0.0, 1.0, samples_half))
        right = np.column_stack([np.full(samples_half, x), yi, zi])
        left = right[-2:0:-1].copy()
        left[:, 1] *= -1.0
        return np.vstack([right, left])

    def semantic_surface_mesh(self, samples_x: int = 72, samples_half: int = 34) -> trimesh.Trimesh:
        xs = np.linspace(self.rear, self.front, samples_x)
        rings = [self.section_ring(float(x), samples_half=samples_half) for x in xs]
        ring_size = len(rings[0])
        vertices = np.vstack(rings)
        faces: list[list[int]] = []
        for i in range(samples_x - 1):
            a0 = i * ring_size
            b0 = (i + 1) * ring_size
            for j in range(ring_size):
                jn = (j + 1) % ring_size
                faces.append([a0 + j, b0 + j, b0 + jn])
                faces.append([a0 + j, b0 + jn, a0 + jn])

        # Close front and rear with centroid fans.
        verts_list = vertices.tolist()
        for ring_index, reverse_winding in ((0, True), (samples_x - 1, False)):
            ring = rings[ring_index]
            centre_idx = len(verts_list)
            verts_list.append(ring.mean(axis=0).tolist())
            start = ring_index * ring_size
            for j in range(ring_size):
                jn = (j + 1) % ring_size
                face = [centre_idx, start + j, start + jn]
                if reverse_winding:
                    face = [centre_idx, start + jn, start + j]
                faces.append(face)
        mesh = trimesh.Trimesh(vertices=np.asarray(verts_list), faces=np.asarray(faces), process=True)
        mesh.fix_normals(multibody=True)
        mesh.visual.material = legacy.pbr("SemanticSurface", (36, 94, 215), 0.25, 0.38, 120)
        return legacy.set_preview(mesh, (0.05, 0.23, 0.85), 0.30, 0.25, 0.38)

    def as_records(self) -> list[dict[str, Any]]:
        return [asdict(station) for station in self.stations]


# =============================================================================
# Hybrid implicit body and engineering model
# =============================================================================


def smooth_max(a: np.ndarray, b: np.ndarray, k: float = 28.0) -> np.ndarray:
    return np.logaddexp(k * a, k * b) / k


@dataclass(frozen=True)
class WheelEnvelope:
    name: str
    axle: str
    side: str
    centre: tuple[float, float, float]
    radius_x: float
    radius_y: float
    radius_z: float
    steer_range_deg: tuple[float, float]
    travel_range: tuple[float, float]
    clearance: float


class ModernCarModelV2:
    """Hybrid semantic-section and implicit-scaffold modern-car model."""

    def __init__(self, variant: VehicleVariantV2):
        self.v = variant
        self.reference_frame = variant.reference_frame
        self.sections = SemanticSectionModel(variant)
        self.front = self.sections.front
        self.rear = self.sections.rear
        self.front_axle = self.sections.front_axle
        self.rear_axle = self.sections.rear_axle
        self.cabin_front = self.sections.cabin_front
        self.cabin_rear = self.sections.cabin_rear
        self.parameter_graph = self._build_parameter_graph()
        self.parameters = self.parameter_graph.evaluate()
        self.wheel_envelopes = self._build_wheel_envelopes()
        self._profile_x = np.linspace(self.rear, self.front, 420)
        self._profile_z = np.linspace(0.0, self.v.height, 260)
        self._profile_width = self.sections.width_table(self._profile_x, self._profile_z)

    def _build_parameter_graph(self) -> ParameterGraph:
        """Build a complete, unit-aware graph of causal model inputs."""
        v = self.v
        graph = ParameterGraph()

        def explicit(name: str, value: ParameterScalar, unit: str = "1", value_type: str | None = None) -> None:
            graph.add_explicit(
                name,
                value,
                parameter_evidence_for_variant(v, name),
                unit=unit,
                value_type=value_type,
            )

        # Package: all dimensions, including the overhangs which were formerly
        # captured by Python closures outside the graph.
        for name in (
            "length", "width", "height", "wheelbase", "track_front", "track_rear",
            "ground_clearance", "front_overhang", "rear_overhang",
        ):
            explicit(name, getattr(v.package, name), "m")

        # Platform and occupant datums.
        for name in PlatformParameters.__dataclass_fields__:
            explicit(name, getattr(v.platform, name), "m")

        # Style fields are dimensionless controlled design variables.
        for name in StyleParameters.__dataclass_fields__:
            explicit(name, getattr(v.style, name), "1")

        # Wheel and suspension package.
        for name in WheelSuspensionParameters.__dataclass_fields__:
            unit = "deg" if name.endswith("_deg") else "m"
            explicit(name, getattr(v.wheel_suspension, name), unit)

        # Powertrain and closure inputs, including categorical topology.
        explicit("powertrain_architecture", v.powertrain.architecture, "enum", "enum")
        explicit("drive_layout", v.powertrain.drive_layout, "enum", "enum")
        explicit("battery_pack", v.powertrain.battery_pack, "bool", "bool")
        explicit("battery_thickness", v.powertrain.battery_thickness, "m")
        explicit("engine_envelope_length", v.powertrain.engine_envelope_length, "m")
        explicit("engine_envelope_width", v.powertrain.engine_envelope_width, "m")
        explicit("engine_envelope_height", v.powertrain.engine_envelope_height, "m")
        explicit("exhaust_count", v.powertrain.exhaust_count, "count", "int")
        for name in ClosureParameters.__dataclass_fields__:
            unit = "deg" if name.endswith("_deg") else "m"
            explicit(name, getattr(v.closures, name), unit)
        explicit("crossover_cladding", v.crossover_cladding, "bool", "bool")

        graph.add_derived(
            "front_axle_x", ["wheelbase"],
            lambda d: 0.5 * float(d["wheelbase"]),
            "wheelbase / 2", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "rear_axle_x", ["wheelbase"],
            lambda d: -0.5 * float(d["wheelbase"]),
            "-wheelbase / 2", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "front_extent_x", ["front_axle_x", "front_overhang"],
            lambda d: float(d["front_axle_x"]) + float(d["front_overhang"]),
            "front_axle_x + front_overhang", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "rear_extent_x", ["rear_axle_x", "rear_overhang"],
            lambda d: float(d["rear_axle_x"]) - float(d["rear_overhang"]),
            "rear_axle_x - rear_overhang", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "resolved_length", ["front_extent_x", "rear_extent_x"],
            lambda d: float(d["front_extent_x"]) - float(d["rear_extent_x"]),
            "front_extent_x - rear_extent_x", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "package_length_residual", ["resolved_length", "length"],
            lambda d: float(d["resolved_length"]) - float(d["length"]),
            "resolved_length - length", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "steer_max_abs_deg", ["steer_min_deg", "steer_max_deg"],
            lambda d: max(abs(float(d["steer_min_deg"])), abs(float(d["steer_max_deg"]))),
            "max(abs(steer_min_deg), abs(steer_max_deg))", DERIVED_EVIDENCE, unit="deg",
        )
        graph.add_derived(
            "travel_down", ["travel_min"],
            lambda d: abs(float(d["travel_min"])),
            "abs(travel_min)", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "travel_up", ["travel_max"],
            lambda d: abs(float(d["travel_max"])),
            "abs(travel_max)", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "front_wheelhouse_rx", ["wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"],
            lambda d: float(d["wheel_radius"]) + 0.5 * float(d["wheel_width"]) * math.sin(math.radians(float(d["steer_max_abs_deg"]))) + float(d["wheelhouse_clearance"]),
            "r + 0.5*w*sin(delta_max) + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "front_wheelhouse_ry", ["wheel_radius", "wheel_width", "steer_max_abs_deg", "wheelhouse_clearance"],
            lambda d: 0.5 * float(d["wheel_width"]) + float(d["wheel_radius"]) * math.sin(math.radians(float(d["steer_max_abs_deg"]))) + float(d["wheelhouse_clearance"]),
            "0.5*w + r*sin(delta_max) + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "front_wheelhouse_rz", ["wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"],
            lambda d: float(d["wheel_radius"]) + max(float(d["travel_down"]), float(d["travel_up"])) + float(d["wheelhouse_clearance"]),
            "r + max(jounce,rebound) + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "rear_wheelhouse_rx", ["wheel_radius", "wheelhouse_clearance"],
            lambda d: float(d["wheel_radius"]) + float(d["wheelhouse_clearance"]),
            "r + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "rear_wheelhouse_ry", ["wheel_width", "wheelhouse_clearance"],
            lambda d: 0.5 * float(d["wheel_width"]) + float(d["wheelhouse_clearance"]),
            "0.5*w + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "rear_wheelhouse_rz", ["wheel_radius", "travel_down", "travel_up", "wheelhouse_clearance"],
            lambda d: float(d["wheel_radius"]) + max(float(d["travel_down"]), float(d["travel_up"])) + float(d["wheelhouse_clearance"]),
            "r + max(jounce,rebound) + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "minimum_roof_from_occupant", ["hpoint_z", "head_clearance"],
            lambda d: float(d["hpoint_z"]) + 0.72 + float(d["head_clearance"]),
            "H-point + seated-head envelope + clearance", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "occupant_roof_margin", ["height", "minimum_roof_from_occupant"],
            lambda d: float(d["height"]) - float(d["minimum_roof_from_occupant"]),
            "height - minimum_roof_from_occupant", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "front_track_body_margin", ["width", "track_front"],
            lambda d: float(d["width"]) - float(d["track_front"]),
            "width - track_front", DERIVED_EVIDENCE, unit="m",
        )
        graph.add_derived(
            "rear_track_body_margin", ["width", "track_rear"],
            lambda d: float(d["width"]) - float(d["track_rear"]),
            "width - track_rear", DERIVED_EVIDENCE, unit="m",
        )
        return graph

    def _build_wheel_envelopes(self) -> list[WheelEnvelope]:
        p = self.parameters
        envelopes: list[WheelEnvelope] = []
        for axle, x, track in (
            ("front", self.front_axle, self.v.track_front),
            ("rear", self.rear_axle, self.v.track_rear),
        ):
            for sign, side in ((-1.0, "left"), (1.0, "right")):
                if axle == "front":
                    rx, ry, rz = p["front_wheelhouse_rx"], p["front_wheelhouse_ry"], p["front_wheelhouse_rz"]
                    steer = (self.v.wheel_suspension.steer_min_deg, self.v.wheel_suspension.steer_max_deg)
                else:
                    rx, ry, rz = p["rear_wheelhouse_rx"], p["rear_wheelhouse_ry"], p["rear_wheelhouse_rz"]
                    steer = (0.0, 0.0)
                envelopes.append(WheelEnvelope(
                    name=f"{axle}_{side}",
                    axle=axle,
                    side=side,
                    centre=(x, sign * track / 2.0, self.v.wheel_radius),
                    radius_x=float(rx),
                    radius_y=float(ry),
                    radius_z=float(rz),
                    steer_range_deg=steer,
                    travel_range=(self.v.wheel_suspension.travel_min, self.v.wheel_suspension.travel_max),
                    clearance=self.v.wheel_suspension.wheelhouse_clearance,
                ))
        return envelopes

    def profile_width(self, x: np.ndarray, z: np.ndarray) -> np.ndarray:
        # Bilinear lookup over the precomputed semantic width field.
        x_flat = np.asarray(x, dtype=float).ravel()
        z_flat = np.asarray(z, dtype=float).ravel()
        ix = np.interp(x_flat, self._profile_x, np.arange(len(self._profile_x)))
        iz = np.interp(z_flat, self._profile_z, np.arange(len(self._profile_z)))
        ix0 = np.clip(np.floor(ix).astype(int), 0, len(self._profile_x) - 1)
        iz0 = np.clip(np.floor(iz).astype(int), 0, len(self._profile_z) - 1)
        ix1 = np.clip(ix0 + 1, 0, len(self._profile_x) - 1)
        iz1 = np.clip(iz0 + 1, 0, len(self._profile_z) - 1)
        tx = ix - ix0
        tz = iz - iz0
        a = self._profile_width[ix0, iz0]
        b = self._profile_width[ix1, iz0]
        c = self._profile_width[ix0, iz1]
        d = self._profile_width[ix1, iz1]
        out = (1 - tx) * (1 - tz) * a + tx * (1 - tz) * b + (1 - tx) * tz * c + tx * tz * d
        return out.reshape(np.broadcast_shapes(np.shape(x), np.shape(z)))

    def _wheel_cavity_on_axes(
        self,
        X: np.ndarray,
        Y: np.ndarray,
        Z: np.ndarray,
        envelope: WheelEnvelope,
    ) -> np.ndarray:
        """Conservative signed cavity for the declared tyre pose sweep.

        The tyre is treated as a finite-width circular section.  Suspension
        travel sweeps that circle along a vertical segment; front steering
        rotates the finite-width section about Z.  The union is sampled over
        steering angle and dilated by the declared wheelhouse clearance plus a
        small tessellation allowance.  Negative values lie inside the cavity.
        """
        cx, cy, cz = envelope.centre
        dx = X - cx
        dy = Y - cy
        z_low = cz + min(envelope.travel_range)
        z_high = cz + max(envelope.travel_range)
        dz = np.where(Z < z_low, z_low - Z, np.where(Z > z_high, Z - z_high, 0.0))

        # The allowance compensates for finite marching-cubes and triangle
        # discretization. It is evidence metadata, not a hidden body rescale.
        mesh_allowance = 0.012
        radial_limit = self.v.wheel_radius + envelope.clearance + mesh_allowance
        lateral_limit = 0.5 * self.v.wheel_width + envelope.clearance + mesh_allowance
        steer_values = (
            np.linspace(envelope.steer_range_deg[0], envelope.steer_range_deg[1], 7)
            if envelope.axle == "front"
            else np.asarray([0.0])
        )

        cavity = None
        for steer_deg in steer_values:
            steer = math.radians(float(steer_deg))
            cos_s = math.cos(steer)
            sin_s = math.sin(steer)
            longitudinal = cos_s * dx + sin_s * dy
            lateral = -sin_s * dx + cos_s * dy
            radial_distance = np.sqrt(longitudinal * longitudinal + dz * dz) - radial_limit
            lateral_distance = np.abs(lateral) - lateral_limit
            pose_cavity = np.maximum(radial_distance, lateral_distance)
            cavity = pose_cavity if cavity is None else np.minimum(cavity, pose_cavity)
        assert cavity is not None
        return cavity

    def _wheel_cavity_at_points(
        self,
        points: np.ndarray,
        envelope: WheelEnvelope,
    ) -> np.ndarray:
        pts = np.asarray(points, dtype=float)
        cx, cy, cz = envelope.centre
        dx = pts[:, 0] - cx
        dy = pts[:, 1] - cy
        z_low = cz + min(envelope.travel_range)
        z_high = cz + max(envelope.travel_range)
        z = pts[:, 2]
        dz = np.where(z < z_low, z_low - z, np.where(z > z_high, z - z_high, 0.0))
        mesh_allowance = 0.012
        radial_limit = self.v.wheel_radius + envelope.clearance + mesh_allowance
        lateral_limit = 0.5 * self.v.wheel_width + envelope.clearance + mesh_allowance
        steer_values = (
            np.linspace(envelope.steer_range_deg[0], envelope.steer_range_deg[1], 7)
            if envelope.axle == "front"
            else np.asarray([0.0])
        )
        cavity = np.full(len(pts), np.inf, dtype=float)
        for steer_deg in steer_values:
            steer = math.radians(float(steer_deg))
            cos_s = math.cos(steer)
            sin_s = math.sin(steer)
            longitudinal = cos_s * dx + sin_s * dy
            lateral = -sin_s * dx + cos_s * dy
            radial_distance = np.sqrt(longitudinal * longitudinal + dz * dz) - radial_limit
            lateral_distance = np.abs(lateral) - lateral_limit
            cavity = np.minimum(cavity, np.maximum(radial_distance, lateral_distance))
        return cavity

    def field_on_axes(self, xs: np.ndarray, ys: np.ndarray, zs: np.ndarray, include_wheelhouses: bool = True) -> np.ndarray:
        width = self.sections.width_table(xs, zs)
        side = np.abs(ys)[None, :, None] - width[:, None, :]
        zmin = self.sections.value("underbody_z", xs)[:, None, None] - zs[None, None, :]
        zmax = zs[None, None, :] - self.sections.value("roof_crown_z", xs)[:, None, None]
        rear_cap = (self.rear - xs)[:, None, None]
        front_cap = (xs - self.front)[:, None, None]
        field = smooth_max(side, zmin)
        field = smooth_max(field, zmax)
        field = smooth_max(field, rear_cap)
        field = smooth_max(field, front_cap)

        if include_wheelhouses:
            X = xs[:, None, None]
            Y = ys[None, :, None]
            Z = zs[None, None, :]
            for envelope in self.wheel_envelopes:
                cavity = self._wheel_cavity_on_axes(X, Y, Z, envelope)
                field = np.maximum(field, -cavity)
        return field

    def evaluate_field_points(self, points: np.ndarray, include_wheelhouses: bool = True) -> np.ndarray:
        pts = np.asarray(points, dtype=float)
        x, y, z = pts[:, 0], pts[:, 1], pts[:, 2]
        width = self.profile_width(x, z)
        side = np.abs(y) - width
        zmin = self.sections.value("underbody_z", x) - z
        zmax = z - self.sections.value("roof_crown_z", x)
        field = smooth_max(side, zmin)
        field = smooth_max(field, zmax)
        field = smooth_max(field, self.rear - x)
        field = smooth_max(field, x - self.front)
        if include_wheelhouses:
            for envelope in self.wheel_envelopes:
                cavity = self._wheel_cavity_at_points(pts, envelope)
                field = np.maximum(field, -cavity)
        return field

    @staticmethod
    def _mesh_from_field(field: np.ndarray, axes: tuple[np.ndarray, np.ndarray, np.ndarray]) -> trimesh.Trimesh:
        xs, ys, zs = axes
        vertices, faces, _, _ = marching_cubes(
            field.astype(np.float32),
            level=0.0,
            spacing=(xs[1] - xs[0], ys[1] - ys[0], zs[1] - zs[0]),
        )
        vertices += np.asarray([xs[0], ys[0], zs[0]])
        return trimesh.Trimesh(vertices=vertices, faces=faces, process=True)

    def build_body(
        self,
        resolution: tuple[int, int, int],
        calibration_iterations: int = 3,
    ) -> tuple[trimesh.Trimesh, dict[str, Any]]:
        """Extract the body in the resolved vehicle frame without post scaling.

        Marching-cubes discretization and smoothing slightly shrink the zero set.
        v2.0.1 compensates by iteratively pre-warping the sampled scalar field.
        The final mesh is never affinely corrected after extraction, preventing
        body/curve/BIW datum desynchronization.
        """
        dense_x = np.linspace(self.rear, self.front, 401)
        target_min = np.asarray([
            self.rear,
            -self.v.width / 2.0,
            float(np.min(self.sections.value("underbody_z", dense_x))),
        ])
        target_max = np.asarray([self.front, self.v.width / 2.0, self.v.height])
        target_center = 0.5 * (target_min + target_max)
        target_extent = target_max - target_min
        margins = np.asarray([0.04, self.v.width * 0.07, 0.025])
        axes = tuple(
            np.linspace(target_min[i] - margins[i], target_max[i] + margins[i], resolution[i])
            for i in range(3)
        )

        prewarp_scale = np.ones(3, dtype=float)
        prewarp_translation = np.zeros(3, dtype=float)
        history: list[dict[str, Any]] = []

        def realize(scale: np.ndarray, translation: np.ndarray) -> trimesh.Trimesh:
            evaluation_axes = tuple(
                (axes[i] - translation[i]) / scale[i]
                for i in range(3)
            )
            field = self.field_on_axes(*evaluation_axes, include_wheelhouses=True)
            candidate = self._mesh_from_field(field, axes)
            trimesh.smoothing.filter_taubin(
                candidate, lamb=0.38, nu=-0.41, iterations=1
            )
            return candidate

        for iteration in range(max(0, calibration_iterations)):
            candidate = realize(prewarp_scale, prewarp_translation)
            bounds = candidate.bounds.copy()
            extent = bounds[1] - bounds[0]
            centre = bounds.mean(axis=0)
            if np.any(extent <= 1e-12) or not np.isfinite(extent).all():
                raise RuntimeError("invalid calibration mesh extent")
            ratio = target_extent / extent
            prewarp_translation = target_center + ratio * (prewarp_translation - centre)
            prewarp_scale = ratio * prewarp_scale
            history.append({
                "iteration": iteration + 1,
                "measured_bounds": bounds.round(8).tolist(),
                "measured_extents": extent.round(8).tolist(),
                "update_ratio": ratio.round(10).tolist(),
                "accumulated_prewarp_scale": prewarp_scale.round(10).tolist(),
                "accumulated_prewarp_translation": prewarp_translation.round(10).tolist(),
            })

        mesh = realize(prewarp_scale, prewarp_translation)
        mesh.process(validate=True)
        mesh.fix_normals(multibody=True)

        # Marching cubes can emit microscopic closed islands around tangent
        # cavity contacts.  They are numerical artifacts rather than intended
        # body components, so retain the unique dominant connected component
        # and record every discarded island.
        components = sorted(
            mesh.split(only_watertight=False),
            key=lambda item: len(item.faces),
            reverse=True,
        )
        removed_components = [
            {
                "faces": int(len(item.faces)),
                "absolute_volume_m3": float(abs(item.volume)),
                "bounds": item.bounds.round(9).tolist(),
            }
            for item in components[1:]
        ]
        if len(components) > 1:
            significant = [
                item for item in components[1:]
                if len(item.faces) >= 32 and abs(item.volume) >= 1e-8
            ]
            if significant:
                raise RuntimeError(
                    f"body extraction produced {1 + len(significant)} significant components"
                )
            mesh = components[0].copy()
            mesh.process(validate=True)
            mesh.fix_normals(multibody=True)

        final_bounds = mesh.bounds.copy()
        final_extent = final_bounds[1] - final_bounds[0]
        bound_error = np.vstack([final_bounds[0] - target_min, final_bounds[1] - target_max])
        extent_error = final_extent - target_extent

        material = legacy.pbr(f"MCSMv2Body_{self.v.key}", self.v.body_rgb, 0.68, 0.25)
        mesh.visual.material = material
        rgb = tuple(c / 255.0 for c in self.v.body_rgb)
        legacy.set_preview(mesh, rgb, 1.0, 0.68, 0.25)
        evidence = {
            "schema": "MCSMv2.0.1.FieldCalibration.v1",
            "method": "iterative inverse affine field prewarp",
            "calibration_iterations": int(calibration_iterations),
            "calibration_history": history,
            "removed_numerical_components": removed_components,
            "removed_numerical_component_count": len(removed_components),
            "target_bounds": np.vstack([target_min, target_max]).round(8).tolist(),
            "final_bounds": final_bounds.round(8).tolist(),
            "final_extents": final_extent.round(8).tolist(),
            "final_bound_error_m": bound_error.round(9).tolist(),
            "maximum_final_bound_error_m": float(np.max(np.abs(bound_error))),
            "final_extent_error_m": extent_error.round(9).tolist(),
            "maximum_final_extent_error_m": float(np.max(np.abs(extent_error))),
            "field_prewarp_scale": prewarp_scale.round(10).tolist(),
            "field_prewarp_translation": prewarp_translation.round(10).tolist(),
            "maximum_field_prewarp_fraction": float(np.max(np.abs(prewarp_scale - 1.0))),
            "post_mesh_affine_correction_applied": False,
            "maximum_post_mesh_correction_fraction": 0.0,
            # Compatibility field: now explicitly means post-mesh correction.
            "maximum_numerical_correction_fraction": 0.0,
            "resolved_geometry_transform": np.eye(4).tolist(),
        }
        return mesh, evidence

    def extract_glass(self, body: trimesh.Trimesh) -> trimesh.Trimesh:
        centres = body.triangles_center
        normals = body.face_normals
        x = centres[:, 0]
        z = centres[:, 2]
        belt = self.sections.value("belt_z", x)
        roof = self.sections.value("roof_crown_z", x)
        mask = (
            (x >= self.cabin_rear - 0.03)
            & (x <= self.cabin_front + 0.03)
            & (z > belt + 0.018)
            & (z < roof - 0.018)
            & (np.abs(normals[:, 2]) < 0.83)
        )
        patch = body.submesh([np.flatnonzero(mask)], append=True, repair=False)
        if patch is None or len(patch.faces) == 0:
            raise RuntimeError("glass extraction produced no faces")
        patch.vertices += patch.vertex_normals * 0.004
        patch.remove_unreferenced_vertices()
        patch.visual.material = legacy.MAT_GLASS
        return legacy.set_preview(patch, (0.035, 0.055, 0.075), 0.58, 0.15, 0.10)

    def extract_panels(self, body: trimesh.Trimesh) -> tuple[dict[str, trimesh.Trimesh], dict[str, Any]]:
        c = body.triangles_center
        n = body.face_normals
        x, y, z = c[:, 0], c[:, 1], c[:, 2]
        belt = self.sections.value("belt_z", x)
        rocker = self.sections.value("rocker_z", x)
        a_x = self.cabin_front - 0.08
        b_x = self.cabin_rear + 0.61 * (self.cabin_front - self.cabin_rear)
        c_x = self.cabin_rear + 0.18 * (self.cabin_front - self.cabin_rear)
        side = np.abs(n[:, 1]) > 0.35
        left = y < 0
        right = y > 0
        lower_side = side & (z > rocker + 0.015) & (z < belt + 0.025)
        masks: dict[str, np.ndarray] = {
            "hood": (x > a_x) & (n[:, 2] > 0.28) & (z > belt - 0.02),
            "roof": (x >= self.cabin_rear) & (x <= self.cabin_front) & (n[:, 2] > 0.52) & (z > belt + 0.12),
            "front_door_left": lower_side & left & (x >= b_x) & (x <= a_x),
            "front_door_right": lower_side & right & (x >= b_x) & (x <= a_x),
            "rear_door_left": lower_side & left & (x >= c_x) & (x < b_x),
            "rear_door_right": lower_side & right & (x >= c_x) & (x < b_x),
            "rear_quarter_left": side & left & (x < c_x) & (x > self.rear + 0.20),
            "rear_quarter_right": side & right & (x < c_x) & (x > self.rear + 0.20),
            "rear_hatch": (x < self.cabin_rear + 0.12) & ((n[:, 0] < -0.20) | (n[:, 2] > 0.35)),
            "front_bumper": x > self.front - 0.25,
            "rear_bumper": x < self.rear + 0.25,
        }
        palette = {
            "hood": (0.95, 0.20, 0.12),
            "roof": (0.88, 0.65, 0.08),
            "front_door_left": (0.35, 0.75, 0.28),
            "front_door_right": (0.35, 0.75, 0.28),
            "rear_door_left": (0.12, 0.72, 0.65),
            "rear_door_right": (0.12, 0.72, 0.65),
            "rear_quarter_left": (0.50, 0.30, 0.78),
            "rear_quarter_right": (0.50, 0.30, 0.78),
            "rear_hatch": (0.35, 0.42, 0.82),
            "front_bumper": (0.25, 0.38, 0.72),
            "rear_bumper": (0.30, 0.34, 0.62),
        }
        panels: dict[str, trimesh.Trimesh] = {}
        graph_nodes: list[dict[str, Any]] = []
        for name, mask in masks.items():
            ids = np.flatnonzero(mask)
            if len(ids) < 8:
                continue
            patch = body.submesh([ids], append=True, repair=False)
            if patch is None or len(patch.faces) == 0:
                continue
            patch.vertices += patch.vertex_normals * 0.0018
            color = palette[name]
            patch.visual.material = legacy.pbr(
                f"Panel_{name}",
                tuple(int(255 * v) for v in color),
                0.45,
                0.28,
                220,
            )
            legacy.set_preview(patch, color, 0.72, 0.45, 0.28)
            panels[name] = patch
            graph_nodes.append({
                "name": name,
                "face_count": int(len(patch.faces)),
                "closure": name in {
                    "hood", "front_door_left", "front_door_right",
                    "rear_door_left", "rear_door_right", "rear_hatch",
                },
            })
        graph = {
            "schema": "MCSMv2.PanelPatchGraph.v1",
            "nodes": graph_nodes,
            "relations": [
                {"a": "hood", "b": "front_bumper", "relation": "panel_gap"},
                {"a": "front_door_left", "b": "rear_door_left", "relation": "panel_gap"},
                {"a": "front_door_right", "b": "rear_door_right", "relation": "panel_gap"},
                {"a": "rear_door_left", "b": "rear_quarter_left", "relation": "panel_gap"},
                {"a": "rear_door_right", "b": "rear_quarter_right", "relation": "panel_gap"},
                {"a": "rear_hatch", "b": "rear_bumper", "relation": "panel_gap"},
            ],
        }
        return panels, graph

    def semantic_sections(self, count: int = 21) -> list[dict[str, Any]]:
        records: list[dict[str, Any]] = []
        for index, x in enumerate(np.linspace(self.rear, self.front, count)):
            ring = self.sections.section_ring(float(x), samples_half=32)
            records.append({
                "index": index,
                "u": float((x - self.rear) / (self.front - self.rear)),
                "x_m": float(x),
                "points_xyz_m": ring.round(6).tolist(),
            })
        return records

    def projected_frontal_area(self, nx: int = 120, ny: int = 240, nz: int = 210) -> float:
        xs = np.linspace(self.rear, self.front, nx)
        ys = np.linspace(-self.v.width / 2.0, self.v.width / 2.0, ny)
        zs = np.linspace(0.0, self.v.height, nz)
        min_field = np.full((ny, nz), np.inf, dtype=np.float32)
        for chunk in np.array_split(xs, 8):
            field = self.field_on_axes(chunk, ys, zs, include_wheelhouses=True)
            min_field = np.minimum(min_field, field.min(axis=0).astype(np.float32))
        return float(np.count_nonzero(min_field <= 0.0) * (ys[1] - ys[0]) * (zs[1] - zs[0]))


# =============================================================================
# Engineering assembly layers
# =============================================================================

MAT_BIW = legacy.pbr("BodyInWhite", (155, 35, 28), 0.72, 0.36)
MAT_OCCUPANT = legacy.pbr("OccupantEnvelope", (35, 120, 235), 0.05, 0.48, 105)
MAT_WHEEL_ENVELOPE = legacy.pbr("WheelEnvelope", (220, 35, 180), 0.05, 0.40, 90)
MAT_POWERTRAIN = legacy.pbr("PowertrainEnvelope", (245, 150, 25), 0.25, 0.42, 145)
MAT_SEAM = legacy.pbr("PanelSeam", (8, 8, 10), 0.15, 0.40)


def _oriented_box_between(a: Sequence[float], b: Sequence[float], width: float, height: float,
                          material: PBRMaterial, preview: tuple[float, float, float]) -> trimesh.Trimesh:
    a_arr = np.asarray(a, dtype=float)
    b_arr = np.asarray(b, dtype=float)
    direction = b_arr - a_arr
    length = float(np.linalg.norm(direction))
    if length <= 1e-8:
        raise ValueError("zero-length structural member")
    mesh = trimesh.creation.box(extents=[length, width, height])
    # Box local X aligns with direction.
    transform = trimesh.geometry.align_vectors([1.0, 0.0, 0.0], direction / length)
    transform[:3, 3] = 0.5 * (a_arr + b_arr)
    mesh.apply_transform(transform)
    mesh.visual.material = material
    return legacy.set_preview(mesh, preview, 1.0, 0.72, 0.36)


def build_biw(model: ModernCarModelV2) -> tuple[dict[str, trimesh.Trimesh], dict[str, Any]]:
    v = model.v
    members: dict[str, trimesh.Trimesh] = {}
    floor_z = v.platform.floor_height
    floor = legacy.make_box(
        [v.wheelbase * 0.93, v.width * 0.64, 0.070],
        [0.0, 0.0, floor_z],
        MAT_BIW,
        (0.58, 0.08, 0.055),
    )
    members["floor_pan"] = floor

    # Rockers.
    for sign, side in ((-1.0, "left"), (1.0, "right")):
        y = sign * v.width * 0.43
        members[f"rocker_{side}"] = legacy.make_box(
            [v.wheelbase * 0.96, 0.095, 0.125],
            [0.0, y, floor_z + 0.06],
            MAT_BIW,
            (0.62, 0.07, 0.045),
        )

    # Rails and crossmembers.
    for sign, side in ((-1.0, "left"), (1.0, "right")):
        y = sign * v.width * 0.19
        members[f"front_rail_{side}"] = _oriented_box_between(
            [model.front_axle - 0.15, y, floor_z + 0.02],
            [model.front - 0.18, y * 0.78, floor_z + 0.16],
            0.080, 0.095, MAT_BIW, (0.64, 0.07, 0.045),
        )
        members[f"rear_rail_{side}"] = _oriented_box_between(
            [model.rear_axle + 0.15, y, floor_z + 0.02],
            [model.rear + 0.22, y * 0.82, floor_z + 0.13],
            0.075, 0.090, MAT_BIW, (0.64, 0.07, 0.045),
        )
    for name, x in (
        ("front_crossmember", model.front_axle - 0.18),
        ("centre_crossmember", 0.0),
        ("rear_crossmember", model.rear_axle + 0.18),
    ):
        members[name] = legacy.make_box(
            [0.10, v.width * 0.70, 0.10],
            [x, 0.0, floor_z + 0.07],
            MAT_BIW,
            (0.60, 0.07, 0.045),
        )

    # Pillars and roof rails derive from semantic curves.
    a_x = model.cabin_front - 0.08
    b_x = model.cabin_rear + 0.61 * (model.cabin_front - model.cabin_rear)
    c_x = model.cabin_rear + 0.18 * (model.cabin_front - model.cabin_rear)
    for sign, side in ((-1.0, "left"), (1.0, "right")):
        for role, x in (("a_pillar", a_x), ("b_pillar", b_x), ("c_pillar", c_x)):
            lower_y = sign * float(model.sections.value("rocker_halfwidth", x)) * 0.98
            lower_z = float(model.sections.value("rocker_z", x))
            upper_y = sign * float(model.sections.value("roof_rail_halfwidth", x))
            upper_z = float(model.sections.value("roof_rail_z", x))
            members[f"{role}_{side}"] = _oriented_box_between(
                [x, lower_y, lower_z],
                [x, upper_y, upper_z],
                0.075 if role != "b_pillar" else 0.10,
                0.085,
                MAT_BIW,
                (0.66, 0.08, 0.05),
            )
        roof_points = []
        for x in np.linspace(model.cabin_rear, model.cabin_front, 14):
            roof_points.append([
                x,
                sign * float(model.sections.value("roof_rail_halfwidth", x)),
                float(model.sections.value("roof_rail_z", x)),
            ])
        members[f"roof_rail_{side}"] = legacy.polyline_tube(
            roof_points, 0.042, MAT_BIW, (0.64, 0.07, 0.045), sections=10
        )

    # Suspension towers, centred on wheel datums.
    for axle, x, track in (("front", model.front_axle, v.track_front), ("rear", model.rear_axle, v.track_rear)):
        for sign, side in ((-1.0, "left"), (1.0, "right")):
            members[f"{axle}_tower_{side}"] = legacy.make_ellipsoid(
                [0.22, 0.16, 0.28],
                [x, sign * track * 0.36, floor_z + 0.32],
                MAT_BIW,
                (0.60, 0.07, 0.045),
            )

    edges = [
        ("front_rail_left", "front_crossmember"),
        ("front_rail_right", "front_crossmember"),
        ("rear_rail_left", "rear_crossmember"),
        ("rear_rail_right", "rear_crossmember"),
        ("rocker_left", "floor_pan"), ("rocker_right", "floor_pan"),
        ("front_crossmember", "floor_pan"), ("centre_crossmember", "floor_pan"),
        ("rear_crossmember", "floor_pan"),
    ]
    for side in ("left", "right"):
        edges.extend([
            (f"a_pillar_{side}", f"rocker_{side}"),
            (f"b_pillar_{side}", f"rocker_{side}"),
            (f"c_pillar_{side}", f"rocker_{side}"),
            (f"a_pillar_{side}", f"roof_rail_{side}"),
            (f"b_pillar_{side}", f"roof_rail_{side}"),
            (f"c_pillar_{side}", f"roof_rail_{side}"),
            (f"front_tower_{side}", f"front_rail_{side}"),
            (f"rear_tower_{side}", f"rear_rail_{side}"),
        ])
    graph = {
        "schema": "MCSMv2.BodyInWhiteGraph.v1",
        "nodes": [{"name": name, "role": name} for name in members],
        "edges": [{"a": a, "b": b, "join": "spot_weld_and_adhesive"} for a, b in edges],
    }
    return members, graph


def build_occupant_envelopes(model: ModernCarModelV2) -> dict[str, trimesh.Trimesh]:
    v = model.v
    result: dict[str, trimesh.Trimesh] = {}
    for row, x in (("front", v.platform.front_hpoint_x), ("rear", v.platform.rear_hpoint_x)):
        for sign, side in ((-1.0, "left"), (1.0, "right")):
            y = sign * v.platform.seat_lateral
            torso = legacy.make_ellipsoid(
                [0.24, 0.18, 0.34],
                [x, y, v.platform.hpoint_z + 0.31],
                MAT_OCCUPANT,
                (0.05, 0.34, 0.90),
                opacity=0.34,
            )
            head = legacy.make_ellipsoid(
                [0.11, 0.09, 0.13],
                [x - 0.04, y, min(v.height - 0.25, v.platform.eye_z + 0.03)],
                MAT_OCCUPANT,
                (0.05, 0.34, 0.90),
                opacity=0.34,
            )
            result[f"occupant_{row}_{side}_torso"] = torso
            result[f"occupant_{row}_{side}_head"] = head
    return result


def build_functional_envelopes(model: ModernCarModelV2) -> dict[str, trimesh.Trimesh]:
    v = model.v
    result: dict[str, trimesh.Trimesh] = {}
    if v.powertrain.battery_pack:
        result["battery_envelope"] = legacy.make_box(
            [v.wheelbase * 0.70, v.width * 0.62, v.powertrain.battery_thickness],
            [0.0, 0.0, v.ground_clearance + v.powertrain.battery_thickness * 0.55],
            MAT_POWERTRAIN,
            (0.95, 0.45, 0.05),
            opacity=0.48,
        )
        result["front_eaxle_envelope"] = legacy.make_box(
            [0.42, 0.72, 0.32],
            [model.front_axle, 0.0, v.platform.floor_height + 0.15],
            MAT_POWERTRAIN, (0.95, 0.45, 0.05), opacity=0.48,
        )
        result["rear_eaxle_envelope"] = legacy.make_box(
            [0.42, 0.72, 0.32],
            [model.rear_axle, 0.0, v.platform.floor_height + 0.15],
            MAT_POWERTRAIN, (0.95, 0.45, 0.05), opacity=0.48,
        )
    else:
        result["engine_envelope"] = legacy.make_box(
            [v.powertrain.engine_envelope_length, v.powertrain.engine_envelope_width, v.powertrain.engine_envelope_height],
            [model.front_axle + 0.33, 0.0, v.platform.floor_height + 0.28],
            MAT_POWERTRAIN,
            (0.95, 0.45, 0.05),
            opacity=0.48,
        )
        result["rear_differential_envelope"] = legacy.make_box(
            [0.34, 0.46, 0.28],
            [model.rear_axle, 0.0, v.platform.floor_height + 0.10],
            MAT_POWERTRAIN, (0.95, 0.45, 0.05), opacity=0.48,
        )
    return result


def build_wheel_envelope_meshes(model: ModernCarModelV2) -> dict[str, trimesh.Trimesh]:
    result: dict[str, trimesh.Trimesh] = {}
    for envelope in model.wheel_envelopes:
        mesh = legacy.make_ellipsoid(
            [envelope.radius_x, envelope.radius_y, envelope.radius_z],
            envelope.centre,
            MAT_WHEEL_ENVELOPE,
            (0.86, 0.08, 0.72),
            opacity=0.26,
        )
        result[f"wheel_envelope_{envelope.name}"] = mesh
    return result


# =============================================================================
# Scene construction
# =============================================================================


def add_geometry(scene: trimesh.Scene, name: str, mesh: trimesh.Trimesh,
                 transform: np.ndarray | None = None) -> None:
    scene.add_geometry(mesh, geom_name=name, node_name=name, transform=transform)


def build_vehicle_scene(
    model: ModernCarModelV2,
    body_resolution: tuple[int, int, int],
) -> tuple[trimesh.Scene, trimesh.Trimesh, dict[str, Any], dict[str, trimesh.Trimesh], dict[str, Any]]:
    v = model.v
    scene = trimesh.Scene()
    body, body_evidence = model.build_body(body_resolution)
    add_geometry(scene, "body", body)

    glass = model.extract_glass(body)
    add_geometry(scene, "greenhouse_glass", glass)

    panels, panel_graph = model.extract_panels(body)
    for name, panel in panels.items():
        add_geometry(scene, f"panel_{name}", panel)

    # Wheels, brakes and visible equipment reuse the tested MCSMv1 helpers.
    lv = v.to_legacy_variant()
    for axle_name, x, track in (
        ("front", model.front_axle, v.track_front),
        ("rear", model.rear_axle, v.track_rear),
    ):
        for side in (-1.0, 1.0):
            label = f"{axle_name}_{'R' if side > 0 else 'L'}"
            for name, mesh in legacy.wheel_assembly(lv, x, side * track / 2.0, v.wheel_radius, label):
                add_geometry(scene, name, mesh)

    # Exterior details remain visual/packaging objects at this LOD.
    grille = legacy.make_box(
        [0.060, v.width * 0.52 * v.grille_scale, 0.27 * v.grille_scale],
        [model.front - 0.015, 0.0, v.height * 0.33],
        legacy.MAT_BLACK, (0.025, 0.025, 0.028),
    )
    add_geometry(scene, "front_grille", grille)
    for sign, side in ((-1.0, "L"), (1.0, "R")):
        add_geometry(scene, f"headlamp_{side}", legacy.make_ellipsoid(
            [0.055, v.width * 0.16, 0.070],
            [model.front - 0.020, sign * v.width * 0.28, v.height * 0.52],
            legacy.MAT_LIGHT, (0.78, 0.90, 1.0),
        ))
        add_geometry(scene, f"taillamp_{side}", legacy.make_ellipsoid(
            [0.050, v.width * 0.13, 0.060],
            [model.rear + 0.020, sign * v.width * 0.29, v.height * 0.54],
            legacy.MAT_REARLIGHT, (0.90, 0.03, 0.03),
        ))
        add_geometry(scene, f"mirror_{side}", legacy.make_ellipsoid(
            [0.10, 0.12, 0.055],
            [model.cabin_front - 0.05, sign * v.width * 0.53, v.height * 0.67],
            legacy.MAT_BLACK, (0.02, 0.022, 0.026),
        ))
    add_geometry(scene, "front_splitter", legacy.make_box(
        [0.18 * v.splitter_scale, v.width * 0.85, 0.045],
        [model.front - 0.07, 0.0, max(0.07, v.ground_clearance * 0.65)],
        legacy.MAT_BLACK, (0.02, 0.02, 0.023),
    ))
    for sign, side in ((-1.0, "L"), (1.0, "R")):
        add_geometry(scene, f"side_skirt_{side}", legacy.make_box(
            [v.wheelbase * 0.72, 0.055 if not v.crossover_cladding else 0.085, 0.060 if not v.crossover_cladding else 0.100],
            [0.0, sign * v.width * 0.495, v.ground_clearance + 0.06],
            legacy.MAT_BLACK, (0.02, 0.02, 0.024),
        ))
    spoiler_x = model.cabin_rear + 0.10
    spoiler_z = min(v.height - 0.015, float(model.sections.value("roof_crown_z", spoiler_x)) + 0.015)
    add_geometry(scene, "rear_spoiler", legacy.make_box(
        [0.28 * v.spoiler_scale, v.width * 0.70, 0.045 * max(0.8, v.spoiler_scale)],
        [spoiler_x, 0.0, spoiler_z], legacy.MAT_BLACK, (0.02, 0.02, 0.024),
    ))
    add_geometry(scene, "rear_diffuser", legacy.make_box(
        [0.16, v.width * 0.66, 0.13],
        [model.rear + 0.03, 0.0, max(0.10, v.ground_clearance + 0.03)],
        legacy.MAT_BLACK, (0.02, 0.02, 0.024),
    ))
    add_geometry(scene, "underbody", legacy.make_box(
        [v.wheelbase * 0.78, v.width * 0.65, 0.060],
        [0.0, 0.0, v.ground_clearance], legacy.MAT_BLACK, (0.03, 0.032, 0.038),
    ))
    if v.powertrain.battery_pack:
        add_geometry(scene, "battery_pack", legacy.make_box(
            [v.wheelbase * 0.72, v.width * 0.66, v.powertrain.battery_thickness],
            [0.0, 0.0, v.ground_clearance + v.powertrain.battery_thickness * 0.55],
            legacy.MAT_BATTERY, (0.15, 0.16, 0.18),
        ))
    if v.exhaust_count:
        per_side = max(1, v.exhaust_count // 2)
        for sign, side in ((-1.0, "L"), (1.0, "R")):
            for i in range(per_side):
                tip = trimesh.creation.cylinder(radius=0.042, height=0.13, sections=20)
                tip.apply_transform(trimesh.transformations.rotation_matrix(np.pi / 2, [0, 1, 0]))
                tip.apply_translation([model.rear - 0.03, sign * v.width * (0.27 + i * 0.075), v.ground_clearance + 0.10])
                tip.visual.material = legacy.MAT_METAL
                legacy.set_preview(tip, (0.45, 0.47, 0.50), 1.0, 0.85, 0.22)
                add_geometry(scene, f"exhaust_{side}_{i}", tip)

    return scene, body, body_evidence, panels, panel_graph


def build_engineering_scene(
    model: ModernCarModelV2,
    body: trimesh.Trimesh,
    semantic_surface: trimesh.Trimesh,
) -> tuple[trimesh.Scene, dict[str, Any], dict[str, Any]]:
    scene = trimesh.Scene()
    body_transparent = body.copy()
    body_transparent.visual.material = legacy.pbr(
        "TransparentBody", model.v.body_rgb, 0.25, 0.30, 70
    )
    legacy.set_preview(body_transparent, tuple(c / 255 for c in model.v.body_rgb), 0.22, 0.25, 0.30)
    add_geometry(scene, "body_reference", body_transparent)
    add_geometry(scene, "semantic_surface", semantic_surface)

    biw, biw_graph = build_biw(model)
    for name, mesh in biw.items():
        add_geometry(scene, name, mesh)
    occupants = build_occupant_envelopes(model)
    for name, mesh in occupants.items():
        add_geometry(scene, name, mesh)
    functional = build_functional_envelopes(model)
    for name, mesh in functional.items():
        add_geometry(scene, name, mesh)
    wheel_env = build_wheel_envelope_meshes(model)
    for name, mesh in wheel_env.items():
        add_geometry(scene, name, mesh)

    curve_data = model.sections.character_curves(samples=88)
    for name, points in curve_data.items():
        try:
            tube = legacy.polyline_tube(points, 0.0055, legacy.MAT_CURVE_BLUE, (0.04, 0.20, 0.90), sections=7)
            add_geometry(scene, f"curve_{name}", tube)
        except ValueError:
            pass
    for idx, section in enumerate(model.semantic_sections(count=15)[::2]):
        points = section["points_xyz_m"]
        tube = legacy.polyline_tube(points, 0.004, legacy.MAT_SECTION_YELLOW, (0.95, 0.75, 0.05), sections=6)
        add_geometry(scene, f"section_{idx:02d}", tube)
    return scene, biw_graph, {
        "occupants": sorted(occupants),
        "functional_envelopes": sorted(functional),
        "wheel_envelopes": [asdict(e) for e in model.wheel_envelopes],
    }


# =============================================================================
# Validation
# =============================================================================


def mesh_hash(mesh: trimesh.Trimesh) -> str:
    h = hashlib.sha256()
    h.update(np.asarray(mesh.vertices, dtype=np.float64).round(8).tobytes())
    h.update(np.asarray(mesh.faces, dtype=np.int64).tobytes())
    return h.hexdigest()


def _graph_connected(nodes: Sequence[str], edges: Sequence[Mapping[str, str]]) -> bool:
    if not nodes:
        return True
    adjacency = {node: set() for node in nodes}
    for edge in edges:
        a, b = edge["a"], edge["b"]
        if a in adjacency and b in adjacency:
            adjacency[a].add(b)
            adjacency[b].add(a)
    seen: set[str] = set()
    stack = [nodes[0]]
    while stack:
        node = stack.pop()
        if node in seen:
            continue
        seen.add(node)
        stack.extend(adjacency[node] - seen)
    return len(seen) == len(nodes)


def _segment_triangle_hits(
    p0: np.ndarray,
    p1: np.ndarray,
    triangles: np.ndarray,
    epsilon: float,
) -> np.ndarray:
    direction = p1 - p0
    edge1 = triangles[:, 1] - triangles[:, 0]
    edge2 = triangles[:, 2] - triangles[:, 0]
    h = np.cross(direction, edge2)
    a = np.einsum("ij,ij->i", edge1, h)
    valid = np.abs(a) > epsilon
    f = np.zeros_like(a)
    f[valid] = 1.0 / a[valid]
    s = p0 - triangles[:, 0]
    u = f * np.einsum("ij,ij->i", s, h)
    q = np.cross(s, edge1)
    v = f * np.einsum("ij,ij->i", direction, q)
    parameter = f * np.einsum("ij,ij->i", edge2, q)
    return (
        valid
        & (u >= -epsilon)
        & (v >= -epsilon)
        & (u + v <= 1.0 + epsilon)
        & (parameter >= -epsilon)
        & (parameter <= 1.0 + epsilon)
    )


def _orientation_2d(a: np.ndarray, b: np.ndarray, c: np.ndarray) -> float:
    return float((b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]))


def _on_segment_2d(a: np.ndarray, b: np.ndarray, p: np.ndarray, epsilon: float) -> bool:
    return bool(
        min(a[0], b[0]) - epsilon <= p[0] <= max(a[0], b[0]) + epsilon
        and min(a[1], b[1]) - epsilon <= p[1] <= max(a[1], b[1]) + epsilon
        and abs(_orientation_2d(a, b, p)) <= epsilon
    )


def _segments_intersect_2d(
    a: np.ndarray,
    b: np.ndarray,
    c: np.ndarray,
    d: np.ndarray,
    epsilon: float,
) -> bool:
    o1 = _orientation_2d(a, b, c)
    o2 = _orientation_2d(a, b, d)
    o3 = _orientation_2d(c, d, a)
    o4 = _orientation_2d(c, d, b)
    strict = (
        ((o1 > epsilon and o2 < -epsilon) or (o1 < -epsilon and o2 > epsilon))
        and ((o3 > epsilon and o4 < -epsilon) or (o3 < -epsilon and o4 > epsilon))
    )
    if strict:
        return True
    return bool(
        (abs(o1) <= epsilon and _on_segment_2d(a, b, c, epsilon))
        or (abs(o2) <= epsilon and _on_segment_2d(a, b, d, epsilon))
        or (abs(o3) <= epsilon and _on_segment_2d(c, d, a, epsilon))
        or (abs(o4) <= epsilon and _on_segment_2d(c, d, b, epsilon))
    )


def _point_in_triangle_2d(point: np.ndarray, triangle: np.ndarray, epsilon: float) -> bool:
    orientation = [
        _orientation_2d(triangle[i], triangle[(i + 1) % 3], point)
        for i in range(3)
    ]
    return bool(
        all(value >= -epsilon for value in orientation)
        or all(value <= epsilon for value in orientation)
    )


def _triangles_overlap_2d(a: np.ndarray, b: np.ndarray, epsilon: float) -> bool:
    for i in range(3):
        for j in range(3):
            if _segments_intersect_2d(
                a[i], a[(i + 1) % 3], b[j], b[(j + 1) % 3], epsilon
            ):
                return True
    return _point_in_triangle_2d(a[0], b, epsilon) or _point_in_triangle_2d(b[0], a, epsilon)


def _triangle_pair_intersections(
    triangle_a: np.ndarray,
    triangle_b: np.ndarray,
    epsilon: float,
) -> np.ndarray:
    hit = np.zeros(len(triangle_a), dtype=bool)
    for edge in range(3):
        hit |= _segment_triangle_hits(
            triangle_a[:, edge], triangle_a[:, (edge + 1) % 3], triangle_b, epsilon
        )
    for edge in range(3):
        hit |= _segment_triangle_hits(
            triangle_b[:, edge], triangle_b[:, (edge + 1) % 3], triangle_a, epsilon
        )

    normal_a = np.cross(triangle_a[:, 1] - triangle_a[:, 0], triangle_a[:, 2] - triangle_a[:, 0])
    normal_b = np.cross(triangle_b[:, 1] - triangle_b[:, 0], triangle_b[:, 2] - triangle_b[:, 0])
    length_a = np.linalg.norm(normal_a, axis=1)
    length_b = np.linalg.norm(normal_b, axis=1)
    parallel_error = np.linalg.norm(np.cross(normal_a, normal_b), axis=1)
    parallel = parallel_error <= 1e-8 * (length_a * length_b + 1e-30)
    plane_distance = np.abs(np.einsum(
        "ij,ij->i", triangle_b[:, 0] - triangle_a[:, 0], normal_a
    )) / (length_a + 1e-30)
    coplanar_indices = np.flatnonzero((~hit) & parallel & (plane_distance <= epsilon * 10.0))
    for index in coplanar_indices:
        axis = int(np.argmax(np.abs(normal_a[index])))
        keep = [item for item in range(3) if item != axis]
        hit[index] = _triangles_overlap_2d(
            triangle_a[index][:, keep],
            triangle_b[index][:, keep],
            epsilon * 10.0,
        )
    return hit


def mesh_self_intersection_screen(
    mesh: trimesh.Trimesh,
    neighbour_count: int = 32,
    maximum_candidate_pairs: int = 250_000,
) -> dict[str, Any]:
    """Independent local triangle-intersection screen using a centroid BVH proxy.

    Intersecting triangles must have overlapping bounding spheres.  A cKDTree
    supplies nearby candidates; adjacent faces sharing a vertex are excluded;
    the remaining pairs receive an exact edge/triangle and coplanar 2D test.
    """
    triangles = np.asarray(mesh.triangles, dtype=float)
    face_count = len(triangles)
    if face_count < 2:
        return {
            "candidate_pair_count": 0,
            "checked_pair_count": 0,
            "intersection_count": 0,
            "complete": True,
            "pairs": [],
            "pass": True,
        }
    centroids = triangles.mean(axis=1)
    radius = np.linalg.norm(triangles - centroids[:, None, :], axis=2).max(axis=1)
    tree = cKDTree(centroids)
    k = min(neighbour_count + 1, face_count)
    try:
        distance, neighbour = tree.query(centroids, k=k, workers=1)
    except TypeError:  # older SciPy
        distance, neighbour = tree.query(centroids, k=k)
    if k == 1:
        neighbour = neighbour[:, None]
        distance = distance[:, None]
    left = np.repeat(np.arange(face_count), k - 1)
    right = neighbour[:, 1:].reshape(-1)
    separation = distance[:, 1:].reshape(-1)
    mask = (right > left) & (separation <= radius[left] + radius[right] + 1e-12)
    left = left[mask]
    right = right[mask]

    bounds_min = triangles.min(axis=1)
    bounds_max = triangles.max(axis=1)
    overlap = np.all(
        (bounds_max[left] >= bounds_min[right] - 1e-12)
        & (bounds_max[right] >= bounds_min[left] - 1e-12),
        axis=1,
    )
    left = left[overlap]
    right = right[overlap]

    face_left = mesh.faces[left]
    face_right = mesh.faces[right]
    shares_vertex = (face_left[:, :, None] == face_right[:, None, :]).any(axis=(1, 2))
    left = left[~shares_vertex]
    right = right[~shares_vertex]
    candidate_count = int(len(left))

    complete = candidate_count <= maximum_candidate_pairs
    if not complete:
        select = np.linspace(0, candidate_count - 1, maximum_candidate_pairs).astype(int)
        left = left[select]
        right = right[select]

    characteristic = max(float(np.max(mesh.extents)), 1.0)
    epsilon = characteristic * 1e-10
    hit = _triangle_pair_intersections(triangles[left], triangles[right], epsilon)
    intersections = np.flatnonzero(hit)
    pairs = [
        [int(left[index]), int(right[index])]
        for index in intersections[:32]
    ]
    return {
        "candidate_pair_count": candidate_count,
        "checked_pair_count": int(len(left)),
        "intersection_count": int(len(intersections)),
        "complete": bool(complete),
        "pairs": pairs,
        "pass": bool(complete and len(intersections) == 0),
    }


def mesh_integrity_report(mesh: trimesh.Trimesh, self_intersection: bool = True) -> dict[str, Any]:
    characteristic = max(float(np.max(mesh.extents)), 1.0)
    areas = np.asarray(mesh.area_faces, dtype=float)
    degenerate = int(np.count_nonzero(areas <= characteristic * characteristic * 1e-14))
    sorted_faces = np.sort(np.asarray(mesh.faces, dtype=np.int64), axis=1)
    duplicate_faces = int(len(sorted_faces) - len(np.unique(sorted_faces, axis=0)))
    edge_counts = np.bincount(
        mesh.edges_unique_inverse,
        minlength=len(mesh.edges_unique),
    )
    boundary_edges = int(np.count_nonzero(edge_counts == 1))
    nonmanifold_edges = int(np.count_nonzero(edge_counts > 2))
    components = trimesh.graph.connected_components(
        mesh.face_adjacency,
        min_len=1,
        nodes=np.arange(len(mesh.faces)),
        engine="scipy",
    )
    intersection = (
        mesh_self_intersection_screen(mesh)
        if self_intersection
        else {"pass": None, "complete": False, "intersection_count": None}
    )
    finite = bool(np.isfinite(mesh.vertices).all() and np.isfinite(mesh.faces).all())
    positive_volume = bool(np.isfinite(mesh.volume) and mesh.volume > 0.0)
    passed = bool(
        finite
        and mesh.is_watertight
        and mesh.is_winding_consistent
        and mesh.is_volume
        and positive_volume
        and degenerate == 0
        and duplicate_faces == 0
        and boundary_edges == 0
        and nonmanifold_edges == 0
        and len(components) == 1
        and (intersection["pass"] is True)
    )
    return {
        "finite": finite,
        "watertight": bool(mesh.is_watertight),
        "winding_consistent": bool(mesh.is_winding_consistent),
        "is_volume": bool(mesh.is_volume),
        "positive_volume": positive_volume,
        "volume_m3": float(mesh.volume),
        "surface_area_m2": float(mesh.area),
        "degenerate_face_count": degenerate,
        "duplicate_face_count": duplicate_faces,
        "boundary_edge_count": boundary_edges,
        "nonmanifold_edge_count": nonmanifold_edges,
        "connected_component_count": int(len(components)),
        "self_intersection_screen": intersection,
        "pass": passed,
    }


class MeshSpatialIndex:
    """Dependency-free point containment and approximate clearance index."""

    def __init__(self, mesh: trimesh.Trimesh):
        if not mesh.is_watertight:
            raise ValueError("point containment requires a watertight mesh")
        self.mesh = mesh
        self.triangles = np.asarray(mesh.triangles, dtype=float)
        self.characteristic = max(float(np.max(mesh.extents)), 1.0)
        yz = self.triangles[:, :, 1:3]
        self.yz_centres = yz.mean(axis=1)
        self.yz_radii = np.linalg.norm(yz - self.yz_centres[:, None, :], axis=2).max(axis=1)
        self.yz_min = yz.min(axis=1)
        self.yz_max = yz.max(axis=1)
        self.yz_tree = cKDTree(self.yz_centres)
        self.maximum_yz_radius = float(self.yz_radii.max())
        self.vertex_tree = cKDTree(np.asarray(mesh.vertices, dtype=float))
        self.vertex_faces = np.asarray(mesh.vertex_faces, dtype=np.int64)

    def contains_points(self, points: np.ndarray) -> np.ndarray:
        query = np.asarray(points, dtype=float)
        result = np.zeros(len(query), dtype=bool)
        epsilon = self.characteristic * 1e-9
        ray_tolerance = self.characteristic * 1e-7
        direction = np.asarray([1.0, 0.0, 0.0])
        for index, original in enumerate(query):
            point = original.copy()
            # Deterministic sub-micron jitter avoids rays exactly following a
            # mesh vertex or edge without altering engineering-scale results.
            point[1] += epsilon * (0.37 + (index % 7) * 0.11)
            point[2] += epsilon * (0.53 + (index % 11) * 0.07)
            candidates = np.asarray(self.yz_tree.query_ball_point(
                point[1:3], self.maximum_yz_radius + epsilon
            ), dtype=np.int64)
            if len(candidates) == 0:
                continue
            delta = self.yz_centres[candidates] - point[1:3]
            radial = np.linalg.norm(delta, axis=1) <= self.yz_radii[candidates] + epsilon
            bounds = np.all(
                (point[1:3] >= self.yz_min[candidates] - epsilon)
                & (point[1:3] <= self.yz_max[candidates] + epsilon),
                axis=1,
            )
            candidates = candidates[radial & bounds]
            if len(candidates) == 0:
                continue
            triangles = self.triangles[candidates]
            edge1 = triangles[:, 1] - triangles[:, 0]
            edge2 = triangles[:, 2] - triangles[:, 0]
            h = np.cross(np.broadcast_to(direction, edge2.shape), edge2)
            a = np.einsum("ij,ij->i", edge1, h)
            valid = np.abs(a) > epsilon
            f = np.zeros_like(a)
            f[valid] = 1.0 / a[valid]
            s = point - triangles[:, 0]
            u = f * np.einsum("ij,ij->i", s, h)
            q = np.cross(s, edge1)
            v = f * np.einsum("ij,ij->i", np.broadcast_to(direction, q.shape), q)
            parameter = f * np.einsum("ij,ij->i", edge2, q)
            hit = (
                valid
                & (u >= -epsilon)
                & (v >= -epsilon)
                & (u + v <= 1.0 + epsilon)
                & (parameter > epsilon)
            )
            distances = np.sort(parameter[hit])
            if len(distances) == 0:
                continue
            unique_count = 1 + int(np.count_nonzero(np.diff(distances) > ray_tolerance))
            result[index] = bool(unique_count % 2 == 1)
        return result

    def distance_to_surface(self, points: np.ndarray, nearest_vertices: int = 12) -> np.ndarray:
        query = np.asarray(points, dtype=float)
        k = min(nearest_vertices, len(self.mesh.vertices))
        _, vertex_ids = self.vertex_tree.query(query, k=k)
        if k == 1:
            vertex_ids = vertex_ids[:, None]
        distances = np.empty(len(query), dtype=float)
        for index, point in enumerate(query):
            face_ids = self.vertex_faces[vertex_ids[index]].reshape(-1)
            face_ids = np.unique(face_ids[face_ids >= 0])
            triangles = self.triangles[face_ids]
            repeated = np.broadcast_to(point, (len(triangles), 3))
            closest = trimesh.triangles.closest_point(triangles, repeated)
            distances[index] = float(np.min(np.linalg.norm(closest - point, axis=1)))
        return distances


def _occupant_sample_points(model: ModernCarModelV2) -> np.ndarray:
    samples: list[list[float]] = []
    v = model.v

    def add_ellipsoid(center: Sequence[float], radii: Sequence[float], n_theta: int, n_phi: int) -> None:
        cx, cy, cz = center
        rx, ry, rz = radii
        for theta in np.linspace(0, 2 * np.pi, n_theta, endpoint=False):
            for phi in np.linspace(-np.pi / 2, np.pi / 2, n_phi):
                samples.append([
                    cx + rx * math.cos(phi) * math.cos(theta),
                    cy + ry * math.cos(phi) * math.sin(theta),
                    cz + rz * math.sin(phi),
                ])

    for x in (v.platform.front_hpoint_x, v.platform.rear_hpoint_x):
        for sign in (-1.0, 1.0):
            y0 = sign * v.platform.seat_lateral
            add_ellipsoid([x, y0, v.platform.hpoint_z + 0.31], [0.24, 0.18, 0.34], 18, 9)
            add_ellipsoid(
                [x - 0.04, y0, min(v.height - 0.25, v.platform.eye_z + 0.03)],
                [0.11, 0.09, 0.13], 16, 8,
            )
    return np.asarray(samples, dtype=float)


def _tyre_sweep_sample_points(model: ModernCarModelV2, envelope: WheelEnvelope) -> np.ndarray:
    v = model.v
    if envelope.axle == "front":
        steer_values = np.linspace(
            v.wheel_suspension.steer_min_deg,
            v.wheel_suspension.steer_max_deg,
            3,
        )
    else:
        steer_values = np.asarray([0.0])
    travel_values = np.asarray([
        v.wheel_suspension.travel_min,
        0.0,
        v.wheel_suspension.travel_max,
    ])
    lateral_values = np.asarray([-0.5 * v.wheel_width, 0.0, 0.5 * v.wheel_width])
    points: list[list[float]] = []
    cx, cy, cz = envelope.centre
    for steer_deg in steer_values:
        steer = math.radians(float(steer_deg))
        cos_s = math.cos(steer)
        sin_s = math.sin(steer)
        for travel in travel_values:
            for theta in np.linspace(0.0, 2.0 * np.pi, 28, endpoint=False):
                radial_x = v.wheel_radius * math.cos(theta)
                radial_z = v.wheel_radius * math.sin(theta)
                for lateral in lateral_values:
                    x_rotated = cos_s * radial_x - sin_s * lateral
                    y_rotated = sin_s * radial_x + cos_s * lateral
                    points.append([
                        cx + x_rotated,
                        cy + y_rotated,
                        cz + float(travel) + radial_z,
                    ])
    return np.asarray(points, dtype=float)


def validate_occupants(
    model: ModernCarModelV2,
    body: trimesh.Trimesh,
    spatial: MeshSpatialIndex | None = None,
) -> dict[str, Any]:
    spatial = spatial or MeshSpatialIndex(body)
    samples = _occupant_sample_points(model)
    inside = spatial.contains_points(samples)
    clearance = spatial.distance_to_surface(samples)
    inside_fraction = float(np.mean(inside))
    return {
        "schema": "MCSMv2.0.1.FinalMeshOccupantCheck.v1",
        "sample_count": int(len(samples)),
        "inside_fraction": inside_fraction,
        "minimum_surface_distance_m": float(clearance.min()),
        "p05_surface_distance_m": float(np.quantile(clearance, 0.05)),
        "pass": bool(inside_fraction >= 0.94),
        "basis": "independent final-mesh ray parity and triangle distance",
        "model": "four torso ellipsoids plus four head ellipsoids",
    }


def validate_wheel_envelopes(
    model: ModernCarModelV2,
    body: trimesh.Trimesh,
    spatial: MeshSpatialIndex | None = None,
) -> dict[str, Any]:
    spatial = spatial or MeshSpatialIndex(body)
    records: list[dict[str, Any]] = []
    all_pass = True
    for envelope in model.wheel_envelopes:
        points = _tyre_sweep_sample_points(model, envelope)
        inside = spatial.contains_points(points)
        distances = spatial.distance_to_surface(points)
        outside_fraction = float(np.mean(~inside))
        minimum_distance = float(distances.min())
        p01_distance = float(np.quantile(distances, 0.01))
        clearance_tolerance = 0.003
        collision_free_pass = bool(outside_fraction >= 0.985 and minimum_distance >= 0.0015)
        clearance_target_pass = bool(
            minimum_distance >= float(envelope.clearance) - clearance_tolerance
        )
        passed = bool(collision_free_pass and clearance_target_pass)
        all_pass = all_pass and passed
        records.append({
            "name": envelope.name,
            "sample_count": int(len(points)),
            "outside_final_body_fraction": outside_fraction,
            "minimum_surface_distance_m": minimum_distance,
            "p01_surface_distance_m": p01_distance,
            "declared_clearance_m": float(envelope.clearance),
            "clearance_tolerance_m": clearance_tolerance,
            "collision_free_pass": collision_free_pass,
            "clearance_target_pass": clearance_target_pass,
            "pass": passed,
        })
    return {
        "schema": "MCSMv2.0.1.FinalMeshTyreSweepCheck.v1",
        "envelopes": records,
        "pass": bool(all_pass),
        "basis": "sampled tyre surface over declared steer and suspension poses against final mesh",
    }


def _assurance_record(
    v0: bool,
    v1: bool,
    v2: bool,
    static_envelopes: bool,
    release_gate: bool,
) -> dict[str, Any]:
    if v0 and v1 and v2:
        achieved = "V2"
        label = "Package and semantic geometry consistency"
    elif v0 and v1:
        achieved = "V1"
        label = "Deterministic manifold mesh integrity"
    elif v0:
        achieved = "V0"
        label = "Finite parameters and terminating algorithms"
    else:
        achieved = "UNRESOLVED"
        label = "Integrity review required"
    return {
        "schema": "MCSMv2.0.1.Assurance.v1",
        "achieved_level": achieved,
        "achieved_label": label,
        "release_gate_pass": bool(release_gate),
        "levels": {
            "V0": {"pass": bool(v0), "claim": "finite, framed, acyclic parameter model"},
            "V1": {"pass": bool(v1), "claim": "watertight, wound, non-self-intersecting final body mesh"},
            "V2": {"pass": bool(v2), "claim": "package, continuous sections, curve network, datums and BIW graph consistent"},
            "V3": {
                "pass": False,
                "partial_static_envelopes": bool(static_envelopes),
                "claim": "independent closure, glass and suspension kinematics",
                "note": "Static occupant and sampled tyre checks exist; full closure/glass/suspension kinematics remain deferred.",
            },
            "V4": {"pass": False, "claim": "manufacturing and structural analysis"},
            "V5": {"pass": False, "claim": "CFD, crash, thermal, ergonomic and physical validation"},
        },
        "known_limitations": [
            "panel patches remain semantic mesh masks rather than exclusive surface-domain partitions",
            "BIW validation is graph connectivity rather than geometric joint/load-path analysis",
            "semantic-surface to implicit-field agreement is diagnostic, not yet constrained",
            "inverse fitting remains a synthetic semantic metric demonstration",
        ],
    }


def validate_model(
    model: ModernCarModelV2,
    body: trimesh.Trimesh,
    semantic_surface: trimesh.Trimesh,
    body_evidence: Mapping[str, Any],
    scene: trimesh.Scene,
    panel_graph: Mapping[str, Any],
    biw_graph: Mapping[str, Any],
) -> dict[str, Any]:
    v = model.v
    dense_x = np.linspace(model.rear, model.front, 4001)
    target_min = np.asarray([
        model.rear,
        -v.width / 2.0,
        float(np.min(model.sections.value("underbody_z", dense_x))),
    ])
    target_max = np.asarray([model.front, v.width / 2.0, v.height])
    target_bounds = np.vstack([target_min, target_max])
    bounds = body.bounds
    extents = bounds[1] - bounds[0]
    target_extents = target_max - target_min
    bound_error = bounds - target_bounds
    extent_error = extents - target_extents
    package_tolerance = 0.0025
    package_pass = bool(
        np.max(np.abs(bound_error)) <= package_tolerance
        and np.max(np.abs(extent_error)) <= package_tolerance
    )

    finite_scene = all(
        np.isfinite(mesh.vertices).all() and np.isfinite(mesh.faces).all()
        for _, mesh in legacy._mesh_world_instances(scene)
    )
    parameter_acyclic = True
    parameter_error = None
    try:
        model.parameter_graph.evaluate()
    except Exception as exc:  # pragma: no cover
        parameter_acyclic = False
        parameter_error = str(exc)

    frame_validation = model.reference_frame.validate()
    continuous_sections = model.sections.continuous_order_diagnostics(samples=4001)
    curve_network = model.sections.curve_network_intersection_diagnostics()
    body_integrity = mesh_integrity_report(body, self_intersection=True)
    semantic_integrity = mesh_integrity_report(semantic_surface, self_intersection=True)
    semantic_field = np.abs(model.evaluate_field_points(
        np.asarray(semantic_surface.vertices), include_wheelhouses=False
    ))
    semantic_alignment = {
        "schema": "MCSMv2.0.1.SemanticImplicitAlignment.v1",
        "vertex_count": int(len(semantic_field)),
        "median_abs_residual_m": float(np.median(semantic_field)),
        "mean_abs_residual_m": float(np.mean(semantic_field)),
        "p95_abs_residual_m": float(np.quantile(semantic_field, 0.95)),
        "maximum_abs_residual_m": float(np.max(semantic_field)),
        "gate": "diagnostic only in v2.0.1",
    }

    spatial = MeshSpatialIndex(body)
    occupant = validate_occupants(model, body, spatial)
    wheel = validate_wheel_envelopes(model, body, spatial)
    biw_nodes = [node["name"] for node in biw_graph.get("nodes", [])]
    biw_connected = _graph_connected(biw_nodes, biw_graph.get("edges", []))
    no_post_mesh_correction = not bool(body_evidence.get("post_mesh_affine_correction_applied", True))
    field_prewarp_ok = float(body_evidence.get("maximum_field_prewarp_fraction", math.inf)) < 0.02

    v0 = bool(
        frame_validation["valid"]
        and parameter_acyclic
        and finite_scene
        and continuous_sections["pass"]
    )
    v1 = bool(v0 and body_integrity["pass"])
    v2 = bool(
        v1
        and package_pass
        and curve_network["pass"]
        and semantic_integrity["pass"]
        and biw_connected
        and no_post_mesh_correction
        and field_prewarp_ok
    )
    static_envelopes = bool(occupant["pass"] and wheel["pass"])
    release_gate = bool(v2 and static_envelopes)
    assurance = _assurance_record(v0, v1, v2, static_envelopes, release_gate)

    return {
        "schema": "MCSMv2.0.1.Validation.v2",
        "version": MCSM_VERSION,
        "variant": v.key,
        "label": v.label,
        "reference_frame": model.reference_frame.as_dict(),
        "body_vertices": int(len(body.vertices)),
        "body_faces": int(len(body.faces)),
        "body_watertight": bool(body.is_watertight),
        "body_winding_consistent": bool(body.is_winding_consistent),
        "body_is_volume": bool(body.is_volume),
        "exterior_envelope_volume_m3": float(body.volume),
        "body_volume_m3": float(body.volume),
        "body_surface_area_m2": float(body.area),
        "body_bounds_m": bounds.round(6).tolist(),
        "body_extents_m": extents.round(6).tolist(),
        "target_bounds_m": target_bounds.round(6).tolist(),
        "bound_error_m": bound_error.round(8).tolist(),
        "dimension_error_m": np.abs(extent_error).round(8).tolist(),
        "package_tolerance_m": package_tolerance,
        "package_pass": package_pass,
        "numerical_package_resolution": dict(body_evidence),
        "finite_body": body_integrity["finite"],
        "finite_scene": finite_scene,
        "final_body_mesh_integrity": body_integrity,
        "semantic_surface_mesh_integrity": semantic_integrity,
        "semantic_surface_implicit_alignment": semantic_alignment,
        "parameter_graph_acyclic": parameter_acyclic,
        "parameter_graph_error": parameter_error,
        "parameter_graph_node_count": len(model.parameter_graph.nodes),
        "semantic_station_count": len(model.sections.stations),
        "semantic_section_continuous_order": continuous_sections,
        "semantic_section_z_ordered": continuous_sections["negative_z_gap_count"] == 0,
        "semantic_section_widths_nonnegative": continuous_sections["negative_width_count"] == 0,
        "character_curve_count": len(model.sections.character_curves()),
        "curve_network_intersection": curve_network,
        "curve_network_intersection_residual_m": curve_network["maximum_residual_m"],
        "panel_patch_count": len(panel_graph.get("nodes", [])),
        "panel_model_status": "semantic masks; exclusive domain partition deferred",
        "body_in_white_node_count": len(biw_nodes),
        "body_in_white_edge_count": len(biw_graph.get("edges", [])),
        "body_in_white_connected": biw_connected,
        "occupant_containment": occupant,
        "wheel_envelope_clearance": wheel,
        "frontal_area_proxy_m2": model.projected_frontal_area(),
        "body_sha256": mesh_hash(body),
        "assurance": assurance,
        "release_gate_pass": release_gate,
        # Compatibility alias. Consumers should migrate to assurance/release_gate_pass.
        "pass": release_gate,
    }


# =============================================================================
# Inverse fit demonstration
# =============================================================================


def section_metrics(variant: VehicleVariantV2) -> dict[str, float]:
    model = SemanticSectionModel(variant)
    b_x = model.cabin_rear + 0.61 * (model.cabin_front - model.cabin_rear)
    rear_quarter_x = model.rear_axle + 0.34
    return {
        "roof_height": float(model.value("roof_crown_z", b_x)),
        "b_pillar_roof_width": 2.0 * float(model.value("roof_rail_halfwidth", b_x)),
        "rear_quarter_shoulder_width": 2.0 * float(model.value("shoulder_halfwidth", rear_quarter_x)),
        "hood_height": float(model.value("roof_crown_z", model.front - 0.55)),
    }


def inverse_fit_demo(reference: VehicleVariantV2) -> dict[str, Any]:
    target = section_metrics(reference)
    start_style = replace(
        reference.style,
        roof_scale=reference.style.roof_scale * 0.94,
        tumblehome=reference.style.tumblehome * 0.82,
        rear_haunch_amplitude=reference.style.rear_haunch_amplitude * 0.55,
        hood_wedge=reference.style.hood_wedge * 1.25,
    )
    start = replace(reference, style=start_style, key="fit_start", label="Inverse-fit start")

    names = ["roof_scale", "tumblehome", "rear_haunch_amplitude", "hood_wedge"]
    x0 = np.asarray([getattr(start.style, name) for name in names], dtype=float)
    scales = np.asarray([0.05, 0.12, 0.06, 0.25], dtype=float)

    def residual(x: np.ndarray) -> np.ndarray:
        style = replace(start.style, **{name: float(value) for name, value in zip(names, x)})
        candidate = replace(start, style=style)
        metrics = section_metrics(candidate)
        return np.asarray([
            (metrics["roof_height"] - target["roof_height"]) / 0.015,
            (metrics["b_pillar_roof_width"] - target["b_pillar_roof_width"]) / 0.020,
            (metrics["rear_quarter_shoulder_width"] - target["rear_quarter_shoulder_width"]) / 0.020,
            (metrics["hood_height"] - target["hood_height"]) / 0.020,
        ])

    lower = np.maximum(x0 - 3.0 * scales, np.asarray([0.80, 0.05, 0.0, 0.10]))
    upper = np.minimum(x0 + 3.0 * scales, np.asarray([1.20, 0.65, 0.16, 1.00]))
    result = least_squares(residual, x0, bounds=(lower, upper), max_nfev=90, xtol=1e-10, ftol=1e-10)
    fitted_style = replace(start.style, **{name: float(value) for name, value in zip(names, result.x)})
    fitted = replace(start, style=fitted_style)
    return {
        "schema": "MCSMv2.InverseFitDemo.v1",
        "target_metrics": target,
        "initial_parameters": {name: float(value) for name, value in zip(names, x0)},
        "initial_metrics": section_metrics(start),
        "fitted_parameters": {name: float(value) for name, value in zip(names, result.x)},
        "fitted_metrics": section_metrics(fitted),
        "initial_residual_norm": float(np.linalg.norm(residual(x0))),
        "final_residual_norm": float(np.linalg.norm(result.fun)),
        "success": bool(result.success),
        "message": result.message,
        "evaluations": int(result.nfev),
        "note": "This fits semantic section metrics, not pixels or production CAD.",
    }


# =============================================================================
# Preview and export helpers
# =============================================================================


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    return legacy.font(size, bold)


def mcsmv2_contact_sheet(label: str, variant: VehicleVariantV2, images: Mapping[str, Path], output: Path) -> None:
    cells = {key: Image.open(path).convert("RGB") for key, path in images.items() if path.exists()}
    if len(cells) < 4:
        return
    cw, ch, header = 1100, 700, 165
    canvas = Image.new("RGB", (cw * 2, ch * 2 + header), "white")
    draw = ImageDraw.Draw(canvas)
    draw.text((40, 18), f"MCSMv2 · {label}", fill=(18, 23, 31), font=font(42, True))
    draw.text(
        (42, 78),
        f"L {variant.length*1000:.0f}  W {variant.width*1000:.0f}  H {variant.height*1000:.0f}  "
        f"WB {variant.wheelbase*1000:.0f} mm · semantic sections + implicit scaffold",
        fill=(65, 72, 82), font=font(25),
    )
    positions = {"iso": (0, header), "side": (cw, header), "front": (0, header + ch), "top": (cw, header + ch)}
    for key, pos in positions.items():
        image = cells[key].resize((cw, ch), Image.Resampling.LANCZOS)
        canvas.paste(image, pos)
        draw.rectangle([pos[0], pos[1], pos[0] + cw - 1, pos[1] + ch - 1], outline=(190, 195, 203), width=2)
        draw.text((pos[0] + 25, pos[1] + 18), key.upper(), fill=(26, 32, 42), font=font(25, True))
    canvas.save(output)


def family_poster(items: Sequence[tuple[VehicleVariantV2, Path]], output: Path) -> None:
    cell_w, cell_h, header = 1200, 780, 180
    canvas = Image.new("RGB", (cell_w * 2, cell_h * 2 + header), "white")
    draw = ImageDraw.Draw(canvas)
    draw.text((45, 23), "MCSMv2 HYBRID MODERN-CAR FAMILY", fill=(17, 22, 30), font=font(45, True))
    draw.text((47, 91), "Semantic sections · character curves · implicit scaffold · engineering envelopes", fill=(70, 77, 88), font=font(27))
    for index, (variant, path) in enumerate(items):
        image = Image.open(path).convert("RGB").resize((cell_w, cell_h), Image.Resampling.LANCZOS)
        x = (index % 2) * cell_w
        y = header + (index // 2) * cell_h
        canvas.paste(image, (x, y))
        draw.rectangle([x, y, x + cell_w - 1, y + cell_h - 1], outline=(188, 194, 202), width=2)
        draw.text((x + 30, y + 25), variant.label, fill=(22, 27, 35), font=font(31, True))
        draw.text((x + 31, y + 71), f"{variant.length*1000:.0f} × {variant.width*1000:.0f} × {variant.height*1000:.0f} mm", fill=(65, 72, 82), font=font(22))
    canvas.save(output)


def side_curve_comparison(variants: Mapping[str, VehicleVariantV2], output: Path) -> None:
    # Matplotlib is deliberately avoided; Pillow is enough for a deterministic
    # engineering-style side-curve comparison.
    width, height = 2450, 1100
    canvas = Image.new("RGB", (width, height), "white")
    draw = ImageDraw.Draw(canvas)
    draw.text((50, 25), "MCSMv2 SEMANTIC SIDE-CURVE COMPARISON", fill=(18, 22, 30), font=font(38, True))
    draw.text((52, 82), "Roof centre, belt, shoulder and rocker curves derived from the same section schema", fill=(70, 77, 87), font=font(23))
    colors = {
        "reference": (190, 15, 8), "track": (100, 30, 175),
        "aero": (20, 95, 200), "crossover": (20, 145, 92),
    }
    rows = list(variants.items())
    for row, (key, variant) in enumerate(rows):
        model = SemanticSectionModel(variant)
        x0, y0 = 120, 185 + row * 205
        box_w, box_h = 1500, 150
        draw.rectangle([x0, y0, x0 + box_w, y0 + box_h], outline=(190, 195, 202), width=2)
        curves = model.character_curves(samples=110)
        for curve_name in ("roof_centre", "belt_left", "shoulder_left", "rocker_left"):
            pts = np.asarray(curves[curve_name])
            px = x0 + (pts[:, 0] - model.rear) / (model.front - model.rear) * box_w
            py = y0 + box_h - pts[:, 2] / variant.height * box_h
            col = colors[key]
            if curve_name == "belt_left": col = tuple(min(255, c + 45) for c in col)
            elif curve_name == "shoulder_left": col = tuple(max(0, c - 35) for c in col)
            elif curve_name == "rocker_left": col = (80, 85, 95)
            draw.line(list(zip(px.tolist(), py.tolist())), fill=col, width=4)
        draw.text((x0 + box_w + 35, y0 + 40), variant.label, fill=(30, 35, 45), font=font(24, True))
        draw.text((x0 + box_w + 35, y0 + 80), f"L {variant.length*1000:.0f} · H {variant.height*1000:.0f}", fill=(78, 84, 94), font=font(20))
    canvas.save(output)


def export_scene(scene: trimesh.Scene, glb: Path, obj: Path | None = None) -> None:
    glb.write_bytes(scene.export(file_type="glb"))
    if obj is not None:
        obj.write_text(scene.export(file_type="obj"), encoding="utf-8")


# =============================================================================
# Grammar and documentation
# =============================================================================


def serialize_variant(variant: VehicleVariantV2) -> dict[str, Any]:
    return {
        "key": variant.key,
        "label": variant.label,
        "description": variant.description,
        "package_basis": variant.package_basis,
        "package": asdict(variant.package),
        "platform": asdict(variant.platform),
        "style": asdict(variant.style),
        "wheel_suspension": asdict(variant.wheel_suspension),
        "powertrain": asdict(variant.powertrain),
        "closures": asdict(variant.closures),
        "body_rgb": list(variant.body_rgb),
        "reference_frame": variant.reference_frame.as_dict(),
        "parameter_provenance": {
            name: asdict(parameter_evidence_for_variant(variant, name))
            for name in sorted(
                _PACKAGE_FIELDS | _PLATFORM_FIELDS | _STYLE_FIELDS | _WHEEL_FIELDS |
                _POWERTRAIN_FIELDS | _CLOSURE_FIELDS | {"crossover_cladding"}
            )
        },
        "crossover_cladding": variant.crossover_cladding,
        "provenance": {key: asdict(value) for key, value in variant.provenance.items()},
    }


def write_target_grammar(path: Path, variants: Mapping[str, VehicleVariantV2]) -> None:
    variant_rules = []
    for key, v in variants.items():
        variant_rules.append(f"""
MCSMv2_{key}() ->
    ModernCarV2(
        name(MCSMv2_{key})
        package(
            length({v.length:.6f}) width({v.width:.6f}) height({v.height:.6f})
            wheelbase({v.wheelbase:.6f})
            trackFront({v.track_front:.6f}) trackRear({v.track_rear:.6f})
            groundClearance({v.ground_clearance:.6f})
            frontOverhang({v.front_overhang:.6f}) rearOverhang({v.rear_overhang:.6f})
        )
        style(
            roofScale({v.style.roof_scale:.6f})
            tumblehome({v.style.tumblehome:.6f})
            shoulderStrength({v.style.shoulder_strength:.6f})
            frontFenderAmplitude({v.style.front_fender_amplitude:.6f})
            rearHaunchAmplitude({v.style.rear_haunch_amplitude:.6f})
            rockerTuck({v.style.rocker_tuck:.6f})
            doorScallop({v.style.door_scallop:.6f})
            tailTaper({v.style.tail_taper:.6f})
        )
        wheelSystem(
            radius({v.wheel_radius:.6f}) width({v.wheel_width:.6f})
            steerRange({v.wheel_suspension.steer_min_deg:.3f} {v.wheel_suspension.steer_max_deg:.3f})
            travelRange({v.wheel_suspension.travel_min:.6f} {v.wheel_suspension.travel_max:.6f})
            clearance({v.wheel_suspension.wheelhouse_clearance:.6f})
        )
        powertrain({v.powertrain.architecture})
        output(MCSMv2_{key})
    )
""")
    path.write_text(f"""# ============================================================================
# MCSMv2.0.1 HYBRID MODERN-CAR FAMILY
# Integrity-patched FGKv1/MVPv2.5 target grammar generated by modern_car_mcsmv2.py
#
# Implemented in the Python reference generator:
#   ParameterDependencyGraph
#   SemanticSectionField
#   CharacterCurveNetwork
#   SemanticSurfaceLoft
#   ImplicitScaffold
#   SweptWheelEnvelope approximation
#   PanelPatchGraph
#   BodyInWhiteGraph
#   OccupantEnvelopeSystem
#   FunctionalPackageSystem
#   EvidenceValidation
# ============================================================================

Start ->
    MCSMv2Family()

MCSMv2Family() ->
    MCSMv2_reference()
    MCSMv2_track()
    MCSMv2_aero()
    MCSMv2_crossover()

ModernCarV2(name package style wheelSystem powertrain output) ->
    VehicleReferenceFrame(x(Front) y(Right) z(Up) handedness(Right) units(m))
    ParameterDependencyGraph(package style wheelSystem powertrain)

    SemanticSectionField(
        stationRoles(
            TailFace RearBumper RearAxle RearDoor BPillar FrontDoor
            APillar HoodRear FrontAxle Nose FrontFace
        )
        landmarks(
            Underbody Rocker LowerBody Shoulder Belt
            GlassShoulder RoofRail RoofCrown
        )
        interpolation(PCHIP)
        output(name.Sections)
    )

    CharacterCurveNetwork(
        source(name.Sections)
        curves(
            CentreSpine RoofCentre
            RoofRailL RoofRailR
            GlassShoulderL GlassShoulderR
            BeltL BeltR ShoulderL ShoulderR
            RockerL RockerR UnderbodyEdgeL UnderbodyEdgeR
        )
        output(name.Curves)
    )

    SemanticSurfaceLoft(
        sections(name.Sections)
        curves(name.Curves)
        method(PCHIPSectionNetwork)
        output(name.SemanticSurface)
    )

    ImplicitScaffold(
        semanticWidthField(name.Sections)
        smoothBoundary(k(28))
        wheelhouseDifference(
            SweptWheelEnvelope(wheelSystem)
        )
        output(name.BodyField)
    )

    IsoSurface(field(name.BodyField) isoValue(0) output(name.BodyMesh))

    PanelPatchGraph(
        source(name.BodyMesh)
        panels(Hood Roof FrontDoors RearDoors RearQuarters RearHatch Bumpers)
    )

    BodyInWhiteGraph(
        sharedDatums(package)
        members(FrontRails RearRails Rockers FloorPan Crossmembers Pillars RoofRails Towers)
    )

    OccupantEnvelopeSystem(package)
    FunctionalPackageSystem(powertrain)
    ValidateMCSMv2(name)

{''.join(variant_rules)}

ValidateMCSMv2(name) ->
    RequireAcyclicParameterGraph(name)
    RequireOrderedSemanticSections(name)
    RequireCurveNetworkIntersections(name tolerance(0.002))
    RequireWatertightBody(name)
    RequireWheelEnvelopeClearance(name)
    RequireOccupantContainment(name)
    RequireConnectedBodyInWhite(name)
    RequireFiniteGeometry(name)
    RequireDeterministicMeshHash(name)
    CommitOnlyIfValid()
""", encoding="utf-8")


def write_math_model(path: Path) -> None:
    path.write_text(r"""# MCSMv2.0.1 Mathematical Model

## 1. Hybrid representation

MCSMv2 preserves the MCSMv1 implicit body as a topology-stable scaffold, but
promotes named semantic sections and character curves to first-class geometry.

\[
\mathcal M = (P,G_P,\Sigma,C,S_I,S_E,A,B,K,E,V)
\]

- \(P\): package and platform
- \(G_P\): parameter dependency graph
- \(\Sigma\): semantic section field
- \(C\): longitudinal character curves
- \(S_I\): implicit scaffold
- \(S_E\): explicit semantic surface
- \(A\): panels and closures
- \(B\): body-in-white
- \(K\): kinematic envelopes
- \(E\): evidence/provenance
- \(V\): validation

## 2. Semantic section

At longitudinal position \(x\), the right half-section contains named landmarks

\[
\Gamma_x = \{(w_k(x), z_k(x))\}_{k=0}^{7}
\]

for underbody, rocker, lower body, shoulder, belt, glass shoulder, roof rail and
roof crown. Width fields use PCHIP. Vertical fields are reconstructed from an
interpolated underbody base plus exponentiated log-gap curves, which guarantees
that adjacent semantic heights cannot cross between stations.

The section-width field is

\[
w(x,z)=\operatorname{Interp}_z\left(\Gamma_x\right).
\]

## 3. Implicit scaffold

The base body is the intersection of signed constraints

\[
F_s=|y|-w(x,z),\quad
F_l=z_{min}(x)-z,\quad
F_u=z-z_{max}(x),
\]

plus exact front and rear package bounds. Smooth maximum combines these:

\[
\operatorname{smax}_k(a,b)=\frac{1}{k}\log(e^{ka}+e^{kb}).
\]

The zero level set is the principal body surface.

## 4. Swept wheel envelope approximation

For each wheel, steering and suspension ranges produce an envelope represented
by a fourth-order superellipsoid. Front longitudinal/lateral radii include the
maximum steering excursion; vertical radius includes jounce/rebound:

\[
r_x=r+\tfrac12 w\sin\delta_{max}+c,
\]
\[
r_y=\tfrac12w+r\sin\delta_{max}+c,
\]
\[
r_z=r+\max(|j_{min}|,|j_{max}|)+c.
\]

Each side-specific envelope is subtracted from the body field.

## 5. Explicit semantic surface

A PCHIP curve is fitted through each half-section. Mirrored closed section rings
are connected longitudinally to form an explicit semantic surface. It is not yet
an approved Class-A patch graph; it is a deterministic bridge to FGKv1
`CurveNetworkSurface`.

## 6. Evidence and uncertainty

Every parameter belongs to one of:

`Measured`, `Derived`, `Fitted`, `Interpolated`, `DesignChoice`, `Unknown`.

Records retain source, confidence and optional uncertainty. Unknown dimensions
are not silently promoted to measurements.

## 7. Integrity realization and assurance

Marching-cubes discretization is calibrated before final extraction with an
inverse affine scalar-field prewarp. No post-mesh affine body correction is
applied, so body, curves, datums, BIW and engineering envelopes remain in the
same vehicle frame. The release independently checks final triangle topology,
winding, local self-intersections, package bounds, continuous section order,
curve/section residuals, occupant containment and sampled tyre sweeps.

Validation is reported as assurance levels V0 through V5. This implementation
claims V2 concept geometry when V0 parameter integrity, V1 final mesh integrity,
and V2 package/semantic consistency all pass. V3-V5 remain explicitly deferred.

## 8. Inverse fitting

The package includes a constrained nonlinear least-squares demonstration over
semantic section metrics. It proves the same model can run forward and inverse,
but it is not an image-calibrated production fit.
""", encoding="utf-8")


def write_implementation_report(path: Path, validations: Mapping[str, Any], inverse_fit: Mapping[str, Any]) -> None:
    rows = []
    for value in validations.values():
        intersections = value["final_body_mesh_integrity"]["self_intersection_screen"]["intersection_count"]
        bound_mm = 1000.0 * max(abs(item) for row in value["bound_error_m"] for item in row)
        curve_mm = 1000.0 * value["curve_network_intersection_residual_m"]
        rows.append(
            f"| {value['label']} | {value['assurance']['achieved_level']} | "
            f"{value['release_gate_pass']} | {bound_mm:.3f} | {curve_mm:.3f} | "
            f"{intersections} | {value['wheel_envelope_clearance']['pass']} | {value['body_faces']:,} |"
        )
    path.write_text(f"""# MCSMv2.0.1 Implementation Report

## Integrity patch delivered

MCSMv2.0.1 retains the MCSMv2 hybrid semantic-section and implicit-scaffold
architecture while repairing the integrity weaknesses identified in the
MCSMv2 problem audit.

Implemented changes:

1. exact per-parameter provenance rather than blanket package attribution;
2. explicit right-handed vehicle reference frame, units and Progen3D adapter;
3. a 70+ node dependency graph containing overhangs, style, platform, wheel,
   suspension, powertrain and closure inputs;
4. non-crossing continuous vertical section fields based on positive log gaps;
5. measured curve-network intersection residuals;
6. iterative pre-tessellation scalar-field calibration with no post-mesh affine correction;
7. removal and reporting of microscopic marching-cubes islands;
8. independent final-mesh occupant and sampled tyre-sweep checks;
9. winding, manifold, duplicate, degenerate and local self-intersection checks;
10. V0-V5 assurance reporting instead of a single unqualified PASS.

## Generated release summary

| Variant | Assurance | Release gate | Max bound error mm | Curve residual max mm | Self intersections | Tyre sweep | Body faces |
|---|---:|---:|---:|---:|---:|---:|---:|
{chr(10).join(rows)}

## Inverse fitting demonstration

- Initial residual norm: `{inverse_fit['initial_residual_norm']:.6f}`
- Final residual norm: `{inverse_fit['final_residual_norm']:.6f}`
- Solver success: `{inverse_fit['success']}`
- Evaluations: `{inverse_fit['evaluations']}`

The inverse fit remains a synthetic semantic-measurement demonstration. A low
metric residual is not treated as proof that all generating parameters are
identifiable.

## Assurance boundary

A released variant can achieve **V2 concept geometry** in this version:

- V0: finite framed parameter model and acyclic dependency graph;
- V1: watertight, consistently wound, positive-volume, non-self-intersecting body;
- V2: package bounds, continuous semantic sections, curve network, datums and
  BIW graph are internally consistent.

Static occupant and tyre sweeps are independently checked, but V3 is not claimed
because door, hatch, bonnet, glass and exact suspension kinematics remain
deferred. V4 manufacturing/structural analysis and V5 CFD/crash/physical
validation are also outside this release.
""", encoding="utf-8")


def write_readme(path: Path) -> None:
    path.write_text("""# Modern Car MCSMv2.0.1

MCSMv2.0.1 is the integrity-patched hybrid semantic-section and
implicit-scaffold modern-car generator.

## Regenerate

```bash
python modern_car_mcsmv2.py --output generated --resolution medium
```

Use `--no-preview` when VTK is unavailable or only mesh/data outputs are needed.

## Integrity changes

- parameter-specific evidence and units
- explicit MCSMv2 vehicle frame and Progen3D axis adapter
- complete causal parameter DAG including overhangs and hidden model inputs
- non-crossing continuous semantic section fields
- measured curve-network residual
- pre-tessellation field calibration with no post-mesh affine correction
- final-mesh winding, manifold and self-intersection validation
- independent final-mesh occupant and sampled tyre-sweep checks
- V0-V5 assurance record

## Main outputs

- `models/modern_car_v2_*.glb`: complete visible scenes
- `models/modern_car_v2_*_body.stl`: watertight principal bodies
- `models/modern_car_v2_*_engineering.glb`: engineering evidence scenes
- `semantic_stations/*.csv`: named station values
- `curves/*.json`: 3D curve and section data with explicit frame metadata
- `dependency_graphs/*.json`: unit-aware evaluated dependency graphs
- `validation.json`: independent integrity checks and assurance levels
- `MCSMv2_Modern_Car_Family.p3d`: FGKv1 target grammar

## Assurance

This release targets V2 concept geometry. It does not claim production Class-A,
closure/glass/suspension kinematics, manufacturing, structural FEA, CFD, crash,
homologation or physical validation.

## Dependencies

NumPy, SciPy, scikit-image, trimesh and Pillow. VTK is optional for previews.
""", encoding="utf-8")


# =============================================================================
# Generation
# =============================================================================


def resolution_tuple(name: str) -> tuple[int, int, int]:
    return {
        "low": (113, 65, 81),
        "medium": (143, 79, 97),
        "high": (177, 97, 121),
    }[name]


def write_station_csv(path: Path, stations: Sequence[SemanticStation]) -> None:
    with path.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(asdict(stations[0]).keys()))
        writer.writeheader()
        for station in stations:
            writer.writerow(asdict(station))


def build_curve_scene(model: ModernCarModelV2) -> trimesh.Scene:
    scene = trimesh.Scene()
    curves = model.sections.character_curves(samples=96)
    for name, points in curves.items():
        try:
            add_geometry(scene, f"curve_{name}", legacy.polyline_tube(
                points, 0.006, legacy.MAT_CURVE_BLUE, (0.04, 0.20, 0.90), sections=7
            ))
        except ValueError:
            pass
    for section in model.semantic_sections(count=19)[::2]:
        add_geometry(scene, f"section_{section['index']:02d}", legacy.polyline_tube(
            section["points_xyz_m"], 0.0045, legacy.MAT_SECTION_YELLOW,
            (0.95, 0.75, 0.05), sections=6,
        ))
    return scene


def build_design_manifold(reference: VehicleVariantV2, crossover: VehicleVariantV2) -> trimesh.Scene:
    scene = trimesh.Scene()
    for index, t in enumerate(np.linspace(0.0, 1.0, 6)):
        variant = interpolate_variant(reference, crossover, float(t), f"manifold_{index}", f"Manifold {t:.2f}")
        surface = SemanticSectionModel(variant).semantic_surface_mesh(samples_x=55, samples_half=26)
        hue = (0.80 - 0.45 * t, 0.12 + 0.42 * t, 0.08 + 0.32 * t)
        surface.visual.material = legacy.pbr(
            f"Manifold_{index}", tuple(int(255 * c) for c in hue), 0.35, 0.32, 225
        )
        legacy.set_preview(surface, hue, 0.78, 0.35, 0.32)
        transform = trimesh.transformations.translation_matrix([index * 5.2, 0.0, 0.0])
        add_geometry(scene, f"manifold_{index}", surface, transform)
    return scene


def generate(
    output: Path,
    resolution_name: str = "medium",
    previews: bool = True,
    selected: Sequence[str] | None = None,
) -> dict[str, Any]:
    variants = derive_v2_variants()
    selected_keys = list(selected) if selected else list(variants)
    unknown = [key for key in selected_keys if key not in variants]
    if unknown:
        raise ValueError(f"unknown variants: {', '.join(unknown)}")

    output.mkdir(parents=True, exist_ok=True)
    for name in ("models", "curves", "semantic_stations", "dependency_graphs", "previews", "data"):
        (output / name).mkdir(exist_ok=True)

    (output / "VERSION.json").write_text(
        json.dumps({
            "version": MCSM_VERSION,
            "model_schema": MODEL_SCHEMA,
            "reference_frame": VehicleReferenceFrame().as_dict(),
            "assurance_model": "MCSMv2.0.1.Assurance.v1",
        }, indent=2),
        encoding="utf-8",
    )

    (output / "data" / "research_data.json").write_text(
        json.dumps({
            "vehicle_sources": legacy.RESEARCH_SOURCES,
            "method_sources": legacy.METHOD_SOURCES,
            "source_note": "Copied from MCSMv1; no new external package claims introduced by MCSMv2.",
        }, indent=2), encoding="utf-8",
    )
    (output / "model_parameters.json").write_text(
        json.dumps({key: serialize_variant(value) for key, value in variants.items()}, indent=2),
        encoding="utf-8",
    )

    res = resolution_tuple(resolution_name)
    validations: dict[str, Any] = {}
    bodies: dict[str, trimesh.Trimesh] = {}
    iso_images: list[tuple[VehicleVariantV2, Path]] = []


    for key in selected_keys:
        variant = variants[key]
        model = ModernCarModelV2(variant)
        vehicle_scene, body, body_evidence, panels, panel_graph = build_vehicle_scene(model, res)
        semantic_surface = model.sections.semantic_surface_mesh()
        engineering_scene, biw_graph, engineering_manifest = build_engineering_scene(model, body, semantic_surface)
        bodies[key] = body.copy()

        export_scene(
            vehicle_scene,
            output / "models" / f"modern_car_v2_{key}.glb",
            output / "models" / f"modern_car_v2_{key}.obj",
        )
        body.export(output / "models" / f"modern_car_v2_{key}_body.stl")
        export_scene(engineering_scene, output / "models" / f"modern_car_v2_{key}_engineering.glb")
        semantic_surface.export(output / "models" / f"modern_car_v2_{key}_semantic_surface.stl")

        write_station_csv(output / "semantic_stations" / f"{key}_semantic_stations.csv", model.sections.stations)
        curves = model.sections.character_curves(samples=96)
        sections = model.semantic_sections(count=21)
        (output / "curves" / f"{key}_curve_network.json").write_text(
            json.dumps({
                "schema": "MCSMv2.CurveNetwork.v1",
                "coordinate_system": model.reference_frame.as_dict(),
                "curves": curves,
                "sections": sections,
            }, indent=2), encoding="utf-8",
        )
        (output / "dependency_graphs" / f"{key}_parameter_graph.json").write_text(
            json.dumps(model.parameter_graph.as_dict(), indent=2), encoding="utf-8",
        )
        (output / "data" / f"{key}_panel_graph.json").write_text(json.dumps(panel_graph, indent=2), encoding="utf-8")
        (output / "data" / f"{key}_biw_graph.json").write_text(json.dumps(biw_graph, indent=2), encoding="utf-8")
        (output / "data" / f"{key}_engineering_manifest.json").write_text(json.dumps(engineering_manifest, indent=2), encoding="utf-8")

        validation = validate_model(model, body, semantic_surface, body_evidence, vehicle_scene, panel_graph, biw_graph)
        validations[key] = validation
        (output / "validation.json").write_text(
            json.dumps(validations, indent=2), encoding="utf-8"
        )

        if key == "reference":
            curve_scene = build_curve_scene(model)
            export_scene(curve_scene, output / "models" / "modern_car_v2_reference_curve_network.glb")
            export_scene(trimesh.Scene(semantic_surface), output / "models" / "modern_car_v2_reference_semantic_surface.glb")

        if previews and legacy.vtk is not None:
            view_paths: dict[str, Path] = {}
            for view in ("iso", "side", "front", "top"):
                path = output / "previews" / f"modern_car_v2_{key}_{view}.png"
                legacy.render_scene(vehicle_scene, variant, path, view)
                view_paths[view] = path
            mcsmv2_contact_sheet(variant.label, variant, view_paths, output / "previews" / f"modern_car_v2_{key}_views.png")
            iso_images.append((variant, view_paths["iso"]))
            eng_path = output / "previews" / f"modern_car_v2_{key}_engineering_iso.png"
            legacy.render_scene(engineering_scene, variant, eng_path, "iso")

        # Release full scene arrays before the next variant. Only the compact
        # body mesh is retained for the body-only comparison artifact.
        del vehicle_scene, engineering_scene, panels, semantic_surface

    # Lightweight release comparison: body-only meshes keep the artifact
    # responsive and avoid serializing hundreds of repeated scene components.
    comparison = trimesh.Scene()
    offsets = {
        "reference": np.asarray([0.0, -2.8, 0.0]),
        "track": np.asarray([0.0, 2.8, 0.0]),
        "aero": np.asarray([5.5, -2.8, 0.0]),
        "crossover": np.asarray([5.5, 2.8, 0.0]),
    }
    for key, body_mesh in bodies.items():
        comparison.add_geometry(
            body_mesh,
            geom_name=f"{key}_body",
            node_name=f"{key}_body",
            transform=trimesh.transformations.translation_matrix(offsets[key]),
        )
    export_scene(comparison, output / "models" / "modern_car_v2_variations_comparison.glb")

    manifold = build_design_manifold(variants["reference"], variants["crossover"])
    export_scene(manifold, output / "models" / "modern_car_v2_design_manifold.glb")

    if previews and len(iso_images) == 4:
        family_poster(iso_images, output / "previews" / "modern_car_v2_family_overview.png")
    side_curve_comparison(variants, output / "previews" / "modern_car_v2_semantic_curve_comparison.png")

    inv_fit = inverse_fit_demo(variants["reference"])
    (output / "inverse_fit_demo.json").write_text(json.dumps(inv_fit, indent=2), encoding="utf-8")
    (output / "validation.json").write_text(json.dumps(validations, indent=2), encoding="utf-8")
    write_target_grammar(output / "MCSMv2_Modern_Car_Family.p3d", variants)
    write_math_model(output / "MCSMv2_MATHEMATICAL_MODEL.md")
    write_implementation_report(output / "MCSMv2_IMPLEMENTATION_REPORT.md", validations, inv_fit)
    write_readme(output / "README.md")

    summary_lines = [
        "# MCSMv2.0.1 Validation and Assurance Summary",
        "",
        "| Variant | Assurance | Release gate | Body integrity | Package | Sections | Curves | Static envelopes |",
        "|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for result in validations.values():
        summary_lines.append(
            f"| {result['label']} | {result['assurance']['achieved_level']} | "
            f"{result['release_gate_pass']} | {result['final_body_mesh_integrity']['pass']} | "
            f"{result['package_pass']} | {result['semantic_section_continuous_order']['pass']} | "
            f"{result['curve_network_intersection']['pass']} | "
            f"{result['occupant_containment']['pass'] and result['wheel_envelope_clearance']['pass']} |"
        )
    summary_lines.extend([
        "",
        "V2 denotes concept-level package and semantic geometry assurance. V3-V5 are not claimed.",
        "The `release_gate_pass` field is a release-integrity gate, not production vehicle approval.",
    ])
    (output / "VALIDATION_SUMMARY.md").write_text("\n".join(summary_lines) + "\n", encoding="utf-8")

    return {
        "schema": "MCSMv2.GenerationSummary.v1",
        "version": MCSM_VERSION,
        "output": str(output),
        "resolution": resolution_name,
        "variants": selected_keys,
        "validation": validations,
        "inverse_fit": inv_fit,
    }


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("Modern_Car_MCSMv2_Generated"))
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
