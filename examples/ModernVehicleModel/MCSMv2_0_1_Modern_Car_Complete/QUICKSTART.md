# MCSMv2.0.1 Quick Start

## Run tests

```bash
cd source
python -m unittest discover -s tests -v
```

## Regenerate all variants

```bash
cd source
python modern_car_mcsmv2.py \
  --output ../regenerated \
  --resolution medium \
  --no-preview
```

Remove `--no-preview` to generate VTK preview images when VTK is installed.

## Generate one variant

```bash
python modern_car_mcsmv2.py \
  --output ../reference_only \
  --resolution medium \
  --variant reference \
  --no-preview
```

Variant keys:

```text
reference
track
aero
crossover
```

## Principal outputs

- `validation.json`: V0-V5 assurance and independent integrity evidence
- `dependency_graphs/*_parameter_graph.json`: typed, unit-aware DAGs
- `semantic_stations/*.csv`: semantic section stations
- `curves/*_curve_network.json`: framed 3D curve and section data
- `models/*_body.stl`: watertight principal body meshes
- `models/*.glb`: visible and engineering scenes
- `MCSMv2_Modern_Car_Family.p3d`: FGKv1 target grammar

## Coordinate conversion

MCSMv2.0.1:

```text
X = front
Y = right
Z = up
```

Progen3D vehicle convention:

```text
X = right
Y = up
Z = front
```

The explicit matrix is stored in `VERSION.json` and each framed data export.
