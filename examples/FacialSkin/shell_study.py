#!/usr/bin/env python3
"""Compare native shell bending and mouth discretization without a second solver.

The required 2x2 comparison separates mesh changes from bending. Weak-foundation
diagnostic candidates retain their actual rejection or convergence records.
"""
from __future__ import annotations

import argparse
from dataclasses import asdict, dataclass
import html
import json
import math
from pathlib import Path
import subprocess

from elastic_study import (
    FacialElasticStudyCase,
    FacialElasticStudyGrammar,
    NativeElasticDiagnosticService,
    NativeElasticSampleService,
    NativeElasticSceneObjReader,
    package_case as package_elastic_case,
)
from face_study import NativeMeshAssetCatalog, maximum_position_difference


@dataclass(frozen=True)
class FacialShellStudyCase(FacialElasticStudyCase):
    bending_scale: float = 0.0
    mouth_mesh_mode: str = "legacy"
    horizontal_segments: int = 48
    vertical_segments: int = 64
    support_n_per_m3: float = 1000000.0
    maximum_shortening: float = .04

    def reference_key(self) -> tuple[str, int, int]:
        return self.mouth_mesh_mode, self.horizontal_segments, self.vertical_segments


class FacialShellStudyGrammar:
    """Extend the existing native material/cable authoring with explicit C options."""

    @staticmethod
    def create(case: FacialShellStudyCase, eyes: dict | None = None) -> str:
        source = FacialElasticStudyGrammar.create(case, eyes)
        replacements = [
            ("# Native ProGen3D FacialSkin: neutral-reference static membrane and tensile cables.",
             "# Native ProGen3D FacialSkin: independent mouth mesh and rest-curvature bending."),
            ("response(elastic)",
             f"response(elastic) bending({case.bending_scale:.12g}) mouthMesh({case.mouth_mesh_mode})"),
            ("segments(48 64)", f"segments({case.horizontal_segments} {case.vertical_segments})"),
            (".35 .001 1000000) solve", f".35 .001 {case.support_n_per_m3:.12g}) solve"),
            ("leftActivation .04 40)", f"leftActivation {case.maximum_shortening:.12g} 40)"),
            ("rightActivation .04 40)", f"rightActivation {case.maximum_shortening:.12g} 40)"),
        ]
        for before, after in replacements:
            if source.count(before) != 1:
                raise ValueError(f"Shared elastic grammar changed the expected authoring field: {before}")
            source = source.replace(before, after, 1)
        return source


class NativeShellSceneObjReader(NativeElasticSceneObjReader):
    def read(self, path: Path) -> dict:
        scene = super().read(path)
        mechanics = scene["elastic"]
        required = {"bendingEnergyJ", "maximumBendingCurvatureChangePerM", "bendingScale",
                    "mouthMeshMode", "bendingAssembly", "initialFreeForceL2N"}
        if not required.issubset(mechanics):
            raise ValueError("Incomplete native shell metadata; rebuild form-study-sample")
        assembly_fields = {
            "activeStencils", "fixedOnlyTriangles", "fixedOnlyUnreconstructableTriangles",
            "expandedStencils", "maximumPatchVertices", "maximumNeighborhoodRings",
            "maximumReconstructionCondition", "maximumPolynomialReproductionError",
            "maximumRestCurvaturePerM", "maximumPatchHeightRatio",
        }
        if not assembly_fields.issubset(mechanics["bendingAssembly"]):
            raise ValueError("Incomplete native curvature assembly diagnostics")
        return scene


