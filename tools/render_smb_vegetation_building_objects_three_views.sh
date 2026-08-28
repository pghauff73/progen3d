#!/usr/bin/env bash
set -euo pipefail

repository_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
suite_directory="$repository_root/examples/SimpleModernBuilding/VegetationBuildingObjects"
suite_manifest="${1:-$suite_directory/suite_manifest.json}"
output_directory="${2:-$suite_directory/evidence/camera_views}"
parallel_jobs="${PROGEN3D_VIEW_JOBS:-5}"
comparison_field_of_view_degrees="${PROGEN3D_COMPARISON_FOV_DEGREES:-5}"

for command_name in python3 magick xvfb-run; do
	command -v "$command_name" >/dev/null 2>&1 || {
		echo "Required command is unavailable: $command_name" >&2
		exit 1
	}
done

python3 "$suite_directory/generate_vegetation_building_objects.py" --check
make -C "$repository_root" -j2 progen3d-editor-gui
rm -rf "$output_directory"
mkdir -p "$output_directory"

task_path="$output_directory/render_tasks.tsv"
python3 - "$suite_manifest" "$task_path" <<'PY'
import json
import pathlib
import re
import sys

manifest_path = pathlib.Path(sys.argv[1])
task_path = pathlib.Path(sys.argv[2])
manifest = json.loads(manifest_path.read_text())
if manifest["object_count"] != 50:
    raise SystemExit("Vegetation suite must contain exactly fifty objects")
if manifest["comparison_views"] != ["front", "right", "top"]:
    raise SystemExit("Vegetation suite must use front, right, top")

tasks = []
for record in manifest["objects"]:
    grammar_path = pathlib.Path(record["grammar"])
    object_ids = re.findall(
        r"\bObject\s*\(\s*id\(([^)]+)\)",
        grammar_path.read_text(),
        re.DOTALL,
    )
    if object_ids != [record["object_id"]]:
        raise SystemExit(
            f"Vegetation grammar object identity mismatch: {grammar_path} {object_ids}"
        )
    for view in manifest["comparison_views"]:
        tasks.append((record["object_id"], str(grammar_path), view))

expected_task_count = manifest["object_count"] * len(manifest["comparison_views"])
if len(tasks) != expected_task_count:
    raise SystemExit(
        f"Expected {expected_task_count} vegetation render tasks, found {len(tasks)}"
    )
task_path.write_text("".join("\t".join(task) + "\n" for task in tasks))
print(
    "Vegetation render contract: "
    f"objects={manifest['object_count']} views={len(manifest['comparison_views'])} "
    f"captures={expected_task_count}"
)
PY

export repository_root output_directory comparison_field_of_view_degrees
cat "$task_path" | xargs -P "$parallel_jobs" -d '\n' -I '{}' bash -c '
	set -euo pipefail
	IFS=$'"'"'\t'"'"' read -r object_id grammar_path view_name <<< "$1"
	object_directory="$output_directory/$object_id"
	worker_home="$output_directory/worker_homes/${object_id}_${view_name}"
	mkdir -p "$object_directory" "$worker_home"
	capture_path="$object_directory/$view_name.ppm"
	render_path="$object_directory/$view_name.png"
	log_path="$object_directory/$view_name.log"
	capture_succeeded=false
	for attempt in 1 2 3; do
		rm -f "$capture_path" "$render_path"
		if HOME="$worker_home" xvfb-run -a "$repository_root/progen3d-editor-gui" \
				--smoke-test \
				--visual-test \
				--preview-object "$object_id" \
				--preview-view "$view_name" \
				--preview-field-of-view "$comparison_field_of_view_degrees" \
				--preview-fit-extents \
				--preview-hide-grid \
				--capture-preview "$capture_path" \
				--open "$grammar_path" \
				>"$log_path" 2>&1 && \
		   test -s "$capture_path" && \
		   grep -Fq "PROGEN3D_GUI_OBJECT_VIEW_PASSED object=$object_id" "$log_path" && \
		   grep -Fq "PROGEN3D_GUI_VIEW_PASSED name=$view_name" "$log_path" && \
		   grep -Fq "PROGEN3D_GUI_PREVIEW_FOV_PASSED degrees=$comparison_field_of_view_degrees" "$log_path" && \
		   grep -Fq "PROGEN3D_GUI_SMOKE_TEST_PASSED" "$log_path"; then
			magick "$capture_path" -strip "$render_path"
			rm -f "$capture_path" "$log_path" "$log_path".attempt-*
			capture_succeeded=true
			break
		fi
		mv "$log_path" "$log_path.attempt-$attempt" 2>/dev/null || true
		sleep "$attempt"
	done
	if ! "$capture_succeeded"; then
		cat "$log_path".attempt-* >&2
		exit 1
	fi
