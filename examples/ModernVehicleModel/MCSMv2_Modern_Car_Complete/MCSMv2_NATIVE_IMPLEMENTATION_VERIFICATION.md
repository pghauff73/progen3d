# MCSMv2.0.1 Native Integration Verification

**Date:** Monday, August 24, 2026

**MCSMv2.0.1 native integration status:** Accepted at source assurance level V2

**MCSMv2.1 status:** Deferred. Semantic/implicit convergence, exclusive panel
domain partition, and explicit glass apertures are not claimed.

**Cross-version release status:** Not fully certified because the inherited
`MCP_OMv1` visual baseline remains below `0.80` for three side views. The MCP
native and grammar gates remain unchanged and pass.

## Implemented Object Model

The integration extends the existing native MCSMv2 architecture rather than
creating a parallel vehicle implementation.

- `McsMv201SourceRelease` identifies the immutable signed source release.
- `ModernCarSemanticFamily` composes the four accepted variants.
- Each `ModernCarSemanticVariant` composes its explicit
  `VehicleReferenceFrameDefinition`, `ImplicitFieldCalibration`,
  `ModernCarSourceAssurance`, 74-node parameter dependency graph, package,
  platform, style, wheels, powertrain, closures, and semantic section field.
- `VehicleSemanticSectionFieldEvaluationService` implements the v2.0.1
  non-crossing vertical field as an underbody PCHIP plus PCHIP-interpolated
  logarithmic positive gaps. Width fields remain independent PCHIP curves.
- `VehicleCharacterCurveNetwork` derives all fourteen accepted curves from the
  canonical section field.
- `InverseAffinePrewarpedField` and
  `ImplicitFieldCalibrationResolutionService` calibrate the scalar field before
  tessellation. No body vertex is affinely corrected after extraction.
- `VehiclePanelPatchGraph`, `VehicleBodyInWhiteAssembly`,
  `VehicleOccupantEnvelopeSystem`, and `VehicleFunctionalPackageSystem` retain
  the source engineering relationships without promoting them to structural or
  manufacturing claims.
- `McsMv2GeneratedMeshProvider` exposes deterministic native bodies to current
  grammar through `GeneratedMeshReference`.

## Immutable Source Contract

The authoritative source is
`examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_0_1_Modern_Car_Complete/`.
It remains byte-for-byte governed by its signed release manifest.

| Evidence | Result |
|---|---:|
| Release manifest SHA-256 | `c49934afea36ad14f9f6b848324b652334ef3db9421ee340d454abc32421a6d1` |
| Signed artifacts | 86 |
| Signed source bytecode artifacts retained in place | 4 |
| Clean Python tests | 25 / 25 passed |
| Regenerated core artifacts | 36 / 36 byte-identical |
| Parameter provenance records per variant | 55 |
| Dependency nodes per variant | 74 |
| Semantic stations per variant | 11 |
| Character curves per variant | 14 |
| Imported source assurance | V2 passed; V3-V5 deferred |

The clean verification runtime was CPython `3.14.6` with NumPy `2.5.2`, SciPy
`1.18.1`, scikit-image `0.26.0`, trimesh `5.0.0`, and Pillow `12.3.0`.

## Canonical Native Catalog

| Artifact | SHA-256 |
|---|---|
| `source/generate_native_catalog.py` | `62fc02330220ad0507591f35dc6a4a4c9f19f5361d810cfc3acd374636242762` |
| `GeneratedMcsMv2Catalog.h` | `24c319d41e1547f8bb88a2d84820ed77894015f5f018f3dd881e27e5609e298d` |
| `native_catalog_manifest.json` | `e29a389b2fefd567772fcc549fad5dd8599954d2ec45c2ce00c29416f5294202` |
| `verify_mcsmv201_source_contract.py` | `c89fb29cc5e294a48b4256d22de771f895988f5634b3a63a1a5bdf8f5e148ff8` |
| `compare_mcsmv201_regeneration.py` | `521e4caef43b56a23be4c550b400207277d863b55f12c7092529120a03276680` |

