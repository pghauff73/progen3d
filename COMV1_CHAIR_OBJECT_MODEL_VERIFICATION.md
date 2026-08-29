# COMv1 Chair Object Model Verification

Date: 2026-08-24

## Result

COMv1 and the required FGKv1 primitives are implemented and accepted against
the repository-native gates.

- The catalog contains exactly 15 chair designs across dining, kitchen, study,
  living, and patio contexts.
- Every design resolves through the generic procedural mesh repository.
- Every built chair owns a finite `ChairPlacement` and `ChairOrientation` with a
  world ground-contact position, yaw, local ground height, and deterministic
  local-Z-up to world-Y-up transform.
- Default chairs stand at the world origin facing world `+Z`; elevated and
  yaw-rotated placement tests prove that local ground and height references map
  to the requested world position.
- Every backrest-bearing chair now shares the backrest bottom curve with the seat
  rear curve; the catalog harness inspects the curve-network specifications and
  rejects a detached seat/backrest junction.
- Serialized chair grammar is parsed and evaluated independently before view
  comparison.
- The generated `COMv1_Chair_Catalog.p3d` is an executable scene with one
  `Start` production and fifteen named chair productions. Each chair production
  supplies canonical ground placement, upright orientation, and an explicit
  material before instancing its `CompoundShape`.
- All 45 front, side, and top views exceed raw silhouette IoU `0.80`; the
  measured minimum is recorded in the dated acceptance evidence.
- Width, depth, seat height, and overall height retain zero candidate/reference
  residual in the fixed-envelope comparison.
- Four dining instances and one study instance bind to the two existing SMB
  `ChairSet` objects without changing the 124-object spatial model. Each binding
  publishes the canonical grounded placement plus legacy pose projections.
- The study chair now allocates `Function.SupportStudyActivity` rather than the
  generic dining function.
- The native `progen3d-editor-gui` target builds successfully.
- The native editor parses all 16 catalog rules, regenerates the complete
  fifteen-chair ground-grid scene, fits the isometric preview, and captures it
  without blocking grammar diagnostics or draw-index overflow.

## Evidence

- `tests/evidence/comv1_chair_acceptance_2026-08-24.json`
- `examples/COMv1_Chair_Catalog/COMv1_Chair_Catalog.json`
- `examples/COMv1_Chair_Catalog/COMv1_Chair_Catalog.p3d`
- `examples/COMv1_Chair_Catalog/fit_reports/COMv1_Three_View_Fit_Report.json`
- `examples/SMB_OMv2_Instance_SMB_001/COMv1_Chair_Bindings.json`

## Validation Commands

```text
tests/run_fgkv1_curve_surface_checks.sh
tests/run_comv1_chair_catalog_checks.sh
tests/run_comv1_three_view_fit_checks.sh
tests/run_comv1_smb_omv21_integration_checks.sh
tests/run_three_view_projection_checks.sh
tests/run_profile_extrusion_checks.sh
tests/run_smb_omv21_all_objects_checks.sh
make -j2
```
