# Modern Car MCSMv2

MCSMv2 is a hybrid semantic-section and implicit-scaffold modern-car generator
implemented as both a Python reference model and a native ProGen3D object model.

## Native catalog

```bash
PYTHONDONTWRITEBYTECODE=1 python3 -B source/generate_native_catalog.py \
  --source-package ../MCSMv2_0_1_Modern_Car_Complete \
  --header ../../../include/vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h \
  --manifest native_catalog_manifest.json
```

The signed MCSMv2.0.1 source release is immutable. Regenerate it only to a
temporary directory through `tests/run_mcsmv201_source_contract_checks.sh`.
The historical files and `RELEASE_MANIFEST.json` retained in this directory are
the earlier 2.0.0 package artifacts; native runtime authority is the generated
2.0.1 catalog and `native_catalog_manifest.json`.

## Main outputs

- `models/modern_car_v2_*.glb`: complete visible scenes
- `models/modern_car_v2_*_body.stl`: watertight principal bodies
- `models/modern_car_v2_*_engineering.glb`: body, semantic surface, BIW,
  occupants, wheel envelopes and functional packages
- `models/modern_car_v2_reference_semantic_surface.glb`: explicit section loft
- `models/modern_car_v2_reference_curve_network.glb`: semantic curves/sections
- `models/modern_car_v2_design_manifold.glb`: continuous reference-crossover samples
- `semantic_stations/*.csv`: named station values
- `curves/*.json`: 3D curve and section data
- `dependency_graphs/*.json`: evaluated dependency graphs
- `validation.json`: model gates and deterministic hashes
- `MCSMv2_Modern_Car_Family.p3d`: FGKv1 target grammar
- `MCSMv2_Executable_Family_Preview.p3d`: native four-variant preview
- `MCSMv2_*_Preview.p3d`: native per-variant executable grammars

## Native ProGen3D preview

From the repository root:

```bash
./progen3d-editor-gui --open \
  examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_Modern_Car_Complete/MCSMv2_Executable_Family_Preview.p3d
```

Run the focused native acceptance gates:

```bash
tests/run_mcsmv2_source_contract_checks.sh
tests/run_mcsmv2_semantic_section_checks.sh
tests/run_mcsmv2_implicit_body_checks.sh
tests/run_mcsmv2_semantic_surface_checks.sh
tests/run_mcsmv2_engineering_checks.sh
tests/run_mcsmv2_grammar_checks.sh
tests/run_mcsmv2_visual_checks.sh
```

See `MCSMv2_NATIVE_IMPLEMENTATION_VERIFICATION.md` for exact source hashes,
native topology hashes, IoUs, assurance boundaries, compatibility results, and
the inherited MCP_OMv1 visual exception.

## Dependencies

NumPy, SciPy, scikit-image, trimesh and Pillow. VTK is optional for Python
previews. Trimesh and Pillow are required by the native three-view visual gate.
