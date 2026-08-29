# MCSMv2.0.1 Implementation Report

## Integrity patch delivered

MCSMv2.0.1 retains the MCSMv2 hybrid semantic-section and implicit-scaffold
architecture while repairing the integrity weaknesses identified in the
MCSMv2 problem audit.

Implemented changes:

1. exact per-parameter provenance rather than blanket package attribution;
2. explicit right-handed vehicle reference frame, units and Progen3D adapter;
3. a 70+ node dependency graph containing overhangs, style, platform, wheel,
   suspension, powertrain and closure inputs;
4. non-crossing continuous vertical section fields based on positive log gaps;
5. measured curve-network intersection residuals;
6. iterative pre-tessellation scalar-field calibration with no post-mesh affine correction;
7. removal and reporting of microscopic marching-cubes islands;
8. independent final-mesh occupant and sampled tyre-sweep checks;
9. winding, manifold, duplicate, degenerate and local self-intersection checks;
10. V0-V5 assurance reporting instead of a single unqualified PASS.

## Generated release summary

| Variant | Assurance | Release gate | Max bound error mm | Curve residual max mm | Self intersections | Tyre sweep | Body faces |
|---|---:|---:|---:|---:|---:|---:|---:|
| Research Median AWD Hot Hatch | V2 | True | 0.766 | 7.053 | 0 | True | 105,312 |
| Track Widebody AWD | V2 | True | 0.784 | 6.393 | 0 | True | 104,612 |
| Aero Fastback AWD | V2 | True | 0.662 | 6.962 | 0 | True | 104,600 |
| Urban Crossover EV AWD | V2 | True | 1.235 | 7.526 | 0 | True | 104,092 |

## Inverse fitting demonstration

- Initial residual norm: `1.844761`
- Final residual norm: `0.000000`
- Solver success: `True`
- Evaluations: `9`

The inverse fit remains a synthetic semantic-measurement demonstration. A low
metric residual is not treated as proof that all generating parameters are
identifiable.

## Assurance boundary

A released variant can achieve **V2 concept geometry** in this version:

- V0: finite framed parameter model and acyclic dependency graph;
- V1: watertight, consistently wound, positive-volume, non-self-intersecting body;
- V2: package bounds, continuous semantic sections, curve network, datums and
  BIW graph are internally consistent.

Static occupant and tyre sweeps are independently checked, but V3 is not claimed
because door, hatch, bonnet, glass and exact suspension kinematics remain
deferred. V4 manufacturing/structural analysis and V5 CFD/crash/physical
validation are also outside this release.
