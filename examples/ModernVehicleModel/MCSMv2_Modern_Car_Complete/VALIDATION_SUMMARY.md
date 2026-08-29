# MCSMv2 Validation Summary

| Variant | Body watertight | Parameter DAG | Occupants | Wheel envelopes | BIW connected | Result |
|---|---:|---:|---:|---:|---:|---:|
| Research Median AWD Hot Hatch | True | True | True | True | True | PASS |
| Track Widebody AWD | True | True | True | True | True | PASS |
| Aero Fastback AWD | True | True | True | True | True | PASS |
| Urban Crossover EV AWD | True | True | True | True | True | PASS |

The body mesh is a validated concept-level hybrid model. Class-A, CFD, crash and production manufacturing approval are outside this implementation.

## Native ProGen3D validation

- Four editor-safe bodies are watertight, manifold, grounded, outward-oriented,
  deterministic, and hash-identical through direct service and grammar paths.
- Fourteen semantic curves, the semantic surface, eleven panel patches, six
  panel relationships, twenty-two BIW members, twenty-five BIW joints, eight
  occupant envelopes, and two or three functional envelopes validate per variant.
- All twelve front/side/top silhouette comparisons pass; the minimum IoU is
  `0.967752` against a required `0.80`.
- MVP1, MVP2, MVP2.5, MVP2.6, MCP_OMv1 native, and MCP_OMv1 grammar gates pass.
- Cross-version release certification remains open only because the inherited
  MCP_OMv1 track, aero, and crossover side-view visual baselines remain below
  `0.80`.

Full evidence: `MCSMv2_NATIVE_IMPLEMENTATION_VERIFICATION.md`.
