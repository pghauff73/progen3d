#!/usr/bin/env python3
"""Verify the courtyard film and audio cue timing with the staged Windows CLI."""
import argparse
from array import array
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import wave


EXAMPLES = Path(__file__).resolve().parents[1] / "examples"


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n", encoding="utf-8")
    return path


def invoke(cli, command, manifest, *arguments, expected=0):
    result = subprocess.run(
        [str(cli), command, str(manifest), *map(str, arguments)],
        capture_output=True, text=True, encoding="utf-8", timeout=300,
    )
    assert result.stdout, result.stderr
    report = json.loads(result.stdout)
    assert result.returncode == expected == report["exit_code"], report
    return report


def render(cli, directory, name, job):
    path = write_json(directory / (name + ".json"), job)
    return Path(invoke(cli, "render", path)["sequence_manifest"])


def compose(cli, directory, name, edit):
    manifest = write_json(directory / (name + ".edit.json"), edit)
    return invoke(cli, "compose", manifest, "--out", directory / (name + ".mp4"))["composition"]


def check_movie(ffprobe, path, frame_count, fps):
    probe = json.loads(subprocess.check_output([
        str(ffprobe), "-v", "error", "-count_frames", "-show_streams", "-of", "json", path,
    ]))
    video = next(stream for stream in probe["streams"] if stream["codec_type"] == "video")
    audio = next(stream for stream in probe["streams"] if stream["codec_type"] == "audio")
    assert video["codec_name"] == "h264" and audio["codec_name"] == "aac"
    assert video["avg_frame_rate"] == f"{fps}/1" and int(video["nb_read_frames"]) == frame_count
    for stream in (video, audio):
        assert math.isclose(float(stream["duration"]), frame_count / fps, abs_tol=0.001), stream


def read_stereo_wav(path, frame_count):
    with wave.open(path) as source:
        assert (source.getnchannels(), source.getsampwidth(), source.getframerate()) == (2, 2, 48000)
        assert source.getnframes() == frame_count
        return array("h", source.readframes(frame_count))


def caption_band(ffmpeg, path):
    return subprocess.check_output([
        str(ffmpeg), "-v", "error", "-i", path, "-vf", r"select=eq(n\,78),crop=iw:72:0:ih-72",
        "-frames:v", "1", "-pix_fmt", "rgb24", "-f", "rawvideo", "-",
    ])


