# MCSMv2.0.1 Audit Closure

| ID | Original problem | Status | Resolution or next gate |
|---|---|---|---|
| A01 | blanket or incorrect parameter provenance | `closed` | parameter-specific status, source, confidence, uncertainty, unit and value type |
| A02 | implicit or conflicting coordinate conventions | `closed` | explicit right-handed X-front/Y-right/Z-up frame and stored Progen3D adapter matrix |
| A03 | overhangs and hidden Python inputs outside the DAG | `closed` | 70+ node typed causal graph including package, platform, style, suspension, powertrain and closure inputs |
| A04 | curve-network residual assigned as zero | `closed` | 429 evaluated curve/section samples with measured mean, RMS, p95 and maximum residual |
| A05 | post-mesh affine correction desynchronizing geometry layers | `closed` | iterative pre-tessellation field calibration; no final affine body rescale |
| A06 | semantic section fields cross between stations | `closed` | positive log-gap interpolation and 4,001-sample continuous-domain audit |
| A07 | mesh validity omitted winding and self-intersection evidence | `closed` | final-mesh winding, manifold, component, duplicate, degenerate and local self-intersection checks |
| A08 | occupant and tyre validation reused generating fields | `closed_for_static_evidence` | independent final-triangle ray-parity/distance tests and sampled steer/travel tyre surfaces |
| A09 | single PASS overstated assurance | `closed` | V0-V5 record; release claims V2 and explicitly leaves V3-V5 false |
| A10 | declared wheelhouse clearance was not part of the pass criterion | `closed` | minimum final-mesh tyre distance must meet declared clearance within 3 mm tessellation tolerance |
| A11 | semantic surface and implicit scaffold disagreement | `deferred_to_2_1` | diagnostic retained; authoritative-surface convergence gate not yet implemented |
| A12 | overlapping/uncovered panel masks | `deferred_to_2_1` | future exclusive UV-domain panel partition |
| A13 | BIW graph connectivity without geometric joint proof | `deferred_to_structural_stage` | future typed geometric interfaces and load-path analysis |
| A14 | inverse-fit non-identifiability | `deferred_to_fitting_stage` | future overdetermined observations, Jacobian/SVD conditioning, priors and uncertainty |

`closed_for_static_evidence` means the original circular static test was replaced
with an independent final-mesh test, while exact continuous kinematics remain a
V3 non-claim. Deferred items do not block V2 because they belong to later
assurance levels and are listed in `KNOWN_LIMITATIONS.md`.
