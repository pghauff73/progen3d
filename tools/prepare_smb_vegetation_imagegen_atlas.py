#!/usr/bin/env python3

from __future__ import annotations

import argparse
import hashlib
import json
import math
from pathlib import Path

from PIL import Image, ImageOps


ATLAS_SIZE = (1536, 1024)
ATLAS_COLUMNS = 2
ATLAS_ROWS = 3
OBJECTS_PER_PAGE = 5


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def cell_bounds(cell_index: int) -> tuple[int, int, int, int]:
    cell_width = ATLAS_SIZE[0] // ATLAS_COLUMNS
    cell_height = ATLAS_SIZE[1] // ATLAS_ROWS
    row = cell_index // ATLAS_COLUMNS
    column = cell_index % ATLAS_COLUMNS
    left = column * cell_width
    top = row * cell_height
    right = ATLAS_SIZE[0] if column == ATLAS_COLUMNS - 1 else left + cell_width
    bottom = ATLAS_SIZE[1] if row == ATLAS_ROWS - 1 else top + cell_height
    return left, top, right, bottom


def page_prompt(page_objects: list[dict[str, object]]) -> str:
    object_lines = "\n".join(
        f"{index + 1}. {record['object_id']} — {record['title']} "
        f"({record['category']}): {record['design_intent']}"
        for index, record in enumerate(page_objects)
    )
    return (
        "Edit this exact 2-column by 3-row botanical building-object atlas. The first "
        "five outer cells are occupied and the sixth cell must remain empty. Every "
        "occupied cell contains exactly three flattened elevation panels ordered FRONT, "
        "RIGHT, TOP. Preserve every outer cell boundary, all three internal panel "
        "boundaries, camera order, silhouette, framing, scale, orientation, support "
        "geometry, stems, branches, blades, foliage masses, planter, soil, trellis, and "
        "empty margin. Do not add, remove, merge, or move structural parts. Upgrade only "
        "botanical and material realism: layered species-appropriate foliage, visible leaf "
        "or blade structure, refined bark, soil, premium planters and supports, and subtle "
        "species-appropriate flowers. Use consistent soft studio illumination. Keep the "
        "exact flat front/right/top technical presentation on a near-white background. "
        "No perspective, isometric views, labels, text, arrows, dimensions, people, extra "
        "pots, scenery, or shadows crossing panel boundaries. Occupied cells in row-major "
        f"order:\n{object_lines}\n6. EMPTY — preserve as blank near-white background.\n"
    )


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("suite_manifest", type=Path)
    parser.add_argument("render_validation", type=Path)
    parser.add_argument("output_directory", type=Path)
    arguments = parser.parse_args()

    suite = json.loads(arguments.suite_manifest.read_text())
    render = json.loads(arguments.render_validation.read_text())
    if suite["object_count"] != 50 or render["object_count"] != 50:
        raise SystemExit("Vegetation ImageGen atlas requires exactly fifty objects")
    if suite["comparison_views"] != ["front", "right", "top"]:
        raise SystemExit("Vegetation ImageGen atlas requires front, right, top")
    if render["projection"]["vertical_field_of_view_degrees"] != 5.0:
        raise SystemExit("Vegetation ImageGen atlas requires the five-degree comparison lens")

    pages_directory = arguments.output_directory / "pages"
    pages_directory.mkdir(parents=True, exist_ok=True)
    render_by_object = {record["object_id"]: record for record in render["objects"]}
    page_count = math.ceil(suite["object_count"] / OBJECTS_PER_PAGE)
    page_records = []
    object_records = []

    for page_index in range(page_count):
        page_objects = suite["objects"][
            page_index * OBJECTS_PER_PAGE:(page_index + 1) * OBJECTS_PER_PAGE
        ]
        atlas = Image.new("RGB", ATLAS_SIZE, (246, 246, 243))
        page_object_records = []
        for cell_index, object_record in enumerate(page_objects):
            left, top, right, bottom = cell_bounds(cell_index)
            render_record = render_by_object[object_record["object_id"]]
            camera_card_path = Path(render_record["camera_card"])
            camera_card = Image.open(camera_card_path).convert("RGB")
            fitted = ImageOps.contain(
                camera_card,
                (right - left, bottom - top),
                method=Image.Resampling.LANCZOS,
            )
            paste_x = left + ((right - left) - fitted.width) // 2
            paste_y = top + ((bottom - top) - fitted.height) // 2
            atlas.paste(fitted, (paste_x, paste_y))
            record = {
                "object_id": object_record["object_id"],
                "object_class": object_record["object_class"],
                "title": object_record["title"],
                "category": object_record["category"],
                "design_intent": object_record["design_intent"],
                "page_index": page_index,
                "page_number": page_index + 1,
                "cell_index": cell_index,
                "row": cell_index // ATLAS_COLUMNS,
                "column": cell_index % ATLAS_COLUMNS,
                "crop": [left, top, right, bottom],
                "source_camera_card": str(camera_card_path),
                "source_camera_card_sha256": sha256(camera_card_path),
            }
            page_object_records.append(record)
            object_records.append(record)

        page_stem = f"SMB_Vegetation_Source_Atlas_Page_{page_index + 1:02d}"
        atlas_path = pages_directory / f"{page_stem}.png"
        prompt_path = pages_directory / f"{page_stem}.prompt.txt"
        atlas.save(atlas_path)
        prompt_path.write_text(page_prompt(page_objects))
        page_records.append({
            "page_index": page_index,
            "page_number": page_index + 1,
            "atlas": str(atlas_path),
            "atlas_sha256": sha256(atlas_path),
            "prompt": str(prompt_path),
            "prompt_sha256": sha256(prompt_path),
            "object_count": len(page_object_records),
            "objects": page_object_records,
        })

    manifest = {
        "schema": "ProGen3D-SMB-VegetationImageGenAtlas-v2",
        "suite_manifest": str(arguments.suite_manifest),
        "suite_manifest_sha256": sha256(arguments.suite_manifest),
        "render_validation": str(arguments.render_validation),
        "render_validation_sha256": sha256(arguments.render_validation),
        "atlas_dimensions": list(ATLAS_SIZE),
        "atlas_grid": [ATLAS_COLUMNS, ATLAS_ROWS],
        "camera_card_grid": [3, 1],
        "objects_per_page": OBJECTS_PER_PAGE,
        "page_count": len(page_records),
        "object_count": len(object_records),
        "view_count": len(object_records) * 3,
        "comparison_projection": render["projection"],
        "pages": page_records,
        "objects": object_records,
    }
    (arguments.output_directory / "atlas_manifest.json").write_text(
        json.dumps(manifest, indent=2) + "\n"
    )
    print(
        "Prepared vegetation ImageGen atlas pages: "
        f"pages={len(page_records)} objects={len(object_records)} views={len(object_records) * 3}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
