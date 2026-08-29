# COMv1 Chair Catalog

This directory contains deterministic generated evidence for the COMv1 chair
object model.

- `COMv1_Chair_Catalog.json` lists exactly 15 data-defined chair designs across
  dining, kitchen, study, living, and patio contexts.
- `COMv1_Chair_Catalog.p3d` is an executable grammar scene. Its `Start` rule
  expands fifteen named chair productions arranged on the world ground plane;
  every production wraps its parser/evaluator-tested `CurveNetworkSurface`,
  `ShellOffset`, and `SweepDisk` compound shape with canonical COMv1 placement
  and an explicit material.
- `reference_views/` contains the direct ProGen3D object-model silhouettes.
- `generated_views/` contains silhouettes from the serialized grammar after it
  is parsed, evaluated, and resolved through the generic mesh repository.
- `fit_reports/COMv1_Three_View_Fit_Report.json` records raw per-view IoU and
  dimensional residuals. Every view must be strictly greater than `0.80`.

Run `tests/run_comv1_chair_catalog_checks.sh` and
`tests/run_comv1_three_view_fit_checks.sh` to regenerate and validate these
artifacts.
