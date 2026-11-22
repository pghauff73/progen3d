#!/usr/bin/env python3
"""Evaluate the native FacialSkin grammar and package measured diagnostic poses.

The viewer consumes actual native samples. It does not implement a second face
deformer. All generated geometry is authored by native procedural primitives.
"""
from __future__ import annotations

import argparse
from collections import Counter, defaultdict
from dataclasses import asdict, dataclass
import json
import math
from pathlib import Path
import struct
import subprocess
from typing import Any


@dataclass(frozen=True)
class FacialStudyControls:
    smileLeft: float = 0.0
    smileRight: float = 0.0
    jawOpen: float = 0.0
    blinkLeft: float = 0.0
    blinkRight: float = 0.0
    browLeft: float = 0.0
    browRight: float = 0.0

    def __post_init__(self) -> None:
        if any(not math.isfinite(v) or not 0.0 <= v <= 1.0 for v in asdict(self).values()):
            raise ValueError("Facial controls must be finite values in [0, 1]")


@dataclass(frozen=True)
class FacialStudyCase:
    identifier: str
    label: str
    controls: FacialStudyControls
    seconds: float = 0.0


class FacialStudyGrammar:
    """Author native grammar. Dimensions are metres; anatomical left is +X."""

    @staticmethod
    def create(controls: FacialStudyControls, eyes: dict | None = None,
               animated: bool = False, transform: str = "") -> str:
        values = list(asdict(controls).values())
        args = " ".join(format(v, ".9g") for v in values)
        if animated:
            args = (
                "0.75*(0.5-0.5*cos(radians(60)*t)) "
                "0.75*(0.5-0.5*cos(radians(60)*t)) "
                "0.65*(0.5-0.5*cos(radians(120)*t)) "
                "(0.5-0.5*cos(radians(120)*t))^12 "
                "(0.5-0.5*cos(radians(120)*t))^12 "
                "0.35*(0.5-0.5*cos(radians(60)*t)) "
                "0.35*(0.5-0.5*cos(radians(60)*t))"
            )
        lines = [
            "# Native ProGen3D FacialSkin milestone A: procedural geometric rig.",
            "# Units: metres; +Y up; +Z forward; +X anatomical left.",
            "# Connected open front-face patch. No imported mesh or tissue physics.",
            "# Animated version returns to neutral at t=6 seconds.",
            f"Start -> [ {transform} Face({args}) ]",
            "",
            "Face(smileL smileR jaw blinkL blinkR browL browR) ->",
            "    I(FacialSkin(size(0.16 0.21 0.045) segments(48 64)",
            "      smile(smileL smileR) jawOpen(jaw) blink(blinkL blinkR)",
            "      browRaise(browL browR)) material(softpinkmatteplaster) 1 1)",
        ]
        if eyes:
            radius = eyes["radius"]
            for key in ("left_eyeball_center", "right_eyeball_center"):
                x, y, z = eyes[key]
                lines.extend([
                    f"    [ T({x:.9g} {y:.9g} {z:.9g}) S({2*radius:.9g} {2*radius:.9g} {2*radius:.9g})",
                    "      I(Sphere(segments(32 24)) material(warmwhitematteplaster) 1 1) ]",
                    "    # Iris and pupil follow the globe as concentric surface caps.",
                    "    # A(90 0) rotates the native +Y pole toward the +Z front.",
                    f"    [ T({x:.9g} {y:.9g} {z:.9g}) A(90 0) S({2*(radius+0.000002):.9g} {2*(radius+0.000002):.9g} {2*(radius+0.000002):.9g})",
                    "      I(Sphere(polar(0 27) topology(surface) close(none) segments(32 16)) material(tealmattemetal) 1 1) ]",
                    f"    [ T({x:.9g} {y:.9g} {z:.9g}) A(90 0) S({2*(radius+0.000004):.9g} {2*(radius+0.000004):.9g} {2*(radius+0.000004):.9g})",
                    "      I(Sphere(polar(0 12) topology(surface) close(none) segments(24 12)) material(blackmattepaint) 1 1) ]",
                ])
            x, y, z = eyes["mouth_upper_margin"]
            lines.extend([
                "    # Dark recessed mouth proxy, not a complete oral cavity.",
                f"    [ T(0 {y:.9g}-0.004-jaw*0.006 0.005) S(0.075 0.055 0.016)",
                "      I(Sphere(segments(28 18)) material(blackmattepaint) 1 1) ]",
            ])
        return "\n".join(lines) + "\n"