class NativeShellSampleService(NativeElasticSampleService):
    def __init__(self, sampler: Path, destination: Path):
        super().__init__(sampler, destination)
        self.reader = NativeShellSceneObjReader()

    def sample(self, identifier: str, grammar: str, seconds: float = 0.0) -> dict:
        source = self.destination/(identifier+".p3d")
        output = self.destination/(identifier+".obj")
        source.write_text(grammar)
        # An unsuccessful new evaluation must never reuse an older export.
        output.unlink(missing_ok=True)
        completed = subprocess.run(
            [str(self.sampler), str(source), format(seconds, ".12g"), str(output), "--require-facial-skin"],
            text=True, capture_output=True, timeout=90)
        native_diagnostics = ("Native stderr:\n"+completed.stderr+
                              "\nNative stdout:\n"+completed.stdout)
        if completed.returncode:
            raise RuntimeError(f"Native evaluation failed for {identifier} "
                               f"(exit {completed.returncode}):\n{native_diagnostics}")
        try:
            return self.reader.read(output)
        except (ValueError, OSError) as error:
            # Some engine generation failures are logged before an empty OBJ
            # is written. Preserve that actual cause rather than only the
            # reader's secondary 'missing metadata' symptom.
            raise RuntimeError(f"Native export for {identifier} could not be read: {error}\n"
                               f"{native_diagnostics}") from error


class NativeShellDiagnosticService(NativeElasticDiagnosticService):
    ENERGY_TERMS = (*NativeElasticDiagnosticService.ENERGY_TERMS, "bendingEnergyJ")

    def measure(self, case: FacialShellStudyCase, native: dict) -> dict:
        result = super().measure(case, native)
        mechanics = native["elastic"]
        current = native["objects"][0]["positions"]
        reference = self.neutral["objects"][0]["positions"]
        result["maximumVertexDisplacementMm"] = 1000*max(
            math.sqrt(sum((current[index+axis]-reference[index+axis])**2 for axis in range(3)))
            for index in range(0, len(current), 3))
        result["authoredCase"] = asdict(case)
        result["bendingScaleMatches"] = abs(mechanics["bendingScale"]-case.bending_scale) < 1e-6
        result["mouthMeshModeMatches"] = mechanics["mouthMeshMode"] == case.mouth_mesh_mode
        result["materialSupportMatches"] = abs(
            mechanics["material"]["supportNPerM3"]-case.support_n_per_m3) <= 1e-6*case.support_n_per_m3
        result["maximumShorteningError"] = max(
            abs(muscle["maximumShortening"]-case.maximum_shortening) for muscle in mechanics["muscles"])
        result["stencilActivationMatches"] = (
            mechanics["bendingAssembly"]["activeStencils"] > 0 if case.bending_scale > 0
            else mechanics["bendingAssembly"]["activeStencils"] == 0)
        result["neutralReferencePreserved"] = (
            result["maximumVertexDisplacementMm"] < 1e-6
            if case.left_activation == case.right_activation == 0 else True)
        active = case.left_activation > 0 or case.right_activation > 0
        initial_force = mechanics["initialFreeForceL2N"]
        result["initialLoadRelativeFreeForce"] = (
            mechanics["freeForceL2N"]/initial_force if active and initial_force > 0 else None)
        result["forceScaleToInitialLoadRatio"] = (
            mechanics["forceScaleN"]/initial_force if initial_force > 0 else None)
        result["normalizationFloorDominatesInitialForce"] = (
            mechanics["forceScaleN"] > initial_force*(1+1e-6))
        result["initialLoadConvergencePassed"] = (
            not active or (result["initialLoadRelativeFreeForce"] is not None
                           and result["initialLoadRelativeFreeForce"] <= 1.0001e-4))
        result["passed"] = bool(
            result["passed"] and result["bendingScaleMatches"] and result["mouthMeshModeMatches"]
            and result["materialSupportMatches"] and result["stencilActivationMatches"]
            and result["neutralReferencePreserved"] and result["maximumShorteningError"] < 1e-6
            and result["initialLoadConvergencePassed"])
        return result


