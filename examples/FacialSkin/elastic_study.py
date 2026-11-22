#!/usr/bin/env python3
"""Package native FacialSkin membrane/cable equilibria and measured evidence.

Every displayed surface, cable endpoint and solver value comes from the native
grammar sampler. The browser presents stored samples; it runs no second solver.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import json
import math
from pathlib import Path
from typing import Any

from face_study import (
    NativeFaceSampleService,
    NativeFacialDiagnosticService,
    NativeMeshAssetCatalog,
    NativeSceneObjReader,
    maximum_position_difference,
)


@dataclass(frozen=True)
class FacialElasticStudyCase:
    identifier: str
    label: str
    left_activation: float = 0.0
    right_activation: float = 0.0
    young_modulus_pa: float = 50000.0
    left_region_young_scale: float | None = None
    left_attachment_stiffness_n_per_m: float | None = None
    seconds: float = 0.0
    optional: bool = False


class FacialElasticStudyGrammar:
    """Author independent native snapshots on the original neutral material."""

    @staticmethod
    def create(case: FacialElasticStudyCase, eyes: dict | None = None,
               animated: bool = False) -> str:
        activation = "0.25*(1-cos(radians(60)*t))"
        left = activation if animated else format(case.left_activation, ".12g")
        right = activation if animated else format(case.right_activation, ".12g")
        options = [
            "size(0.16 0.21 0.045) segments(48 64) response(elastic)",
            f"skinMaterial({case.young_modulus_pa:.12g} .35 .001 1000000) solve(2000 .0001)",
            "smile(0 0) jawOpen(0) blink(0 0) browRaise(0 0)",
            "muscle(left_smile .87 .56 34/48 20/64 .055 .08 leftActivation .04 40)",
            "muscle(right_smile .13 .56 14/48 20/64 .055 .08 rightActivation .04 40)",
        ]
        if case.left_region_young_scale is not None:
            options.append("tissueRegion(left_cheek 34/48 20/64 .11 .13 "
                           f"{case.left_region_young_scale:.12g} 1)")
        if case.left_attachment_stiffness_n_per_m is not None:
            options.append("attachment(left_anchor 34/48 20/64 .055 .08 "
                           f"{case.left_attachment_stiffness_n_per_m:.12g})")
        lines = [
            "# Native ProGen3D FacialSkin: neutral-reference static membrane and tensile cables.",
            "# Metres, newtons, pascals. +X character left, +Y up, +Z forward.",
            "# Synthetic material and attachment choices; not calibrated human anatomy.",
            "# Legacy analytic controls remain zero. Time selects independent equilibria.",
            f"Start -> [ Face({left} {right}) ]",
            "",
            "Face(leftActivation rightActivation) ->",
            "    I(FacialSkin(" + "\n      ".join(options) +
            ") material(softpinkmatteplaster) 1 1)",
        ]
        if eyes:
            # These native props share the measured neutral skull references.
            # Their geometry is illustrative and does not drive the solve.
            radius = eyes["radius"]
            for key in ("left_eyeball_center", "right_eyeball_center"):
                x, y, z = eyes[key]
                diameter = 2*radius
                lines.extend([
                    f"    [ T({x:.9g} {y:.9g} {z:.9g}) S({diameter:.9g} {diameter:.9g} {diameter:.9g})",
                    "      I(Sphere(segments(32 24)) material(warmwhitematteplaster) 1 1) ]",
                ])
                for offset, angle, rings, material in (
                    (0.000002, 27, 16, "tealmattemetal"),
                    (0.000004, 11.8125, 7, "blackmattepaint"),
                ):
                    diameter = 2*(radius+offset)
                    lines.extend([
                        f"    [ T({x:.9g} {y:.9g} {z:.9g}) A(90 0) S({diameter:.9g} {diameter:.9g} {diameter:.9g})",
                        f"      I(Sphere(polar(0 {angle}) topology(surface) close(none) segments(32 {rings})) material({material}) 1 1) ]",
                    ])
            mouth_y = eyes["mouth_upper_margin"][1]-0.004
            lines.extend([
                "    # Recessed dark oral proxy; not a physical oral cavity.",
                f"    [ T(0 {mouth_y:.9g} 0.005) S(0.075 0.055 0.016)",
                "      I(Sphere(segments(28 18)) material(blackmattepaint) 1 1) ]",
            ])
        return "\n".join(lines) + "\n"


class NativeElasticSceneObjReader(NativeSceneObjReader):
    """Read the native solver record alongside the existing mesh export."""

    def read(self, path: Path) -> dict[str, Any]:
        scene = super().read(path)
        prefix = "# facial_elastic "
        records = [json.loads(line[len(prefix):]) for line in path.read_text().splitlines()
                   if line.startswith(prefix)]
        if len(records) != 1:
            raise ValueError("Expected one native facial_elastic record; rebuild form-study-sample")
        record = records[0]
        required = {
            "converged", "iterations", "relativeFreeForce", "freeForceL2N", "forceScaleN",
            "initialEnergyJ", "finalEnergyJ", "membraneEnergyJ", "supportEnergyJ", "muscleEnergyJ",
            "maximumGreenStrain", "totalRestAreaM2", "hardAttachmentVertices", "material", "muscles",
            "attachments", "tissueRegions", "shapeKey", "canonicalText",
            "meshFreeForceL2N", "meshRelativeFreeForce", "maximumPositionQuantizationM",
            "solverPrecision", "meshPrecision",
        }
        if not isinstance(record, dict) or not required.issubset(record):
            raise ValueError("Incomplete native facial_elastic record")
        # JSON's permissive NaN extension must never reach a user artifact.
        json.dumps(record, allow_nan=False)
        scene["elastic"] = record
        return scene


class NativeElasticSampleService(NativeFaceSampleService):
    def __init__(self, sampler: Path, destination: Path):
        super().__init__(sampler, destination)
        self.reader = NativeElasticSceneObjReader()


class NativeElasticDiagnosticService:
    """Combine exported geometric checks with actual equilibrium evidence."""

    def __init__(self, neutral: dict):
        self.neutral = neutral
        self.geometry = NativeFacialDiagnosticService()

    def measure(self, case: FacialElasticStudyCase, native: dict) -> dict:
        result = self.geometry.measure(native)
        # The inherited analytic study checks front-chart projection. A valid
        # elastic triangle can turn away from +Z while remaining close to its
        # actual neutral orientation, so use the native triangle's own normal.
        result["negativeFrontProjectionTriangles"] = result.pop("backwardFacingTriangles")
        current = native["objects"][0]["positions"]
        reference = self.neutral["objects"][0]["positions"]
        if len(current) != len(reference):
            raise ValueError("Elastic sample changed the neutral triangle count")
        minimum_cosine, minimum_area_ratio, violations = 1.0, math.inf, 0
        minimum_cross = 1.0e-10*.16*.16
        for offset in range(0, len(reference), 9):
            normals = []
            for coordinates in (reference, current):
                a, b, c = [coordinates[offset+j:offset+j+3] for j in (0, 3, 6)]
                first = [b[k]-a[k] for k in range(3)]
                second = [c[k]-a[k] for k in range(3)]
                normals.append([first[1]*second[2]-first[2]*second[1],
                                first[2]*second[0]-first[0]*second[2],
                                first[0]*second[1]-first[1]*second[0]])
            rest_normal, normal = normals
            rest_length = math.sqrt(sum(v*v for v in rest_normal))
            length = math.sqrt(sum(v*v for v in normal))
            if rest_length <= 0:
                raise ValueError("Native neutral reference has a degenerate triangle")
            projection = sum(x*y for x, y in zip(rest_normal, normal))/rest_length
            if length <= minimum_cross or projection <= minimum_cross:
                violations += 1
            minimum_cosine = min(minimum_cosine, projection/length if length > 0 else -1.0)
            minimum_area_ratio = min(minimum_area_ratio, length/rest_length)
        result["referenceHemisphereViolations"] = violations
        result["minimumReferenceNormalCosine"] = minimum_cosine
        result["minimumReferenceAreaRatio"] = minimum_area_ratio
        result["orientationCriterion"] = "Modest-deformation hemisphere of each triangle's native neutral normal"
        geometry_passed = (
            result["connectedComponents"] == 1 and result["boundaryLoops"] == 4
            and result["boundaryBranches"] == 0 and result["nonmanifoldEdges"] == 0
            and result["minimumTriangleAreaM2"] > 1e-14 and violations == 0
            and result["opposingShadingTriangles"] == 0 and result["maximumNormalLengthError"] < 1e-4
            and min(result[key] for key in ("leftLidGapMm", "rightLidGapMm", "mouthGapMm")) > 0)
        elastic = native["elastic"]
        result["elastic"] = elastic
        result["mouthCornerDisplacements"] = {}
        for side in ("left", "right"):
            name = f"mouth_{side}_corner"
            delta = [1000*(x-y) for x, y in
                     zip(native["landmarks"][name], self.neutral["landmarks"][name])]
            result["mouthCornerDisplacements"][side] = {
                "xyzMm": delta, "lengthMm": math.sqrt(sum(v*v for v in delta))}
        result["maximumSurfaceCoordinateDisplacementMm"] = 1000*maximum_position_difference(
            {"objects": native["objects"][:1]}, {"objects": self.neutral["objects"][:1]})
        expected_activations = {"left_smile": case.left_activation, "right_smile": case.right_activation}
        muscles = {entry["name"]: entry for entry in elastic["muscles"]}
        if set(muscles) != set(expected_activations):
            raise ValueError("Native export does not contain both authored smile cables")
        result["maximumActivationError"] = max(
            abs(muscles[name]["activation"]-expected) for name, expected in expected_activations.items())
        result["cableShorteningMm"] = {
            name: 1000*(muscle["restLengthM"]-muscle["lengthM"])
            for name, muscle in muscles.items()}
        result["energyPartitionErrorJ"] = abs(elastic["finalEnergyJ"]-sum(
            elastic[name] for name in ("membraneEnergyJ", "supportEnergyJ", "muscleEnergyJ")))
        result["relativeForceTolerance"] = 1.0e-4
        result["passed"] = bool(
            geometry_passed and elastic["converged"]
            and 0 <= elastic["relativeFreeForce"] <= 1.0001e-4
            and elastic["finalEnergyJ"] <= elastic["initialEnergyJ"]+1.0e-12
            and min(elastic[name] for name in ("membraneEnergyJ", "supportEnergyJ", "muscleEnergyJ")) >= -1.0e-12
            and elastic["maximumGreenStrain"] <= 0.35000001
            and result["maximumActivationError"] < 2.0e-6
            and result["energyPartitionErrorJ"] < 1.0e-10)
        return result


def create_cases() -> list[FacialElasticStudyCase]:
    return [
        FacialElasticStudyCase("neutral", "Neutral"),
        FacialElasticStudyCase("smile_left", "Left cable · 0.5", .5),
        FacialElasticStudyCase("smile_right", "Right cable · 0.5", 0, .5),
        FacialElasticStudyCase("smile", "Both cables · 0.5", .5, .5),
        FacialElasticStudyCase("soft", "Softer skin · 25 kPa", .5, young_modulus_pa=25000),
        FacialElasticStudyCase("stiff", "Stiffer skin · 100 kPa", .5, young_modulus_pa=100000),
        FacialElasticStudyCase("stiff_left_region", "Stiffer left patch", .5, left_region_young_scale=3),
        FacialElasticStudyCase("strong_left_attachment", "Left attachment · 100 N/m", .5,
                               left_attachment_stiffness_n_per_m=100),
        FacialElasticStudyCase("higher_activation", "Left cable · 0.75", .75, optional=True),
    ]


def package_case(case: FacialElasticStudyCase, native: dict, measured: dict) -> dict:
    elastic = native["elastic"]
    muscles = [dict(muscle, points=[muscle["origin"], muscle["insertion"]])
               for muscle in elastic["muscles"]]
    rows = [
        {"label": "Equilibrium", "value": "Converged" if elastic["converged"] else "Not converged"},
        {"label": "Solver iterations", "value": str(elastic["iterations"])},
        {"label": "Double solve residual", "value": f'{elastic["relativeFreeForce"]:.3g}'},
        {"label": "Float surface residual", "value": f'{elastic["meshRelativeFreeForce"]:.3g}'},
        {"label": "Position rounding", "value": f'{1e9*elastic["maximumPositionQuantizationM"]:.3g} nm'},
        {"label": "Young modulus", "value": f'{elastic["material"]["youngModulusPa"]/1000:g} kPa'},
        {"label": "Total energy", "value": f'{1000*elastic["finalEnergyJ"]:.4g} mJ'},
        {"label": "Membrane energy", "value": f'{1000*elastic["membraneEnergyJ"]:.4g} mJ'},
        {"label": "Support energy", "value": f'{1000*elastic["supportEnergyJ"]:.4g} mJ'},
        {"label": "Cable energy", "value": f'{1000*elastic["muscleEnergyJ"]:.4g} mJ'},
    ]
    for side in ("left", "right"):
        muscle = next(item for item in muscles if item["name"] == f"{side}_smile")
        rows.append({"label": f"{side.title()} cable tension", "value": f'{muscle["tensionN"]:.4g} N'})
        displacement = measured["mouthCornerDisplacements"][side]["lengthMm"]
        rows.append({"label": f"{side.title()} corner motion", "value": f"{displacement:.4g} mm"})
    return {
        "id": case.identifier, "label": case.label, "time": case.seconds,
        "controls": {"leftActivation": case.left_activation, "rightActivation": case.right_activation},
        "authoredCase": asdict(case),
        "objects": [dict(obj, positions=[round(x, 8) for x in obj["positions"]],
                         normals=[round(x, 6) for x in obj["normals"]]) for obj in native["objects"]],
        "landmarks": [{"name": name.replace("_", " "), "position": point}
                      for name, point in native["landmarks"].items() if "center" not in name],
        "muscles": muscles, "diagnostics": measured, "diagnosticRows": rows,
    }


def material_sensitivity(summaries: dict) -> dict:
    reference = summaries["smile_left"]
    reference_shortening = reference["cableShorteningMm"]["left_smile"]
    comparisons = {}
    for identifier in ("soft", "stiff", "stiff_left_region", "strong_left_attachment"):
        measured = summaries[identifier]
        shortening = measured["cableShorteningMm"]["left_smile"]
        comparisons[identifier] = {
            "leftCableShorteningMm": shortening,
            "leftCableShorteningChangeMm": shortening-reference_shortening,
            "leftCornerMotionMm": measured["mouthCornerDisplacements"]["left"]["lengthMm"],
            "rightCornerMotionMm": measured["mouthCornerDisplacements"]["right"]["lengthMm"],
            "leftCableTensionN": next(m["tensionN"] for m in measured["elastic"]["muscles"]
                                      if m["name"] == "left_smile"),
            "finalEnergyJ": measured["elastic"]["finalEnergyJ"],
        }
    return {
        "referenceCase": "smile_left", "activation": .5,
        "referenceLeftCableShorteningMm": reference_shortening,
        "referenceLeftCornerMotionMm": reference["mouthCornerDisplacements"]["left"]["lengthMm"],
        "comparisons": comparisons,
        "globalModulusShorteningOrder": (
            comparisons["soft"]["leftCableShorteningMm"] > reference_shortening >
            comparisons["stiff"]["leftCableShorteningMm"]),
        "interpretation": "Same authored cable activation and geometry; only the stated material or attachment changes.",
    }


def verify_grammar_boundaries(service: NativeElasticSampleService, neutral: dict) -> dict:
    """Exercise native parsing/evaluation/key boundaries without active solves."""
    source = FacialElasticStudyGrammar.create(FacialElasticStudyCase("neutral", "Neutral"))
    canonical = neutral["elastic"]["canonicalText"]
    round_trip = service.sample("grammar_canonical_round_trip",
                                f"Start -> I({canonical} material(softpinkmatteplaster) 1 1)\n")
    same_key = round_trip["elastic"]["shapeKey"] == neutral["elastic"]["shapeKey"]
    round_trip_error = maximum_position_difference(neutral, round_trip)
    variant_keys = {}
    for case in (
        FacialElasticStudyCase("grammar_material_key", "Material key", young_modulus_pa=55000),
        FacialElasticStudyCase("grammar_attachment_key", "Attachment key", left_attachment_stiffness_n_per_m=100),
        FacialElasticStudyCase("grammar_region_key", "Region key", left_region_young_scale=3),
    ):
        sample = service.sample(case.identifier, FacialElasticStudyGrammar.create(case))
        if maximum_position_difference(neutral, sample) > 1e-9:
            raise AssertionError("An unloaded grammar material variant changed the neutral reference")
        variant_keys[case.identifier] = sample["elastic"]["shapeKey"]
    distinct_keys = len({neutral["elastic"]["shapeKey"], *variant_keys.values()}) == 4
    invalid = {
        "analytic_ignored_elastic_options": source.replace("response(elastic)", "response(analytic)"),
        "elastic_legacy_control": source.replace("jawOpen(0)", "jawOpen(.1)"),
        "invalid_material": source.replace("skinMaterial(50000", "skinMaterial(0"),
        "fractional_iteration_limit": source.replace("solve(2000 .0001)", "solve(8.5 .0001)"),
        "duplicate_muscle_name": source.replace("muscle(right_smile", "muscle(left_smile"),
        "zero_muscle_radius": source.replace(".055 .08 leftActivation", "0 .08 leftActivation"),
        "invalid_attachment": source.replace("response(elastic)", "response(elastic) attachment(bad .5 .2 .1 .1 -1)"),
        "invalid_region": source.replace("response(elastic)", "response(elastic) tissueRegion(bad .5 .2 .1 .1 0 1)"),
        "muscle_argument_count": source.replace("leftActivation .04 40)", "leftActivation .04)"),
    }
    for identifier, grammar in invalid.items():
        service.require_rejection("grammar_"+identifier, grammar)
    result = {
        "canonicalRoundTripSameKey": same_key,
        "canonicalRoundTripMaximumDifferenceM": round_trip_error,
        "distinctNeutralConfigurationKeys": distinct_keys,
        "variantKeys": variant_keys,
        "invalidGrammarRejections": list(invalid),
        "passed": same_key and round_trip_error < 1e-9 and distinct_keys,
    }
    if not result["passed"]:
        raise AssertionError("Native elastic canonical text/key regression failed: " + json.dumps(result))
    return result


def create_study(sampler: Path, destination: Path, timeline_frames: int) -> dict:
    destination.mkdir(parents=True, exist_ok=True)
    service = NativeElasticSampleService(sampler, destination / "samples")
    neutral_case = create_cases()[0]
    neutral = service.sample("skin_reference", FacialElasticStudyGrammar.create(neutral_case))
    grammar_checks = verify_grammar_boundaries(service, neutral)
    eyes = dict(neutral["landmarks"], radius=neutral["radius"])
    diagnostic_service = NativeElasticDiagnosticService(neutral)
    catalog = NativeMeshAssetCatalog()
    study = {
        "title": "Elastic facial skin & cables", "units": "m", "cases": [],
        "badge": "Native elastic prototype", "eyebrow": "Material response · Study 02",
        "intro": "Compare native membrane equilibria under authored cable contraction. Change skin stiffness or add a local attachment, then inspect the measured displacement, energy and cable tension.",
        "overlayMeaning": "Cable paths connect the actual fixed origin and solved insertion centroid. These are tensile actuators with measured forces, not reconstructed anatomical muscle fibers.",
        "scopeNote": "Neutral-reference static membrane with elastic skull support and two authored cables. Synthetic material settings; no physiological calibration, bending, inertia, lip self-contact or physical jaw/blink model. Float display surfaces derive from retained double-precision equilibria; both force residuals are reported separately.",
        "model": "Plane-stress St Venant–Kirchhoff membrane; area-weighted neutral support; tensile origin/insertion cables; static energy minimization",
        "controlLabels": [["leftActivation", "Left cable"], ["rightActivation", "Right cable"]],
    }
    summaries = {}
    omitted = []
    for case in create_cases():
        try:
            native = service.sample(case.identifier, FacialElasticStudyGrammar.create(case, eyes))
            measured = diagnostic_service.measure(case, native)
            if case.optional and not measured["passed"]:
                omitted.append({"id": case.identifier, "reason": "Native diagnostic gate failed", "diagnostics": measured})
                continue
        except (RuntimeError, ValueError) as error:
            if not case.optional:
                raise
            omitted.append({"id": case.identifier, "reason": str(error)})
            continue
        summaries[case.identifier] = measured
        study["cases"].append(catalog.store_case(package_case(case, native, measured)))
        print(f"{case.identifier}: iterations={measured['elastic']['iterations']} "
              f"relative_force={measured['elastic']['relativeFreeForce']:.4g} passed={measured['passed']}", flush=True)
    animated = FacialElasticStudyGrammar.create(neutral_case, eyes, animated=True)
    (destination / "Facial_Elastic_Study.p3d").write_text(animated)
    for index in range(timeline_frames):
        seconds = 6*index/(timeline_frames-1)
        activation = .25*(1-math.cos(math.pi*seconds/3))
        case = FacialElasticStudyCase(f"timeline_{index:03}", f"Equilibrium · {seconds:.2f} s",
                                     activation, activation, seconds=seconds)
        native = service.sample(case.identifier, animated, seconds)
        measured = diagnostic_service.measure(case, native)
        summaries[case.identifier] = measured
        study["cases"].append(catalog.store_case(package_case(case, native, measured)))
        print(f"{case.identifier}: iterations={measured['elastic']['iterations']} "
              f"relative_force={measured['elastic']['relativeFreeForce']:.4g} passed={measured['passed']}", flush=True)
    # These are complete native evaluations even with no displayed timeline.
    # Their order deliberately revisits an earlier time after a distant time.
    first = service.sample("seek_first", animated, 1.5)
    far = service.sample("seek_far", animated, 5.25)
    repeated = service.sample("seek_repeat", animated, 1.5)
    end = service.sample("neutral_return", animated, 6)
    start = service.sample("neutral_start", animated, 0)
    seek_error = maximum_position_difference(first, repeated)
    return_error = maximum_position_difference(start, end)
    replay_diagnostics = {}
    for identifier, seconds, native in (
        ("seek_first", 1.5, first), ("seek_far", 5.25, far), ("seek_repeat", 1.5, repeated),
        ("neutral_return", 6, end), ("neutral_start", 0, start),
    ):
        activation = .25*(1-math.cos(math.pi*seconds/3))
        replay_diagnostics[identifier] = diagnostic_service.measure(
            FacialElasticStudyCase(identifier, identifier, activation, activation, seconds=seconds), native)
    counts = {(m["vertices"], m["triangles"]) for m in summaries.values()}
    report = {
        "schema": "progen3d.facial-elastic.validation.v1", "cases": summaries,
        "caseCount": len(summaries), "omittedOptionalCases": omitted,
        "timelineFrames": timeline_frames, "stableTopologyCounts": len(counts) == 1,
        "seekMaximumDifferenceM": seek_error, "neutralReturnMaximumDifferenceM": return_error,
        "replayProcessMode": "A fresh native sampler process evaluates each requested snapshot",
        "replayDiagnostics": replay_diagnostics,
        "grammarBoundaryChecks": grammar_checks,
        "materialSensitivity": material_sensitivity(summaries),
        "presentation": catalog.summary(),
        "scope": "Native equilibrium convergence, exported energies/forces, surface geometry, material response and repeatable time evaluation",
        "limitations": [
            "Open front-face patch with four boundary loops; not a complete head or solid tissue volume",
            "Synthetic homogeneous base material plus authored UV regions; no human anatomical calibration",
            "Neutral material reference only; nonzero legacy analytic controls are rejected in elastic mode",
            "Static equilibria only; no inertia, damping, bending stiffness or activation dynamics",
            "Fixed outline and protective eye collars; eye/facet feasibility checks are not a general contact-reaction solver",
            "No lip self-contact or physical jaw/blink response; illustrated eyes and mouth are visual props",
            "The convergence tolerance applies to the retained double state; float display rounding has a separately reported force residual",
        ],
    }
    report["passed"] = (
        all(m["passed"] for m in summaries.values())
        and all(m["passed"] for m in replay_diagnostics.values())
        and grammar_checks["passed"] and len(counts) == 1
        and seek_error < 1e-9 and return_error < 1e-7)
    (destination / "validation.json").write_text(json.dumps(report, indent=2, allow_nan=False)+"\n")
    study.update(validation=report, meshAssets=catalog.assets, meshTopologies=catalog.topologies,
                 presentation=catalog.summary())
    serialized = json.dumps(study, separators=(",", ":"), allow_nan=False).replace("</", "<\\/")
    template = Path(__file__).with_name("inspector.template.html").read_text()
    if template.count("__FACIAL_STUDY_DATA__") != 1:
        raise ValueError("Inspector must contain exactly one data token")
    (destination / "Facial_Elastic_Study.html").write_text(template.replace("__FACIAL_STUDY_DATA__", serialized))
    (destination / "study.json").write_text(serialized)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampler", type=Path, default=Path("build/form-study-sample"))
    parser.add_argument("--out", type=Path, default=Path("build/facial-elastic-study"))
    parser.add_argument("--timeline-frames", type=int, default=0,
                        help="0 omits the timeline; 13 samples the six-second activation cycle")
    args = parser.parse_args()
    if args.timeline_frames != 0 and not 2 <= args.timeline_frames <= 181:
        parser.error("--timeline-frames must be 0 or between 2 and 181")
    report = create_study(args.sampler, args.out, args.timeline_frames)
    print(json.dumps({key: report[key] for key in (
        "passed", "caseCount", "timelineFrames", "stableTopologyCounts",
        "seekMaximumDifferenceM", "neutralReturnMaximumDifferenceM", "materialSensitivity")}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