' _ '{}'

python3 - "$suite_manifest" "$output_directory" "$comparison_field_of_view_degrees" <<'PY'
import hashlib
import json
import pathlib
import subprocess
import sys

manifest_path = pathlib.Path(sys.argv[1])
output_directory = pathlib.Path(sys.argv[2])
field_of_view_degrees = float(sys.argv[3])
manifest = json.loads(manifest_path.read_text())

def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()

object_records = []
for record in manifest["objects"]:
    object_directory = output_directory / record["object_id"]
    views = []
    view_paths = []
    for view_name in manifest["comparison_views"]:
        view_path = object_directory / f"{view_name}.png"
        if not view_path.is_file():
            raise SystemExit(f"Missing vegetation camera view: {view_path}")
        view_paths.append(view_path)
        views.append({
            "view": view_name,
            "image": str(view_path),
            "sha256": sha256(view_path),
        })
    camera_card_path = object_directory / "camera_three_views.png"
    subprocess.run(
        ["magick", *map(str, view_paths), "+append", str(camera_card_path)],
        check=True,
    )
    object_records.append({
        "object_id": record["object_id"],
        "object_class": record["object_class"],
        "title": record["title"],
        "category": record["category"],
        "grammar": record["grammar"],
        "grammar_sha256": sha256(pathlib.Path(record["grammar"])),
        "views": views,
        "camera_card": str(camera_card_path),
        "camera_card_sha256": sha256(camera_card_path),
    })

contact_sheet_directory = output_directory / "contact_sheets"
contact_sheet_directory.mkdir(exist_ok=True)
contact_sheets = {}
for view_name in manifest["comparison_views"]:
    sources = [
        output_directory / record["object_id"] / f"{view_name}.png"
        for record in manifest["objects"]
    ]
    contact_sheet_path = contact_sheet_directory / f"vegetation_{view_name}.png"
    subprocess.run([
        "magick", "montage", *map(str, sources),
        "-thumbnail", "288x266", "-tile", "5x10", "-geometry", "+4+4",
        str(contact_sheet_path),
    ], check=True)
    contact_sheets[view_name] = {
        "image": str(contact_sheet_path),
        "sha256": sha256(contact_sheet_path),
    }

validation = {
    "schema": "ProGen3D-SMB-VegetationThreeViewRender-v1",
    "suite_manifest": str(manifest_path),
    "suite_manifest_sha256": sha256(manifest_path),
    "comparison_views": manifest["comparison_views"],
    "projection": {
        "mode": "narrow-perspective",
        "vertical_field_of_view_degrees": field_of_view_degrees,
        "purpose": "Flatten perspective for elevation-like vegetation object comparisons",
    },
    "object_count": len(object_records),
    "capture_count": sum(len(record["views"]) for record in object_records),
    "contact_sheets": contact_sheets,
    "objects": object_records,
    "passed": (
        len(object_records) == manifest["object_count"]
        and sum(len(record["views"]) for record in object_records)
        == manifest["object_count"] * len(manifest["comparison_views"])
    ),
}
(output_directory / "validation.json").write_text(
    json.dumps(validation, indent=2) + "\n"
)
print(
    "Vegetation three-view rendering passed: "
    f"objects={len(object_records)} captures={validation['capture_count']}"
)
PY

rm -rf "$output_directory/worker_homes"
echo "SMB vegetation building-object rendering complete."
