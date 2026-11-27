# Building ProGen3D Editor GUI

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

Double-click **Open Lyra.cmd** next to `progen3d.exe`, then select **Play**
in the Preview panel to run Lyra’s four-second projection greeting.
From a running editor, use **Open...** (Ctrl+O) and select
`examples/Characters/Lyra/Lyra_Greeting.p3d` inside the app folder.
The grammar embeds the character meshes and animation. To slow the greeting
to eight seconds, change `Start -> Speed(1)` to `Start -> Speed(0.5)` and
select **Run**.

The body is based on MakeHuman core assets. The rig and motion derive from
Kay Lousberg’s KayKit Adventurers. [MakeHuman](https://www.makehumancommunity.org/about/license.html)
and [KayKit](https://kaylousberg.itch.io/kaykit-adventurers) publish these assets under CC0.

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
