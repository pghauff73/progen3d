#!/usr/bin/env python3
"""Check native grammar warnings for directed electrical power connectivity."""

import argparse
import json
from pathlib import Path
import re
import subprocess
from tempfile import TemporaryDirectory


WARNING_CODE = "MOTOR_WITHOUT_POWER_SOURCE"


def object_rule(name, category, taxonomy, port="ElectricalPort"):
    return f"""[ Object(id({name}) class({category}) taxonomy({taxonomy})) [
        Interface(id(Input) type({port}) origin(0 0 0) normal(0 0 1) tangent(1 0 0) region(Point))
        Interface(id(Output) type({port}) origin(0 0 1) normal(0 0 1) tangent(1 0 0) region(Point))
        !I(Cube material(silverbrushedmetal) 1 1)
    ] ]"""


def grammar(
    transfers, connections, *, motor_class="ElectricMotor",
    motor_taxonomy="Electrical Drive", source_class="PowerSource", connection_type="Powers",
):
    objects = [
        ("Source", source_class, "Power Source Battery"),
        ("FirstMotor", motor_class, motor_taxonomy),
        ("SecondMotor", "ServoMotor", "Electrical Drive"),
        *transfers,
    ]
    rules = [object_rule(*specification) for specification in objects]
    for index, (source, target) in enumerate(connections, 1):
        rules.append(
            f"Connect(id(Wire{index}) source({source} Output) target({target} Input) type({connection_type}))"
        )
    return (
        "Start -> [ Object(id(PowerFixture) class(TestAssembly) taxonomy(Test Fixture)) [\n    "
        + "\n    ".join(rules) + "\n] ]\n"
    )


def run_case(cli, directory, name, source, expected_ids):
    grammar_path = directory / f"{name}.p3d"
    grammar_path.write_text(source, encoding="utf-8")
    job = directory / f"{name}.json"
    job.write_text(json.dumps({
        "schema_version": 1, "grammar": grammar_path.name, "strict": True,
        "seed": 42, "width": 64, "height": 64, "output_dir": name,
    }), encoding="utf-8")
    process = subprocess.run(
        [str(cli), "validate", str(job)],
        check=False, capture_output=True, text=True, encoding="utf-8", timeout=120,
    )
    report = json.loads(process.stdout)
    if process.returncode != 0 or not report["ok"]:
        raise RuntimeError(f"{name} failed strict compilation: {report}")
    motor_warnings = [str(message) for message in report["warnings"] if WARNING_CODE in str(message)]
    if len(motor_warnings) != len(expected_ids):
        raise AssertionError(f"{name}: expected {expected_ids}, got {motor_warnings}")
    for expected_id in expected_ids:
        if sum(expected_id in message for message in motor_warnings) != 1:
            raise AssertionError(f"{name}: missing unique warning for {expected_id}: {motor_warnings}")
    return motor_warnings


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cli", type=Path)
    parser.add_argument("--meridian", type=Path, help="Check the powered Meridian U11 drive grammar too.")
    args = parser.parse_args()
    cli = args.cli.resolve(strict=True)
    closed = ("Switch", "ElectricalSwitch", "Power Transfer Switch Closed")
    opened = ("Switch", "ElectricalSwitch", "Power Switch Open")
    open_transfer = ("Switch", "ElectricalSwitch", "Power Transfer Switch Open")
    regulator = ("Regulator", "PowerRegulator", "Power Transfer Regulator")
    switched_path = [
        ("Source", "Switch"), ("Switch", "Regulator"), ("Regulator", "FirstMotor"),
    ]
    cases = {
        "isolated": (grammar([], []), ["FirstMotor", "SecondMotor"]),
        "direct": (grammar([], [("Source", "FirstMotor")]), ["SecondMotor"]),
        "closed_switch_regulator": (grammar([closed, regulator], switched_path), ["SecondMotor"]),
        "open_switch": (grammar([opened, regulator], switched_path), ["FirstMotor", "SecondMotor"]),
        "explicit_open_transfer": (
            grammar([open_transfer, regulator], switched_path), ["FirstMotor", "SecondMotor"],
        ),
        "unrelated_connection": (
            grammar([], [("Source", "FirstMotor")], connection_type="ConnectedTo"),
            ["FirstMotor", "SecondMotor"],
        ),
        "reversed": (grammar([], [("FirstMotor", "Source")]), ["FirstMotor", "SecondMotor"]),
        "motor_is_not_a_transfer": (
            grammar([], [("Source", "FirstMotor"), ("FirstMotor", "SecondMotor")]),
            ["SecondMotor"],
        ),
        "catalog_is_not_a_source": (
            grammar([], [("Source", "FirstMotor")], source_class="PowerSourceCatalogEntry"),
            ["FirstMotor", "SecondMotor"],
        ),
        "existing_meridian_stator_class": (
            grammar([], [("Source", "SecondMotor")], motor_class="PermanentMagnetMotorStator"),
            ["FirstMotor"],
        ),
        "custom_electric_motor_role": (
            grammar([], [("Source", "SecondMotor")], motor_class="CustomDrive",
                    motor_taxonomy="Power ElectricMotor Custom"),
            ["FirstMotor"],
        ),
    }
    examples = Path(__file__).resolve().parent.parent / "examples" / "PowerAndMotion"
    utility = (examples / "Utility_Bench.p3d").read_text(encoding="utf-8")
    utility_motor, = re.findall(r"Object\(id\((\w+)\) class\(ElectricMotor\)", utility)
    utility_open, switch_count = re.subn(
        r"(?m)^Start -> Bench\(1 ", "Start -> Bench(0 ", utility,
    )
    assert switch_count == 1
    cases.update({
        "utility_bench_closed": (utility, []),
        "utility_bench_open": (utility_open, [utility_motor]),
        "power_source_catalog": (
            (examples / "Power_Source_Catalog.p3d").read_text(encoding="utf-8"), [],
        ),
    })
    if args.meridian:
        powered_u11 = args.meridian.read_text(encoding="utf-8")
        u11_without_socket_feed, feed_count = re.subn(
            r"(?m)^ *Connect\(id\(U11SocketDriveFeed\).*\n", "", powered_u11,
        )
        assert feed_count == 1
        cases["meridian_powered"] = powered_u11, []
        cases["meridian_socket_feed_open"] = u11_without_socket_feed, ["U11SocketStator"]
    with TemporaryDirectory(prefix="progen3d-motor-power-") as temporary:
        root = Path(temporary)
        results = {
            name: run_case(cli, root, name, source, expected)
            for name, (source, expected) in cases.items()
        }
    print(json.dumps({"passed": True, "cases": results}, indent=2))


if __name__ == "__main__":
    main()
