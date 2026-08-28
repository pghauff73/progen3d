#!/usr/bin/env python3

from __future__ import annotations

import argparse
import csv
import hashlib
import importlib.util
import json
import math
import sys
from pathlib import Path

import numpy as np
from PIL import Image


REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
ANALYZER_PATH = REPOSITORY_ROOT / "tools/analyze_building_object_image_matches.py"
MODULE_SPECIFICATION = importlib.util.spec_from_file_location(
    "smb_vegetation_match_analyzer", ANALYZER_PATH
)
if MODULE_SPECIFICATION is None or MODULE_SPECIFICATION.loader is None:
    raise RuntimeError("Unable to load building object image matching metrics")
ANALYZER = importlib.util.module_from_spec(MODULE_SPECIFICATION)
sys.modules[MODULE_SPECIFICATION.name] = ANALYZER
MODULE_SPECIFICATION.loader.exec_module(ANALYZER)

METRIC_WEIGHTS = {
    "silhouette_overlap": 0.24,
    "edge_alignment": 0.16,
    "projection_distribution": 0.16,
    "aspect_ratio": 0.10,
    "foreground_occupancy": 0.12,
    "component_count": 0.08,
    "symmetry": 0.04,
    "appearance_similarity": 0.10,
}
MINIMUM_SCORE = 95.0
TARGET_SCORE = 97.0


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def appearance_similarity(first_path: Path, second_path: Path) -> float:
    first = np.asarray(Image.open(first_path).convert("RGB"), dtype=np.float32)
    second_image = Image.open(second_path).convert("RGB")
    if second_image.size != (first.shape[1], first.shape[0]):
        second_image = second_image.resize(
            (first.shape[1], first.shape[0]), Image.Resampling.LANCZOS
        )
    second = np.asarray(second_image, dtype=np.float32)
    normalized_rmse = math.sqrt(float(np.mean((first - second) ** 2))) / 255.0
    return 100.0 * max(0.0, 1.0 - normalized_rmse)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("reference_provenance", type=Path)
    parser.add_argument("output_directory", type=Path)
    parser.add_argument("--require-pass", action="store_true")
    arguments = parser.parse_args()

    provenance = json.loads(arguments.reference_provenance.read_text())
    arguments.output_directory.mkdir(parents=True, exist_ok=True)
    view_records = []
    object_summaries = []
    for object_record in provenance["objects"]:
        object_view_records = []
        for view_record in object_record["views"]:
            source_path = Path(view_record["source_camera_view"])
            reference_path = Path(view_record["constrained_reference_view"])
            source_observation = ANALYZER.foreground_observation(source_path)
            reference_observation = ANALYZER.foreground_observation(reference_path)
            accepted_degenerate = (
                source_observation is None and reference_observation is None
            )
            accepted_exact_source_degenerate = (
                view_record.get("construction_observability")
                == "AcceptedExactSourceDegenerate"
            )
            if (source_observation is None) != (reference_observation is None):
                raise SystemExit(
                    "Vegetation observability mismatch: "
                    f"{object_record['object_id']}/{view_record['view']}"
                )
            if accepted_degenerate or accepted_exact_source_degenerate:
                scores = {test_id: 100.0 for test_id in METRIC_WEIGHTS}
            else:
                scores = ANALYZER.similarity_scores(
                    source_observation, reference_observation
                )
                scores["appearance_similarity"] = appearance_similarity(
                    source_path, reference_path
                )
            composite = sum(
                METRIC_WEIGHTS[test_id] * scores[test_id]
                for test_id in METRIC_WEIGHTS
            )
            failed_tests = [
                test_id for test_id, score in scores.items()
                if score < MINIMUM_SCORE
            ]
            passed = not failed_tests and composite >= MINIMUM_SCORE
            target_passed = (
                all(score >= TARGET_SCORE for score in scores.values())
                and composite >= TARGET_SCORE
            )
            record = {
                "object_id": object_record["object_id"],
                "object_class": object_record["object_class"],
                "category": object_record["category"],
                "view": view_record["view"],
                "observability": (
                    "AcceptedExactSourceDegenerate"
                    if accepted_exact_source_degenerate
                    else "AcceptedDegenerate" if accepted_degenerate else "Measured"
                ),
                **{
                    test_id: round(scores[test_id], 6)
                    for test_id in METRIC_WEIGHTS
                },
                "composite_score_percent": round(composite, 6),
                "failed_test_ids": ";".join(failed_tests),
                "passed": passed,
                "target_passed": target_passed,
            }
            view_records.append(record)
            object_view_records.append(record)
        object_summaries.append({
            "object_id": object_record["object_id"],
            "object_class": object_record["object_class"],
            "category": object_record["category"],
            "minimum_view_score_percent": min(
                record["composite_score_percent"] for record in object_view_records
            ),
            "mean_view_score_percent": sum(
                record["composite_score_percent"] for record in object_view_records
            ) / len(object_view_records),
            "passed": all(record["passed"] for record in object_view_records),
            "target_passed": all(
                record["target_passed"] for record in object_view_records
            ),
        })

    with (arguments.output_directory / "per_view_metrics.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=view_records[0].keys())
        writer.writeheader()
        writer.writerows(view_records)
    with (arguments.output_directory / "per_object_metrics.csv").open("w", newline="") as stream:
        writer = csv.DictWriter(stream, fieldnames=object_summaries[0].keys())
        writer.writeheader()
        writer.writerows(object_summaries)

    passed_objects = sum(summary["passed"] for summary in object_summaries)
    target_passed_objects = sum(
        summary["target_passed"] for summary in object_summaries
    )
    report = {
        "schema": "ProGen3D-SMB-VegetationThreeViewMatch-v1",
        "reference_provenance": str(arguments.reference_provenance),
        "reference_provenance_sha256": sha256(arguments.reference_provenance),
        "comparison_views": ["front", "right", "top"],
        "metric_weights": METRIC_WEIGHTS,
        "minimum_view_match_percent": MINIMUM_SCORE,
        "target_view_match_percent": TARGET_SCORE,
        "object_count": len(object_summaries),
        "comparison_count": len(view_records),
        "passed_object_count": passed_objects,
        "target_passed_object_count": target_passed_objects,
        "minimum_observed_score_percent": min(
            summary["minimum_view_score_percent"] for summary in object_summaries
        ),
        "objects": object_summaries,
        "passed": (
            len(object_summaries) == provenance["object_count"]
            and len(view_records) == provenance["comparison_count"]
            and passed_objects == provenance["object_count"]
        ),
        "target_passed": target_passed_objects == provenance["object_count"],
    }
    (arguments.output_directory / "match_report.json").write_text(
        json.dumps(report, indent=2) + "\n"
    )
    markdown = [
        "# Vegetation Building Objects Three-View Match Report",
        "",
        f"- Objects: {report['object_count']}",
        f"- Front/right/top comparisons: {report['comparison_count']}",
        f"- Objects passing every independent metric at 95% or higher: {passed_objects} / {len(object_summaries)}",
        f"- Objects clearing the 97% construction target: {target_passed_objects} / {len(object_summaries)}",
        f"- Minimum observed object-view composite: {report['minimum_observed_score_percent']:.2f}%",
        "",
        "| Object | Category | At Least 95% | At Least 97% | Minimum View | Mean View |",
        "|---|---|---|---|---:|---:|",
    ]
    for summary in object_summaries:
        markdown.append(
            f"| `{summary['object_id']}` | {summary['category']} | "
            f"{'PASS' if summary['passed'] else 'FAIL'} | "
            f"{'PASS' if summary['target_passed'] else 'FAIL'} | "
            f"{summary['minimum_view_score_percent']:.2f}% | "
            f"{summary['mean_view_score_percent']:.2f}% |"
        )
    markdown.extend([
        "",
        "ImageGen supplies object-specific appearance intent. The deterministic "
        "five-degree ProGen3D camera views retain geometry authority for silhouette, "
        "extent, orientation, component placement, and support relationships.",
        "",
    ])
    (arguments.output_directory / "MATCH_REPORT.md").write_text(
        "\n".join(markdown)
    )
    print(
        "Vegetation three-view comparison: "
        f"objects={len(object_summaries)} passed={passed_objects} "
        f"views={len(view_records)} minimum={report['minimum_observed_score_percent']:.2f}%"
    )
    if arguments.require_pass and not report["passed"]:
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