class NativeSceneObjReader:
    COLORS = {
        "softpinkmatteplaster": [0.75, 0.49, 0.40],
        "warmwhitematteplaster": [0.90, 0.91, 0.88],
        "tealmattemetal": [0.10, 0.36, 0.34],
        "blackmattepaint": [0.055, 0.035, 0.045],
    }

    def read(self, path: Path) -> dict[str, Any]:
        positions: list[list[float]] = []
        normals: list[list[float]] = []
        objects: list[dict] = []
        landmarks: dict[str, list[float]] = {}
        current = None
        radius = None
        for line in path.read_text().splitlines():
            fields = line.split()
            if not fields:
                continue
            if fields[:2] == ["#", "facial_landmark"]:
                landmarks[fields[2]] = [float(x) for x in fields[3:6]]
            elif fields[:2] == ["#", "facial_eyeball_radius_local"]:
                radius = float(fields[2])
            elif fields[0] == "o":
                current = {"name": fields[1], "material": "", "color": [0.7]*3,
                           "positions": [], "normals": []}
                objects.append(current)
            elif fields[0] == "usemtl" and current is not None:
                current["material"] = fields[1]
                current["color"] = self.COLORS.get(fields[1], [0.5, 0.55, 0.6])
            elif fields[0] == "v":
                positions.append([float(v) for v in fields[1:4]])
            elif fields[0] == "vn":
                normals.append([float(v) for v in fields[1:4]])
            elif fields[0] == "f":
                if current is None or len(fields) != 4:
                    raise ValueError("Expected named native triangle objects")
                for item in fields[1:]:
                    indices = item.split("/")
                    if len(indices) != 3 or not indices[2]:
                        raise ValueError("Native normals are required; rebuild form-study-sample")
                    vi, ni = int(indices[0])-1, int(indices[2])-1
                    if not 0 <= vi < len(positions) or not 0 <= ni < len(normals):
                        raise ValueError("Invalid OBJ position/normal index")
                    current["positions"].extend(positions[vi])
                    current["normals"].extend(normals[ni])
        if not objects or not landmarks or radius is None:
            raise ValueError("No native FacialSkin metadata found in export")
        if not all(math.isfinite(v) for obj in objects for key in ("positions", "normals") for v in obj[key]):
            raise ValueError("Nonfinite native export")
        return {"objects": objects, "landmarks": landmarks, "radius": radius}


