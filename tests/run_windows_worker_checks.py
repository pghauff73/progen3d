#!/usr/bin/env python3
"""Interrupt and resume real Windows GPU renders through the durable queue."""
import argparse
from contextlib import contextmanager
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time


REPOSITORY = Path(__file__).resolve().parents[1]


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    return path


def invoke(cli, *args, expected=0):
    process = subprocess.run(
        [str(cli), *map(str, args)], capture_output=True, text=True, encoding="utf-8", timeout=300,
    )
    assert process.stdout, (args, process.returncode, process.stderr)
    result = json.loads(process.stdout)
    assert process.returncode == expected == result["exit_code"], (args, result, process.stderr)
    return result


@contextmanager
def running_worker(cli, queue, once=False):
    arguments = [str(cli), "worker", "--queue", str(queue)]
    if once:
        arguments.append("--once")
    with subprocess.Popen(arguments, stdout=subprocess.PIPE, stderr=subprocess.PIPE, encoding="utf-8") as worker:
        try:
            yield worker
        finally:
            if worker.poll() is None:
                worker.kill()


def wait_for_state(cli, queue, predicate, *ids):
    deadline = time.monotonic() + 180
    while time.monotonic() < deadline:
        record = invoke(cli, "status", *ids, "--queue", queue)
        if predicate(record):
            return record
        if ids:
            assert record["job"]["state"] != "failed", record
        time.sleep(0.05)
    raise AssertionError(f"Timed out waiting for render queue status: {record}")


def inventory(output):
    frames = {}
    for directory in sorted(output.glob("frame-*")):
        manifest = read_json(directory / "frame.json")
        checksums = read_json(directory / "checksums.json")
        assert checksums["schema_version"] == 1
        assert set(checksums["sha256"]) == {"frame.json", *manifest["files"].values()}
        evidence = {}
        for name, digest in checksums["sha256"].items():
            path = directory / name
            assert hashlib.sha256(path.read_bytes()).hexdigest() == digest, path
            evidence[name] = (digest, path.stat().st_mtime_ns)
        evidence["checksums.json"] = (
            hashlib.sha256((directory / "checksums.json").read_bytes()).hexdigest(),
            (directory / "checksums.json").stat().st_mtime_ns,
        )
        frames[manifest["frame"]] = evidence
    return frames


def assert_preserved(before, after):
    for frame, evidence in before.items():
        assert after[frame] == evidence, f"Verified frame {frame} was rewritten."


def prepare_job(root):
    example = REPOSITORY / "examples" / "drop_and_settle.json"
    job = read_json(example)
    grammar = root / "submitted scene.p3d"
    original = (example.parent / job["grammar"]).read_bytes()
    cue = b'audioEvent(impact collision(DROPPING_BALL DROP_FLOOR 0.1) speak("The ball hit the floor."))\n'
    grammar.write_bytes(cue + original)
    job.update(grammar=grammar.name, width=352, height=224, layers=True, output_dir="direct")
    job["timeline"].update({
        "fps": 25, "frame_count": 58, "start_frame": 3,
        "shutter": {"frame_fraction": 0.75, "samples": 2},
    })
    path = write_json(root / "submitted job.json", job)
    return path, job, grammar


