#!/usr/bin/env python3
"""Verify the Windows CLI camera pull, layer registration, and manifest errors."""
import argparse
import copy
import json
import math
from pathlib import Path
import statistics
import struct
import subprocess
import tempfile


def invoke(cli, command, job_path, expected=0):
    result = subprocess.run(
        [str(cli), command, str(job_path)],
        capture_output=True, text=True, encoding="utf-8", timeout=120, cwd=job_path.parent,
    )
    report = json.loads(result.stdout)
    assert result.returncode == report["exit_code"] == expected, (result.stdout, result.stderr)
    return report


def write_job(directory, name, job):
    job["output_dir"] = name
    path = directory / (name + ".json")
    path.write_text(json.dumps(job), encoding="utf-8")
    return path


def read_image(path, expected_magic, width, height):
    magic, dimensions, maximum, data = Path(path).read_bytes().split(b"\n", 3)
    assert magic == expected_magic and tuple(map(int, dimensions.split())) == (width, height), path
    bytes_per_pixel = {b"P6": 3, b"P5": 1, b"Pf": 4}
    assert len(data) == width * height * bytes_per_pixel[magic], path
    assert maximum == (b"-1.0" if magic == b"Pf" else b"255"), path
    return data


def load_layers(report):
    width, height = report["width"], report["height"]
    paths = report["layers"]
    assert Path(paths["preview"]) == Path(report["image"])
    rgb = read_image(paths["preview"], b"P6", width, height)
    sharp = read_image(paths["color"], b"P6", width, height)
    mask = read_image(paths["object_mask"], b"P6", width, height)
    labels = [int.from_bytes(mask[index:index + 3], "big") for index in range(0, len(mask), 3)]
    coverage = read_image(paths["foreground_coverage"], b"P5", width, height)
    depth_data = read_image(paths["linear_depth"], b"Pf", width, height)
    inverse_rows = tuple(value for (value,) in struct.iter_unpack("<f", depth_data))
    depth = [value for row in range(height - 1, -1, -1)
             for value in inverse_rows[row * width:(row + 1) * width]]
    assert all(math.isfinite(value) and ((value > 0) if label else value == 0)
               for label, value in zip(labels, depth)), "Depth and mask disagree"
    manifest = json.loads(Path(paths["manifest"]).read_text(encoding="utf-8"))
    assert manifest["width"] == width and manifest["height"] == height
    assert sum(entry["pixels"] for entry in manifest["object_ids"]) == width * height
    assert not any(Path(report["image"]).parent.parent.glob(".*.tmp")), "Incomplete frame directories remain"
    return rgb, sharp, labels, coverage, depth, manifest["object_ids"]


def edge_energy(rgb, labels, width, subject, silhouette):
    total = 0
    samples = 0
    for index, label in enumerate(labels):
        if label != subject:
            continue
        for adjacent in (index + 1, index + width):
            if adjacent >= len(labels) or (adjacent == index + 1 and adjacent % width == 0):
                continue
            if (labels[adjacent] != subject) != silhouette:
                continue
            total += sum(abs(rgb[index * 3 + channel] - rgb[adjacent * 3 + channel])
                         for channel in range(3))
            samples += 1
    assert samples > 100, "The focus subject is too small to measure"
    return total / samples


def assert_vec_close(vector, expected):
    assert all(math.isclose(first, second, abs_tol=1e-5) for first, second in zip(vector, expected)), vector


