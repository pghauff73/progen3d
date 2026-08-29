# MCSMv2.2 Audit Closure

## Roadmap item 1: suspension hardpoints

**Implemented at concept level.** Each vehicle exports 32 named hardpoints covering front MacPherson and rear multi-link hypotheses. Validation checks finite coordinates, exact left/right mirroring and rigid wheel-pose transforms.

**Not closed at production level.** Coordinates are derived from package geometry rather than measured vehicle data, and no compliance or link-length closure solution is claimed.

## Roadmap item 2: actual tyre pose sweeps

**Implemented.** MCSMv2.2 creates an elliptical-section tyre mesh, applies 15 front poses and 3 rear poses per side, records 36 poses in total, constructs sweep hulls, and checks sampled tyre points against the final exported body triangles.

**Boundary.** The pose grid is discrete and the hull is conservative. Continuous interval proof is deferred.

## Roadmap item 3: opening doors, bonnet and hatch

**Implemented at semantic outer-surface level.** Four doors, the bonnet and rear hatch have independent hinge systems, state transforms, open/closed scenes and sampled collision checks.

**Boundary.** Closure objects are exterior semantic skins. Inner panels, hems, hardware, seals and production aperture thickness are deferred.

## Roadmap item 4: helical moving side glass

**Implemented at guide-motion level.** Four side-glass systems combine vertical drop, inward shift, longitudinal shift and barrel-following rotation in the parent door frame. Six states per glass are validated.

**Boundary.** No physical regulator, carrier, cable, scissor, motor or measured guide channel is claimed.

## Roadmap item 5: independent collision validation

**Implemented for the V3 concept claim.** Closure and glass checks use final triangle meshes, nearest-surface distance, broad-phase candidate filtering, triangle-intersection screening and cavity containment rather than merely re-evaluating the generating field.

## Assurance conclusion

MCSMv2.2 closes the planned **V3 sampled concept-kinematic** slice. It does not close V4 manufacturing/structural assurance or V5 CFD/crash/physical assurance.