def create_cases() -> list[FacialShellStudyCase]:
    cases = []
    for mode in ("legacy", "quality"):
        for scale, response in ((0.0, "membrane"), (1.0, "shell")):
            for pose, left, right in (("neutral", 0, 0), ("left", .5, 0), ("right", 0, .5)):
                cases.append(FacialShellStudyCase(
                    f"{mode}_{response}_{pose}", f"{mode.title()} · {response} · {pose}", left, right,
                    bending_scale=scale, mouth_mesh_mode=mode))
    cases.append(FacialShellStudyCase(
        "quality_shell_fine_left", "Quality shell · 96 × 128", .5,
        bending_scale=1, mouth_mesh_mode="quality", horizontal_segments=96, vertical_segments=128))
    for mode in ("legacy", "quality"):
        cases.append(FacialShellStudyCase(
            f"{mode}_shell_weak_foundation_left", f"Weak support · {mode} · diagnostic", .5,
            bending_scale=1, mouth_mesh_mode=mode, support_n_per_m3=10000,
            maximum_shortening=.12, optional=True))
    return cases


def package_case(case: FacialShellStudyCase, native: dict, measured: dict) -> dict:
    result = package_elastic_case(case, native, measured)
    mechanics = native["elastic"]
    assembly = mechanics["bendingAssembly"]
    initial_load_residual = measured["initialLoadRelativeFreeForce"]
    result["diagnosticRows"] = [
        {"label": "Mouth mesh", "value": case.mouth_mesh_mode},
        {"label": "Segments", "value": f"{case.horizontal_segments} × {case.vertical_segments}"},
        {"label": "Bending scale", "value": f"{case.bending_scale:g}"},
        {"label": "Bending energy", "value": f'{1e6*mechanics["bendingEnergyJ"]:.4g} µJ'},
        {"label": "Curvature change", "value": f'{mechanics["maximumBendingCurvatureChangePerM"]:.4g} m⁻¹'},
        {"label": "Active stencils", "value": str(assembly["activeStencils"])},
        {"label": "Worst reconstruction condition", "value": f'{assembly["maximumReconstructionCondition"]:.4g}'},
        {"label": "Largest skin motion", "value": f'{measured["maximumVertexDisplacementMm"]:.4g} mm'},
        {"label": "Residual / initial load", "value": f"{initial_load_residual:.3g}" if initial_load_residual is not None else "Not applicable"},
        {"label": "Normalization floor", "value": (
            "Used for zero load" if initial_load_residual is None else
            "Dominant" if measured["normalizationFloorDominatesInitialForce"] else "Not dominant")},
        *result["diagnosticRows"],
    ]
    return result


def skin_only(native: dict) -> dict:
    return dict(native, objects=native["objects"][:1])