Two independent native catalog generations are byte-identical. Generated
paths are repository-relative and contain no timestamps, absolute build paths,
or Python bytecode.

## Semantic Parity

| Variant | Curves | Maximum accepted curve residual | Semantic surface hash |
|---|---:|---:|---:|
| Reference | 14 | `6.66134e-16` | `13294461414748673349` |
| Track | 14 | `5.08768e-16` | `17149885356355141933` |
| Aero | 14 | `6.28037e-16` | `10988980163820451771` |
| Crossover | 14 | `2.71948e-16` | `6515549832182969514` |

The frame gate verifies point and direction round trips, X-front/Y-right/Z-up
to ProGen3D Z-forward/X-right/Y-up mapping, wheel-center conversion, local
package origin, and an explicit ProGen3D ground plane at `Y=0`.

## Editor-Safe Native Bodies

The editor-safe policy is `52 x 32 x 32`. Native inverse field calibration uses
three deterministic pre-tessellation iterations starting from the accepted
source prewarp. Every resolved prewarp remains below `0.02`.

| Variant | Vertices | Faces | Topology hash | Signed volume | Resolved prewarp |
|---|---:|---:|---:|---:|---:|
| Reference | 21,066 | 42,128 | `12792849852543862055` | 5.38970 | 0.0163243 |
| Track | 20,780 | 41,552 | `4840609770680284886` | 5.51137 | 0.0160726 |
| Aero | 21,138 | 42,272 | `9541205142429914192` | 5.26733 | 0.0157296 |
| Crossover | 20,612 | 41,216 | `15860322388680273186` | 6.04166 | 0.00792848 |

Every body has zero boundary edges, zero nonmanifold edges, zero degenerate
triangles, positive signed volume, idempotent outward orientation, calibrated
source-frame bounds within the source `0.0025 m` package tolerance, and zero
post-mesh affine correction. Each editor body also remains below the ImGui
16-bit draw-list vertex limit.

## Engineering Relationships

| Variant | Panels / relations | BIW members / joints | Occupants | Functional | Engineering hash |
|---|---:|---:|---:|---:|---:|
| Reference | 11 / 6 | 22 / 25 | 8 | 2 | `3900093925883507132` |
| Track | 11 / 6 | 22 / 25 | 8 | 2 | `11265560866167609141` |
| Aero | 11 / 6 | 22 / 25 | 8 | 2 | `14251043746462812867` |
| Crossover | 11 / 6 | 22 / 25 | 8 | 3 | `17622323454836948570` |

Panel and BIW endpoint validation, BIW graph connectivity, occupant
containment, functional package containment, and side-specific wheel envelope
construction pass for all four variants. These are V2 concept-geometry checks,
not geometric load-path, kinematic, or manufacturing certification.

## Grammar and GUI Acceptance

The current-parser executable grammars are:

- `MCSMv2_reference_Preview.p3d`
- `MCSMv2_track_Preview.p3d`
- `MCSMv2_aero_Preview.p3d`
- `MCSMv2_crossover_Preview.p3d`
- `MCSMv2_Executable_Family_Preview.p3d`
- `MCSMv2_Modern_Car_Family.p3d`

All six regenerate byte-identically, start with an explicit `Start` rule, use
material-complete `!I(...)` syntax, resolve through the native generated-mesh
provider, and pass GUI smoke, rendered-frame, and field-of-view markers. Direct
service and grammar-resolved topology hashes are identical.

## Three-View Silhouette Acceptance

The visual comparison policy uses the accepted watertight `64 x 36 x 36`
native grid and 512-pixel binary silhouettes.

| Variant | Front IoU | Side IoU | Top IoU |
|---|---:|---:|---:|
| Reference | 0.981584 | 0.975788 | 0.977732 |
| Track | 0.984103 | 0.975235 | 0.982674 |
| Aero | 0.983645 | 0.974237 | 0.988257 |
| Crossover | 0.969681 | 0.980955 | 0.980249 |

