#!/usr/bin/env python3
"""Verify calibrated fitting and repeatable experiment sweeps on Windows."""
import argparse
from itertools import product
import json
import math
from pathlib import Path
import struct
import subprocess
import tempfile


EXAMPLES = Path(__file__).resolve().parents[1] / "examples"
SIZE = (220, 172)
TRUE_WIDTH = 2.375
PANEL_HEIGHT = 1.25
PANEL_DEPTH = 0.75


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path, body):
    path.write_text(json.dumps(body, indent=2) + "\n", encoding="utf-8")
    return path


def invoke(cli, *arguments, expected=0):
    completed = subprocess.run(
        [str(cli), *map(str, arguments)],
        capture_output=True, text=True, encoding="utf-8", timeout=300,
    )
    assert completed.stdout, completed.stderr
    report = json.loads(completed.stdout)
    assert completed.returncode == expected == report["exit_code"], (arguments, report, completed.stderr)
    return report


def unit(vector):
    length = math.sqrt(sum(x * x for x in vector))
    return tuple(x / length for x in vector)


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])


def project(point, camera):
    forward = unit(camera["direction"])
    right = unit(cross(forward, camera.get("up", [0, 1, 0])))
    up = cross(right, forward)
    offset = tuple(a - b for a, b in zip(point, camera["center"]))
    scale = camera["pixels_per_unit"]
    return (SIZE[0] / 2 + scale * sum(a * b for a, b in zip(offset, right)),
            SIZE[1] / 2 - scale * sum(a * b for a, b in zip(offset, up)))


def turn(a, b, c):
    return (b[0] - a[0]) * (c[1] - a[1]) - (c[0] - a[0]) * (b[1] - a[1])


def convex_hull(points):
    def chain(ordered):
        result = []
        for point in ordered:
            while len(result) >= 2 and turn(result[-2], result[-1], point) <= 0:
                result.pop()
            result.append(point)
        return result
    points = sorted(set(points))
    return chain(points)[:-1] + chain(reversed(points))[:-1]


def analytic_mask(path, camera, width):
    # Project the known cuboid corners and fill their hull at pixel centers.
    # This oracle uses no ProGen3D mesh, depth buffer, or production rasterizer.
    corners = product((-width / 2, width / 2), (-PANEL_HEIGHT / 2, PANEL_HEIGHT / 2),
                      (-PANEL_DEPTH / 2, PANEL_DEPTH / 2))
    hull = convex_hull([project(point, camera) for point in corners])
    edges = list(zip(hull, hull[1:] + hull[:1]))
    pixels = bytearray()
    for y in range(SIZE[1]):
        for x in range(SIZE[0]):
            lit = all(turn(a, b, (x + 0.5, y + 0.5)) >= -1e-8 for a, b in edges)
            pixels.extend(b"\xff\xff\xff" if lit else b"\x00\x00\x00")
    path.write_bytes(f"P6\n{SIZE[0]} {SIZE[1]}\n255\n".encode("ascii") + pixels)


def check_png(path, size=None):
    with path.open("rb") as image:
        header = image.read(24)
    assert header[:8] == b"\x89PNG\r\n\x1a\n" and header[12:16] == b"IHDR", path
    dimensions = struct.unpack(">II", header[16:24])
    if size is not None:
        assert dimensions == size
    return dimensions


def render_reference(cli, root, grammar, name, camera):
    job = {
        "schema_version": 1, "grammar": str(grammar), "seed": 42, "width": SIZE[0], "height": SIZE[1],
        "output_dir": name, "orthographic": camera,
    }
    job_path = write_json(root / (name + ".json"), job)
    report = invoke(cli, "render", job_path)
    assert report["camera"]["projection"] == "orthographic"
    return Path(report["image"])


def synthetic_references(cli, root):
    original = EXAMPLES / "grammar_matching_panel.p3d"
    text = original.read_text(encoding="utf-8")
    for before, after in (
        ("Panel(1.625) # @param width 1.625", "Panel(2.375) # @param width 2.375"),
        ("candela(6) range(15)) # @param key_candela 6", "candela(24) range(15)) # @param key_candela 24"),
        ("material(navybluemattepaint)", "material(redmattepaint)"),
        ("# @param paint navybluemattepaint", "# @param paint redmattepaint"),
    ):
        assert text.count(before) == 1
        text = text.replace(before, after)
    target = root / "known_target.p3d"
    target.write_text(text, encoding="utf-8")
    reference = {
        "schema_version": 1, "seed": 42,
        "geometry": [{"name": "width", "min": 1.75, "max": 3, "step": 0.125}],
        "appearance": [
            {"name": "paint", "values": ["navybluemattepaint", "redmattepaint", "warmwhiteplaster"]},
            {"name": "key_candela", "values": [6, 24, 65]},
        ],
        "views": [],
    }
    cameras = [
        ("front", [0, 0, -1], [0, 1, 0]),
        ("right", [-1, 0, 0], [0, 1, 0]),
        ("top", [0, -1, 0], [0, 0, -1]),
        ("oblique", [-0.64, -0.34, -1], [0, 1, 0]),
    ]
    for name, direction, up in cameras:
        held_out = name == "oblique"
        camera = {
            "center": [0.08, 0.02, -0.04] if held_out else [0, 0, 0],
            "direction": direction, "up": up, "distance": 8, "pixels_per_unit": 56,
        }
        mask = root / (name + "-silhouette.ppm")
        analytic_mask(mask, camera, TRUE_WIDTH)
        entry = {
            "id": name, "camera": camera, "silhouette": mask.name, "held_out": held_out,
            "landmarks": [{
                "object": "FIT_PANEL", "attachment": "upper_right",
                "pixel": project((TRUE_WIDTH / 2, PANEL_HEIGHT / 2, PANEL_DEPTH / 2), camera),
            }],
        }
        if name in {"front", "right", "oblique"}:
            image = render_reference(cli, root, target, "reference_" + name, camera)
            entry["image"] = image.relative_to(root).as_posix()
        reference["views"].append(entry)
    return original, reference


