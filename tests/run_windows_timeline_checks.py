#!/usr/bin/env python3
"""Check fixed-step simulation and deterministic sequence rendering."""
import argparse
import copy
import json
import math
from pathlib import Path
import subprocess
import tempfile


def read_json(path):
    return Path(path).read_text(encoding="utf-8")


def invoke(cli, root, name, job, command="render", expected=0):
    manifest_path = root / (name + ".json")
    manifest_path.write_text(json.dumps(job), encoding="utf-8")
    result = subprocess.run(
        [str(cli), command, str(manifest_path)],
        cwd=root, capture_output=True, text=True, encoding="utf-8", timeout=240,
    )
    report = json.loads(result.stdout)
    assert result.returncode == expected == report["exit_code"], (result.stdout, result.stderr)
    return report


def sequence(report):
    path = Path(report["sequence_manifest"])
    manifest = json.loads(read_json(path))
    frames = {entry["frame"]: path.parent / entry["manifest"] for entry in manifest["frames"]}
    assert len(frames) == len(manifest["frames"]) == report["completed_frames"]
    assert not list(path.parent.glob(".*.tmp"))
    contacts = (path.parent / manifest["contacts"]).read_bytes()
    events = [json.loads(line) for line in contacts.splitlines()]
    assert len(events) == manifest["contact_count"]
    return frames, contacts, events


def frame(path):
    return json.loads(read_json(path))


def body(frame_report, name):
    return next(body for body in frame_report["transforms"] if body["object_id"] == name)


def assert_same_frames(expected, actual):
    for number, actual_manifest in actual.items():
        expected_manifest = expected[number]
        assert expected_manifest.read_bytes() == actual_manifest.read_bytes(), f"Frame {number} metadata differs"
        preview = frame(actual_manifest)["files"]["preview"]
        assert (expected_manifest.parent / preview).read_bytes() == (actual_manifest.parent / preview).read_bytes(), (
            f"Frame {number} pixels differ"
        )


def rgb_pixels(path):
    magic, dimensions, maximum, pixels = Path(path).read_bytes().split(b"\n", 3)
    width, height = map(int, dimensions.split())
    assert magic == b"P6" and maximum == b"255" and len(pixels) == width * height * 3
    return pixels


def check_shutter(cli, root, source, full_frames):
    blurred_job = copy.deepcopy(source)
    blurred_job["layers"] = True
    blurred_job["timeline"].update({
        "frame_count": 14, "start_frame": 8,
        "shutter": {"frame_fraction": 0.8, "samples": 2},
    })
    blurred_report = invoke(cli, root, "blurred", blurred_job)
    blurred_frames, blur_contacts, _ = sequence(blurred_report)
    exposure = frame(blurred_frames[11])
    assert exposure["transforms"] == frame(full_frames[11])["transforms"]
    exposure_times = exposure["shutter"]["sample_times_seconds"]
    assert len(exposure_times) == 2
    assert math.isclose(exposure_times[0], 56 / 120) and math.isclose(exposure_times[1], 58 / 120)

    blurred_folder = blurred_frames[11].parent
    assert (blurred_folder / exposure["files"]["linear_depth"]).is_file()
    blurred = rgb_pixels(blurred_folder / exposure["files"]["preview"])
    sharp = rgb_pixels(blurred_folder / exposure["files"]["color"])
    assert sharp == rgb_pixels(full_frames[11].parent / "preview.ppm")
    changed_channels = sum(a != b for a, b in zip(sharp, blurred))
    assert changed_channels > 100

    # Render the two exposure samples independently on the physics clock.
    subframes_job = copy.deepcopy(source)
    subframes_job["timeline"].update({"fps": 120, "frame_count": 59, "start_frame": 56})
    subframes_report = invoke(cli, root, "shutter-subframes", subframes_job)
    subframes, _, _ = sequence(subframes_report)
    left = rgb_pixels(subframes[56].parent / "preview.ppm")
    right = rgb_pixels(subframes[58].parent / "preview.ppm")
    transfer = [(value / 255) ** 2.2 for value in range(256)]
    expected = [int(255 * ((transfer[a] + transfer[b]) / 2) ** (1 / 2.2) + 0.5) for a, b in zip(left, right)]
    assert max(abs(result - reference) for result, reference in zip(blurred, expected)) <= 1

    blurred_job["timeline"]["start_frame"] = 11
    blur_resumed = invoke(cli, root, "blurred-resumed", blurred_job)
    resumed_frames, resumed_contacts, _ = sequence(blur_resumed)
    assert list(resumed_frames) == [11, 12, 13]
    assert resumed_contacts == blur_contacts
    assert_same_frames(blurred_frames, resumed_frames)
    return {
        "blurred": blurred_report, "blurred_resume": blur_resumed, "subframes": subframes_report,
        "test_frame": str(blurred_frames[11]), "changed_channels": changed_channels,
    }


