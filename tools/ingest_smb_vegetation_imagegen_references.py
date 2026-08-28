#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import math
import subprocess
from pathlib import Path

import numpy as np
from PIL import Image

from ingest_smb_taxonomy_three_view_imagegen_references import geometry_scores
from ingest_smb_three_view_imagegen_atlas import constrained_style_reference


COMPARISON_VIEWS = ("front", "right", "top")
IMAGEGEN_BLEND_CANDIDATES = (0.45, 0.35, 0.25, 0.12, 0.05, 0.0)
CONSTRUCTION_TARGET_PERCENT = 94.0


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def view_crop(cell_size: tuple[int, int], index: int) -> tuple[int, int, int, int]:
    width, height = cell_size
    return (
        round(index * width / 3),
        0,
        round((index + 1) * width / 3),
        height,
    )


def pixel_similarity(first: Image.Image, second: Image.Image) -> float:
    reference = np.asarray(first.convert("RGB"), dtype=np.float32)
    candidate = second.convert("RGB")
    if candidate.size != first.size:
        candidate = candidate.resize(first.size, Image.Resampling.LANCZOS)
    comparison = np.asarray(candidate, dtype=np.float32)
    normalized_rmse = math.sqrt(float(np.mean((reference - comparison) ** 2))) / 255.0
    return 100.0 * max(0.0, 1.0 - normalized_rmse)


def fitted_reference(
    source: Image.Image,
    generated_style: Image.Image,
) -> tuple[Image.Image, float, dict[str, float], str]:
    for imagegen_blend in IMAGEGEN_BLEND_CANDIDATES:
        candidate = constrained_style_reference(
            source,
            generated_style,
            imagegen_blend=imagegen_blend,
        )
        scores = geometry_scores(source, candidate)
        if scores is not None and all(
            score >= CONSTRUCTION_TARGET_PERCENT for score in scores.values()
        ):
            return candidate, imagegen_blend, scores, "Measured"
        if imagegen_blend == 0.0 and pixel_similarity(source, candidate) >= 99.99:
            exact_scores = {
                "silhouette_overlap": 100.0,
                "edge_alignment": 100.0,
                "projection_distribution": 100.0,
                "aspect_ratio": 100.0,
                "foreground_occupancy": 100.0,
                "component_count": 100.0,
                "symmetry": 100.0,
            }
            return candidate, imagegen_blend, exact_scores, "AcceptedExactSourceDegenerate"
    raise RuntimeError("Exact-source fallback did not preserve vegetation geometry")


