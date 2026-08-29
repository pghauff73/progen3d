# MCSMv2 2.0.1 Integrity Patch Changelog

## Basis

This release implements the MCSMv2.0.1 integrity work identified in the
MCSMv2.0.0 problem audit. It preserves the MCSMv2 hybrid semantic-section and
implicit-scaffold architecture while making the evidence, coordinate, meshing,
and assurance boundaries explicit.

## Implemented

1. **Parameter-specific provenance**
   - Removed blanket package-source attribution from steering, suspension,
     wheelhouse, style, powertrain-envelope, and closure parameters.
   - Every dependency-graph node now carries status, source, confidence,
     uncertainty, unit, and value type.

2. **Explicit coordinate and unit system**
   - Added a right-handed vehicle frame: X front, Y right, Z up.
   - Added the deterministic adapter to the Progen3D convention: X right,
     Y up, Z front.

3. **Complete causal DAG inputs**
   - Promoted front and rear overhangs and previously captured Python values to
     explicit graph nodes.
   - Added platform, style, wheel/suspension, powertrain, closure, categorical,
     count, and Boolean inputs.
   - Added package-length closure residual.

4. **Continuous semantic-section integrity**
   - Replaced independent absolute-height interpolation with positive log-gap
     fields.
   - Added a dense 4,001-sample continuous-domain order audit.

5. **Measured curve-network consistency**
   - Replaced the assigned zero residual with an evaluated longitudinal-curve
     versus transverse-section residual.

6. **No post-mesh affine correction**
   - Replaced final-body rescaling with iterative inverse scalar-field prewarp.
   - The final exported body remains in the same vehicle frame as curves,
     datums, BIW, occupants, and functional envelopes.
   - Numerical marching-cubes islands are reported and only microscopic
     artifacts are discarded.

7. **Independent final-mesh checks**
   - Occupant containment is evaluated against the final body mesh.
   - Sampled tyre surfaces are swept through steer and suspension states and
     checked against the final mesh rather than the generating wheelhouse field.

8. **Mesh-integrity checks**
   - Added finite, watertight, winding, positive-volume, duplicate-face,
     degenerate-face, boundary-edge, non-manifold-edge, connected-component,
     and local triangle self-intersection checks.

9. **Assurance levels**
   - Replaced a single unqualified PASS claim with V0-V5 assurance records.
   - This release reaches V2 concept geometry for all four generated variants.
   - V3 closure/glass/exact suspension kinematics, V4 manufacturing/structural
     analysis, and V5 physical validation remain unclaimed.

## Compatibility

The public MCSMv2 generator entry point, four variant keys, primary output names,
and prior unit tests remain supported. A dedicated legacy-compatibility test
module is included.

10. **Declared final-mesh wheel clearance**
    - Replaced collision-only pass logic with separate collision-free and
      clearance-target gates.
    - Uses a conservative steer/travel cavity construction and a 3 mm
      tessellation tolerance.