class NativeFacialDiagnosticService:
    """Measure exported geometry; do not substitute an alternate facial model."""

    def measure(self, native: dict) -> dict:
        skin = native["objects"][0]
        raw = skin["positions"]
        normal_data = skin["normals"]
        unique: dict[tuple, int] = {}
        triangle_indices = []
        for i in range(0, len(raw), 3):
            # Native export is triangle soup. Merge exact round-tripped float
            # positions only; close lip/lid vertices must remain distinct.
            point = tuple(raw[i:i+3])
            triangle_indices.append(unique.setdefault(point, len(unique)))
        edges = Counter()
        adjacency = defaultdict(set)
        minimum_area = math.inf
        backward_facing = 0
        opposing_shading = 0
        for start in range(0, len(triangle_indices), 3):
            tri = triangle_indices[start:start+3]
            for a, b in zip(tri, tri[1:]+tri[:1]):
                edges[tuple(sorted((a, b)))] += 1
                adjacency[a].add(b)
                adjacency[b].add(a)
            offset = start*3
            a, b, c = [raw[offset+j:offset+j+3] for j in (0, 3, 6)]
            u, v = [b[k]-a[k] for k in range(3)], [c[k]-a[k] for k in range(3)]
            cross = [u[1]*v[2]-u[2]*v[1], u[2]*v[0]-u[0]*v[2], u[0]*v[1]-u[1]*v[0]]
            minimum_area = min(minimum_area, 0.5*math.sqrt(sum(x*x for x in cross)))
            # Aperture wall triangles are perpendicular to the front plane;
            # their zero Z projection is expected, not a degenerate triangle.
            if cross[2] < -1e-14:
                backward_facing += 1
            mean_normal = [sum(normal_data[offset+j+k] for j in (0, 3, 6)) for k in range(3)]
            denominator = math.sqrt(sum(v*v for v in cross)*sum(v*v for v in mean_normal))
            if denominator > 0 and sum(cross[k]*mean_normal[k] for k in range(3))/denominator < -1e-4:
                opposing_shading += 1
        def count_components(graph):
            remaining = set(graph)
            count = 0
            while remaining:
                count += 1
                stack = [remaining.pop()]
                while stack:
                    for neighbor in graph[stack.pop()]:
                        if neighbor in remaining:
                            remaining.remove(neighbor)
                            stack.append(neighbor)
            return count
        boundaries = defaultdict(set)
        for (a, b), count in edges.items():
            if count == 1:
                boundaries[a].add(b)
                boundaries[b].add(a)
        normal_error = max(abs(math.sqrt(sum(v*v for v in normal_data[i:i+3]))-1)
                           for i in range(0, len(normal_data), 3))
        lm = native["landmarks"]
        result = {
            "vertices": len(unique), "triangles": len(triangle_indices)//3,
            "connectedComponents": count_components(adjacency),
            "boundaryLoops": count_components(boundaries),
            "boundaryBranches": sum(len(n) != 2 for n in boundaries.values()),
            "nonmanifoldEdges": sum(count > 2 for count in edges.values()),
            "minimumTriangleAreaM2": minimum_area,
            "backwardFacingTriangles": backward_facing,
            "opposingShadingTriangles": opposing_shading,
            "maximumNormalLengthError": normal_error,
            "leftLidGapMm": 1000*(lm["left_eye_upper_margin"][1]-lm["left_eye_lower_margin"][1]),
            "rightLidGapMm": 1000*(lm["right_eye_upper_margin"][1]-lm["right_eye_lower_margin"][1]),
            "mouthGapMm": 1000*(lm["mouth_upper_margin"][1]-lm["mouth_lower_margin"][1]),
        }
        result["passed"] = (result["connectedComponents"] == 1 and result["boundaryLoops"] == 4
                            and result["boundaryBranches"] == 0 and result["nonmanifoldEdges"] == 0
                            and minimum_area > 1e-14 and backward_facing == 0 and opposing_shading == 0 and normal_error < 1e-4
                            and result["leftLidGapMm"] > 0 and result["rightLidGapMm"] > 0
                            and result["mouthGapMm"] > 0)
        return result


class NativeFaceSampleService:
    def __init__(self, sampler: Path, destination: Path):
        self.sampler = sampler.resolve()
        self.destination = destination
        self.destination.mkdir(parents=True, exist_ok=True)
        self.reader = NativeSceneObjReader()

    def sample(self, identifier: str, grammar: str, seconds: float = 0.0) -> dict:
        source = self.destination / (identifier + ".p3d")
        output = self.destination / (identifier + ".obj")
        source.write_text(grammar)
        completed = subprocess.run([str(self.sampler), str(source), format(seconds, ".12g"), str(output)],
                                   text=True, capture_output=True, timeout=90)
        if completed.returncode:
            raise RuntimeError(f"Native evaluation failed for {identifier}:\n{completed.stderr[-5000:]}\n{completed.stdout[-1000:]}")
        return self.reader.read(output)

    def require_rejection(self, identifier: str, grammar: str) -> None:
        source = self.destination / (identifier + ".p3d")
        source.write_text(grammar)
        completed = subprocess.run([str(self.sampler), str(source), "0", str(source.with_suffix(".obj"))],
                                   text=True, capture_output=True, timeout=90)
        if completed.returncode == 0:
            raise AssertionError(f"Native grammar unexpectedly accepted {identifier}")


