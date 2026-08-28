#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import math
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any


EXPECTED_VARIANTS = ("reference", "track", "aero", "crossover")
EXPECTED_MANIFEST_SHA256 = "9053996bc0e8ba0017ad8eeffb616ab801b0b709dbe0fef93708daa93518882b"
EXPECTED_FILE_COUNT = 109
EXPECTED_TOTAL_SIZE_BYTES = 103068076
EXPECTED_TEST_COUNT = 34
EXPECTED_HARDPOINT_COUNT = 32
EXPECTED_POSE_COUNT = 36
EXPECTED_CLOSURE_COUNT = 6
EXPECTED_GLASS_COUNT = 4
EXPECTED_PANEL_COUNT = 17
EXPECTED_APERTURE_COUNT = 6
EXPECTED_EXCLUSIVE_OWNER_COUNT = 23


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


@dataclass(frozen=True)
class McsMv220SourceArtifact:
    relative_path: str
    byte_count: int
    sha256: str


@dataclass
class McsMv220SourceContractValidationResult:
    source_package: str
    errors: list[str] = field(default_factory=list)
    facts: dict[str, Any] = field(default_factory=dict)

    def succeeded(self) -> bool:
        return not self.errors

    def as_dict(self) -> dict[str, Any]:
        return {
            "schema": "MCSMv2.2.SourceContractValidation.v1",
            "source_package": self.source_package,
            "passed": self.succeeded(),
            "errors": self.errors,
            "facts": self.facts,
        }


