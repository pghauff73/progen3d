# Building ProGen3D

## Windows (MSYS2 UCRT64)

Open the **MSYS2 UCRT64** shell and install the native Windows build dependencies:

```sh
pacman -S --needed \
  mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake \
  mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-pkgconf \
  mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm \
  mingw-w64-ucrt-x86_64-stb mingw-w64-ucrt-x86_64-curl \
  mingw-w64-ucrt-x86_64-openssl mingw-w64-ucrt-x86_64-jsoncpp \
  mingw-w64-ucrt-x86_64-libxml2 mingw-w64-ucrt-x86_64-eigen3 \
  mingw-w64-ucrt-x86_64-ca-certificates
```

From the repository root, configure and build the editor:

```sh
cmake -S . -B build/ucrt64 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/ucrt64 --parallel 1
```

The build stages `progen3d.exe`, its DLLs, a CA bundle, and available editor
resources in `build/ucrt64`. Keep the staged files together when moving the
app. The preview requires a Windows graphics driver with OpenGL 4.6 support.
The editor stores per-user state in `%LOCALAPPDATA%\ProGen3d` by default.

Launch `build/ucrt64/progen3d.exe` from Explorer or run the deterministic
GUI startup check from the UCRT64 shell:

```sh
./build/ucrt64/progen3d.exe --smoke-test
```

### Open Lyra on Windows

Double-click **Open Lyra.cmd** beside `progen3d.exe`, then select **Play**
in Preview to run the four-second greeting. From the editor, use **File > Open**
and select `examples/Characters/Lyra/Lyra_Greeting.p3d` in the app folder.
The grammar includes its meshes and animation and is also staged with the
headless CLI. Set `Speed(0.5)` in the `Start` rule for an eight-second greeting.

Configure with `-DBUILD_TESTING=ON`, build `lyra_greeting_harness`, and run
`ctest --test-dir build/ucrt64 -R bundled_lyra_greeting --output-on-failure`
to check the staged grammar, character poses, and document-local mesh handling.

