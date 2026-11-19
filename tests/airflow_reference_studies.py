#!/usr/bin/env python3
"""Check imported CFD wall tractions and compare the native plate law to T3A measurements."""
import argparse
import csv
import math
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
from import_airflow_faces import FIELDS, surface_loads


def read_zones(path):
    zones = {}
    for line in path.read_text().splitlines():
        if line.lower().lstrip().startswith("zone"):
            label = re.search(r'\bt\s*=\s*"([^"]+)"', line, re.I)[1]
            zones[label] = []
        elif re.match(r"\s*[.\d+-]", line):
            zones[label].append(tuple(map(float, line.split())))
    return zones


def check_wall_fields(profiles, drag_history):
    # Unit dynamic pressure, unit span and two units of plate length reproduce Cd.
    # Integrate each piecewise-linear profile by midpoint quadrature. The source
    # excludes both endpoints, so hold the nearest sample across each end gap.
    length = 2.0
    height = 0.25
    for code in ("CFL3D", "FUN3D"):
        samples = profiles[code]
        points = [(0, samples[0][1]), *samples, (length, samples[-1][1])]
        faces = []
        for (x0, cf0), (x1, cf1) in zip(points, points[1:]):
            if x1 <= x0:
                raise ValueError("CF station positions must strictly increase")
            fields = (code, (x0+x1)/2, height, 0, 0, x1-x0, 0, 0, (cf0+cf1)/2, 0, 0)
            faces.append(dict(zip(FIELDS, fields)))
        force_x, _, _, _, _, moment_z = surface_loads(faces)[code]
        fine = sorted(drag_history[code], reverse=True)[:3]
        expected = fine[0][-1]
        recovered = force_x/length
        relative_error = abs(recovered/expected-1)
        assert relative_error < 0.002, (code, recovered, expected)
        assert math.isclose(moment_z, -height * expected * length, rel_tol=0.002)
        print(f"{code}: recovered Cd={recovered:.8f}, solver={expected:.8f}, "
              f"difference={100*relative_error:.3f}%; imposed-offset Mz={moment_z:.8f}.")
        print("  Published mesh history (cells, Cd):",
              ", ".join(f"({int(row[0])}, {row[-1]:.8f})" for row in reversed(fine)))


def compare_transition(samples, executable):
    # Local plate coefficient = d(integrated shear force)/d(area) / dynamic pressure.
    # All stations use U=1, rho=1, nu=1e-6, unit span and A=L. Finite difference
    # the actual grammar trace to compare local and measured Cf at equal Rex.
    errors = []
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory)/"plate.p3d"
        for _, rex, cf_times_1000, _ in samples:
            length = rex*1e-6
            delta = length*0.005
            loads = []
            for pos in (length-delta, length+delta):
                path.write_text(f"Start -> AirProperties(1 0.000001) Wind(1 0 0) "
                                f"BodyInertia(1 1 1) PlateFriction({pos:.10f} {pos:.10f} 1) "
                                "!I(Cube material(tealmatteceramic) 1 1)\n")
                result = subprocess.run([str(executable), "--trace", str(path), "0"], cwd=ROOT,
                                        capture_output=True, text=True, check=True)
                row = next(csv.DictReader(result.stdout.splitlines()))
                loads.append(float(row["fx"]))
            predicted = (loads[1]-loads[0])/(2*delta*0.5)
            measured = cf_times_1000*0.001
            errors.append((rex, predicted, measured))
    largest = max(errors, key=lambda r: abs(r[1]-r[2]))
    rms = math.sqrt(sum((p-m)**2 for _, p, m in errors)/len(errors))
    print(f"T3A: {len(errors)} stations, local Cf RMS difference {rms:.6f}; largest absolute "
          f"difference at Rex={largest[0]:.0f}, native={largest[1]:.6f}, measured={largest[2]:.6f}.")
    print("The native assumed-transition correlation is a reference case. T3A bypass transition "
          "requires a model for freestream turbulence and transition onset.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cf_plate", type=Path)
    parser.add_argument("drag_convergence", type=Path)
    parser.add_argument("t3a", type=Path)
    parser.add_argument("--native", type=Path, default=ROOT/"build/airflow-test")
    args = parser.parse_args()
    check_wall_fields(read_zones(args.cf_plate), read_zones(args.drag_convergence))
    compare_transition(read_zones(args.t3a)["Data"], args.native.resolve())
