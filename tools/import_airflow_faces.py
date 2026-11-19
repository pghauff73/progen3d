#!/usr/bin/env python3
"""Convert one steady CFD surface snapshot, in body axes about the CM, to grammar loads."""

import argparse
import csv
import math
from pathlib import Path

FIELDS = "patch x y z area_x area_y area_z pressure shear_x shear_y shear_z".split()


def cross(a, b):
    return (a[1]*b[2] - a[2]*b[1], a[2]*b[0] - a[0]*b[2], a[0]*b[1] - a[1]*b[0])


def surface_loads(rows):
    """Sum -p*dA + shear*|dA| and its moment for each named patch."""
    result = {}
    for index, row in enumerate(rows, start=2):
        name = (row["patch"] or "").strip()
        if not name or any(c in name for c in "\r\n"):
            raise ValueError(f"row {index}: expected a single-line patch name")
        values = [float(row[key]) for key in FIELDS[1:]]
        if not all(map(math.isfinite, values)):
            raise ValueError(f"row {index}: nonfinite field")
        point, normal_area, pressure, shear = values[:3], values[3:6], values[6], values[7:]
        area = math.hypot(*normal_area)
        if area == 0:
            raise ValueError(f"row {index}: zero area vector")
        force = tuple(-pressure * a + area * t for a, t in zip(normal_area, shear))
        if not all(map(math.isfinite, force)):
            raise ValueError(f"row {index}: force overflow")
        moment = cross(point, force)
        totals = result.setdefault(name, [0.0] * 6)
        for i, value in enumerate((*force, *moment)):
            totals[i] += value
            if not math.isfinite(totals[i]):
                raise ValueError(f"row {index}: accumulated load overflow")
    if not result:
        raise ValueError("expected at least one surface face")
    return result


def grammar(loads):
    for patch, values in loads.items():
        yield "# CFD patch " + patch
        yield "LoadPoint(0 0 0)"
        for action, vector in [("BodyForce", values[:3]), ("BodyTorque", values[3:])]:
            yield action + "(" + " ".join(format(x, ".9g") for x in vector) + ")"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("faces", type=Path)
    args = parser.parse_args()
    try:
        with args.faces.open(newline="") as source:
            rows = csv.DictReader(source)
            if not rows.fieldnames or not set(FIELDS).issubset(rows.fieldnames):
                raise ValueError("expected columns " + ",".join(FIELDS))
            print("\n".join(grammar(surface_loads(rows))))
    except (OSError, TypeError, ValueError) as exc:
        parser.exit(1, str(exc) + "\n")
