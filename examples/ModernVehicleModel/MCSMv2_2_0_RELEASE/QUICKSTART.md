# MCSMv2.2 Quick Start

## Install dependencies

```bash
python -m pip install -r source/requirements.txt
```

## Generate the complete release

```bash
python source/generate_release.py \
  --output generated_mcsmv22 \
  --resolution medium \
  --no-preview
```

Remove `--no-preview` to render the reference preview set when VTK is installed.

## Generate one variant

```bash
MCSM_DISABLE_VTK=1 PYTHONPATH=source \
python source/modern_car_mcsmv2.py \
  --output generated_reference \
  --resolution medium \
  --variant reference \
  --worker \
  --no-preview
```

Valid variant keys are `reference`, `track`, `aero`, and `crossover`.

## Run tests

```bash
MCSM_DISABLE_VTK=1 PYTHONPATH=source \
python -m unittest discover -s source/tests -v
```

## Principal outputs

```text
models/*_body.stl                         watertight principal bodies
models/*.glb                              visible vehicle scenes
models/*_surface_partition.glb            semantic panel/aperture ledger
models/*_suspension_kinematics.glb        hardpoints and tyre sweep hulls
models/*_closures_closed.glb              closed closure configuration
models/*_closures_open.glb                open closure configuration
kinematics/*_suspension_hardpoints.json   hardpoints and pose evidence
kinematics/*_wheel_pose_sweeps.json       actual tyre sweep records
kinematics/*_closure_system.json          hinge and helical-glass records
validation.json                           complete assurance evidence
```

## Coordinate frame

MCSMv2 uses a right-handed vehicle frame:

```text
+X = front
+Y = right
+Z = up
```

The exported frame record includes the adapter into the Progen3D vehicle convention.