def verify_grammar_boundaries(service: NativeShellSampleService, reference: dict,
                              shell_neutral: dict) -> dict:
    source = FacialElasticStudyGrammar.create(FacialElasticStudyCase("neutral", "Neutral"))
    omitted = service.sample("grammar_omitted_c_options", source)
    legacy_key_equal = omitted["elastic"]["shapeKey"] == reference["elastic"]["shapeKey"]
    legacy_text_equal = omitted["elastic"]["canonicalText"] == reference["elastic"]["canonicalText"]
    legacy_error = maximum_position_difference(omitted, reference)
    canonical = shell_neutral["elastic"]["canonicalText"]
    round_trip = service.sample("grammar_shell_canonical",
                                f"Start -> I({canonical} material(softpinkmatteplaster) 1 1)\n")
    expression = canonical.replace("bending(1)", "bending(0.5+0.5+0*t)")
    if expression == canonical:
        raise AssertionError("Expected an explicit unit bending scale in native canonical text")
    expression_sample = service.sample("grammar_bending_expression",
                                      f"Start -> I({expression} material(softpinkmatteplaster) 1 1)\n", 1.25)
    same_shell_key = round_trip["elastic"]["shapeKey"] == shell_neutral["elastic"]["shapeKey"]
    expression_key_equal = expression_sample["elastic"]["shapeKey"] == shell_neutral["elastic"]["shapeKey"]
    round_trip_error = maximum_position_difference(skin_only(shell_neutral), round_trip)
    expression_error = maximum_position_difference(skin_only(shell_neutral), expression_sample)
    def with_option(option: str) -> str:
        return source.replace("response(elastic)", "response(elastic) "+option, 1)
    invalid = {
        "bending_negative": with_option("bending(-1)"),
        "bending_above_bound": with_option("bending(101)"),
        "bending_nonfinite_expression": with_option("bending(0/0)"),
        "bending_missing_argument": with_option("bending()"),
        "bending_extra_argument": with_option("bending(1 2)"),
        "bending_duplicate_option": with_option("bending(1) bending(2)"),
        "mouth_mesh_unknown": with_option("mouthMesh(wrong)"),
        "mouth_mesh_missing_argument": with_option("mouthMesh()"),
        "mouth_mesh_extra_argument": with_option("mouthMesh(quality legacy)"),
        "analytic_bending_zero": "Start -> I(FacialSkin(bending(0)) material(softpinkmatteplaster) 1 1)\n",
        "analytic_explicit_legacy": "Start -> I(FacialSkin(mouthMesh(legacy)) material(softpinkmatteplaster) 1 1)\n",
        "analytic_quality": "Start -> I(FacialSkin(mouthMesh(quality)) material(softpinkmatteplaster) 1 1)\n",
    }
    for identifier, grammar in invalid.items():
        service.require_rejection("grammar_"+identifier, grammar)
    result = {
        "explicitZeroLegacyPreservesV2Key": legacy_key_equal,
        "explicitZeroLegacyPreservesCanonicalText": legacy_text_equal,
        "explicitZeroLegacyMaximumDifferenceM": legacy_error,
        "shellCanonicalRoundTripSameKey": same_shell_key,
        "shellCanonicalRoundTripMaximumDifferenceM": round_trip_error,
        "bendingExpressionAndTimeSameKey": expression_key_equal,
        "bendingExpressionMaximumDifferenceM": expression_error,
        "invalidGrammarRejections": list(invalid),
    }
    result["passed"] = all((legacy_key_equal, legacy_text_equal, same_shell_key, expression_key_equal,
                            legacy_error < 1e-9, round_trip_error < 1e-9, expression_error < 1e-9))
    return result


def comparisons(summaries: dict) -> dict:
    result = {}
    for mode in ("legacy", "quality"):
        identifiers = [f"{mode}_membrane_left", f"{mode}_shell_left"]
        if not all(identifier in summaries for identifier in identifiers):
            continue
        result[mode] = {}
        for identifier in identifiers:
            measured = summaries[identifier]
            result[mode][identifier] = {
                "maximumVertexDisplacementMm": measured["maximumVertexDisplacementMm"],
                "leftCornerMotionMm": measured["mouthCornerDisplacements"]["left"]["lengthMm"],
                "leftCableShorteningMm": measured["cableShorteningMm"]["left_smile"],
                "bendingEnergyJ": measured["elastic"]["bendingEnergyJ"],
                "finalEnergyJ": measured["elastic"]["finalEnergyJ"],
                "relativeFreeForce": measured["elastic"]["relativeFreeForce"],
            }
    coarse, fine = summaries.get("quality_shell_left"), summaries.get("quality_shell_fine_left")
    if coarse and fine:
        result["refinement"] = {
            "coarseMaximumDisplacementMm": coarse["maximumVertexDisplacementMm"],
            "fineMaximumDisplacementMm": fine["maximumVertexDisplacementMm"],
            "coarseLeftCableShorteningMm": coarse["cableShorteningMm"]["left_smile"],
            "fineLeftCableShorteningMm": fine["cableShorteningMm"]["left_smile"],
            "leftCornerDisplacementDifferenceMm": math.sqrt(sum(
                (x-y)**2 for x, y in zip(coarse["mouthCornerDisplacements"]["left"]["xyzMm"],
                                        fine["mouthCornerDisplacements"]["left"]["xyzMm"]))),
            "scope": "One bounded refinement observation; not a mesh-convergence proof",
        }
    return result