def check_courtyard(cli, ffmpeg, ffprobe, directory, sequence_path):
    if sequence_path is None:
        job = read_json(EXAMPLES / "courtyard_video.json")
        job.update(grammar=str(EXAMPLES / job["grammar"]), output_dir="renders")
        sequence_path = render(cli, directory, "courtyard", job)

    sequence = read_json(sequence_path)
    frame_paths = [sequence_path.parent / entry["manifest"] for entry in sequence["frames"]]
    assert len(frame_paths) == 144
    first_camera = read_json(frame_paths[0])["camera"]
    focus_frame = read_json(frame_paths[72])
    assert focus_frame["frame"] == 72 and focus_frame["time_seconds"] == 3.0
    assert math.dist(focus_frame["camera"]["focus"], [0, 1.69, -2.41]) < 0.001
    assert first_camera["focus_depth"] < focus_frame["camera"]["focus_depth"]
    assert first_camera["position"] != focus_frame["camera"]["position"]
    cues = sequence["audio_cues"]
    assert len(cues) == 1 and cues[0]["id"] == "entrance_focus" and cues[0]["time_seconds"] == 3.0

    saved_files = [sequence_path, *frame_paths]
    saved_files += [path.parent / read_json(path)["files"]["preview"] for path in frame_paths]
    hashes = [hashlib.sha256(path.read_bytes()).digest() for path in saved_files]

    edit = read_json(EXAMPLES / "courtyard_edit.json")
    edit["cuts"][0]["sequence"] = str(sequence_path)
    original = compose(cli, directory, "courtyard", edit)
    check_movie(ffprobe, original["video"], 144, 24)
    assert read_json(Path(original["composition_manifest"])) == original
    assert original["narration"][0]["start_sample"] == 144000
    waveform = read_stereo_wav(original["narration_wav"], 288000)
    onset = next(index for index, sample in enumerate(waveform[::2]) if abs(sample) > 16)
    assert not any(waveform[:288000])
    assert abs(onset / 48000 - focus_frame["time_seconds"]) <= 1 / 24, onset
    encoded_voice = array("h", subprocess.check_output([
        str(ffmpeg), "-v", "error", "-i", original["video"], "-map", "0:a:0", "-f", "s16le", "-",
    ]))
    encoded_onset = next(index for index, sample in enumerate(encoded_voice[::2]) if abs(sample) > 128)
    assert abs(encoded_onset / 48000 - focus_frame["time_seconds"]) <= 1 / 24
    captions = Path(original["captions_srt"]).read_text(encoding="utf-8")
    assert "00:00:03,000 --> 00:00:05,583" in captions and cues[0]["text"] in captions

    revised_caption = "Focus moves from the branch to the courtyard entrance."
    edit["captions"][0]["text"] = revised_caption
    edit["use_audio_events"] = False
    edit["narration"] = [{"start_frame": 0, "wav": "courtyard.wav"}]
    revised = compose(cli, directory, "courtyard-caption-revised", edit)
    check_movie(ffprobe, revised["video"], 144, 24)
    assert read_stereo_wav(revised["narration_wav"], 288000) == waveform
    assert revised_caption in Path(revised["captions_srt"]).read_text(encoding="utf-8")
    old_pixels = caption_band(ffmpeg, original["video"])
    new_pixels = caption_band(ffmpeg, revised["video"])
    assert len(old_pixels) == len(new_pixels) == 640 * 72 * 3
    assert sum(abs(a - b) > 50 for a, b in zip(old_pixels, new_pixels)) > 1000
    assert hashes == [hashlib.sha256(path.read_bytes()).digest() for path in saved_files]
    return sequence_path, original, revised, onset, encoded_onset


def check_cuts(cli, ffprobe, directory, sequence):
    tone_path = directory / "editor's tone.wav"
    with wave.open(str(tone_path), "wb") as tone:
        tone.setparams((1, 2, 48000, 0, "NONE", "not compressed"))
        tone.writeframes(array("h", [1000] * 9600).tobytes())
    edit = {
        "schema_version": 1,
        "cuts": [
            {"sequence": str(sequence), "start_frame": 70, "end_frame": 82},
            {"sequence": str(sequence), "start_frame": 0, "end_frame": 12},
        ],
        "narration": [{"start_frame": 14, "wav": tone_path.name, "gain": 0.5}],
    }
    report = compose(cli, directory, "two cuts", edit)
    check_movie(ffprobe, report["video"], 24, 24)
    assert [cut["output_start_frame"] for cut in report["cuts"]] == [0, 12]
    cue, manual = report["narration"]
    assert cue["id"] == "entrance_focus" and (cue["start_sample"], cue["end_limit_sample"]) == (4000, 24000)
    assert manual["trigger"] == "wav" and manual["start_sample"] == 28000
    samples = read_stereo_wav(report["narration_wav"], 48000)[::2]
    assert not any(samples[24000:28000])
    assert 300 < samples[29000] < 500
    prior = Path(report["video"]).read_bytes()
    manifest = directory / "two cuts.edit.json"
    failed = invoke(cli, "compose", manifest, "--out", report["video"], expected=5)
    assert "already exists" in failed["error"] and Path(report["video"]).read_bytes() == prior
    edit["cuts"][0]["end_frame"] = 145
    invalid = write_json(directory / "unsaved.edit.json", edit)
    failed = invoke(cli, "compose", invalid, "--out", directory / "unsaved.mp4", expected=2)
    assert "unsaved frame 144" in failed["error"] and not (directory / "unsaved.mp4").exists()


