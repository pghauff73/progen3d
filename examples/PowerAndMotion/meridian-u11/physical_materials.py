"""Assign provisional bulk materials to the U11 appearance palette.

Density in kg/m³, effective sheet thickness in metres. Paint colours
identify nominal substrates. The canonical asset is an appearance model.
"""

from dataclasses import dataclass
import re


@dataclass(frozen=True)
class PhysicalMaterial:
    substrate: str
    density: int
    sheet_thickness: float


PALETTE = {
    "blackbluepaintmatte": PhysicalMaterial("carbon fibre laminate", 1550, 0.0008),
    "bluegreymattepaint": PhysicalMaterial("glass-filled nylon", 1350, 0.0012),
    "charcoalmattemetal": PhysicalMaterial("anodized aluminium alloy", 2700, 0.0015),
    "coolwhitemattepaint": PhysicalMaterial("painted aluminium alloy", 2700, 0.0012),
    "goldbrushedmetal": PhysicalMaterial("brass", 8500, 0.0006),
    "goldpolishedmetal": PhysicalMaterial("brass", 8500, 0.0006),
    "greenemissivepaint": PhysicalMaterial("polycarbonate indicator", 1200, 0.0010),
    "offwhiteceramic": PhysicalMaterial("alumina ceramic", 3900, 0.0012),
    "orangeglosspaint": PhysicalMaterial("painted aluminium alloy", 2700, 0.0012),
    "silverbrushedmetal": PhysicalMaterial("aluminium alloy", 2700, 0.0015),
    "smokytransparentglass": PhysicalMaterial("laminated silica glass", 2500, 0.012),
}

INSTANCE = re.compile(r"(?<!\w)!?I\(([A-Za-z][A-Za-z0-9_]*)\b")
MATERIAL = re.compile(r"\bmaterial\(([A-Za-z][A-Za-z0-9_]*)\)")


def annotate_legacy_materials(source: str) -> str:
    """Insert P before each original I action without changing its appearance."""
    # Mask comments to keep the same offsets and avoid annotating code examples.
    code = re.sub(r"(?m)#.*$", lambda match: " " * len(match[0]), source)
    parts = []
    cursor = 0
    for instance in INSTANCE.finditer(code):
        opening = instance.start(1) - 1
        depth = 1
        ending = opening + 1
        while ending < len(code) and depth:
            depth += (code[ending] == "(") - (code[ending] == ")")
            ending += 1
        if depth:
            raise ValueError(f"Unclosed instance at source offset {opening}")
        materials = MATERIAL.findall(code, opening, ending)
        if len(materials) != 1 or materials[0] not in PALETTE:
            raise ValueError(f"Missing physical substrate for instance at {opening}: {materials}")
        material = PALETTE[materials[0]]
        shape = instance[1]
        if shape not in {"Cube", "Sphere", "Loft", "SweepDisk"}:
            raise ValueError(f"Select a solid or sheet model for inherited {shape}")
        is_solid = shape in {"Cube", "Sphere"} and materials[0] != "smokytransparentglass"
        density = f"P({material.density})" if is_solid else f"P({material.density} {material.sheet_thickness:g})"
        parts.extend((source[cursor:instance.start()], density, " "))
        cursor = instance.start()
    if not parts:
        raise ValueError("The source contains no material-bearing primitives")
    parts.append(source[cursor:])
    annotations = (
        "# Physical density in kg/m³ with optional sheet thickness in metres.\n"
        "# The inherited render surfaces use estimated substrates and thicknesses.\n"
    )
    return annotations + "".join(parts)
