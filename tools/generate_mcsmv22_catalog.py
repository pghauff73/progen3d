#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Any


VARIANTS = ("reference", "track", "aero", "crossover")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cpp_string(value: str) -> str:
    return json.dumps(value)


def cpp_number(value: float) -> str:
    return format(float(value), ".17g")


def vector3(value: list[float]) -> str:
    return "{" + ", ".join(cpp_number(component) for component in value) + "}"


def matrix4(value: list[list[float]]) -> str:
    components = [component for row in value for component in row]
    return "{{" + ", ".join(cpp_number(component) for component in components) + "}}"


def load_json(path: Path) -> dict[str, Any]:
    value = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(value, dict):
        raise ValueError(f"expected JSON object: {path}")
    return value


def hardpoint_record(record: dict[str, Any]) -> str:
    return (
        "{" + ", ".join(
            (
                cpp_string(record["name"]),
                cpp_string(record["axle"]),
                cpp_string(record["side"]),
                cpp_string(record["role"]),
                vector3(record["position"]),
                cpp_string(record["evidence_status"]),
                cpp_number(record["confidence"]),
            )
        ) + "}"
    )


def pose_record(record: dict[str, Any]) -> str:
    return (
        "{" + ", ".join(
            (
                cpp_string(record["wheel"]),
                cpp_number(record["steer_deg"]),
                cpp_number(record["travel_m"]),
                cpp_number(record["camber_deg"]),
                cpp_number(record["toe_deg"]),
                vector3(record["centre_m"]),
                matrix4(record["transform"]),
            )
        ) + "}"
    )


def hinge_record(record: dict[str, Any]) -> str:
    return (
        "{" + ", ".join(
            (
                cpp_string(record["name"]),
                cpp_string(record["closure"]),
                vector3(record["point"]),
                vector3(record["axis"]),
                cpp_number(record["maximum_angle_deg"]),
                cpp_number(record["direction"]),
                cpp_number(record["rise_m"]),
                vector3(record["translation_axis"]),
                cpp_string(record["joint_type"]),
            )
        ) + "}"
    )


def glass_record(record: dict[str, Any]) -> str:
    return (
        "{" + ", ".join(
            (
                cpp_string(record["name"]),
                cpp_string(record["parent_closure"]),
                cpp_string(record["side"]),
                cpp_number(record["travel_m"]),
                cpp_number(record["inward_m"]),
                cpp_number(record["longitudinal_m"]),
                cpp_number(record["rotation_deg"]),
                vector3(record["pivot"]),
            )
        ) + "}"
    )


def surface_owner_record(record: dict[str, Any], kind: str) -> str:
    return (
        "{" + ", ".join(
            (
                cpp_string(record["name"]),
                cpp_string(kind),
                "true" if record.get("closure", False) else "false",
                str(int(record.get("face_count", 0))),
            )
        ) + "}"
    )


def format_array(records: list[str], indent: str) -> str:
    return "{{\n" + ",\n".join(indent + record for record in records) + "\n" + indent[:-4] + "}}"


