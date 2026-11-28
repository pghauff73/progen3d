"""Derive the drive-enabled Meridian U11 grammar from the pinned source asset."""
from argparse import ArgumentParser
from hashlib import sha1
from pathlib import Path

from physical_materials import annotate_legacy_materials


SOURCE_BLOB = "6afed97762b480f04d6587b46ddb1b5f88fdc586"
DIRECTORY = Path(__file__).resolve().parent


def replace_rule_site(grammar, current, replacement):
    if grammar.count(current) != 1:
        raise ValueError(f"Expected one Meridian grammar seam for {current!r}")
    return grammar.replace(current, replacement, 1)


def integrate_mechanisms(source, profiles, annotate=True):
    source_blob = sha1(f"blob {len(source)}\0".encode() + source).hexdigest()
    if source_blob != SOURCE_BLOB:
        raise ValueError(f"The U11 source blob is {source_blob}; expected {SOURCE_BLOB}.")
    grammar = source.decode("utf-8").replace("\r\n", "\n")
    mounts = (
        "      Interface(id(DriveHousingMount) type(Mate) origin(0 0 0) "
        "normal(0 0 1) tangent(1 0 0) region(Point))\n"
    )
    negative_contact = "      Interface(id(Contact) type(Mate) origin(-1*0.084 0 0.205)"
    positive_contact = "      Interface(id(Contact) type(Mate) origin(1*0.084 0 0.205)"
    rotary_mount = "      Interface(id(DriveSocket) type(Socket)"
    guide_mount = "      Interface(id(GuideDatum) type(InspectionInterface)"
    bus_input = "      Interface(id(BatteryIn) type(ElectricalPort)"
    sites = (
        ("\nStart -> MeridianRobot(1 0)\n", "\nStart -> U11DriveMotion\n"),
        (
            "RobotAssembly(surfaceDetail socketRoll) ->\n",
            "RobotAssembly(surfaceDetail socketRoll) ->\n"
            "    RobotAssemblyDrive(surfaceDetail socketRoll 0)\n\n"
            "RobotAssemblyDrive(surfaceDetail socketRoll jawTravel) ->\n",
        ),
        (
            "      GuideArmCommand(surfaceDetail)\n",
            "      GuideArmChecked(surfaceDetail 0 0 0 jawTravel)\n",
        ),
        (rotary_mount, mounts + rotary_mount),
        (guide_mount, mounts.replace('origin(0 0 0)', 'origin(0 -0.08 0)') + guide_mount),
        (
            "id(D1BayBattery) class(BatteryPack) taxonomy(Electrical EnergyStorage)",
            "id(D1BayBattery) class(PowerSource) taxonomy(Power Source Battery UnspecifiedChemistry)",
        ),
        (
            "id(VehiclePowerBus) class(PowerDistribution) taxonomy(Electrical DesignIntent)",
            "id(VehiclePowerBus) class(PowerDistribution) taxonomy(Power Transfer DistributionBus)",
        ),
        (
            bus_input,
            "      Interface(id(SocketDriveOut) type(ElectricalPort) origin(-0.06 1.92 -0.56) "
            "normal(0 0 1) tangent(1 0 0) region(Point))\n"
            "      Interface(id(GuideDriveOut) type(ElectricalPort) origin(0.06 1.92 -0.56) "
            "normal(0 0 1) tangent(1 0 0) region(Point))\n" + bus_input,
        ),
        (
            negative_contact,
            "      Interface(id(MotionGuide) type(Slide) origin(-0.109 -0.08 0.145) "
            "normal(1 0 0) tangent(0 1 0) region(Point))\n" + negative_contact,
        ),
        (
            positive_contact,
            "      Interface(id(MotionGuide) type(Slide) origin(0.109 -0.08 0.145) "
            "normal(-1 0 0) tangent(0 1 0) region(Point))\n" + positive_contact,
        ),
    )
    for current, replacement in sites:
        grammar = replace_rule_site(grammar, current, replacement)
    if annotate:
        grammar = annotate_legacy_materials(grammar)
    geometry = (DIRECTORY / "mechanism_geometry.p3d").read_text(encoding="utf-8")
    return "\n\n".join((grammar.rstrip(), profiles.strip(), geometry.strip())) + "\n"


def main():
    parser = ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path, help="Pinned Meridian U11 Equipment Networks grammar")
    parser.add_argument("destination", type=Path, help="Output path for the derived grammar")
    parser.add_argument("--profiles", type=Path, default=DIRECTORY / "motion_profiles.p3d")
    options = parser.parse_args()
    if options.destination.resolve() in {options.source.resolve(), options.profiles.resolve()}:
        parser.error("The destination must be separate from the source and profile fragments.")
    content = integrate_mechanisms(
        options.source.read_bytes(), options.profiles.read_text(encoding="utf-8")
    )
    options.destination.parent.mkdir(parents=True, exist_ok=True)
    options.destination.write_bytes(content.encode("utf-8"))


if __name__ == "__main__":
    main()
