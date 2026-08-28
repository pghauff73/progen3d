#!/usr/bin/env python3
"""Generate deterministic native evidence meshes from signed MCSMv2.2 GLBs."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
from typing import Iterable

import trimesh


VARIANTS = ("reference", "track", "aero", "crossover")
SOURCE_TO_NATIVE_GEOMETRY_NAME = (
    ("fixed_body", "panel_fixed_body"),
    ("closure_front_door_left", "panel_front_door_left"),
    ("closure_front_door_right", "panel_front_door_right"),
    ("closure_rear_door_left", "panel_rear_door_left"),
    ("closure_rear_door_right", "panel_rear_door_right"),
    ("closure_hood", "panel_hood"),
    ("closure_rear_hatch", "panel_rear_hatch"),
    ("glass_front_side_glass_left", "aperture_front_side_glass_left"),
    ("glass_front_side_glass_right", "aperture_front_side_glass_right"),
    ("glass_rear_side_glass_left", "aperture_rear_side_glass_left"),
    ("glass_rear_side_glass_right", "aperture_rear_side_glass_right"),
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def require_exact_geometry_names(
    available_names: Iterable[str], source_path: Path
) -> None:
    available = set(available_names)
    missing = sorted(
        {source_name for source_name, _ in SOURCE_TO_NATIVE_GEOMETRY_NAME}
        - available
    )
    if missing:
        raise RuntimeError(
            f"{source_path} is missing required semantic meshes: {missing}"
        )


def export_variant(release_root: Path, output_root: Path, variant: str) -> dict:
    source_relative_path = Path("models") / f"modern_car_v2_{variant}_closures_closed.glb"
    source_path = release_root / source_relative_path
    scene = trimesh.load(source_path, force="scene", process=False)
    require_exact_geometry_names(scene.geometry.keys(), source_path)

    variant_output = output_root / variant
    variant_output.mkdir(parents=True, exist_ok=True)
    output_records = []
    for source_geometry_name, native_geometry_name in SOURCE_TO_NATIVE_GEOMETRY_NAME:
        mesh = scene.geometry[source_geometry_name].copy()
        output_path = variant_output / f"{native_geometry_name}.stl"
        output_path.write_bytes(mesh.export(file_type="stl"))
        output_records.append(
            {
                "source_geometry_name": source_geometry_name,
                "native_geometry_name": native_geometry_name,
                "relative_path": output_path.relative_to(output_root).as_posix(),
                "sha256": sha256(output_path),
                "vertex_count": int(len(mesh.vertices)),
                "face_count": int(len(mesh.faces)),
            }
        )
    return {
        "variant": variant,
        "source_relative_path": source_relative_path.as_posix(),
        "source_sha256": sha256(source_path),
        "meshes": output_records,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--release-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path, required=True)
    arguments = parser.parse_args()

    release_root = arguments.release_root.resolve()
    output_root = arguments.output_root.resolve()
    output_root.mkdir(parents=True, exist_ok=True)
    records = [
        export_variant(release_root, output_root, variant) for variant in VARIANTS
    ]
    manifest = {
        "schema": "MCSMv2.2.NativePartitionMeshDerivatives.v1",
        "source_release_root": release_root.name,
        "coordinate_frame": "source_x_forward_y_right_z_up",
        "welding_tolerance_metres": 1.0e-6,
        "variants": records,
    }
    manifest_path = output_root / "manifest.json"
    manifest_path.write_text(
        json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(
        f"Generated {len(VARIANTS) * len(SOURCE_TO_NATIVE_GEOMETRY_NAME)} "
        f"MCSMv2.2 native partition meshes at {output_root}"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