def variant_record(source_release: Path, variant: str) -> str:
    validation = load_json(source_release / "validation.json")[variant]
    parameters = load_json(source_release / "model_parameters.json")[variant]
    hardpoints = load_json(source_release / "kinematics" / f"{variant}_suspension_hardpoints.json")
    poses = load_json(source_release / "kinematics" / f"{variant}_wheel_pose_sweeps.json")
    closure_system = load_json(source_release / "kinematics" / f"{variant}_closure_system.json")
    partition = load_json(source_release / "partitions" / f"{variant}_surface_partition.json")

    hardpoint_records = [hardpoint_record(record) for record in hardpoints["hardpoints"]]
    pose_records = [pose_record(record) for record in poses["poses"]]
    hinge_records = [hinge_record(record) for record in closure_system["hinges"].values()]
    glass_records = [glass_record(record) for record in closure_system["glass_systems"].values()]
    panel_records = [surface_owner_record(record, "panel") for record in partition["panels"]]
    aperture_records = [surface_owner_record(record, "aperture") for record in partition["apertures"]]

    values = [
        cpp_string(variant),
        cpp_string(validation["label"]),
        cpp_string(validation["model_schema"]),
        format_array(hardpoint_records, "            "),
        format_array(pose_records, "            "),
        format_array(hinge_records, "            "),
        format_array(glass_records, "            "),
        format_array(panel_records, "            "),
        format_array(aperture_records, "            "),
        cpp_number(parameters["wheel_suspension"]["wheel_radius"]),
        cpp_number(parameters["wheel_suspension"]["wheel_width"]),
        cpp_number(parameters["wheel_suspension"]["steer_min_deg"]),
        cpp_number(parameters["wheel_suspension"]["steer_max_deg"]),
        cpp_number(parameters["wheel_suspension"]["travel_min"]),
        cpp_number(parameters["wheel_suspension"]["travel_max"]),
        cpp_number(poses["validation"]["declared_clearance_m"]),
        cpp_number(poses["validation"]["minimum_clearance_m"]),
        cpp_number(max(wheel["tessellation_tolerance_m"] for wheel in poses["validation"]["wheels"])),
        "true" if validation["release_gate_pass"] else "false",
        cpp_string(validation["assurance"]["achieved_level"]),
    ]
    return "{\n        " + ",\n        ".join(values) + "\n    }"


def generate_header(source_release: Path) -> str:
    manifest = load_json(source_release / "RELEASE_MANIFEST.json")
    version = load_json(source_release / "VERSION.json")
    reference_frame = version["reference_frame"]
    variants = [variant_record(source_release, variant) for variant in VARIANTS]
    source_record = (
        "{" + ", ".join(
            (
                cpp_string(version["version"]),
                cpp_string(version["model_schema"]),
                cpp_string(manifest["schema"]),
                cpp_string(sha256(source_release / "RELEASE_MANIFEST.json")),
                str(manifest["file_count"]),
                str(manifest["total_size_bytes"]),
                cpp_string(reference_frame["schema"]),
                matrix4(reference_frame["to_progen3d_matrix"]),
                cpp_string("V3 sampled concept kinematics only; V4 and V5 remain unclaimed."),
            )
        ) + "}"
    )
    variant_array = "{{\n" + ",\n".join("    " + record.replace("\n", "\n    ") for record in variants) + "\n}}"
    return f'''#pragma once

#include <array>
#include <cstddef>
#include <string_view>

struct GeneratedMcsMv22Vector3Record
{{
    double x;
    double y;
    double z;
}};

struct GeneratedMcsMv22Matrix4Record
{{
    std::array<double, 16> row_major_values;
}};

struct GeneratedMcsMv22SourceReleaseRecord
{{
    std::string_view version;
    std::string_view model_schema;
    std::string_view manifest_schema;
    std::string_view release_manifest_sha256;
    std::size_t signed_artifact_count;
    std::size_t signed_total_size_bytes;
    std::string_view reference_frame_schema;
    GeneratedMcsMv22Matrix4Record to_progen3d_matrix;
    std::string_view assurance_boundary;
}};

struct GeneratedMcsMv22HardpointRecord
{{
    std::string_view name;
    std::string_view axle;
    std::string_view side;
    std::string_view role;
    GeneratedMcsMv22Vector3Record position;
    std::string_view evidence_status;
    double confidence;
}};

struct GeneratedMcsMv22WheelPoseRecord
{{
    std::string_view wheel;
    double steer_degrees;
    double travel_metres;
    double camber_degrees;
    double toe_degrees;
    GeneratedMcsMv22Vector3Record centre_metres;
    GeneratedMcsMv22Matrix4Record transform;
}};

struct GeneratedMcsMv22HingeRecord
{{
    std::string_view name;
    std::string_view closure;
    GeneratedMcsMv22Vector3Record point;
    GeneratedMcsMv22Vector3Record axis;
    double maximum_angle_degrees;
    double direction;
    double rise_metres;
    GeneratedMcsMv22Vector3Record translation_axis;
    std::string_view joint_type;
}};

struct GeneratedMcsMv22GlassRecord
{{
    std::string_view name;
    std::string_view parent_closure;
    std::string_view side;
    double travel_metres;
    double inward_metres;
    double longitudinal_metres;
    double rotation_degrees;
    GeneratedMcsMv22Vector3Record pivot;
}};

struct GeneratedMcsMv22SurfaceOwnerRecord
{{
    std::string_view name;
    std::string_view kind;
    bool closure;
    std::size_t face_count;
}};

struct GeneratedMcsMv22VariantRecord
{{
    std::string_view identifier;
    std::string_view display_name;
    std::string_view model_schema;
    std::array<GeneratedMcsMv22HardpointRecord, 32> hardpoints;
    std::array<GeneratedMcsMv22WheelPoseRecord, 36> wheel_poses;
    std::array<GeneratedMcsMv22HingeRecord, 6> hinges;
    std::array<GeneratedMcsMv22GlassRecord, 4> glass_systems;
    std::array<GeneratedMcsMv22SurfaceOwnerRecord, 17> panel_owners;
    std::array<GeneratedMcsMv22SurfaceOwnerRecord, 6> aperture_owners;
    double wheel_radius_metres;
    double wheel_width_metres;
    double minimum_steer_degrees;
    double maximum_steer_degrees;
    double minimum_travel_metres;
    double maximum_travel_metres;
    double declared_tyre_clearance_metres;
    double accepted_minimum_tyre_clearance_metres;
    double tyre_tessellation_tolerance_metres;
    bool source_release_gate_pass;
    std::string_view assurance_level;
}};

inline const GeneratedMcsMv22SourceReleaseRecord &generatedMcsMv22SourceReleaseRecord()
{{
    static const GeneratedMcsMv22SourceReleaseRecord record = {source_record};
    return record;
}}

inline const std::array<GeneratedMcsMv22VariantRecord, 4> &generatedMcsMv22VariantRecords()
{{
    static const std::array<GeneratedMcsMv22VariantRecord, 4> records = {variant_array};
    return records;
}}
'''