class McsMv220SourceContractValidationService:
    def validate(self, source_package: Path) -> McsMv220SourceContractValidationResult:
        source_package = source_package.resolve()
        result = McsMv220SourceContractValidationResult(str(source_package))
        self._validate_required_files(source_package, result)
        if result.errors:
            return result

        manifest = self._read_json(source_package / "RELEASE_MANIFEST.json", result)
        version = self._read_json(source_package / "VERSION.json", result)
        verification = self._read_json(source_package / "RELEASE_VERIFICATION.json", result)
        grammar = self._read_json(source_package / "GRAMMAR_VALIDATION.json", result)
        checklist = self._read_json(source_package / "IMPLEMENTATION_CHECKLIST.json", result)
        validation = self._read_json(source_package / "validation.json", result)
        if result.errors:
            return result

        self._validate_manifest(source_package, manifest, result)
        self._validate_version(version, result)
        self._validate_verification(manifest, verification, result)
        self._validate_grammar(grammar, result)
        self._validate_checklist(checklist, result)
        self._validate_variants(source_package, validation, result)
        self._validate_classification(source_package, result)
        return result

    @staticmethod
    def _validate_required_files(
        source_package: Path,
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        required = (
            "RELEASE_MANIFEST.json",
            "RELEASE_VERIFICATION.json",
            "VERSION.json",
            "GRAMMAR_VALIDATION.json",
            "IMPLEMENTATION_CHECKLIST.json",
            "validation.json",
            "source/generate_release.py",
            "source/modern_car_mcsmv2.py",
            "source/mcsmv201_base.py",
            "source/mcsmv1_legacy.py",
            "source/pyproject.toml",
            "source/requirements.txt",
            "source/tests/test_mcsmv22.py",
        )
        for relative_path in required:
            if not (source_package / relative_path).is_file():
                result.errors.append(f"missing required source artifact: {relative_path}")

    @staticmethod
    def _read_json(
        path: Path,
        result: McsMv220SourceContractValidationResult,
    ) -> dict[str, Any]:
        try:
            value = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            result.errors.append(f"could not parse {path.name}: {error}")
            return {}
        if not isinstance(value, dict):
            result.errors.append(f"expected JSON object: {path.name}")
            return {}
        return value

    @staticmethod
    def _validate_manifest(
        source_package: Path,
        manifest: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        manifest_path = source_package / "RELEASE_MANIFEST.json"
        manifest_sha256 = sha256(manifest_path)
        if manifest_sha256 != EXPECTED_MANIFEST_SHA256:
            result.errors.append(
                "release manifest SHA-256 does not match the accepted MCSMv2.2 release"
            )
        if manifest.get("schema") != "MCSMv2.2.ReleaseManifest.v1":
            result.errors.append("release manifest schema is not MCSMv2.2.ReleaseManifest.v1")
        if manifest.get("version") != "2.2.0":
            result.errors.append("release manifest version is not 2.2.0")
        if manifest.get("root_name") != "MCSMv2_2_0_RELEASE":
            result.errors.append("release manifest root_name is not MCSMv2_2_0_RELEASE")
        if manifest.get("manifest_excludes") != ["RELEASE_MANIFEST.json"]:
            result.errors.append("release manifest exclusions are not exact")

        records = manifest.get("files")
        if not isinstance(records, list):
            result.errors.append("release manifest files must be an array")
            return
        if manifest.get("file_count") != EXPECTED_FILE_COUNT or len(records) != EXPECTED_FILE_COUNT:
            result.errors.append("release manifest must contain exactly 109 signed artifacts")
        if manifest.get("total_size_bytes") != EXPECTED_TOTAL_SIZE_BYTES:
            result.errors.append("release manifest total_size_bytes does not match the accepted release")

        seen: set[str] = set()
        artifacts: list[McsMv220SourceArtifact] = []
        for record in records:
            if not isinstance(record, dict):
                result.errors.append("release manifest contains a non-object file record")
                continue
            relative_path = str(record.get("path", ""))
            if not relative_path or relative_path in seen:
                result.errors.append(f"invalid or duplicate manifest path: {relative_path}")
                continue
            seen.add(relative_path)
            artifact_path = source_package / relative_path
            if not artifact_path.is_file():
                result.errors.append(f"missing signed artifact: {relative_path}")
                continue
            byte_count = artifact_path.stat().st_size
            digest = sha256(artifact_path)
            if record.get("size_bytes") != byte_count:
                result.errors.append(f"signed artifact size mismatch: {relative_path}")
            if record.get("sha256") != digest:
                result.errors.append(f"signed artifact hash mismatch: {relative_path}")
            artifacts.append(McsMv220SourceArtifact(relative_path, byte_count, digest))

        actual_paths = {
            str(path.relative_to(source_package))
            for path in source_package.rglob("*")
            if path.is_file() and path.name != "RELEASE_MANIFEST.json"
        }
        unexpected = sorted(actual_paths - seen)
        omitted = sorted(seen - actual_paths)
        if unexpected:
            result.errors.append("unexpected files inside signed release: " + ", ".join(unexpected))
        if omitted:
            result.errors.append("manifest files absent from signed release: " + ", ".join(omitted))

        total_size = sum(artifact.byte_count for artifact in artifacts)
        if total_size != EXPECTED_TOTAL_SIZE_BYTES:
            result.errors.append("signed artifact byte total does not match the accepted release")
        result.facts["release_manifest_sha256"] = manifest_sha256
        result.facts["signed_artifact_count"] = len(artifacts)
        result.facts["signed_total_size_bytes"] = total_size

    @staticmethod
    def _validate_version(
        version: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        if version.get("version") != "2.2.0":
            result.errors.append("VERSION.json version is not 2.2.0")
        if version.get("model_schema") != "MCSMv2.2-uv-semantic-closure-suspension-kinematics":
            result.errors.append("VERSION.json model_schema is not the accepted MCSMv2.2 schema")
        reference_frame = version.get("reference_frame")
        if not isinstance(reference_frame, dict):
            result.errors.append("VERSION.json reference_frame is missing")
            return
        if reference_frame.get("handedness") != "right":
            result.errors.append("MCSMv2.2 source frame must be right-handed")
        if reference_frame.get("length_unit") != "m" or reference_frame.get("angle_unit") != "deg":
            result.errors.append("MCSMv2.2 source units must be metres and degrees")
        expected_matrix = [
            [0.0, 1.0, 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [1.0, 0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0, 1.0],
        ]
        if reference_frame.get("to_progen3d_matrix") != expected_matrix:
            result.errors.append("MCSMv2.2 source-to-ProGen3D matrix is not exact")
        if not reference_frame.get("validation", {}).get("valid", False):
            result.errors.append("MCSMv2.2 reference-frame validation is not accepted")
        result.facts["reference_frame_schema"] = reference_frame.get("schema")

    @staticmethod
    def _validate_verification(
        manifest: dict[str, Any],
        verification: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        if verification.get("schema") != "MCSMv2.2.ReleaseVerification.v1":
            result.errors.append("release verification schema is not MCSMv2.2.ReleaseVerification.v1")
        if verification.get("version") != "2.2.0" or verification.get("pass") is not True:
            result.errors.append("release verification does not record a passing 2.2.0 release")
        if verification.get("issues") != []:
            result.errors.append("release verification contains unresolved issues")
        checks = verification.get("checks", {})
        if checks.get("automated_tests", {}).get("test_count") != EXPECTED_TEST_COUNT:
            result.errors.append("release verification does not record 34 automated tests")
        if checks.get("automated_tests", {}).get("pass") is not True:
            result.errors.append("release verification automated tests are not passing")
        if checks.get("grammar_validation", {}).get("current_parser_compatible") is not False:
            result.errors.append("release target grammar must remain classified as non-executable")
        if checks.get("validation_gate", {}).get("pass") is not True:
            result.errors.append("release validation gate is not passing")
        manifest_total = manifest.get("total_size_bytes")
        verification_total = verification.get("total_size_bytes")
        if not isinstance(manifest_total, int) or not isinstance(verification_total, int):
            result.errors.append("release byte totals are not integers")
        else:
            difference = manifest_total - verification_total
            if difference != 45:
                result.errors.append("release verification self-accounting difference is not the reviewed 45 bytes")
            result.facts["verification_self_accounting_difference_bytes"] = difference

    @staticmethod
    def _validate_grammar(
        grammar: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        if grammar.get("schema") != "MCSMv2.2.GrammarValidation.v1":
            result.errors.append("grammar validation schema is not MCSMv2.2.GrammarValidation.v1")
        if grammar.get("rule_definitions") != 7 or grammar.get("unique_rule_definitions") != 7:
            result.errors.append("target grammar must contain seven unique rule definitions")
        if grammar.get("duplicate_rule_names") != [] or grammar.get("delimiter_errors") != []:
            result.errors.append("target grammar contains structural diagnostics")
        if grammar.get("current_parser_compatible") is not False:
            result.errors.append("target grammar must be classified as non-executable")
        if grammar.get("pass") is not True:
            result.errors.append("target grammar structural validation is not passing")

    @staticmethod
    def _validate_checklist(
        checklist: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        if checklist.get("schema") != "MCSMv2.2.ImplementationChecklist.v1":
            result.errors.append("implementation checklist schema is not accepted")
        completed = checklist.get("completed")
        deferred = checklist.get("deferred")
        if not isinstance(completed, dict) or not completed or not all(completed.values()):
            result.errors.append("implementation checklist completed section is not fully true")
        if not isinstance(deferred, dict) or not deferred or not all(deferred.values()):
            result.errors.append("implementation checklist deferred section is not explicit")
        result.facts["completed_requirement_count"] = len(completed) if isinstance(completed, dict) else 0
        result.facts["deferred_requirement_count"] = len(deferred) if isinstance(deferred, dict) else 0

    def _validate_variants(
        self,
        source_package: Path,
        validation: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        if tuple(validation.keys()) != EXPECTED_VARIANTS:
            result.errors.append("validation.json variants are not in the accepted four-variant order")
        variant_facts: dict[str, Any] = {}
        for variant in EXPECTED_VARIANTS:
            record = validation.get(variant)
            if not isinstance(record, dict):
                result.errors.append(f"validation record is missing for variant: {variant}")
                continue
            self._validate_variant_record(source_package, variant, record, result)
            variant_facts[variant] = {
                "assurance": record.get("assurance", {}).get("achieved_level"),
                "release_gate_pass": record.get("release_gate_pass"),
                "hardpoint_count": record.get("suspension_hardpoint_model", {}).get("hardpoint_count"),
                "pose_count": record.get("actual_tyre_pose_sweeps", {}).get("pose_count"),
                "closure_count": len(record.get("closure_kinematics", {}).get("closures", [])),
                "glass_count": len(record.get("helical_glass_kinematics", {}).get("glass_systems", [])),
            }
        result.facts["variants"] = variant_facts

    def _validate_variant_record(
        self,
        source_package: Path,
        variant: str,
        record: dict[str, Any],
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        if record.get("variant") != variant:
            result.errors.append(f"variant identifier mismatch: {variant}")
        if record.get("model_schema") != "MCSMv2.2-uv-semantic-closure-suspension-kinematics":
            result.errors.append(f"model schema mismatch: {variant}")
        if record.get("release_gate_pass") is not True:
            result.errors.append(f"release gate is not passing: {variant}")
        assurance = record.get("assurance", {})
        if assurance.get("achieved_level") != "V3":
            result.errors.append(f"variant does not achieve V3: {variant}")
        levels = assurance.get("levels", {})
        for level in ("V0", "V1", "V2", "V3"):
            if levels.get(level, {}).get("pass") is not True:
                result.errors.append(f"variant does not pass {level}: {variant}")
        for level in ("V4", "V5"):
            if levels.get(level, {}).get("pass") is not False:
                result.errors.append(f"variant incorrectly claims {level}: {variant}")

        partition = record.get("surface_partition", {})
        if len(partition.get("panels", [])) != EXPECTED_PANEL_COUNT:
            result.errors.append(f"variant does not contain 17 panel records: {variant}")
        if len(partition.get("apertures", [])) != EXPECTED_APERTURE_COUNT:
            result.errors.append(f"variant does not contain six aperture records: {variant}")
        if partition.get("exclusive_owner_count") != EXPECTED_EXCLUSIVE_OWNER_COUNT:
            result.errors.append(f"variant does not contain 23 exclusive owners: {variant}")
        if partition.get("coverage_fraction") != 1.0:
            result.errors.append(f"variant partition coverage is incomplete: {variant}")
        if partition.get("unassigned_face_count") != 0:
            result.errors.append(f"variant partition has unassigned faces: {variant}")
        if partition.get("resolved_multiple_owner_face_count") != 0:
            result.errors.append(f"variant partition has unresolved ownership: {variant}")
        if len(partition.get("closure_names", [])) != EXPECTED_CLOSURE_COUNT:
            result.errors.append(f"variant closure domain count is not six: {variant}")

        hardpoints = record.get("suspension_hardpoint_model", {})
        tyre_sweeps = record.get("actual_tyre_pose_sweeps", {})
        closures = record.get("closure_kinematics", {})
        glass = record.get("helical_glass_kinematics", {})
        if hardpoints.get("hardpoint_count") != EXPECTED_HARDPOINT_COUNT or hardpoints.get("pass") is not True:
            result.errors.append(f"variant hardpoint gate is incomplete: {variant}")
        if tyre_sweeps.get("pose_count") != EXPECTED_POSE_COUNT or tyre_sweeps.get("pass") is not True:
            result.errors.append(f"variant tyre sweep gate is incomplete: {variant}")
        if len(closures.get("closures", [])) != EXPECTED_CLOSURE_COUNT or closures.get("pass") is not True:
            result.errors.append(f"variant closure gate is incomplete: {variant}")
        if len(glass.get("glass_systems", [])) != EXPECTED_GLASS_COUNT or glass.get("pass") is not True:
            result.errors.append(f"variant glass gate is incomplete: {variant}")

        hardpoint_json = self._read_json(
            source_package / "kinematics" / f"{variant}_suspension_hardpoints.json",
            result,
        )
        pose_json = self._read_json(
            source_package / "kinematics" / f"{variant}_wheel_pose_sweeps.json",
            result,
        )
        closure_json = self._read_json(
            source_package / "kinematics" / f"{variant}_closure_system.json",
            result,
        )
        partition_json = self._read_json(
            source_package / "partitions" / f"{variant}_surface_partition.json",
            result,
        )
        if len(hardpoint_json.get("hardpoints", [])) != EXPECTED_HARDPOINT_COUNT:
            result.errors.append(f"hardpoint JSON count is not 32: {variant}")
        poses = pose_json.get("poses", [])
        if len(poses) != EXPECTED_POSE_COUNT:
            result.errors.append(f"wheel pose JSON count is not 36: {variant}")
        if any(not self._finite_pose(pose) for pose in poses if isinstance(pose, dict)):
            result.errors.append(f"wheel pose JSON contains non-finite values: {variant}")
        hinges = closure_json.get("hinges", {})
        glass_systems = closure_json.get("glass_systems", {})
        if not isinstance(hinges, dict) or len(hinges) != EXPECTED_CLOSURE_COUNT:
            result.errors.append(f"closure JSON hinge count is not six: {variant}")
        if not isinstance(glass_systems, dict) or len(glass_systems) != EXPECTED_GLASS_COUNT:
            result.errors.append(f"closure JSON glass count is not four: {variant}")
        if len(partition_json.get("panels", [])) != EXPECTED_PANEL_COUNT:
            result.errors.append(f"partition JSON panel count is not 17: {variant}")
        if len(partition_json.get("apertures", [])) != EXPECTED_APERTURE_COUNT:
            result.errors.append(f"partition JSON aperture count is not six: {variant}")

    @staticmethod
    def _finite_pose(pose: dict[str, Any]) -> bool:
        scalars = (
            pose.get("steer_deg"),
            pose.get("travel_m"),
            pose.get("camber_deg"),
            pose.get("toe_deg"),
        )
        if any(not isinstance(value, (int, float)) or not math.isfinite(value) for value in scalars):
            return False
        centre = pose.get("centre_m")
        transform = pose.get("transform")
        if not isinstance(centre, list) or len(centre) != 3:
            return False
        if any(not isinstance(value, (int, float)) or not math.isfinite(value) for value in centre):
            return False
        if not isinstance(transform, list) or len(transform) != 4:
            return False
        return all(
            isinstance(row, list)
            and len(row) == 4
            and all(isinstance(value, (int, float)) and math.isfinite(value) for value in row)
            for row in transform
        )

    @staticmethod
    def _validate_classification(
        source_package: Path,
        result: McsMv220SourceContractValidationResult,
    ) -> None:
        classification_path = source_package.parent / "MCSMv2_2_0_SOURCE_CONTRACT_CLASSIFICATION.json"
        if not classification_path.is_file():
            result.errors.append("missing adjacent MCSMv2.2 source-contract classification")
            return
        classification = McsMv220SourceContractValidationService._read_json(
            classification_path,
            result,
        )
        if classification.get("schema") != "MCSMv2.2.SourceContractClassification.v1":
            result.errors.append("source-contract classification schema is not accepted")
        if classification.get("release_manifest_sha256") != EXPECTED_MANIFEST_SHA256:
            result.errors.append("source-contract classification manifest hash is not exact")
        if classification.get("signed_artifact_count") != EXPECTED_FILE_COUNT:
            result.errors.append("source-contract classification artifact count is not exact")
        if classification.get("source_release_is_immutable") is not True:
            result.errors.append("source-contract classification must mark the release immutable")
        result.facts["classification_path"] = str(classification_path.resolve())


def main() -> None:
    parser = argparse.ArgumentParser(description="Verify the immutable MCSMv2.2 source release.")
    parser.add_argument("source_package", type=Path)
    args = parser.parse_args()
    result = McsMv220SourceContractValidationService().validate(args.source_package)
    print(json.dumps(result.as_dict(), indent=2, sort_keys=True))
    if not result.succeeded():
        raise SystemExit(1)


if __name__ == "__main__":
    main()
