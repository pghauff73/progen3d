"""Check native world-space geometry against independent predictions for all four lessons."""
import argparse
import math
import subprocess
import tempfile
from pathlib import Path
from statistics import mean

ROOT = Path(__file__).resolve().parents[1]
STUDIES = ROOT / "examples" / "FormStudies"
TOLERANCE = 0.00002


def close(actual, expected):
    if not math.isclose(actual, expected, abs_tol=TOLERANCE, rel_tol=0):
        raise AssertionError(f"Measured {actual}, expected {expected}")


def point_close(actual, expected):
    for component, target in zip(actual, expected, strict=True):
        close(component, target)


def sample(binary, grammar, seconds, path):
    subprocess.run([binary, grammar, str(seconds), path], check=True, stdout=subprocess.DEVNULL)
    parts = []
    for row in path.read_text().splitlines():
        fields = row.split()
        if fields[0] == "o":
            parts.append({"vertices": [], "faces": 0})
        elif fields[0] == "usemtl":
            parts[-1]["material"] = fields[1]
        elif fields[:2] == ["#", "centreline"]:
            values = list(map(float, fields[2:]))
            parts[-1]["line"] = values[:3], values[3:]
        elif fields[0] == "v":
            parts[-1]["vertices"].append(tuple(map(float, fields[1:])))
        elif fields[0] == "f":
            parts[-1]["faces"] += 1
    for part in parts:
        assert part["faces"] > 0 and part["faces"] * 3 == len(part["vertices"])
    return parts


def centre(part):
    return tuple((min(axis) + max(axis)) / 2 for axis in zip(*part["vertices"]))


def check_fan(parts, seconds):
    assert len(parts) == 25
    for index, part in enumerate(parts):
        close(centre(part)[1], 2.4 * index / 24)
        angle = math.radians(240 * index / 24)
        for x, _, z in part["vertices"]:
            close(abs(x * math.cos(angle) - z * math.sin(angle)), 1.3)
            close(abs(x * math.sin(angle) + z * math.cos(angle)), 0.11)


def check_saddle(parts, seconds):
    assert len(parts) == 38
    for index, part in enumerate(parts):
        a, b = part["line"]
        for fraction in (0, 0.25, 0.5, 0.75, 1):
            x, y, z = (v + fraction * (w - v) for v, w in zip(a, b))
            close(y, 0.78 * x * z / 1.7**2)
        if index < 34:
            u = 2 * (index // 2 + 1) / 18 - 1
            fixed, spanning = (2, 0) if index % 2 == 0 else (0, 2)
            close(a[fixed], 1.7 * u)
            close(b[fixed], a[fixed])
            close(a[spanning], -1.7)
            close(b[spanning], 1.7)
    frame = [p["line"] for p in parts[-4:]]
    for index, (a, b) in enumerate(frame):
        point_close(b, frame[(index + 1) % 4][0])
        close(max(abs(a[0]), abs(a[2])), 1.7)


def check_choir(parts, seconds):
    assert len(parts) == 23
    for index in range(9):
        suspension, bob = parts[5 + 2 * index:7 + 2 * index]
        pivot, tip = suspension["line"]
        beats = 12 + index
        angle = math.radians(12) * math.cos(math.tau * beats * seconds / 24)
        length = 9.81 * (24 / (math.tau * beats))**2
        point_close(pivot, ((index - 4) * 0.15, 1.12, 0))
        close(math.dist(tip, pivot), length)
        point_close(tip, (pivot[0], pivot[1] - length * math.cos(angle), -length * math.sin(angle)))
        point_close(centre(bob), tip)


def check_lantern(parts, seconds):
    assert len(parts) == 74
    twist = math.radians(86 - 32 * math.cos(math.tau * seconds / 8))
    radius = 1.2
    height = math.sqrt(3**2 - (2 * radius * math.sin(twist / 2))**2)
    close(centre(parts[0])[1], 0)
    close(centre(parts[1])[1], height)
    for index in range(24):
        station = -math.tau * index / 24
        right, left, shoe = parts[2 + 3 * index:5 + 3 * index]
        for sign, rod in ((1, right), (-1, left)):
            a, b = rod["line"]
            point_close(a, (radius * math.cos(station), 0, radius * math.sin(station)))
            point_close(b, (radius * math.cos(station + sign * twist), height,
                            radius * math.sin(station + sign * twist)))
            point_close(centre(shoe), a)
            close(math.dist(a, b), 3)
            close(math.hypot(mean((a[0], b[0])), mean((a[2], b[2]))),
                  radius * math.cos(twist / 2))


CASES = [
    ("01_screw_fan", check_fan, [0]),
    ("02_string_saddle", check_saddle, [0]),
    ("03_pendulum_choir", check_choir, [0, 0.375, 3, 6, 12, 24]),
    ("04_constant_length_lantern", check_lantern, [0, 1, 2, 3, 4, 6, 8]),
]


def check_reduction(sampler, directory):
    source = STUDIES / "05_lantern_essence.p3d"
    for seconds in (0, 1, 2, 3, 4, 6, 8):
        full = sample(sampler, STUDIES / "04_constant_length_lantern.p3d", seconds, directory / "full.obj")
        reduced = sample(sampler, source, seconds, directory / "reduced.obj")
        copper = [part for part in full if part["material"] == "copperbrushedmetal"]
        assert len(copper) == len(reduced) == 24
        for a, b in zip(copper, reduced):
            assert a["material"] == b["material"] and a["faces"] == b["faces"]
            for v, w in zip(a["vertices"], b["vertices"], strict=True):
                point_close(v, w)
    triangles = sum(part["faces"] for part in reduced)
    print(f"05_lantern_essence: 7 time samples matched all 24 copper meshes, {triangles} triangles")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampler", type=Path, default=ROOT / "build" / "form-study-sample")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory() as work:
        path = Path(work) / "sample.obj"
        for name, check, times in CASES:
            for seconds in times:
                parts = sample(args.sampler, STUDIES / (name + ".p3d"), seconds, path)
                check(parts, seconds)
            print(f"{name}: {len(times)} time sample(s), {len(parts)} instances, "
                  f"{sum(p['faces'] for p in parts)} triangles in the final sample")
        check_reduction(args.sampler, Path(work))


if __name__ == "__main__":
    main()