All twelve scores exceed the required `0.80`. Durable comparison images and
the acceptance JSON are in
`tests/evidence/mcsmv2_native_three_view_silhouettes/`. The acceptance JSON
SHA-256 is
`747c7a7e5db79b8444df6b94d62b7b1439f9d95da26b7f08999ac1e3cb77b3b4`.

## Compatibility Audit

| Gate | Result |
|---|---|
| MCP_OMv1 native deterministic hashes | PASS, unchanged |
| MCP_OMv1 generated grammars and GUI smoke | PASS |
| MCP_OMv1 three-view baseline | PARTIAL, inherited side-view exception |
| MVP2.6 parametric integration | PASS |
| MVP2.5 native hatchback gate | PASS |
| MVP2.5 five-view visual gate | PASS |
| `make -j2 progen3d-editor-gui` | PASS |

The unchanged MCP native geometry hashes are reference
`14976451742210595606`, track `8219619488160659468`, aero
`6967662770876233455`, and crossover `3753541085065208904`.

The independently rerun MCP visual exception is:

- track side: `0.778319`;
- aero side: `0.773697`; and
- crossover side: `0.658835`.

All other MCP front, side, and top views pass. The exception is isolated to the
legacy stylized MCP preview and is not produced by the MCSMv2 provider or field
calibration changes.

## Requirement-to-Evidence Audit

| Completion requirement | Evidence | Result |
|---|---|---|
| Immutable historical source | Signed manifest verification | PASS |
| Clean source reproduction | 25 tests and 36 byte-identical artifacts | PASS |
| Deterministic provenance-complete catalog | Dual generation, hashes, source records | PASS |
| Four native variants and explicit frames | Catalog and frame round-trip harness | PASS |
| Source/native semantic parity | Section, curve, field, and surface harnesses | PASS |
| Native topology, package, wheel, occupant acceptance | Body and engineering harnesses | PASS |
| Current-parser executable grammars | Grammar regeneration and GUI smoke | PASS |
| Direct-service and grammar hash parity | Generated mesh reference harness | PASS |
| Front/side/top IoU at least 0.80 | Twelve accepted comparisons | PASS |
| MVP2.5 and MCP compatibility | Native/grammar pass; legacy visual exception explicit | PASS WITH EXCEPTION |
| No transient native release files | Bytecode and `__pycache__` scan | PASS |
| Exact source and generated hashes | This report and machine-readable audit | PASS |
| No unsupported MCSMv2.1 or production claim | Imported V2 boundary and deferred levels | PASS |

Machine-readable evidence is recorded in
`tests/evidence/mcsmv2_native_implementation_2026-08-24.json`.

## Validation Commands

```bash
MCSMV201_PYTHON=/tmp/progen3d-mcsmv201-venv/bin/python \
  tests/run_mcsmv201_source_contract_checks.sh
tests/run_mcsmv2_source_contract_checks.sh
tests/run_mcsmv2_semantic_section_checks.sh
tests/run_mcsmv2_semantic_surface_checks.sh
tests/run_mcsmv2_implicit_body_checks.sh
tests/run_mcsmv2_engineering_checks.sh
tests/run_mcsmv2_grammar_checks.sh
tests/run_mcsmv2_visual_checks.sh
tests/run_mcp_omv1_checks.sh
tests/run_mcp_omv1_grammar_checks.sh
tests/run_mcp_omv1_visual_checks.sh
tests/run_mvp26_parametric_integration_checks.sh
tests/run_mvpv25_hatchback_checks.sh
tests/run_mvpv25_hatchback_visual_checks.sh
```

`tests/run_mcp_omv1_visual_checks.sh` intentionally remains nonzero until the
three inherited side-view baselines are separately corrected or re-approved.