def write_manifest(source_release: Path, header_path: Path, manifest_path: Path) -> None:
    source_paths = [
        source_release / "VERSION.json",
        source_release / "RELEASE_MANIFEST.json",
        source_release / "validation.json",
        source_release / "model_parameters.json",
    ]
    for variant in VARIANTS:
        source_paths.extend(
            (
                source_release / "partitions" / f"{variant}_surface_partition.json",
                source_release / "kinematics" / f"{variant}_suspension_hardpoints.json",
                source_release / "kinematics" / f"{variant}_wheel_pose_sweeps.json",
                source_release / "kinematics" / f"{variant}_closure_system.json",
            )
        )
    repository_root = Path(__file__).resolve().parents[1]
    payload = {
        "schema": "MCSMv2.2.NativeCatalogManifest.v1",
        "version": "2.2.0",
        "release_manifest_sha256": sha256(source_release / "RELEASE_MANIFEST.json"),
        "source_hashes": {
            str(path.relative_to(repository_root)): sha256(path)
            for path in source_paths
        },
        "generator": str(Path(__file__).resolve().relative_to(repository_root)),
        "generator_sha256": sha256(Path(__file__).resolve()),
        "generated_header": "include/vehicle/mcsmv2/generated/GeneratedMcsMv22Catalog.h",
        "generated_header_sha256": sha256(header_path),
        "variant_count": 4,
        "hardpoint_count_per_variant": 32,
        "wheel_pose_count_per_variant": 36,
        "closure_count_per_variant": 6,
        "glass_system_count_per_variant": 4,
    }
    manifest_path.parent.mkdir(parents=True, exist_ok=True)
    manifest_path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the deterministic MCSMv2.2 native catalog.")
    parser.add_argument("--source-release", type=Path, required=True)
    parser.add_argument("--header", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    source_release = args.source_release.resolve()
    header_path = args.header.resolve()
    manifest_path = args.manifest.resolve()
    header_path.parent.mkdir(parents=True, exist_ok=True)
    header_path.write_text(generate_header(source_release), encoding="utf-8")
    write_manifest(source_release, header_path, manifest_path)


if __name__ == "__main__":
    main()
