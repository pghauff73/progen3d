"""Measure the authored geometry with ProGen3D's native form-study sampler."""
import argparse
import math
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
TOL = 2e-5


def near(value, expected):
    if not math.isclose(value, expected, abs_tol=TOL, rel_tol=0):
        raise AssertionError(f"Measured {value}, expected {expected}")


def bounds(part, axis):
    values = [v[axis] for v in part["vertices"]]
    return min(values), max(values)


def span_and_gap(parts):
    intervals = sorted(bounds(part, 0) for part in parts)
    gaps = [second[0] - first[1] for first, second in zip(intervals, intervals[1:])]
    if gaps:
        near(min(gaps), max(gaps))
    return intervals[-1][1] - intervals[0][0], min(gaps) if gaps else None


def max_vertex_error(a, b):
    if len(a) != len(b):
        raise AssertionError("Different primitive counts")
    worst = 0.0
    for left, right in zip(a, b):
        assert left["material"] == right["material"]
        for v, w in zip(left["vertices"], right["vertices"], strict=True):
            worst = max(worst, math.dist(v, w))
    return worst


def check_forks(parts, depth):
    assert len(parts) == 2**(depth + 1) - 1
    # Native emission is preorder. The size of each subtree identifies siblings.
    def visit(i, remaining, start, direction, length):
        first, last = parts[i]["line"]
        near(math.dist(first, start), 0)
        expected = (start[0] - length * math.sin(direction),
                    start[1] + length * math.cos(direction), start[2])
        near(math.dist(last, expected), 0)
        if remaining:
            visit(i + 1, remaining - 1, last, direction + math.radians(28), length * 0.72)
            visit(i + 2**remaining, remaining - 1, last, direction - math.radians(28), length * 0.72)
    visit(0, depth, (0, 0, 0), 0.0, 0.7)


def check_bays(parts, bays, blades):
    assert len(parts) == bays * (blades + 4)
    heights = []
    for i in range(bays):
        block = parts[i*(blades+4):(i+1)*(blades+4)]
        left, right = min(bounds(p, 0)[0] for p in block), max(bounds(p, 0)[1] for p in block)
        near(left, 1.6 * i)
        near(right, 1.6 * i + 1.5)
        for j, p in enumerate(block[4:]):
            xmin, xmax = bounds(p, 0)
            ymin, ymax = bounds(p, 1)
            near((xmin+xmax)/2, 1.6*i + 0.08 + (j+0.5)*1.34/blades)
            near(ymin, 0.08)
            assert xmin >= left+0.08-TOL and xmax <= right-0.08+TOL
            assert ymin+0.68*1.04-TOL <= ymax <= 1.12+TOL
            heights.append(ymax-ymin)
    return heights


def run(repo, work, keep_meshes):
    sys.path.insert(0, str(repo / "tests"))
    from check_form_studies import sample
    binary = repo / "build/form-study-sample"
    grammar_dir = HERE / "grammars"
    def render(name, key, edits=()):
        source = (grammar_dir / (name + ".p3d")).read_text()
        for before, after in edits:
            if source.count(before) != 1:
                raise ValueError(f"Expected one edit site for {before!r}")
            source = source.replace(before, after)
        grammar = work / (key + ".p3d")
        grammar.write_text(source)
        output_dir = keep_meshes or work
        return sample(binary, grammar, 0, output_dir / (key + ".obj"))

    reference = render("01_measured_layout", "measured")
    assert len(reference) == 9
    xs = sorted(sum(bounds(p, 0))/2 for p in reference)
    # Fit centre and slope on the first three, hold out the remaining six.
    pitch = (xs[2] - xs[0]) / 2
    intercept = sum(xs[:3])/3 - pitch
    holdout = max(abs(intercept + i*pitch - xs[i]) for i in range(3, 9))
    near(holdout, 0)
    print(f"Three-centre fit: pitch {pitch:.6f}, origin {intercept:.6f}, held-out error {holdout:.3g}")

    for n in (1, 2, 9, 13, 20, 21, 25, 26):
        fixed = render("02_fixed_pitch", f"pitch_{n}", [("Course(9 ", f"Course({n} ")])
        fitted = render("03_fixed_span", f"span_{n}", [("Screen(9 ", f"Screen({n} ")])
        assert len(fixed) == len(fitted) == n
        fw, fg = span_and_gap(fixed)
        sw, sg = span_and_gap(fitted)
        near(fw, 0.12 + (n-1)*0.36)
        near(sw, 3 if n > 1 else 0.12)
        if n == 1:
            near(sum(bounds(fitted[0], 0))/2, 1.5)
        else:
            near(fg, 0.24)
            near(sg, (3-n*0.12)/(n-1))
        if n in (20, 21):
            assert (sg >= 0.03) == (n == 20)
        if n == 9:
            errors = [max_vertex_error(reference, other) for other in (fixed, fitted)]
            for e in errors:
                near(e, 0)
            print("Baseline maximum vertex errors:", *(f"{e:.3g}" for e in errors))
        print(f"n={n:2}  pitch width {fw:.4f}  span width {sw:.4f}  span gap {sg if sg is None else round(sg, 5)}")

    for bays, blades in ((1, 3), (3, 7), (5, 9)):
        parts = render("04_bay_assembly", f"wall_{bays}_{blades}",
                       [("Wall(3 1.5 0.10 7)", f"Wall({bays} 1.5 0.10 {blades})")])
        heights = check_bays(parts, bays, blades)
        assert max(heights)-min(heights) > TOL
        print(f"{bays} bays x {blades} blades: {len(parts)} parts; heights {min(heights):.4f}..{max(heights):.4f}")
    shared = render("04_bay_assembly", "one_height", [
        ("Start -> Wall", "Start -> R fraction(0.68 1.0) Wall"),
        ("    R fraction(0.68 1.0)\n", "")])
    heights = check_bays(shared, 3, 7)
    near(max(heights)-min(heights), 0)
    print("One assembly-level sample: all 21 blade heights match")

    per_bay = render("04_bay_assembly", "bay_heights", [
        ("    R fraction(0.68 1.0)\n", ""),
        ("Bay(width n border) ->", "Bay(width n border) -> R fraction(0.68 1.0)")])
    heights = check_bays(per_bay, 3, 7)
    groups = [heights[i:i+7] for i in range(0, len(heights), 7)]
    for group in groups:
        near(max(group) - min(group), 0)
    assert max(heights) - min(heights) > TOL
    print("One sample per bay: blade heights match within each of three bays and vary across bays")

    for depth in (0, 1, 2, 4, 6):
        parts = render("05_recursive_rib", f"fork_{depth}", [("Rib(4 0.7", f"Rib({depth} 0.7")])
        check_forks(parts, depth)
        print(f"Fork depth {depth}: {len(parts)} segments, {sum(p['faces'] for p in parts)} triangles")


if __name__ == "__main__":
    args = argparse.ArgumentParser(description=__doc__)
    args.add_argument("--repo", type=Path, required=True, help="ProGen3D checkout with build/form-study-sample")
    args.add_argument("--keep-meshes", type=Path, help="Optional directory for native OBJ exports")
    options = args.parse_args()
    if options.keep_meshes:
        options.keep_meshes.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory() as tmp:
        run(options.repo.resolve(), Path(tmp), options.keep_meshes)
