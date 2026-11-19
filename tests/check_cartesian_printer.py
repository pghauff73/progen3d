"""Measure native printer meshes and connections against an independent segment schedule."""
import argparse
from collections import defaultdict
import math
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
GRAMMAR = ROOT / "examples/Mechanics/CartesianPrinter/CartesianPrinter.p3d"


def ease(fraction):
    fraction = min(1, max(0, fraction))
    return fraction**2 * (3 - 2 * fraction)


def job(t):
    point = [-42.0, -32.0, 10.0]
    moves = [(1, (-28, -20, 10), False), (1, (-28, -20, .24), False)]
    for layer in range(3):
        height = .24 * (layer + 1)
        if layer:
            moves.append((.5, (-28, -20, height), False))
        for duration, x, y in [(2, 28, -20), (1.5, 28, 20), (2, -28, 20), (1.5, -28, -20)]:
            moves.append((duration, (x, y, height), True))
    moves.extend([(1, (-28, -20, 12.72), False), (1.5, (34, 28, 12.72), False)])
    length = 0
    edges = 0
    for duration, target, printing in moves:
        u = ease(t / duration)
        if printing and u > 0:
            edges += 1
            length += u * math.dist(target, point)
        point = [v + u * (goal - v) for v, goal in zip(point, target)]
        if t <= duration:
            break
        t -= duration
    return point, length, edges


def read_scene(path):
    objects = defaultdict(list)
    links = []
    with path.open() as data:
        for line in data:
            if line.startswith("# connection "):
                links.append(tuple(line.split()[2:]))
            elif line.startswith("# owner "):
                vertices = []
                objects[line.split()[2]].append(vertices)
            elif line.startswith("v "):
                vertices.append(tuple(map(float, line.split()[1:])))
    return objects, links


def bounds(vertices):
    return [(min(values), max(values)) for values in zip(*vertices)]


def centre(vertices):
    return [(lo + hi) / 2 for lo, hi in bounds(vertices)]


def near(actual, expected, tolerance=.0002):
    assert abs(actual - expected) < tolerance, (actual, expected)


def angle(actual, expected):
    near(math.remainder(actual - expected, 2 * math.pi), 0, .00004)


def check_guides(objects, links):
    bearings = [row for row in links if row[1] == "Bore" and row[3] == "Journal"]
    assert len(bearings) == len({row[0] for row in bearings}) == 12
    assert len({row[2] for row in bearings}) == 6
    for name, _, shaft, _ in bearings:
        bearing_bounds = bounds(objects[name][0])
        shaft_bounds = bounds(objects[shaft][0])
        direction = {"X": 0, "Y": 2, "Z": 1}[shaft[0]]
        for dim in range(3):
            if dim == direction:
                assert shaft_bounds[dim][0] < bearing_bounds[dim][0] < bearing_bounds[dim][1] < shaft_bounds[dim][1]
            else:
                near(sum(bearing_bounds[dim])/2, sum(shaft_bounds[dim])/2)
        axes = [i for i in range(3) if i != direction]
        middle = centre(objects[name][0])
        bore = min(math.hypot(v[axes[0]]-middle[axes[0]], v[axes[1]]-middle[axes[1]]) for v in objects[name][0])
        near(bore, 4.04)


def solid_volume(vertices):
    # The sampler emits unindexed triangles. Translate near the part before summing.
    origin = vertices[0]
    total = 0
    for i in range(0, len(vertices), 3):
        a,b,c = [[v-o for v,o in zip(pt, origin)] for pt in vertices[i:i+3]]
        total += a[0]*(b[1]*c[2]-b[2]*c[1]) + a[1]*(b[2]*c[0]-b[0]*c[2]) + a[2]*(b[0]*c[1]-b[1]*c[0])
    return abs(total)/6


def check(objects, links, t):
    (x,y,z), length, edges = job(t)
    area = .24*(.48-.24) + math.pi*.24**2/4
    feed = length*area/(math.pi*(1.75/2)**2)
    check_guides(objects, links)
    assert len(links) == len(set(links)) == 26
    bed = bounds(objects["Bed"][1])
    near(bed[1][1], 60)
    near(sum(bed[2])/2, y)
    nozzle = bounds(objects["Nozzle"][0])
    near(nozzle[1][0], 60+z)
    near(sum(nozzle[0])/2, x)
    near(sum(nozzle[2])/2, 0)
    for axis, displacement, horizontal, base, height in [("X",x,0,-109,88+z),("Y",y,2,-100,27)]:
        for part in [axis+"Pulley", axis+"Idler"]:
            marker = centre(objects[part][-1])
            wheel_centre = base if part.endswith("Pulley") else -base
            angle(math.atan2(marker[1]-height, marker[horizontal]-wheel_centre), -2*math.pi*displacement/40)
        near(centre(objects[axis+"Clamp"][0])[horizontal], displacement)
    for end, base in [("Left",-104), ("Right",104)]:
        screw = "Z"+end+"Screw"
        marker = centre(objects[screw][-1])
        angle(math.atan2(-39-marker[2], marker[0]-base), -2*math.pi*z/8)
        nut = centre(objects["Z"+end+"Nut"][0])
        near(nut[1], 89+z)
        near(nut[0], base)
        near(nut[2], -39)
    hob = centre(objects["Hob"][-1])
    angle(math.atan2(hob[1]-199,hob[0]+42), feed/5)
    strands = objects.get("Print", [])
    assert len(strands) == edges
    deposited = 0
    for index, vertices in enumerate(strands):
        box = bounds(vertices)
        along = 0 if index % 2 == 0 else 2
        span = box[along][1] - box[along][0]
        side = index % 4
        # Each segment must begin at its bed-local corner and stay there during a later Y move.
        if side in (0, 2):
            near(sum(box[2])/2 - y, 20 if side == 0 else -20)
            near(box[0][0 if side == 0 else 1], -28 if side == 0 else 28)
        else:
            near(sum(box[0])/2, 28 if side == 1 else -28)
            near(box[2][1 if side == 1 else 0] - y, 20 if side == 1 else -20)
        near(box[1][0], 60 + (index//4)*.24)
        near(box[1][1], 60 + (index//4 + 1)*.24)
        deposited += span
    near(deposited, length, .001)
    # Curved bead ends use eight facets per semicircle in the rendered cross-section.
    if length:
        mesh_area = .24*(.48-.24) + 8*math.sin(math.pi/8)*(.24/2)**2
        near(sum(map(solid_volume,strands)) / length, mesh_area, .00002)
    return length, length*area, feed


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sampler", type=Path, default=ROOT/"build/form-study-sample")
    args = parser.parse_args()
    times = [0,.5,1.5,2,2.0625,3,4,4.75,5.5,6.5,8.25,9,9.25,9.5,12.25,16.5,16.75,17,20,23.25,24,24.5,25.75,26.5,40]
    with tempfile.TemporaryDirectory() as folder:
        path = Path(folder)/"frame.obj"
        baseline = Path(folder)/"IdealPrinter.p3d"
        source = GRAMMAR.read_text()
        assert source.count("Start -> LoadedPrinter") == 1
        baseline.write_text(source.replace("Start -> LoadedPrinter", "Start -> IdealPrinter"))
        for t in times:
            subprocess.run([args.sampler.resolve(), baseline, str(t), path], check=True, stdout=subprocess.DEVNULL)
            result = check(*read_scene(path), t)
    print(f"{len(times)} native samples passed. Path {result[0]:.3f} mm, bead {result[1]:.4f} mm³, feed {result[2]:.4f} mm.")