def check_resumption(cli, root):
    job_path, job, grammar = prepare_job(root)
    source = grammar.read_bytes()
    queue = root / "queue files"

    with running_worker(cli, queue) as worker:
        wait_for_state(cli, queue, lambda status: status["worker_running"])
        other = invoke(cli, "worker", "--queue", root / "second queue", "--once", expected=2)
        assert "already running" in other["error"], other
        submitted = invoke(cli, "enqueue", job_path, "--queue", queue)["job"]
        job_id = submitted["id"]
        output = Path(submitted["output_directory"])
        assert submitted["state"] == "queued" and submitted["total_frames"] == 55
        # The worker must keep using the submitted grammar after this edit.
        grammar.write_text("Start -> [ InvalidOperation ]\n", encoding="utf-8")
        wait_for_state(
            cli, queue, lambda status: status["job"]["state"] == "running" and
            status["job"]["completed_frames"] >= 6, job_id,
        )
        worker.kill()
        worker.wait(timeout=15)

    interrupted = invoke(cli, "status", job_id, "--queue", queue)
    assert not interrupted["worker_running"] and interrupted["job"]["state"] == "interrupted", interrupted
    assert not (output / "sequence.json").exists()
    after_interrupt = inventory(output)
    assert 6 <= len(after_interrupt) < submitted["total_frames"], after_interrupt.keys()

    damaged_frame = sorted(after_interrupt)[1]
    damaged = output / f"frame-{damaged_frame:06d}" / "linear-depth.pfm"
    data = damaged.read_bytes()
    damaged.write_bytes(data[:-1] + bytes([data[-1] ^ 0xFF]))
    verified = {frame: evidence for frame, evidence in after_interrupt.items() if frame != damaged_frame}

    assert invoke(cli, "resume", job_id, "--queue", queue)["job"]["state"] == "queued"
    with running_worker(cli, queue, once=True) as worker:
        wait_for_state(
            cli, queue, lambda status: status["job"]["state"] == "running" and
            status["job"]["completed_frames"] >= len(after_interrupt) + 4, job_id,
        )
        assert invoke(cli, "cancel", job_id, "--queue", queue)["job"]["state"] == "cancelling"
        out, err = worker.communicate(timeout=180)
        assert worker.returncode == 0 and json.loads(out)["job"]["state"] == "cancelled", (out, err)
    cancelled = invoke(cli, "status", job_id, "--queue", queue)["job"]
    assert cancelled["result"]["exit_code"] == 6 and cancelled["attempt"] == 2
    assert not (output / "sequence.json").exists()
    after_cancel = inventory(output)
    assert_preserved(verified, after_cancel)
    assert after_cancel[damaged_frame]["linear-depth.pfm"][0] == after_interrupt[damaged_frame]["linear-depth.pfm"][0]
    assert list(output.glob(f".rejected-frame-{damaged_frame:06d}-*"))

    assert invoke(cli, "resume", job_id, "--queue", queue)["job"]["state"] == "queued"
    finished_worker = invoke(cli, "worker", "--queue", queue, "--once")
    assert finished_worker["opengl"]["version"].startswith("4.6"), finished_worker
    assert finished_worker["processed_jobs"] == 1
    completed = invoke(cli, "status", job_id, "--queue", queue)["job"]
    assert completed["state"] == "succeeded" and completed["attempt"] == 3, completed
    assert completed["completed_frames"] == submitted["total_frames"]
    assert completed["reused_frames"] == len(after_cancel)
    assert completed["rendered_frames"] == submitted["total_frames"] - len(after_cancel)
    final_frames = inventory(output)
    assert_preserved(after_cancel, final_frames)

    grammar.write_bytes(source)
    reference = invoke(cli, "render", job_path)
    reference_manifest = Path(reference["sequence_manifest"])
    recovered_manifest = Path(completed["result"]["sequence_manifest"])
    assert recovered_manifest == output / "sequence.json"
    assert recovered_manifest.read_bytes() == reference_manifest.read_bytes()
    assert (output / "contacts.jsonl").read_bytes() == (reference_manifest.parent / "contacts.jsonl").read_bytes()
    assert read_json(recovered_manifest)["audio_cues"], "The test should exercise a recorded collision cue."
    reference_frames = inventory(reference_manifest.parent)
    assert reference_frames.keys() == final_frames.keys()
    for frame, evidence in final_frames.items():
        assert {name: data[0] for name, data in evidence.items()} == {
            name: data[0] for name, data in reference_frames[frame].items()
        }, frame

    assert not list(output.glob(".*.tmp")), "Worker restart must discard incomplete staging."
    return job, queue, {
        "job": completed,
        "interrupted_attempt": interrupted["job"],
        "cancelled_attempt": cancelled,
        "verified_after_interrupt": len(verified),
        "regenerated_corrupt_frame": damaged_frame,
        "preserved_on_final_resume": len(after_cancel),
        "opengl": finished_worker["opengl"],
    }