def check_timeline_cues(cli, directory):
    job = {
        "schema_version": 1, "grammar": str(EXAMPLES / "audio_events.p3d"), "seed": 42,
        "width": 192, "height": 128, "output_dir": "renders", "layers": False,
        "timeline": {"fps": 24, "frame_count": 96, "physics": True},
        "camera": {
            "position": [{"time": time, "value": value} for time, value in
                         enumerate([[6, 3, 6], [0, 1.3, 2], [5, 3, 5]])],
            "aim": [{"time": 0, "value": [0, 0.5, 0]}],
            "focus": [{"time": 0, "value": [0, 0.5, 0]}],
            "lens": [{"time": 0, "value": {"fov_degrees": 48, "aperture_radius": 0}}],
        },
    }
    sequence_path = render(cli, directory, "audio-events", job)
    sequence = read_json(sequence_path)
    cues = sequence["audio_cues"]
    assert {cue["id"]: cue["trigger"] for cue in cues} == {
        "release": "simulation", "checkpoint": "time", "dropZone": "object_region",
        "impact": "collision", "rest": "simulation", "approach": "camera_region", "departure": "camera_region",
    }
    first_times = {cue["id"]: cue["time_seconds"] for cue in reversed(cues)}
    assert first_times["release"] == 0 and first_times["checkpoint"] == 0.4
    assert first_times["dropZone"] < first_times["impact"] < first_times["rest"]
    assert first_times["approach"] < first_times["departure"]
    contacts = (sequence_path.parent / sequence["contacts"]).read_text(encoding="utf-8")
    impacts = [json.loads(line) for line in contacts.splitlines()]
    qualifying = {impact["time_seconds"] for impact in impacts if impact["peak_impulse"] >= 0.5 and
                  {impact["first"]["object_id"], impact["second"]["object_id"]} == {"Ball", "Floor"}}
    assert all(cue["time_seconds"] in qualifying for cue in cues if cue["id"] == "impact")

    job["timeline"]["start_frame"] = 24
    resumed = read_json(render(cli, directory, "audio-events-resumed", job))
    assert resumed["frames"][0]["frame"] == 24 and resumed["audio_cues"] == cues
    return cues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("cli", type=Path)
    parser.add_argument("--ffmpeg", type=Path)
    parser.add_argument("--courtyard-sequence", type=Path)
    parser.add_argument("--output-root", type=Path)
    args = parser.parse_args()

    cli = args.cli.resolve(strict=True)
    ffmpeg = args.ffmpeg or os.environ.get("PROGEN3D_FFMPEG") or shutil.which("ffmpeg.exe")
    if not ffmpeg:
        parser.error("provide --ffmpeg with a Windows build containing ffmpeg.exe and ffprobe.exe")
    ffmpeg = Path(ffmpeg).resolve(strict=True)
    ffprobe = ffmpeg.with_name("ffprobe.exe").resolve(strict=True)
    os.environ["PROGEN3D_FFMPEG"] = str(ffmpeg)
    if args.output_root:
        args.output_root.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix="progen3d-composition-", dir=args.output_root)).resolve()
    sequence = args.courtyard_sequence.resolve(strict=True) if args.courtyard_sequence else None
    sequence, original, revised, onset, encoded_onset = check_courtyard(cli, ffmpeg, ffprobe, directory, sequence)
    check_cuts(cli, ffprobe, directory, sequence)
    cues = check_timeline_cues(cli, directory)
    report = write_json(directory / "report.json", {
        "original": original, "revised": revised, "timeline_cues": cues,
        "narration_onset_seconds": onset / 48000, "encoded_narration_onset_seconds": encoded_onset / 48000,
    })
    print(f"Composition and audio cue checks passed. Report: {report}")


if __name__ == "__main__":
    main()