def diagnostic_candidate_section(outcomes: list[dict]) -> str:
    parts = [
        '<section class="panel" style="margin-top:20px" aria-label="Diagnostic candidate outcomes">',
        '<h2>Weak-foundation diagnostic trials</h2>',
        '<p class="panel-note">Both meshes use support 10,000 N/m³, maximum cable shortening 0.12 '
        'and left activation 0.5. These deliberately stronger trials are reported separately from '
        'the required comparison. Rejected states are not displayed as equilibria.</p>',
    ]
    for outcome in outcomes:
        parts.append('<details class="details" open><summary>'+html.escape(outcome["label"])+
                     ' — '+html.escape(outcome["status"])+ '</summary>')
        if outcome["status"] == "converged":
            parts.append('<p class="panel-note">Native force, geometry and display checks passed; '
                         'the resulting surface is available among the samples.</p>')
        else:
            parts.append('<pre>'+html.escape(outcome["reason"])+ '</pre>')
        parts.append('</details>')
    parts.append('</section>')
    return "\n".join(parts)


def create_study(sampler: Path, destination: Path) -> dict:
    destination.mkdir(parents=True, exist_ok=True)
    service = NativeShellSampleService(sampler, destination/"samples")
    cases = create_cases()
    references = {}
    for mode, horizontal, vertical in sorted({case.reference_key() for case in cases}):
        identifier = f"reference_{mode}_{horizontal}_{vertical}"
        case = FacialShellStudyCase(identifier, "Neutral reference", mouth_mesh_mode=mode,
                                    horizontal_segments=horizontal, vertical_segments=vertical)
        references[case.reference_key()] = service.sample(identifier, FacialShellStudyGrammar.create(case))
    catalog = NativeMeshAssetCatalog()
    study = {
        "title": "Facial shell & mouth mesh", "units": "m", "cases": [],
        "badge": "Native shell prototype", "eyebrow": "Curvature response · Study 03",
        "intro": "Compare the same cable activation across two mouth meshes, with bending off or on. Inspect the actual native equilibria, curvature reconstruction and force residuals. Stronger weak-foundation trials retain both their successes and their rejection records below.",
        "overlayMeaning": "Cable paths join the actual native origin and solved insertion centroid. They are authored tensile actuators, not reconstructed anatomical muscle fibers.",
        "scopeNote": "Static reference-chart shell prototype with isotropic membrane and bending energies, neutral support and explicit cables. No calibrated anatomy, volume, inertia, general self-contact or physical jaw/lid actuation. Double-state convergence and float display residuals remain distinct.",
        "model": "Native StVK membrane plus quadratic rest-chart tensor-curvature bending; independent mouth mesh option",
        "controlLabels": [["leftActivation", "Left cable"], ["rightActivation", "Right cable"]],
    }
    summaries, failures, candidate_outcomes = {}, [], []
    shell_neutral = None
    example_source = None
    for case in cases:
        reference = references[case.reference_key()]
        eyes = dict(reference["landmarks"], radius=reference["radius"])
        grammar = FacialShellStudyGrammar.create(case, eyes)
        try:
            native = service.sample(case.identifier, grammar)
            measured = NativeShellDiagnosticService(reference).measure(case, native)
            summaries[case.identifier] = measured
            if not measured["passed"]:
                reason = "Native export failed the study diagnostics:\n"+json.dumps(measured, indent=2, allow_nan=False)
                status = "diagnostic checks failed"
            else:
                status, reason = "converged", ""
                study["cases"].append(catalog.store_case(package_case(case, native, measured)))
                if case.identifier == "quality_shell_neutral":
                    shell_neutral = native
                if case.identifier == "quality_shell_left":
                    example_source = grammar
                print(f"{case.identifier}: iterations={native['elastic']['iterations']} "
                      f"relative_force={native['elastic']['relativeFreeForce']:.6g} "
                      f"bending_energy_J={native['elastic']['bendingEnergyJ']:.6g} passed=True", flush=True)
        except (RuntimeError, ValueError, subprocess.TimeoutExpired) as error:
            status, reason = "rejected", str(error)
        outcome = {"id": case.identifier, "label": case.label, "status": status,
                   "authoredCase": asdict(case), "reason": reason}
        if case.optional:
            candidate_outcomes.append(outcome)
        if status != "converged":
            print(f"{case.identifier}: {status}", flush=True)
            print(reason if status == "rejected" else "Full native diagnostics retained in validation.json", flush=True)
            if not case.optional:
                failures.append(outcome)
    grammar_checks = {"passed": False, "reason": "Required quality shell neutral state is unavailable"}
    if shell_neutral is not None:
        grammar_checks = verify_grammar_boundaries(service, references[("legacy", 48, 64)], shell_neutral)
    topology_groups = {}
    for measured in summaries.values():
        authored = measured["authoredCase"]
        group = f'{authored["mouth_mesh_mode"]}_{authored["horizontal_segments"]}_{authored["vertical_segments"]}'
        topology_groups.setdefault(group, set()).add((measured["vertices"], measured["triangles"]))
    topology_stable = all(len(counts) == 1 for counts in topology_groups.values())
    report = {
        "schema": "progen3d.facial-shell.validation.v1",
        "cases": summaries, "caseCount": len(study["cases"]),
        "requiredCaseCount": sum(not case.optional for case in cases),
        "requiredFailures": failures, "diagnosticCandidates": candidate_outcomes,
        "grammarBoundaryChecks": grammar_checks,
        "stableTopologyWithinEachMeshAndResolution": topology_stable,
        "topologyGroups": {key: [list(pair) for pair in sorted(value)] for key, value in topology_groups.items()},
        "comparisons": comparisons(summaries), "presentation": catalog.summary(),
        "scope": "Native 2x2 mesh/bending comparison, one refinement, and two deliberately stronger diagnostic candidates",
        "limitations": [
            "Reference-chart small-strain shell; not a full large-strain continuum, solid tissue or anatomically calibrated model",
            "Each mesh/resolution uses its own neutral material reference and hemisphere test",
            "No general self-contact or constrained contact-equilibrium solver; infeasible force balance is rejected",
            "Allowed inputs do not guarantee convergence; both weak-foundation outcomes are retained",
            "Double equilibrium and float presentation forces are measured separately",
        ],
    }
    report["passed"] = not failures and grammar_checks["passed"] and topology_stable
    (destination/"validation.json").write_text(json.dumps(report, indent=2, allow_nan=False)+"\n")
    study.update(validation=report, meshAssets=catalog.assets, meshTopologies=catalog.topologies,
                 presentation=catalog.summary())
    serialized = json.dumps(study, separators=(",", ":"), allow_nan=False).replace("</", "<\\/")
    template = Path(__file__).with_name("inspector.template.html").read_text()
    if template.count("__FACIAL_STUDY_DATA__") != 1 or template.count('<footer class="footer">') != 1:
        raise ValueError("Unexpected shared inspector packaging markers")
    template = template.replace('<footer class="footer">',
                                diagnostic_candidate_section(candidate_outcomes)+'\n<footer class="footer">', 1)
    (destination/"Facial_Shell_Study.html").write_text(template.replace("__FACIAL_STUDY_DATA__", serialized))
    (destination/"study.json").write_text(serialized)
    if report["passed"] and example_source is not None:
        (destination/"Facial_Shell_Study.p3d").write_text(example_source)
    return report


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampler", type=Path, default=Path("build/form-study-sample"))
    parser.add_argument("--out", type=Path, default=Path("build/facial-shell-study"))
    args = parser.parse_args()
    report = create_study(args.sampler, args.out)
    print(json.dumps({key: report[key] for key in (
        "passed", "caseCount", "requiredCaseCount", "requiredFailures", "diagnosticCandidates",
        "stableTopologyWithinEachMeshAndResolution", "comparisons")}, indent=2))
    return 0 if report["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