def check_still_and_queued_cancel(cli, root, job, queue):
    job.pop("timeline")
    job.update(width=160, height=112, layers=False)
    manifest = write_json(root / "still.json", job)
    queued = invoke(cli, "enqueue", manifest, "--queue", queue)["job"]
    job_id = queued["id"]
    cancelled = invoke(cli, "cancel", job_id, "--queue", queue)["job"]
    assert cancelled["state"] == "cancelled" and cancelled["attempt"] == 0
    assert invoke(cli, "worker", "--once", "--queue", queue)["processed_jobs"] == 0
    assert invoke(cli, "resume", job_id, "--queue", queue)["job"]["state"] == "queued"
    worker = invoke(cli, "worker", "--once", "--queue", queue)
    succeeded = worker["job"]
    assert succeeded["state"] == "succeeded" and succeeded["rendered_frames"] == 1
    assert inventory(Path(succeeded["output_directory"])).keys() == {0}

    output = root / "checkpointed still"
    first = invoke(cli, "render", manifest, "--out", output)
    saved = inventory(output)
    assert first["rendered_frames"] == 1
    reused = invoke(cli, "render", manifest, "--out", output, "--resume")
    assert reused["reused_frames"] == 1 and reused["rendered_frames"] == 0
    assert_preserved(saved, inventory(output))
    assert "--resume" in invoke(cli, "render", manifest, "--out", output, expected=5)["error"]
    job["seed"] += 1
    write_json(manifest, job)
    incompatible = invoke(cli, "render", manifest, "--out", output, "--resume", expected=5)
    assert "different render job or grammar" in incompatible["error"]
    assert_preserved(saved, inventory(output))
    return succeeded


def check_persistent_worker(cli, root, queue):
    still = root / "still.json"
    invalid_grammar = root / "invalid.p3d"
    invalid_grammar.write_text("Start -> [ T(0 0 0)\n", encoding="utf-8")
    invalid_job = read_json(still)
    invalid_job["grammar"] = invalid_grammar.name
    invalid = write_json(root / "invalid.json", invalid_job)
    ids = [invoke(cli, "enqueue", job, "--queue", queue)["job"]["id"] for job in (still, invalid, still)]

    with running_worker(cli, queue):
        wait_for_state(cli, queue, lambda status: status["job"]["state"] == "succeeded", ids[-1])
        statuses = invoke(cli, "status", "--queue", queue)["jobs"]
    first, failed, last = [job for job in statuses if job["id"] in ids]
    assert [first["id"], failed["id"], last["id"]] == ids
    assert first["state"] == last["state"] == "succeeded" and failed["state"] == "failed"
    assert failed["result"]["exit_code"] == 3 and failed["result"]["diagnostics"]
    assert first["result"]["pixel_hash_fnv1a64"] == last["result"]["pixel_hash_fnv1a64"]
    assert first["updated_unix_ms"] <= failed["updated_unix_ms"] <= last["updated_unix_ms"]
    return ids


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cli", type=Path)
    parser.add_argument("--output-root", type=Path, default=Path(tempfile.gettempdir()))
    args = parser.parse_args()

    cli = args.cli.resolve(strict=True)
    args.output_root.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="progen3d-worker-", dir=args.output_root.resolve()))
    job, queue, report = check_resumption(cli, root)
    report["still_job"] = check_still_and_queued_cancel(cli, root, job, queue)
    report["persistent_worker_jobs"] = check_persistent_worker(cli, root, queue)
    evidence = write_json(root / "report.json", report)
    print(f"Windows queue, cancellation and frame recovery checks passed. Report: {evidence}")


if __name__ == "__main__":
    main()