def check_fit(cli, root):
    grammar, references = synthetic_references(cli, root)
    manifest = write_json(root / "views.json", references)
    result = invoke(cli, "fit", grammar, "--views", manifest, "--out", root / "fit")
    fit = result["fit"]
    evidence = read_json(Path(fit["report"]))
    assert evidence["ok"] and fit["parameters"] == evidence["parameters"]
    assert evidence["parameters"] == {"width": TRUE_WIDTH, "key_candela": 24, "paint": "redmattepaint"}
    assert len(evidence["geometry_search"]) == 11 and len(evidence["appearance_search"]) == 9
    assert min(row["color_rmse"] for row in evidence["appearance_search"]) < 1
    assert max(row["color_rmse"] for row in evidence["appearance_search"]) > 2
    assert [view["role"] for view in evidence["views"]] == ["training"] * 3 + ["held_out"]
    for view in evidence["views"]:
        assert view["silhouette_iou"] >= 0.995 and view["landmark_rmse_pixels"] < 0.001, view
        if "fitted_image" in view:
            assert view["color_rmse"] < 1, view
            check_png(root / "fit" / view["fitted_image"], SIZE)
        check_png(root / "fit" / view["difference_overlay"], SIZE)
    check_png(root / "fit" / evidence["contact_sheets"][0])

    oblique = references["views"][-1]
    assert render_reference(cli, root, Path(fit["fitted_grammar"]), "fitted_recheck",
                            oblique["camera"]).read_bytes() == (root / oblique["image"]).read_bytes()
    # A contradictory held-out target must change validation, with the training fit preserved.
    bad_mask = root / "holdout-altered.ppm"
    analytic_mask(bad_mask, oblique["camera"], 1.25)
    oblique["silhouette"] = bad_mask.name
    oblique["landmarks"][0]["pixel"] = project((0.625, PANEL_HEIGHT / 2, PANEL_DEPTH / 2), oblique["camera"])
    references.pop("appearance")
    for entry in references["views"]:
        entry.pop("image", None)
    references["views"].append({
        "id": "behind_camera", "held_out": True, "silhouette": "front-silhouette.ppm",
        "camera": {"center": [0, 0, -2], "direction": [0, 0, -1], "distance": 1, "pixels_per_unit": 56},
    })
    challenge = write_json(root / "views-heldout-challenge.json", references)
    changed = invoke(cli, "fit", grammar, "--views", challenge, "--out", root / "fit-heldout-challenge")["fit"]
    assert changed["parameters"]["width"] == TRUE_WIDTH and changed["views"][-2]["silhouette_iou"] < 0.85, changed
    assert changed["views"][-1]["predicted_pixels"] == 0
    return fit


def check_sweeps(cli, root):
    source = EXAMPLES / "grammar_matching_sweep.json"
    sweep = invoke(cli, "sweep", source, "--out", root / "sweep")["sweep"]
    evidence = read_json(Path(sweep["metrics"]))
    assert evidence["rendered_variants"] == 8
    assert not evidence["failed_variants"]
    assert len({row["rgb_sha256"] for row in evidence["variants"]}) == 8
    widths = {}
    for row in evidence["variants"]:
        pixels = row["visible_object_pixels"]
        assert pixels > 0
        width = row["parameters"]["width"]
        assert widths.setdefault(width, pixels) == pixels
        check_png(root / "sweep" / row["preview"], SIZE)
    assert list(widths.values()) == sorted(set(widths.values()))
    check_png(Path(sweep["contact_sheets"][0]))
    repeat = invoke(cli, "sweep", source, "--out", root / "sweep-repeat")["sweep"]
    assert read_json(Path(repeat["metrics"])) == evidence

    case = read_json(source)
    case["job"] = str((source.parent / case["job"]).resolve())
    case["object"] = "OBJECT_NOT_PRESENT"
    case["parameters"] = [{"name": "width", "values": [2]}]
    negative = write_json(root / "sweep-no-visible-object.json", case)
    failed = invoke(cli, "sweep", negative, "--out", root / "sweep-no-visible-object", expected=4)
    assert failed["sweep"]["failed_variants"] == 1
    assert read_json(Path(failed["sweep"]["metrics"]))["variants"][0]["visible_object_pixels"] == 0

    camera = (EXAMPLES / "courtyard_camera.json").resolve()
    aperture = write_json(root / "aperture.json", {
        "schema_version": 1, "job": str(camera), "aperture_radii": [0, 0.14, 0.23],
    })
    lens = invoke(cli, "sweep", aperture, "--out", root / "sweep-aperture")["sweep"]
    assert lens["rendered_variants"] == 3
    assert len({row["rgb_sha256"] for row in read_json(Path(lens["metrics"]))["variants"]}) == 3
    return sweep, lens


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cli", type=Path)
    parser.add_argument("--output-root", type=Path, default=Path(tempfile.gettempdir()))
    args = parser.parse_args()

    cli = args.cli.resolve(strict=True)
    args.output_root.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="progen3d-experiment-", dir=args.output_root.resolve()))
    fit = check_fit(cli, root)
    sweep, aperture = check_sweeps(cli, root)
    report = write_json(root / "report.json", {
        "known_width": TRUE_WIDTH, "fit": fit, "sweep": sweep, "aperture": aperture,
    })
    print(f"Calibrated fitting and sweep checks passed. Report: {report}")


if __name__ == "__main__":
    main()
