# MCSMv2 Quick Start

From the package root:

```bash
python source/modern_car_mcsmv2.py \
  --output generated \
  --resolution medium
```

Generate one variant without image previews:

```bash
python source/modern_car_mcsmv2.py \
  --output generated-reference \
  --resolution low \
  --variant reference \
  --no-preview
```

Run the tests:

```bash
python -m unittest discover -s source/tests -v
```

Build and open the native executable family from the repository root:

```bash
make -j2 progen3d-editor-gui
./progen3d-editor-gui --open \
  examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_Modern_Car_Complete/MCSMv2_Executable_Family_Preview.p3d
```

Run native acceptance:

```bash
tests/run_mcsmv2_source_contract_checks.sh
tests/run_mcsmv2_semantic_section_checks.sh
tests/run_implicit_surface_meshing_checks.sh
tests/run_mcsmv2_implicit_body_checks.sh
tests/run_mcsmv2_semantic_surface_checks.sh
tests/run_mcsmv2_engineering_checks.sh
tests/run_mcsmv2_grammar_checks.sh
tests/run_mcsmv2_visual_checks.sh
```

Main viewable models:

- `models/modern_car_v2_reference.glb`
- `models/modern_car_v2_reference_engineering.glb`
- `models/modern_car_v2_variations_comparison.glb`
- `models/modern_car_v2_design_manifold.glb`

Native evidence is recorded in
`tests/evidence/mcsmv2_native_three_view_silhouettes/` and
`tests/evidence/mcsmv2_native_implementation_2026-08-24.json`.