def run_checks(cli, job_path, output_root):
    source = json.loads(read_json(job_path))
    source["grammar"] = str((job_path.parent / source["grammar"]).resolve())
    source["output_dir"] = "renders"
    source["timeline"].pop("shutter", None)
    output_root.mkdir(parents=True, exist_ok=True)
    root = Path(tempfile.mkdtemp(prefix="progen3d-timeline-", dir=output_root))

    validated = invoke(cli, root, "validate", source, command="validate")
    assert validated["timeline"]["physics_ticks_per_second"] == 120 and "opengl" not in validated
    continuous = invoke(cli, root, "continuous", source)
    repeated = invoke(cli, root, "repeated", source)
    full_frames, contacts, events = sequence(continuous)
    repeat_frames, repeat_contacts, _ = sequence(repeated)
    assert list(full_frames) == list(range(source["timeline"]["frame_count"]))
    assert list(repeat_frames) == list(full_frames)
    assert_same_frames(full_frames, repeat_frames)
    assert contacts == repeat_contacts

    replay_job = copy.deepcopy(source)
    replay_job["timeline"]["start_frame"] = 16
    resumed = invoke(cli, root, "resumed", replay_job)
    resumed_frames, resumed_contacts, _ = sequence(resumed)
    assert list(resumed_frames) == list(full_frames)[16:]
    assert_same_frames(full_frames, resumed_frames)
    assert resumed_contacts == contacts

    first = frame(full_frames[0])
    halfway = frame(full_frames[12])
    settled = frame(full_frames[max(full_frames)])
    assert first["simulation_tick"] == 0 and halfway["simulation_tick"] == 60
    assert all(stamp["simulation_fraction"] == 0 for stamp in (first, halfway, settled))
    assert len(events) > 1 and 0 < events[0]["tick"] < 120 and events[0]["peak_impulse"] > 0
    assert all({event["first"]["object_id"], event["second"]["object_id"]} ==
               {"DROPPING_BALL", "DROP_FLOOR"} for event in events)
    assert [event["tick"] for event in events] == sorted(event["tick"] for event in events)
    assert math.isclose(body(first, "DROPPING_BALL")["position"][1], 1.60, abs_tol=1e-5)
    ball = body(settled, "DROPPING_BALL")
    assert ball["sleeping"] and not ball["removed"] and ball["velocity"] == [0, 0, 0]
    assert 0.33 < ball["position"][1] < 0.39
    assert math.isclose(body(settled, "CLOCK_MARKER")["position"][0],
                        1.20 + 0.40 * settled["time_seconds"], abs_tol=1e-5)
    assert settled["camera"]["position"] != first["camera"]["position"]
    assert settled["camera"]["time_seconds"] == settled["time_seconds"]
    assert math.isclose(settled["camera"]["focus"][1], ball["position"][1], abs_tol=1e-5)

    fractional_job = copy.deepcopy(source)
    fractional_job["layers"] = True
    fractional_job["timeline"].update({"fps": 25, "frame_count": 15, "start_frame": 10})
    fractional_result = invoke(cli, root, "fractional", fractional_job)
    fractional_frames, fraction_contacts, _ = sequence(fractional_result)
    selected = frame(fractional_frames[11])
    assert selected["simulation_tick"] == 52
    assert math.isclose(selected["simulation_fraction"], 0.8)
    assert (fractional_frames[11].parent / selected["files"]["linear_depth"]).is_file()
    fractional_job["timeline"]["start_frame"] = 11
    fractional_resume = invoke(cli, root, "fractional-resume", fractional_job)
    resumed_fractional, resumed_fraction_contacts, _ = sequence(fractional_resume)
    assert resumed_fraction_contacts == fraction_contacts
    assert_same_frames(fractional_frames, resumed_fractional)

    shutter = check_shutter(cli, root, source, full_frames)

    invalid = copy.deepcopy(source)
    invalid["timeline"]["start_frame"] = invalid["timeline"]["frame_count"]
    rejected = invoke(cli, root, "out-of-range", invalid, command="validate", expected=2)
    assert "timeline.start_frame" in rejected["error"]

    summary = {
        "continuous": continuous, "repeated": repeated, "resumed": resumed,
        "fractional": fractional_result, "fractional_resume": fractional_resume,
        "first_contact_tick": events[0]["tick"], "contact_count": len(events),
        "settled_ball_y": ball["position"][1],
        "shutter": shutter,
        "verified_continuous_frames": len(full_frames),
        "verified_resumed_frames": len(resumed_frames),
    }
    report = root / "report.json"
    report.write_text(json.dumps(summary, indent=2) + "\n", encoding="utf-8")
    print(f"Timeline and shutter checks passed. Report: {report}")


def main():
    default_job = Path(__file__).resolve().parent.parent / "examples" / "drop_and_settle.json"
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cli", type=Path)
    parser.add_argument("--job", type=Path, default=default_job)
    parser.add_argument("--output-root", type=Path, default=Path(tempfile.gettempdir()))
    args = parser.parse_args()
    run_checks(args.cli.resolve(strict=True), args.job.resolve(strict=True), args.output_root.resolve())


if __name__ == "__main__":
    main()