def run_checks(cli, output_root):
    examples = Path(__file__).resolve().parent.parent / "examples"
    source = json.loads((examples / "courtyard_camera.json").read_text(encoding="utf-8"))
    source["grammar"] = str(examples / source["grammar"])
    output_root.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix="progen3d-camera-", dir=output_root))

    def timed_job(name, time):
        job = copy.deepcopy(source)
        job["camera"]["time_seconds"] = time
        return write_job(directory, name, job)

    branch_job = timed_job("branch", 0)
    entrance_job = timed_job("entrance", 2)
    branch = invoke(cli, "render", branch_job)
    entrance = invoke(cli, "render", entrance_job)
    middle = invoke(cli, "validate", timed_job("midpoint", 1))
    entrance_repeat = invoke(cli, "render", entrance_job)

    first = branch["camera"]
    last = entrance["camera"]
    assert first["aim"] == last["aim"]
    assert first["position"] == last["position"]
    assert_vec_close(first["aim"], [0, 1.8, -2.5])
    assert_vec_close(first["focus"], [-0.30, 1.76, 3.41])
    assert_vec_close(last["focus"], [0, 1.69, -2.41])
    assert first["focus_depth"] < middle["camera"]["focus_depth"] < last["focus_depth"]
    assert last["focus_depth"] - first["focus_depth"] > 5
    assert entrance["pixel_hash_fnv1a64"] == entrance_repeat["pixel_hash_fnv1a64"]
    assert entrance["image"] != entrance_repeat["image"]
    assert Path(entrance["image"]).read_bytes() == Path(entrance_repeat["image"]).read_bytes()

    branch_rgb, branch_raw, branch_ids, _, branch_depth, branch_legend = load_layers(branch)
    door_rgb, door_raw, door_ids, coverage, door_depth, door_legend = load_layers(entrance)
    assert branch_raw == door_raw and branch_ids == door_ids and branch_depth == door_depth
    assert branch_legend == door_legend
    named = {entry["object"]: entry["id"] for entry in door_legend if "object" in entry}
    leaf_id = named["FOREGROUND_BRANCH"]
    door_id = named["COURTYARD_ENTRANCE"]
    leaf_depth = statistics.median(value for label, value in zip(door_ids, door_depth) if label == leaf_id)
    door_depth_value = statistics.median(value for label, value in zip(door_ids, door_depth) if label == door_id)
    assert abs(leaf_depth - first["focus_depth"]) < 0.15
    assert abs(door_depth_value - last["focus_depth"]) < 0.15

    width = branch["width"]
    leaf_sharp = edge_energy(branch_rgb, door_ids, width, leaf_id, True)
    leaf_blurred = edge_energy(door_rgb, door_ids, width, leaf_id, True)
    door_blurred = edge_energy(branch_rgb, door_ids, width, door_id, False)
    door_sharp = edge_energy(door_rgb, door_ids, width, door_id, False)
    assert leaf_sharp > leaf_blurred * 1.5, (leaf_sharp, leaf_blurred)
    assert door_sharp > door_blurred * 1.4, (door_sharp, door_blurred)

    soft_overlap = sum(label == door_id and 0 < alpha < 255 for label, alpha in zip(door_ids, coverage))
    clear = [index for index, (label, alpha) in enumerate(zip(door_ids, coverage))
             if label == door_id and alpha == 0]
    clear_sharp = sum(door_rgb[index * 3:index * 3 + 3] == door_raw[index * 3:index * 3 + 3]
                      for index in clear)
    assert soft_overlap > 100 and len(clear) > 100, (soft_overlap, len(clear))
    assert clear_sharp / len(clear) > 0.98, (clear_sharp, len(clear))

    moving_job = copy.deepcopy(source)
    moving_job["camera"]["time_seconds"] = 3
    moving_job["camera"]["aim"].append({"time": 4, "value": [0.5, 1.7, -2.5]})
    moving = invoke(cli, "render", write_job(directory, "moving", moving_job))
    assert_vec_close(moving["camera"]["position"], [-0.325, 1.85, 6.225])
    assert_vec_close(moving["camera"]["aim"], [0.375, 1.725, -2.5])
    assert math.isclose(moving["camera"]["fov_degrees"], 40.5)
    assert Path(moving["layers"]["color"]).read_bytes() != Path(entrance["layers"]["color"]).read_bytes()

    sharp_job = copy.deepcopy(source)
    for keyframe in sharp_job["camera"]["lens"]:
        keyframe["value"]["aperture_radius"] = 0
    pinhole = invoke(cli, "render", write_job(directory, "pinhole", sharp_job))
    pinhole_rgb, pinhole_raw, _, pinhole_coverage, _, _ = load_layers(pinhole)
    assert pinhole_rgb == pinhole_raw and not any(pinhole_coverage)

    (directory / "cube.p3d").write_text(
        "Start -> [ S(1 1 1) I(Cube material(navybluemattepaint) 1 0.20) ]", encoding="utf-8",
    )
    layers_only = {
        "schema_version": 1, "grammar": "cube.p3d", "seed": 42, "width": 128, "height": 96, "layers": True,
    }
    fitted = invoke(cli, "render", write_job(directory, "fitted", layers_only))
    fitted_color, fitted_raw, _, _, _, fitted_legend = load_layers(fitted)
    assert fitted_color == fitted_raw and fitted_legend[1]["primitive_index"] == 0

    invalid = copy.deepcopy(source)
    invalid["camera"]["focus"][0]["value"]["attachment"] = "missing"
    rejected = invoke(cli, "validate", write_job(directory, "missing-attachment", invalid), expected=2)
    assert "Unknown camera attachment" in rejected["error"]

    invalid = copy.deepcopy(source)
    invalid["camera"]["lens"][1]["time"] = 0
    rejected = invoke(cli, "validate", write_job(directory, "duplicate-time", invalid), expected=2)
    assert "strictly increasing" in rejected["error"]

    invalid = copy.deepcopy(source)
    invalid["camera"]["focus"] = [{"time": 0, "value": [0, 1.8, 7]}]
    rejected = invoke(cli, "validate", write_job(directory, "behind-camera", invalid), expected=2)
    assert "in front" in rejected["error"]

    report = {
        "directory": str(directory),
        "branch": branch,
        "entrance": entrance,
        "moving": moving,
        "branch_edge_energy": {"branch_focus": leaf_sharp, "entrance_focus": leaf_blurred},
        "door_edge_energy": {"branch_focus": door_blurred, "entrance_focus": door_sharp},
        "door_pixels_with_partial_foreground_coverage": soft_overlap,
        "uncovered_door_pixels_unchanged": clear_sharp,
        "uncovered_door_pixels": len(clear),
    }
    report_path = directory / "report.json"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Camera checks passed. Report: {report_path}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cli", type=Path)
    parser.add_argument("--output-root", type=Path, default=Path(tempfile.gettempdir()))
    args = parser.parse_args()
    run_checks(args.cli.resolve(strict=True), args.output_root.resolve())


if __name__ == "__main__":
    main()