class NativeMeshAssetCatalog:
    """Index presentation tuples without changing any position or normal value.

    Samples still come from the native sampler. Indexing is applied only after
    the existing presentation rounding in package_case. The exact triangle
    order and the position/normal association are preserved, including signed
    zero. Identical objects and identical index arrays are shared across poses.
    """

    def __init__(self) -> None:
        self.assets: list[dict[str, Any]] = []
        self.topologies: list[list[int]] = []
        self.asset_lookup: dict[str, int] = {}
        self.topology_lookup: dict[tuple[int, ...], int] = {}
        self.checked_objects = 0
        self.checked_triangle_corners = 0

    def store_case(self, case: dict[str, Any]) -> dict[str, Any]:
        references = [{"asset": self.store_object(obj)} for obj in case["objects"]]
        return dict(case, objects=references)

    def store_object(self, obj: dict[str, Any]) -> int:
        positions, normals = obj["positions"], obj["normals"]
        if len(positions) % 9 or len(normals) != len(positions):
            raise ValueError("Native presentation object requires complete position/normal triangles")
        if not all(math.isfinite(value) for values in (positions, normals) for value in values):
            raise ValueError("Native presentation values must be finite")
        tuple_lookup: dict[bytes, int] = {}
        indexed_positions: list[float] = []
        indexed_normals: list[float] = []
        indices: list[int] = []
        for offset in range(0, len(positions), 3):
            point, normal = positions[offset:offset+3], normals[offset:offset+3]
            # Byte keys distinguish signed zero and never merge distinct
            # normals at a shared position (for example, a hard surface seam).
            key = struct.pack("!6d", *point, *normal)
            index = tuple_lookup.get(key)
            if index is None:
                index = len(tuple_lookup)
                tuple_lookup[key] = index
                indexed_positions.extend(point)
                indexed_normals.extend(normal)
            indices.append(index)
        topology_key = tuple(indices)
        topology = self.topology_lookup.get(topology_key)
        if topology is None:
            topology = len(self.topologies)
            self.topology_lookup[topology_key] = topology
            self.topologies.append(indices)
        metadata = {key: value for key, value in obj.items() if key not in ("positions", "normals")}
        asset = dict(metadata, positions=indexed_positions, normals=indexed_normals, topology=topology)
        # A canonical JSON key gives exact comparison without relying on a
        # probabilistic hash or rounding the native presentation a second time.
        asset_key = json.dumps(asset, separators=(",", ":"), allow_nan=False)
        asset_index = self.asset_lookup.get(asset_key)
        if asset_index is None:
            asset_index = len(self.assets)
            self.asset_lookup[asset_key] = asset_index
            self.assets.append(asset)
        self.require_exact_round_trip(obj, self.assets[asset_index])
        return asset_index

    def require_exact_round_trip(self, original: dict, asset: dict) -> None:
        indices = self.topologies[asset["topology"]]
        if len(indices)*3 != len(original["positions"]):
            raise AssertionError("Indexed native presentation changed the triangle count")
        for corner, index in enumerate(indices):
            source_offset, asset_offset = corner*3, index*3
            source = (*original["positions"][source_offset:source_offset+3],
                      *original["normals"][source_offset:source_offset+3])
            restored = (*asset["positions"][asset_offset:asset_offset+3],
                        *asset["normals"][asset_offset:asset_offset+3])
            if struct.pack("!6d", *source) != struct.pack("!6d", *restored):
                raise AssertionError("Indexed native presentation changed a position/normal tuple")
        self.checked_objects += 1
        self.checked_triangle_corners += len(indices)

    def summary(self) -> dict[str, Any]:
        return {
            "schema": "progen3d.facial-study.indexed.v1",
            "assetCount": len(self.assets),
            "topologyCount": len(self.topologies),
            "checkedObjectReferences": self.checked_objects,
            "checkedTriangleCorners": self.checked_triangle_corners,
            "storedPositionNormalTuples": sum(len(asset["positions"])//3 for asset in self.assets),
            "exactPresentationRoundTrip": True,
            "additionalQuantization": False,
            "positionDecimals": 8,
            "normalDecimals": 6,
        }


def create_cases() -> list[FacialStudyCase]:
    return [
        FacialStudyCase("neutral", "Neutral", FacialStudyControls()),
        FacialStudyCase("smile_left", "Left smile", FacialStudyControls(smileLeft=1)),
        FacialStudyCase("smile_right", "Right smile", FacialStudyControls(smileRight=1)),
        FacialStudyCase("smile", "Smile", FacialStudyControls(smileLeft=1, smileRight=1)),
        FacialStudyCase("jaw", "Open jaw", FacialStudyControls(jawOpen=1)),
        FacialStudyCase("blink_left", "Left blink", FacialStudyControls(blinkLeft=1)),
        FacialStudyCase("blink", "Blink", FacialStudyControls(blinkLeft=1, blinkRight=1)),
        FacialStudyCase("brows", "Raised brows", FacialStudyControls(browLeft=1, browRight=1)),
        FacialStudyCase("combined", "Smile + jaw + blink", FacialStudyControls(1, 1, 1, 1, 1, 0.5, 0.5)),
    ]


def create_study(sampler: Path, destination: Path, timeline_frames: int) -> dict:
    service = NativeFaceSampleService(sampler, destination / "samples")
    baseline = service.sample("skin_neutral", FacialStudyGrammar.create(FacialStudyControls()))
    eyes = dict(baseline["landmarks"], radius=baseline["radius"])
    diagnostics = NativeFacialDiagnosticService()
    asset_catalog = NativeMeshAssetCatalog()
    study = {"title": "ProGen3D · Facial Skin Study", "units": "m", "cases": [],
             "model": "Native procedural front-face patch; geometric deformation; no elastic tissue solve",
             "overlayMeaning": "Muscle guides show expression directions, not simulated muscle fibers."}
    summaries = {}
    for case in create_cases():
        native = service.sample(case.identifier, FacialStudyGrammar.create(case.controls, eyes))
        measured = diagnostics.measure(native)
        summaries[case.identifier] = measured
        study["cases"].append(asset_catalog.store_case(package_case(case, native, measured)))
    animated = FacialStudyGrammar.create(FacialStudyControls(), eyes, animated=True)
    (destination / "Lyra_Facial_Study.p3d").write_text(animated)
    for index in range(timeline_frames):
        time = 6*index/max(1, timeline_frames-1)
        one = 0.5-0.5*math.cos(math.pi*time/3)
        two = 0.5-0.5*math.cos(2*math.pi*time/3)
        controls = FacialStudyControls(.75*one, .75*one, .65*two, two**12, two**12, .35*one, .35*one)
        case = FacialStudyCase(f"timeline_{index:03}", f"Timeline · {time:.2f} s", controls, time)
        native = service.sample(case.identifier, animated, time)
        measured = diagnostics.measure(native)
        summaries[case.identifier] = measured
        study["cases"].append(asset_catalog.store_case(package_case(case, native, measured)))
    # Re-evaluate times in a deliberately different order. A stateful or stale
    # cache bug must not make native grammar snapshots depend on query order.
    first = service.sample("seek_first", animated, 1.5)
    service.sample("seek_far", animated, 5.25)
    repeated = service.sample("seek_repeat", animated, 1.5)
    neutral_end = service.sample("return_to_neutral", animated, 6)
    start = service.sample("start_neutral", animated, 0)
    seek_error = maximum_position_difference(first, repeated)
    return_error = maximum_position_difference(start, neutral_end)
    counts = {(m["vertices"], m["triangles"]) for m in summaries.values()}
    valid_grammar = FacialStudyGrammar.create(FacialStudyControls())
    invalid_grammars = {
        "invalid_control": valid_grammar.replace("smile(smileL smileR)", "smile(1.1 0)"),
        "invalid_segments": valid_grammar.replace("segments(48 64)", "segments(49 64)"),
        "invalid_option": valid_grammar.replace("jawOpen(jaw)", "unrecognizedOption(jaw)"),
    }
    for identifier, invalid_grammar in invalid_grammars.items():
        service.require_rejection(identifier, invalid_grammar)
    report = {"schema": "progen3d.facial-skin.validation.v1", "cases": summaries,
              "caseCount": len(summaries), "stableTopologyCounts": len(counts) == 1,
              "seekMaximumDifferenceM": seek_error, "neutralReturnMaximumDifferenceM": return_error,
              "invalidGrammarRejections": list(invalid_grammars),
              "scope": "Geometry, topology, normals, control extrema and deterministic native evaluation",
              "limitations": ["Open facial patch, not the existing full Lyra head", "No elastic skin or muscle-force simulation",
                              "No general self-collision or calibrated anatomy", "Eyelids retain a deliberate nonzero clearance",
                              "Eye and oral proxies are illustrative; exact contacts are a later milestone"]}
    report["passed"] = all(m["passed"] for m in summaries.values()) and len(counts) == 1 and seek_error < 1e-9 and return_error < 1e-7
    report["presentation"] = asset_catalog.summary()
    (destination / "validation.json").write_text(json.dumps(report, indent=2, allow_nan=False)+"\n")
    study["validation"] = report
    study["meshAssets"] = asset_catalog.assets
    study["meshTopologies"] = asset_catalog.topologies
    study["presentation"] = asset_catalog.summary()
    serialized = json.dumps(study, separators=(",", ":"), allow_nan=False).replace("</", "<\\/")
    template = Path(__file__).with_name("inspector.template.html").read_text()
    if template.count("__FACIAL_STUDY_DATA__") != 1:
        raise ValueError("Inspector must contain exactly one data token")
    (destination / "Lyra_Facial_Study.html").write_text(template.replace("__FACIAL_STUDY_DATA__", serialized))
    (destination / "study.json").write_text(serialized)
    return report


def maximum_position_difference(a: dict, b: dict) -> float:
    if len(a["objects"]) != len(b["objects"]):
        return math.inf
    differences = []
    for first, second in zip(a["objects"], b["objects"]):
        if len(first["positions"]) != len(second["positions"]):
            return math.inf
        differences.extend(abs(x-y) for x, y in zip(first["positions"], second["positions"]))
    return max(differences, default=0.0)


def package_case(case: FacialStudyCase, native: dict, measured: dict) -> dict:
    # Quantization is presentation-only; validation above uses full exported
    # precision. Landmarks come from the same native model as the skin.
    objects = []
    for obj in native["objects"]:
        objects.append(dict(obj, positions=[round(x, 8) for x in obj["positions"]],
                            normals=[round(x, 6) for x in obj["normals"]]))
    landmarks = [{"name": name.replace("_", " "), "position": point}
                 for name, point in native["landmarks"].items() if "center" not in name]
    lm = native["landmarks"]
    muscles = []
    for side in ("left", "right"):
        corner = lm[f"mouth_{side}_corner"]
        eye = lm[f"{side}_eye_outer_corner"]
        guide = [eye[0]*1.25, eye[1]-0.005, eye[2]-0.001]
        muscles.append({"name": f"{side.title()} smile direction (guide)", "points": [guide, corner]})
    return {"id": case.identifier, "label": case.label, "time": case.seconds,
            "controls": asdict(case.controls), "objects": objects,
            "landmarks": landmarks, "muscles": muscles, "diagnostics": measured}


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampler", type=Path, default=Path("build/form-study-sample"))
    parser.add_argument("--out", type=Path, default=Path("build/facial-skin-study"))
    parser.add_argument("--timeline-frames", type=int, default=37)
    args = parser.parse_args()
    if not 0 <= args.timeline_frames <= 181:
        parser.error("--timeline-frames must be between 0 and 181")
    report = create_study(args.sampler, args.out, args.timeline_frames)
    print(json.dumps({key: report[key] for key in ("passed", "caseCount", "stableTopologyCounts", "seekMaximumDifferenceM", "neutralReturnMaximumDifferenceM")}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
