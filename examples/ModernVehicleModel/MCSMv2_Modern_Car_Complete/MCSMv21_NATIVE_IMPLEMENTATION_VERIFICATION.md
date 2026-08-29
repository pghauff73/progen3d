# MCSMv2.1 Native Implementation Verification

**Date:** Monday, August 24, 2026
**Implementation status:** Complete
**Cross-version certification status:** Blocked only by the inherited MCP_OMv1 side-view baseline

## Implemented Object Model

The native integration adds explicit model ownership for the MCSMv2.1 source
release, periodic surface coordinates and loops, panel and aperture domains,
registered semantic surfaces, bidirectional semantic/implicit correspondence,
exclusive final-body ownership, derived adjacency, static glass apertures, and
the generated semantic family and geometry.

Services are separated by responsibility: catalog construction, closest-surface
projection, registration, correspondence evaluation, domain evaluation,
exclusive partitioning, aperture-derived glass construction, semantic geometry
generation, generated-mesh resolution, grammar lowering, and shared wheel
grammar lowering.

## Requirement Evidence

| Requirement | Direct evidence | Result |
|---|---|---|
| Repaired 2.1 source contract | `tests/run_mcsmv210_source_contract_checks.sh` | PASS, 74/74 reproducible artifacts |
| Deterministic dual-version catalog | `tests/run_mcsmv21_catalog_checks.sh`, `tests/run_mcsmv2_source_contract_checks.sh` | PASS |
| Four registered variants | `tests/run_mcsmv21_registration_checks.sh` | PASS |
| Fail-closed global correspondence | p95 distance `0.008 m`, maximum `0.120 m`, p95 normal `20 deg` gates | PASS |
| Periodic domains and exclusive ownership | `tests/run_mcsmv21_semantic_geometry_checks.sh` | PASS |
| Nine glass apertures per variant | `tests/run_mcsmv21_semantic_geometry_checks.sh` | PASS |
| Executable family and variant grammars | `tests/run_mcsmv21_grammar_checks.sh` | PASS |
| Current GUI regeneration | family plus four variant headless captures | PASS |
| Twelve front/side/top silhouettes | `tests/run_mcsmv21_silhouette_checks.sh` | PASS, minimum IoU `0.95026155` |
| MCSMv2.0.1 source/native/visual compatibility | source, engineering, implicit, section, surface, grammar, and visual scripts | PASS |
| MVPv2.5 compatibility | native, collision grammar, and five-view visual scripts | PASS |
| MCP_OMv1 native and grammar compatibility | `tests/run_mcp_omv1_checks.sh`, `tests/run_mcp_omv1_grammar_checks.sh` | PASS |
| MVP2.6 integration | `tests/run_mvp26_parametric_integration_checks.sh` | PASS |

## Exact Hashes

| Artifact | SHA-256 |
|---|---|
| MCSMv2.1 release manifest | `283b89da95dbcd3efbebf83f450b710cc8d2618cdc668764bf6670cd25da28bd` |
| Combined native catalog manifest | `0b6bedeab028b6f3dad52a096cdb87bbc5663872382e9eec7092fcb370c876ca` |
| Preserved MCSMv2.0.1 generated catalog | `24c319d41e1547f8bb88a2d84820ed77894015f5f018f3dd881e27e5609e298d` |
| MCSMv2.1 generated catalog | `ae0066b60b248ba245cb47ff480eb23dd33392dfe7462f1894cb3b05855bc741` |
| Family preview grammar | `a67d99136059d1c7038b10831bfc69e9ec282cbdd44a783a827a85a1e4e7f892` |
| Reference preview grammar | `bb73d9d6beb576767420c5544bf0a9ede52ef2b32f7eb04eec5880d9235476f9` |
| Track preview grammar | `41019bb158040b5c72d6c9b7dbcb892ddc30b99c49dd34250d8069971e457467` |
| Aero preview grammar | `57f1f990e11a22b1c0c09fa3ad23c5976e5358290f920e4d3e4bbe4eb38a9124` |
| Crossover preview grammar | `727d72be5f96c0410b16493c5f0d08daad0f53e4d12a104e06a1604c6530f20c` |
| Twelve-view evidence | `c09d69d8d00b5eb7d1fefecf2df5c32dec33522d889112c106e11586668116ae` |

## Compatibility Boundary

The independently rerun MCP_OMv1 visual gate retains its established failures:

| View | IoU | Threshold |
|---|---:|---:|
| `track:side` | `0.778319` | `0.80` |
| `aero:side` | `0.773697` | `0.80` |
| `crossover:side` | `0.658835` | `0.80` |

All nine other MCP_OMv1 views pass. A controlled experiment removing the old
decorative sphere envelopes reduced both front and side scores, so it was
reverted and the prior generated grammars were restored. Correcting this
baseline requires a separate MCP_OMv1 realization/reference reconciliation; it
is not an MCSMv2.1 regression.

## Numerical Reproducibility Note

MCSMv2.0.1 regeneration under Python `3.14.6`, NumPy `2.5.1`, scikit-image
`0.26.0`, and trimesh `5.0.0` differs from the signed release only at floating
point noise scale. The repaired parity gate keeps all stable artifacts
byte-exact and additionally requires exact STL topology plus numerical equality
within `1e-12`. Observed JSON drift is at most `3.552713678800501e-15`; observed
vertex drift is at most `7.828702014997871e-19 m`.

MCSMv2.1 remains V2 concept geometry. This verification does not claim moving
closures or glazing, production Class-A surfaces, manufacturing feasibility,
structural performance, CFD, crashworthiness, homologation, or production
approval.
