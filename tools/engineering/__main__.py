"""Command-line entry point for the engineering study compiler."""
import argparse
import json
from pathlib import Path

from .domains import MODELS, analyse
from .view import render_view, write_result
from .quantities import QuantityRegistry, StudyError
from .simulation import replay, simulate
from .study import load_extensions, parse_study, scene_catalog


def main():
    parser = argparse.ArgumentParser(prog="python3 -m tools.engineering")
    commands = parser.add_subparsers(dest="command", required=True)
    search = commands.add_parser("quantities", help="search source-backed quantity kinds")
    search.add_argument("query", nargs="?", default="")
    search.add_argument("--kinds", help="project quantity definitions (JSON)")
    commands.add_parser("ui", help="launch the desktop study authoring window")
    commands.add_parser("capabilities", help="list enabled study backends")
    domain = commands.add_parser("analyse", help="run a versioned JSON domain model")
    domain.add_argument("model")
    domain.add_argument("--output")
    domain.add_argument("--scene", help="revisioned native scene manifest for a geometry-bound model")
    compare = commands.add_parser("compare-measurements", help="compare recorded samples with a trace")
    compare.add_argument("trace")
    compare.add_argument("measurements")
    compare.add_argument("--source", required=True)
    compare.add_argument("--output")
    fit = commands.add_parser("fit-cohesive", help="fit a monotonic local opening and force CSV")
    fit.add_argument("measurements")
    fit.add_argument("--area", type=float, required=True, help="nominal bonded area in square metres")
    fit.add_argument("--source", required=True)
    fit.add_argument("--output")
    fmi = commands.add_parser("fmi", help="run a trusted local FMI 3 FMU")
    fmi.add_argument("path")
    fmi.add_argument("--mode", choices=("CoSimulation", "ModelExchange"), required=True)
    fmi.add_argument("--stop", type=float, required=True)
    fmi.add_argument("--interval", type=float, required=True)
    fmi.add_argument("--observe", action="append", required=True)
    fmi.add_argument("--require-rollback", action="store_true")
    fmi.add_argument("--output")
    schedule = commands.add_parser("fmi-schedule", help="activate partitions using an explicit JSON schedule")
    schedule.add_argument("path")
    schedule.add_argument("schedule")
    schedule.add_argument("--observe", action="append", required=True)
    schedule.add_argument("--output")
    viewer = commands.add_parser("view", help="build an interactive HTML observation viewer")
    viewer.add_argument("results", nargs="+")
    viewer.add_argument("--output", required=True)
    for name in ("check", "run"):
        command = commands.add_parser(name)
        command.add_argument("study")
        command.add_argument("--scene", help="revisioned scene catalog (JSON)")
        command.add_argument("--kinds", help="project quantity definitions (JSON)")
        if name == "run":
            command.add_argument("--checkpoint", help="verify and replay from a saved trace checkpoint")
            command.add_argument("--save-checkpoint", nargs=2, metavar=("SECONDS", "FILE"))
            command.add_argument("--output", help="write result JSON to a file")
    args = parser.parse_args()
    try:
        if args.command == "ui":
            from .desktop import launch
            launch()
            return
        if args.command == "view":
            render_view([json.loads(Path(path).read_text()) for path in args.results], args.output)
            return
        if args.command == "capabilities":
            print(json.dumps({"study_version": 1, "methods": ["RK4", "RK45", "BDF"],
                              "domain_models": sorted(MODELS)}, indent=2))
            return
        if args.command in ("analyse", "fmi", "fmi-schedule", "compare-measurements", "fit-cohesive"):
            if args.command == "analyse":
                result = analyse(json.loads(Path(args.model).read_text()),
                                 scene_catalog(args.scene) if args.scene else None)
            elif args.command == "fmi":
                from .fmi import run_fmu
                result = run_fmu(args.path, args.stop, args.interval, args.observe,
                                 args.mode, args.require_rollback)
            elif args.command == "fmi-schedule":
                from .fmi import run_scheduled_fmu
                result = run_scheduled_fmu(args.path, json.loads(Path(args.schedule).read_text()), args.observe)
            elif args.command == "compare-measurements":
                from .coupon_measurements import compare_measurements
                result = compare_measurements(json.loads(Path(args.trace).read_text()),
                                              args.measurements, args.source)
            else:
                from .coupon_measurements import fit_cohesive
                result = fit_cohesive(args.measurements, args.area, args.source)
            text = json.dumps(result, indent=2, allow_nan=False) + "\n"
            if args.output:
                write_result(args.output, text)
            else:
                print(text, end="")
            return
        extensions = load_extensions(args.kinds)
        if args.command == "quantities":
            registry = QuantityRegistry(extensions)
            query = args.query.casefold()
            result = [dict(id=k.identifier, label=k.label, lifecycle=k.lifecycle, source=k.source,
                           dimensions=None if k.dimensions is None else list(map(str, k.dimensions)))
                      for k in registry.kinds.values()
                      if query in (k.identifier + " " + k.label + " " + k.source).casefold()]
        else:
            study = parse_study(args.study, scene_catalog(args.scene) if args.scene else None, extensions)
            if args.command == "check":
                result = {"study": study.name, "bindings": study.bindings,
                          "states": list(study.initials), "method": study.method}
            else:
                trace = (replay(study, json.loads(Path(args.checkpoint).read_text()))
                         if args.checkpoint else simulate(study))
                result = trace.results()
                if args.save_checkpoint:
                    time, file = args.save_checkpoint
                    write_result(file, json.dumps(trace.checkpoint(float(time)), indent=2) + "\n")
                if args.output:
                    write_result(args.output, json.dumps(result, indent=2, allow_nan=False) + "\n")
                    return
        print(json.dumps(result, indent=2, allow_nan=False))
    except (OSError, ValueError, ArithmeticError) as exc:
        parser.exit(2, f"{exc}\n")


if __name__ == "__main__":
    main()
