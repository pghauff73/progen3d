#!/usr/bin/env python3
"""Export native form-study triangle geometry as named, metre-scale ASCII STL regions."""
import argparse
from collections import defaultdict
from io import StringIO
import json
import math
from pathlib import Path
import re

from import_airflow_faces import cross


def named_surfaces(lines, patch_map=None):
    points = []
    surfaces = defaultdict(list)
    seen = set()
    region = None
    for line_number, line in enumerate(lines, 1):
        fields = line.split()
        if not fields or fields[0].startswith("#"):
            continue
        tag, values = fields[0], fields[1:]
        if tag == "o":
            if len(values) != 1 or values[0] in seen:
                raise ValueError(f"line {line_number}: expected a unique object name")
            part = values[0]
            seen.add(part)
            region = part if patch_map is None else patch_map.get(part)
            if not isinstance(region, str) or not re.fullmatch(r"[A-Za-z_][A-Za-z_0-9]*", region):
                raise ValueError(f"invalid or missing patch name for {part}")
        elif tag == "v":
            point = tuple(map(float, values))
            if len(point) != 3 or not all(map(math.isfinite, point)):
                raise ValueError(f"line {line_number}: invalid vertex")
            points.append(point)
        elif tag == "f":
            if region is None or len(values) != 3:
                raise ValueError(f"line {line_number}: expected a named triangle")
            indices = tuple(int(value) for value in values)
            if any(i < 1 or i > len(points) for i in indices):
                raise ValueError(f"line {line_number}: vertex index outside native mesh")
            a, b, c = (points[i-1] for i in indices)
            normal = cross(tuple(y-x for x, y in zip(a,b)), tuple(y-x for x, y in zip(a,c)))
            magnitude = math.hypot(*normal)
            if magnitude:
                surfaces[region].append((tuple(v/magnitude for v in normal), a, b, c))
    if not surfaces:
        raise ValueError("expected at least one non-degenerate native triangle")
    if patch_map is not None and set(patch_map) != seen:
        raise ValueError("patch map and native object names must match")
    return surfaces


def stl_text(surfaces):
    text = StringIO()
    def vector(values):
        return " ".join(format(value, ".9g") for value in values)
    for name, triangles in surfaces.items():
        print("solid", name, file=text)
        for normal, a, b, c in triangles:
            print(" facet normal", vector(normal), "\n  outer loop", file=text)
            for point in (a, b, c):
                print("   vertex", vector(point), file=text)
            print("  endloop\n endfacet", file=text)
        print("endsolid", name, file=text)
    return text.getvalue()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("native_obj", type=Path)
    parser.add_argument("output_stl", type=Path)
    parser.add_argument("--patch-map", type=Path, help="JSON mapping every native object to a patch")
    args = parser.parse_args()
    try:
        mapping = json.loads(args.patch_map.read_text()) if args.patch_map else None
        if mapping is not None and not isinstance(mapping, dict):
            raise ValueError("expected a JSON object for the patch map")
        with args.native_obj.open() as source:
            output = stl_text(named_surfaces(source, mapping))
        args.output_stl.write_text(output)
    except (OSError, ValueError) as error:
        parser.exit(1, str(error) + "\n")
