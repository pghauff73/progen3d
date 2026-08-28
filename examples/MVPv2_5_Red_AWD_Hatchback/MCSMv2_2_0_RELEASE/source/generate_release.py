#!/usr/bin/env python3
"""Low-memory MCSMv2.2 release orchestrator."""
from __future__ import annotations
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

VARIANTS = ("reference", "track", "aero", "crossover")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--resolution", choices=("low", "medium", "high"), default="medium")
    parser.add_argument("--no-preview", action="store_true")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    source_dir = Path(__file__).resolve().parent
    main_source = source_dir / "modern_car_mcsmv2.py"
    output = args.output.resolve()
    workers = output / ".variant_workers"
    if workers.exists():
        shutil.rmtree(workers)
    workers.mkdir(parents=True)
    env = {**os.environ, "PYTHONPATH": str(source_dir)}

    for index, key in enumerate(VARIANTS, start=1):
        print(f"[MCSMv2.2] isolated worker {index}/4: {key}", flush=True)
        worker = workers / key
        command = [
            sys.executable, str(main_source),
            "--output", str(worker),
            "--resolution", args.resolution,
            "--variant", key,
            "--worker",
        ]
        worker_env = dict(env)
        # One reference preview set is sufficient for the release dashboard;
        # mesh-only workers skip VTK entirely to avoid retained graphics state.
        if args.no_preview or key != "reference":
            command.append("--no-preview")
            worker_env["MCSM_DISABLE_VTK"] = "1"
        log_path = workers / f"{key}.log"
        with log_path.open("w", encoding="utf-8") as log:
            completed = subprocess.run(command, stdout=log, stderr=subprocess.STDOUT, env=worker_env)
        if completed.returncode != 0:
            tail = log_path.read_text(encoding="utf-8", errors="replace")[-4000:]
            raise SystemExit(f"worker {key} failed ({completed.returncode})\n{tail}")

    print("[MCSMv2.2] merging validated workers", flush=True)
    command = [
        sys.executable, str(main_source),
        "--output", str(output),
        "--resolution", args.resolution,
        "--merge-workers", str(workers),
        "--no-preview",
    ]
    merge_env = {**env, "MCSM_DISABLE_VTK": "1"}
    completed = subprocess.run(command, env=merge_env)
    raise SystemExit(completed.returncode)


if __name__ == "__main__":
    main()