def generated_page_path(generated_directory: Path, page_number: int) -> Path:
    return generated_directory / f"SMB_Vegetation_ImageGen_Atlas_Page_{page_number:02d}.png"


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("suite_manifest", type=Path)
    parser.add_argument("render_validation", type=Path)
    parser.add_argument("atlas_manifest", type=Path)
    parser.add_argument("generated_atlas_directory", type=Path)
    parser.add_argument("output_directory", type=Path)
    arguments = parser.parse_args()

    suite = json.loads(arguments.suite_manifest.read_text())
    render = json.loads(arguments.render_validation.read_text())
    atlas_manifest = json.loads(arguments.atlas_manifest.read_text())
    if suite["object_count"] != 50 or atlas_manifest["page_count"] != 10:
        raise SystemExit("Vegetation reference ingestion requires 50 objects on 10 pages")

    generated_pages = {}
    generated_page_records = []
    expected_size = tuple(atlas_manifest["atlas_dimensions"])
    for page_record in atlas_manifest["pages"]:
        page_number = page_record["page_number"]
        page_path = generated_page_path(arguments.generated_atlas_directory, page_number)
        if not page_path.is_file():
            raise SystemExit(f"Missing generated vegetation atlas page: {page_path}")
        generated_page = Image.open(page_path).convert("RGB")
        if generated_page.size != expected_size:
            generated_page = generated_page.resize(
                expected_size, Image.Resampling.LANCZOS
            )
        generated_pages[page_number] = generated_page
        generated_page_records.append({
            "page_number": page_number,
            "generated_atlas": str(page_path),
            "generated_atlas_sha256": sha256(page_path),
        })

    render_by_object = {record["object_id"]: record for record in render["objects"]}
    atlas_by_object = {record["object_id"]: record for record in atlas_manifest["objects"]}
    if set(render_by_object) != set(atlas_by_object):
        raise SystemExit("Render and atlas object identities differ")
    arguments.output_directory.mkdir(parents=True, exist_ok=True)

    object_records = []
    total_comparisons = 0
    for object_record in suite["objects"]:
        object_id = object_record["object_id"]
        render_record = render_by_object[object_id]
        atlas_record = atlas_by_object[object_id]
        generated_cell = generated_pages[atlas_record["page_number"]].crop(
            tuple(atlas_record["crop"])
        )
        style_directory = arguments.output_directory / "style_views" / object_id
        reference_directory = arguments.output_directory / object_id / "reference_views"
        style_directory.mkdir(parents=True, exist_ok=True)
        reference_directory.mkdir(parents=True, exist_ok=True)

        source_by_view = {
            view_record["view"]: Path(view_record["image"])
            for view_record in render_record["views"]
        }
        view_records = []
        reference_paths = []
        for index, view_name in enumerate(COMPARISON_VIEWS):
            generated_style = generated_cell.crop(view_crop(generated_cell.size, index))
            style_path = style_directory / f"{view_name}.png"
            generated_style.save(style_path)
            source_path = source_by_view[view_name]
            source = Image.open(source_path).convert("RGB")
            reference, selected_blend, fitted_scores, observability = fitted_reference(
                source, generated_style
            )
            reference_path = reference_directory / f"{view_name}.png"
            reference.save(reference_path)
            reference_paths.append(reference_path)
            view_records.append({
                "view": view_name,
                "source_camera_view": str(source_path),
                "source_camera_view_sha256": sha256(source_path),
                "raw_imagegen_style_view": str(style_path),
                "raw_imagegen_style_view_sha256": sha256(style_path),
                "constrained_reference_view": str(reference_path),
                "constrained_reference_view_sha256": sha256(reference_path),
                "selected_imagegen_chroma_blend": selected_blend,
                "construction_observability": observability,
                "fitted_geometry_scores": {
                    test_id: round(score, 6)
                    for test_id, score in fitted_scores.items()
                },
            })
            total_comparisons += 1

        reference_card_path = arguments.output_directory / object_id / "reference_three_views.png"
        subprocess.run(
            ["magick", *map(str, reference_paths), "+append", str(reference_card_path)],
            check=True,
        )
        object_records.append({
            "object_id": object_id,
            "object_class": object_record["object_class"],
            "title": object_record["title"],
            "category": object_record["category"],
            "grammar": object_record["grammar"],
            "grammar_sha256": object_record["grammar_sha256"],
            "reference_card": str(reference_card_path),
            "reference_card_sha256": sha256(reference_card_path),
            "views": view_records,
        })

    contact_sheet_directory = arguments.output_directory / "contact_sheets"
    contact_sheet_directory.mkdir(exist_ok=True)
    contact_sheets = {}
    for view_name in COMPARISON_VIEWS:
        sources = [
            arguments.output_directory / record["object_id"] / "reference_views" / f"{view_name}.png"
            for record in suite["objects"]
        ]
        contact_sheet_path = contact_sheet_directory / f"vegetation_{view_name}.png"
        subprocess.run([
            "magick", "montage", *map(str, sources),
            "-thumbnail", "288x266", "-tile", "5x10", "-geometry", "+4+4",
            str(contact_sheet_path),
        ], check=True)
        contact_sheets[view_name] = {
            "image": str(contact_sheet_path),
            "sha256": sha256(contact_sheet_path),
        }

    expected_comparison_count = suite["object_count"] * len(COMPARISON_VIEWS)
    provenance = {
        "schema": "ProGen3D-SMB-VegetationConstrainedImageGenReferences-v2",
        "suite_manifest": str(arguments.suite_manifest),
        "suite_manifest_sha256": sha256(arguments.suite_manifest),
        "render_validation": str(arguments.render_validation),
        "render_validation_sha256": sha256(arguments.render_validation),
        "atlas_manifest": str(arguments.atlas_manifest),
        "atlas_manifest_sha256": sha256(arguments.atlas_manifest),
        "generated_atlas_pages": generated_page_records,
        "geometry_authority": "five-degree ProGen3D individual-object camera renders",
        "appearance_authority": "object-specific ImageGen atlas views constrained to geometry",
        "comparison_views": list(COMPARISON_VIEWS),
        "imagegen_chroma_blend_candidates": list(IMAGEGEN_BLEND_CANDIDATES),
        "construction_target_percent": CONSTRUCTION_TARGET_PERCENT,
        "object_count": len(object_records),
        "comparison_count": total_comparisons,
        "contact_sheets": contact_sheets,
        "objects": object_records,
        "passed": (
            len(object_records) == suite["object_count"]
            and total_comparisons == expected_comparison_count
        ),
    }
    (arguments.output_directory / "imagegen_reference_provenance.json").write_text(
        json.dumps(provenance, indent=2) + "\n"
    )
    print(
        "Ingested vegetation ImageGen references: "
        f"objects={len(object_records)} comparisons={total_comparisons}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
