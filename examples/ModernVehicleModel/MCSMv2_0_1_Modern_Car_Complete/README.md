# Modern Car MCSMv2.0.1

MCSMv2.0.1 is the integrity-patched hybrid semantic-section and
implicit-scaffold modern-car generator.

## Regenerate

```bash
python modern_car_mcsmv2.py --output generated --resolution medium
```

Use `--no-preview` when VTK is unavailable or only mesh/data outputs are needed.

## Integrity changes

- parameter-specific evidence and units
- explicit MCSMv2 vehicle frame and Progen3D axis adapter
- complete causal parameter DAG including overhangs and hidden model inputs
- non-crossing continuous semantic section fields
- measured curve-network residual
- pre-tessellation field calibration with no post-mesh affine correction
- final-mesh winding, manifold and self-intersection validation
- independent final-mesh occupant and sampled tyre-sweep checks
- V0-V5 assurance record

## Main outputs

- `models/modern_car_v2_*.glb`: complete visible scenes
- `models/modern_car_v2_*_body.stl`: watertight principal bodies
- `models/modern_car_v2_*_engineering.glb`: engineering evidence scenes
- `semantic_stations/*.csv`: named station values
- `curves/*.json`: 3D curve and section data with explicit frame metadata
- `dependency_graphs/*.json`: unit-aware evaluated dependency graphs
- `validation.json`: independent integrity checks and assurance levels
- `MCSMv2_Modern_Car_Family.p3d`: FGKv1 target grammar

## Assurance

This release targets V2 concept geometry. It does not claim production Class-A,
closure/glass/suspension kinematics, manufacturing, structural FEA, CFD, crash,
homologation or physical validation.

## Dependencies

NumPy, SciPy, scikit-image, trimesh and Pillow. VTK is optional for previews.

## Release evidence

- `TEST_REPORT.md`: automated and generated validation results
- `RELEASE_VERIFICATION.json`: artifact load/parse/compile verification
- `AUDIT_CLOSURE.md`: integrity-audit resolution map
- `KNOWN_LIMITATIONS.md`: explicit V3-V5 and MCSMv2.1 boundaries
- `GRAMMAR_VALIDATION.json`: structural target-grammar check