The [upstream Windows bundle](https://github.com/pghauff73/progen3d/blob/20fc8f0d5a25f2b1d11da7fbf955e58e0a132d83/BUILDING.md#open-lyra-on-windows)
credits the MakeHuman core assets for the body and KayKit Adventurers for
the rig and motion, both under CC0.

### Windows headless rendering

Build the staged CLI and run the graphics check from PowerShell.

```powershell
cmake --build build/ucrt64 --target stage_progen3d_cli --parallel 1
.\build\ucrt64\headless\progen3d-cli.exe doctor
```

Keep the contents of `build/ucrt64/headless` together when moving the CLI.
The build reuses the editor's compiled engine objects. The staged CLI runs
as a console program and renders through a hidden GLFW window. Run `doctor`
in the GPU-enabled Windows desktop session that will run render jobs. It
checks the OpenGL 4.6 context, shaders, and framebuffer readback. For a
Windows Server deployment that starts in Session 0, run the renderer in a
logged-in GPU-enabled desktop session. `validate` runs without OpenGL.

Save a job as `job.json`.

```json
{
  "schema_version": 1,
  "grammar": "examples/SimpleModernBuilding/Single_Floor_Modern_Building_Windows_Doors.p3d",
  "seed": 42,
  "width": 1280,
  "height": 720,
  "output_dir": "build/render-job",
  "strict": true
}
```

Absolute paths and paths relative to the job file are supported. The seed
is an unsigned 64-bit integer. Width and height are integers from 1 to 4096
with a combined limit of 8,388,608 pixels. Validation compiles and executes
the grammar and reports diagnostics and the primitive count. Rendering uses
the editor's mesh and procedural material pipeline at grammar time zero.
A job without a camera track frames the scene isometrically and writes a binary `preview.ppm` once
scene pixels differ from a background-only frame. Use a separate output
directory for each job when keeping multiple images.

```powershell
.\build\ucrt64\headless\progen3d-cli.exe validate .\job.json
.\build\ucrt64\headless\progen3d-cli.exe render .\job.json
```

Each invocation prints one JSON report to stdout. Exit codes are `0` for
success, `2` for command or job input errors, `3` for grammar errors, `4`
for graphics or blank frames, and `5` for output errors. Failed jobs leave
any previously published image in place.

Strict mode is the default. The bundled Fountain Grass grammar currently
reports an unsupported `ServiceConnection` interface. Set `"strict": false` for
a visual inspection of the partial scene. The JSON report retains the
execution error. For a strict Fountain Grass render, make a copy of the
grammar and change the type of `irrigationConnection` to `PipePort`.
User texture slots (`usertextureN`) use a neutral procedural material and
are listed in the report's warnings.

Run the Windows regression checks from the repository root.

```powershell
powershell -ExecutionPolicy Bypass -File .\tests\run_windows_headless_checks.ps1 -CliPath .\build\ucrt64\headless\progen3d-cli.exe
```

### Camera tracks and render layers

The Windows CLI samples one camera time per render. Run the courtyard
example and use the JSON report's `image` and `layers` paths to find the
finished frame.

```powershell
.\build\ucrt64\headless\progen3d-cli.exe render .\examples\courtyard_camera.json
```

Change `camera.time_seconds` to `0` for branch focus, `1` for the midpoint,
`2` for entrance focus, or `3` to sample the camera move. This time selects
the camera tracks for a still. The grammar evaluates at time zero for still jobs.

A camera has four nonempty tracks with strictly increasing, nonnegative
`time` values in seconds. Each keyframe contains a `value`.

| Track | Value |
| --- | --- |
| `position` | World point `[x, y, z]`. |
| `aim` | World point, `{"object":"ID"}` for a grammar object's resolved origin, or `{"object":"ID","attachment":"interfaceID"}` for a grammar interface's world origin. |
| `focus` | Same targets as `aim`. The focus plane is perpendicular to the viewing direction and passes through the sampled target. |
| `lens` | `fov_degrees` from 15 to 100, `aperture_radius` in grammar units, and optionally `max_blur_px` from 0 to 32 (default 24). An aperture radius of zero gives a sharp frame. |

Tracks hold their first or last value outside their time range. Position,
aim, and lens settings interpolate linearly. Focus uses smoothstep between
targets. `validate` resolves the camera targets and reports the sampled
camera without initializing graphics.

Camera jobs export layers by default. Set `"layers": false` for a single
`preview.ppm`, or set `"layers": true` on a job using the fitted camera to
export layers without lens blur. Camera and layer renders omit the editor
grid. The CLI publishes each complete layer set in its own `frame-*`
directory under `output_dir`.

- `preview.ppm` is the finished colour frame. `sharp-color.ppm` is the
  frame before applying depth of field.
- `linear-depth.pfm` stores positive camera-forward depth in grammar
  units, with zero for the background. It is a grayscale little-endian
  float PFM with bottom-to-top rows.
- `object-mask.ppm` stores one packed RGB label per pixel. Decode it as
  `R * 65536 + G * 256 + B` and look up the label in `frame.json`. Zero
  denotes the background. Geometry outside an `Object` scope is
  identified by its zero-based primitive index.
- `foreground-coverage.pgm` stores the amount of blurred foreground
  composited over the background plate, from 0 to 255.

Depth and masks describe the nearest visible surface of the sharp image.
For translucent materials they describe a single surface, while the
colour retains the renderer's blended appearance. The lens pass softens
out-of-focus foreground coverage over a separate background render.

Run the focus, layer, and camera validation checks with Python 3.

```powershell
py -3 .\tests\run_windows_camera_checks.py .\build\ucrt64\headless\progen3d-cli.exe
```

### Animation, physics, and shutter sampling

Add `timeline` to a render job to produce a numbered frame sequence. The
same clock sets grammar `t`, camera track time, and material animation
time. Frame `n` samples `n / fps` seconds. A camera's still-image
`time_seconds` is ignored in a sequence. An automatic camera fits the
initial scene and keeps that framing through the sequence.

```powershell
.\build\ucrt64\headless\progen3d-cli.exe render .\examples\drop_and_settle.json
```

```json
"timeline": {
  "fps": 24,
  "frame_count": 72,
  "physics": true,
  "shutter": { "frame_fraction": 0.75, "samples": 8 }
}
```

- `fps` accepts whole numbers from 1 to 120. `start_frame` defaults to zero
  and `end_frame` is exclusive and defaults to `frame_count`.
- `physics` defaults to false. When enabled, physics advances in the
  editor's fixed 120 Hz steps. Samples between ticks interpolate the
  rendered body poses. Existing movable bodies retain their simulated
  pose when a time-dependent grammar rebuilds. Immovable `!I` geometry
  follows grammar time, and new movable bodies start at their authored
  pose. Use a named `Object` per moving body for stable identity.
- `shutter` is optional. Its `frame_fraction` is the exposure length from
  zero to one frame, beginning at the nominal frame time. The CLI
  averages 2 to 64 evenly spaced midpoint samples in linear colour.
  Zero exposure uses the nominal frame. When layers are requested,
  sharp colour, depth, masks, and transforms describe the nominal frame.
  The final preview includes the shutter exposure.

The render report links to `sequence.json` and `contacts.jsonl`. The
sequence contains numbered folders such as `frame-000000`. Each frame's
`frame.json` records its camera and body transforms, with 4×4 matrices in
column-major order. Contact records contain the 120 Hz tick, body and
object IDs, and peak impulse for each contacting pair on that tick.
Bodies without a named object use a source-based ID.

Set `start_frame` to resume at a chosen frame. The CLI replays the
physics from zero and renders only the requested range. Each invocation
writes a separate sequence directory and retains the contact history from
tick zero. A shutter can advance physics beyond the last nominal frame
to cover its final exposure.

Run the deterministic Windows timeline and shutter checks with Python 3.

```powershell
py -3 .\tests\run_windows_timeline_checks.py .\build\ucrt64\headless\progen3d-cli.exe
```

### Compose saved frames into video

`compose` reads saved sequence manifests and their final preview frames.
It uses a current [FFmpeg Windows build](https://ffmpeg.org/download.html#build-windows)
with the `drawtext` filter and the `libx264` and AAC encoders. Set
`PROGEN3D_FFMPEG` to the path of `ffmpeg.exe`, place it beside the CLI, or
make it available on `PATH`. The courtyard's grammar narration uses the
optional Kokoro bundle described below. Recorded WAV files also work without it.

```powershell
$cli = '.\build\ucrt64\headless\progen3d-cli.exe'
$env:PROGEN3D_FFMPEG = 'C:\tools\ffmpeg\bin\ffmpeg.exe'
$render = & $cli render .\examples\courtyard_video.json | ConvertFrom-Json
$edit = Join-Path (Split-Path $render.sequence_manifest) 'courtyard_edit.json'
Copy-Item .\examples\courtyard_edit.json $edit
& $cli compose $edit --out .\build\courtyard-video\courtyard.mp4
```

The schema-version-1 edit manifest uses source frame numbers for cuts
and output frame numbers for titles, captions, and WAV placement.

```json
{
  "schema_version": 1,
  "cuts": [{"sequence": "sequence.json", "start_frame": 0, "end_frame": 144}],
  "titles": [{"start_frame": 0, "end_frame": 36, "text": "Courtyard approach"}],
  "captions": [{"start_frame": 72, "end_frame": 134, "text": "The courtyard entrance comes into focus."}],
  "narration": [{"start_frame": 72, "wav": "entrance.wav", "gain": 1.0}],
  "use_audio_events": false
}
```

- Cuts play in the order listed and must use saved frames with the same
  dimensions and frame rate. All end frames are exclusive. File paths
  resolve relative to the edit manifest.
- Titles and captions are drawn over the saved lens and shutter effects.
  Use `\n` for a line break in the overlay text. The SRT sidecar contains
  the captions.
- `narration` is optional. Its WAV clips mix at the specified output
  frame and run until the clip or video ends. Gain defaults to 1.
- `use_audio_events` defaults to true. The renderer records grammar cue
  times in `sequence.json`. Time cues keep their authored times. Collision,
  simulation, and simulated object-region cues use 120 Hz physics
  ticks. Camera regions and object regions without physics use nominal
  frame samples. Composition places each included cue at its time
  relative to the cut and stops it at the cut's end.

The command writes an H.264/AAC MP4, a 48 kHz stereo narration WAV, an
SRT, and a composition report listing the cut and cue placements. They
share the requested output basename. To revise titles or captions, edit
the manifest and compose to a new basename using the same saved sequence.

```powershell
py -3 .\tests\run_windows_composition_checks.py $cli --ffmpeg $env:PROGEN3D_FFMPEG
```

### Windows render queue and worker

Start the GPU worker from a Windows desktop session that passes
`doctor`. The worker checks OpenGL 4.6 at startup and takes one render
job at a time. Windows allows one ProGen3D queue worker across sessions
on the PC. Use a local directory for the queue and its frame bundles.

```powershell
$cli = '.\build\ucrt64\headless\progen3d-cli.exe'
$queue = Join-Path $env:LOCALAPPDATA 'ProGen3d\render-queue'
& $cli doctor
& $cli worker --queue $queue
```

In a second PowerShell window, set the same `$cli` and `$queue` and
submit a render job. Omit the ID to list all jobs.

```powershell
$submission = & $cli enqueue .\examples\courtyard_video.json --queue $queue | ConvertFrom-Json
$id = $submission.job.id
& $cli status $id --queue $queue
& $cli cancel $id --queue $queue
& $cli resume $id --queue $queue
```

The worker polls for queued jobs until you stop it with Ctrl+C. Add
`--once` to process the next queued job and exit. Without `--queue`,
the CLI uses the ProGen3D state directory's `render-queue` subfolder.
`status` reports the state, attempt, completed frame count, and how
many frames the current attempt rendered or reused. A finished job
includes its render report in `job.result`.

- The queue stores a copy of the job and its grammar. Queued jobs
  publish to their own `jobs/<id>/render` folder in the queue. The precise
  path is in `job.output_directory`. Keep the same staged app assets
  available across render attempts.
- Cancellation preserves published frames. While the worker is active,
  it checks for cancellation between frames, shutter samples, and
  physics ticks. A worker that exits during a running job leaves it
  `interrupted`. Use `resume` to requeue an interrupted, cancelled, or
  failed job.
- Each published frame directory contains a `checksums.json` with a
  SHA-256 for the image, frame metadata, and any requested layers.
  Resume reuses verified frames, regenerates damaged or missing frames,
  and replays the timeline to reconstruct contacts and audio cues.
  Damaged frame directories are retained with a `.rejected-` prefix.
- Completed sequences expose `job.result.sequence_manifest` for
  composition. Queued stills expose `job.result.image`.

For a direct render with the same checkpoint behavior, choose a dedicated
output folder. Use the same job and grammar when you resume.

```powershell
& $cli render .\examples\courtyard_video.json --out .\build\courtyard-checkpoint
& $cli render .\examples\courtyard_video.json --out .\build\courtyard-checkpoint --resume
```

For a Windows Server deployment, the queue commands can run separately
from the GPU worker. Start the GPU worker in the verified desktop
session. For a Task Scheduler launch from that session, use an
[interactive task for the logged-on user](https://learn.microsoft.com/en-us/windows/win32/taskschd/principal-logontype).
If the worker must render before login, run `doctor` and the worker
check under the exact service account, session, and GPU driver first.

```powershell
py -3 .\tests\run_windows_worker_checks.py $cli
```


### Parameter sweeps and calibrated image fitting

Use `sweep` to render a grid of parameter values from a still job.
The base job sets the render size, seed, and camera. The sweep holds
the camera position, aim, and focus fixed across variants.

```powershell
$cli = '.\build\ucrt64\headless\progen3d-cli.exe'
& $cli sweep .\examples\grammar_matching_sweep.json --out .\build\panel-sweep
```

An experiment JSON uses `schema_version: 1`, a `job` path, and a
`parameters` array. Each parameter has a `name` and either `values`
or a numeric grid specified by `min`, `max`, and `step`. A sweep can
also vary `aperture_radii` with a shot-camera job. An optional `object`
names the object that must have visible pixels in every variant.
Sweeps write labeled PNG contact sheets, individual previews, and
`metrics.json` with parameter values, pixel counts, image checksums,
and diagnostics. The report includes failed variants and the command
returns a nonzero exit code when a variant fails.

Both sweeps and fits bind parameters to editable grammar literals. Place
`# @param name literal` on the code line containing a unique standalone
instance of that literal. A parameter can change a numeric literal or a
bare identifier such as a material name.

```p3d
Start ->
    Panel(1.625) # @param width 1.625
```

Run `fit` with a calibrated references manifest. The fitter minimizes
silhouette error and, where supplied, landmark reprojection error over
the geometry parameter grid. It holds the winning geometry fixed while
testing appearance values against the matching RGB pixels. It scores
held-out views after both searches finish.

```powershell
& $cli fit .\examples\grammar_matching_panel.p3d --views .\references.json --out .\build\panel-fit
```

```json
{
  "schema_version": 1,
  "seed": 42,
  "geometry": [{"name": "width", "min": 1.75, "max": 3, "step": 0.125}],
  "appearance": [
    {"name": "paint", "values": ["navybluemattepaint", "redmattepaint"]},
    {"name": "key_candela", "values": [6, 24, 65]}
  ],
  "views": [
    {
      "id": "front",
      "camera": {
        "center": [0, 0, 0],
        "direction": [0, 0, -1],
        "pixels_per_unit": 56,
        "distance": 8
      },
      "silhouette": "front-mask.png",
      "image": "front-rgb.png",
      "landmarks": [
        {"object": "FIT_PANEL", "attachment": "upper_right", "pixel": [176.5, 51]}
      ]
    }
  ]
}
```

- Each view declares an orthographic camera. `direction` points from the
  camera towards `center`. The default `up` is `[0, 1, 0]`. Supply an
  independent up vector for top or bottom views. `distance` defaults to
  10 world units. A regular render job can use the same camera object in
  its `orthographic` field.
- Use a black background in silhouette images. Nonblack pixels mark the
  object. Image paths are relative to the reference manifest. An optional
  RGB image must have the same dimensions as its silhouette. Fitting
  appearance needs an RGB image in at least one training view.
- Landmark pixels use a top-left origin. They refer to a grammar object
  origin or a named attachment. Optional `landmark_weight` controls the
  contribution of reprojection error relative to silhouette IoU.
- Set `held_out` to true on validation views. Their reference data is
  excluded from parameter selection.

The output directory contains `fitted.p3d`, `fit-report.json`, and each
view's difference overlay. Light gray shows overlap, cyan shows fitted
geometry beyond the reference, and coral shows missing fitted geometry.
The report records parameter trials, per-view silhouette IoU, landmark
RMSE in pixels, and RGB RMSE on matching silhouette pixels where RGB
references are available.

The Windows check builds independent analytic silhouettes for a cuboid
of known width, captures RGB references from the staged renderer, and
checks the fit against a held-out oblique view.

```powershell
py -3 .\tests\run_windows_experiment_checks.py $cli --output-root .\build\experiments
```


### Optional Kokoro grammar audio

Extract the [Sherpa-ONNX 2.0.3 Windows x64 shared MT Release archive](https://github.com/k2-fsa/sherpa-onnx/releases/expanded_assets/v2.0.3)
and the [Kokoro v1.0 int8 model archive](https://github.com/k2-fsa/sherpa-onnx/releases/expanded_assets/tts-models)
into `build/kokoro-windows`. Keep the extracted directory names
`sherpa-onnx-v2.0.3-win-x64-shared-MT-Release` and
`kokoro-int8-multi-lang-v1_0`. Set `-DPROGEN3D_KOKORO_BUNDLE=<path>` at
configuration time to use a different directory. Under MSYS2, use
`tar -xjf` to extract the archives.

Reconfigure and build as above. The staging target copies the runtime and
model into `audio/kokoro` beside the Windows editor and headless CLI
executables. Start the editor with `examples/audio_events.p3d` to try the
feature. A build without the bundle still parses and records grammar audio
events. Set `use_audio_events` to false and supply WAV clips in the edit
manifest to compose with external recordings.

## Linux Supported Baseline

The current desktop editor targets Linux with:

- C++17
- GNU Make
- Dear ImGui 1.90.1
- GLFW
- OpenGL
- GLM
- libcurl with OpenSSL
- D-Bus and an XDG Desktop Portal file chooser

The repository contains the expected ImGui, GLFW, GLAD, curl, and OpenSSL dependency layout under `third_party/`.
Native File Dialog Extended 1.3.0 is pinned under `third_party/vendor/nativefiledialog-extended-1.3.0/`; the Linux build compiles its portal backend and obtains D-Bus flags through `pkg-config dbus-1`.

## Clean Build

```bash
make clean
make -j2 all
```

The resulting executables are:

```text
./progen3d-editor-gui
./p3d_to_3dtexmesh
./p3d_to_3view
```

The compatibility target creates a `progen3d` symbolic link:

```bash
make progen3d
```

## Deterministic Binary Package

Build the editor and both conversion tools into a reproducible Linux archive:

```bash
make package
./tests/run_conversion_package_checks.sh
```

The package target writes
`build/packages/progen3d-linux-x86_64.tar.gz` and a matching `.sha256` file.
The package check creates the archive twice, requires byte-identical archives,
verifies the internal `SHA256SUMS`, and exercises both converter help paths.

## Grammar Verification

Run the complete inherited P0, P1, and P2 grammar verification suite:

```bash
./tests/run_p2_checks.sh
```

The focused grammar suite and the complete application build are separate gates. A release candidate must pass both.

## GUI Startup Verification

Run the deterministic GUI startup smoke check:

```bash
./tests/run_gui_smoke_check.sh
```

The check launches `progen3d-editor-gui --smoke-test` under Xvfb when available, verifies initial scene generation, and requires a clean application shutdown.

The smoke mode also renders one complete editor frame and verifies that the active scene reaches the preview texture.

Capture deterministic preview evidence from a local grammar:

```bash
./progen3d-editor-gui \
  --visual-test \
  --capture-preview /tmp/progen3d-preview.ppm \
  --open tests/fixtures/transform_evidence/100_identity_axes.p3d
```

`--visual-test` includes the normal smoke gate. `--capture-preview` writes the resolved preview framebuffer as a top-left-oriented binary PPM image.

## Startup Document

Open a specific local grammar when the editor starts:

```bash
./progen3d-editor-gui --open examples/P2_time_showcase.p3d
```

`--document` is accepted as an equivalent spelling. The local path remains separate from cloud identity.

## Temporal GUI Verification

Run the packaged P2 showcase through initial generation, a deterministic grammar-time sample, and one rendered preview frame:

```bash
./tests/run_temporal_gui_smoke_check.sh
```

## Editor Service Verification

Run the local document persistence checks:

```bash
./tests/run_document_persistence_checks.sh
```

The harness verifies initial save, replacement save, exact source reload, temporary-file cleanup, and expected failures for invalid paths.

Run the native local-file dialog and unsaved-document workflow checks:

```bash
./tests/run_local_file_dialog_workflow_checks.sh
```

The harness verifies typed dialog outcomes, filters, Unicode paths, one-shot requests, failure-atomic reads, and the shared New/Open/Cloud Open/Exit replacement guard. The normal GUI uses `Ctrl+O`, `Ctrl+S`, and `Ctrl+Shift+S` for Open, Save, and Save As.

Run all focused editor architecture checks, including repository fakes, part-catalog fallback, AI proposal review, preview controllers, diagnostics, logging, and mesh export:

```bash
./tests/run_editor_architecture_checks.sh
```

Run the focused transform, positioning, visual, and supervisor evidence:

```bash
./tests/run_transform_scope_checks.sh
./tests/run_transform_grammar_scene_checks.sh
./tests/run_collision_positioning_checks.sh
./tests/run_transform_visual_checks.sh
./tests/run_transform_supervisor_matrix.sh
```

Run the complete AxialProfilev1 focused set:

```bash
./tests/run_axial_profile_geometry_checks.sh
./tests/run_axial_profile_grammar_scene_checks.sh
./tests/run_axial_profile_editor_checks.sh
./tests/run_axial_profile_gui_smoke_check.sh
./tests/run_axial_profile_temporal_gui_smoke_check.sh
./tests/run_axial_profile_evidence_checks.sh
```

Run the complete clean-build release gate used by CI:

```bash
./tests/run_release_checks.sh
```

Application sources compile with `-Wall -Wextra -Wpedantic`. Vendored Dear ImGui sources retain their upstream warning policy.

## Clean-State Requirement

Do not use retained object files as build evidence. Run `make clean` before validating changes to public headers, source lists, compiler flags, or third-party linkage.
