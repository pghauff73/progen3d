# MCSMv2.1 to MCSMv2.2 Changelog

MCSMv2.2 is the concept-kinematics release built on the MCSMv2.1 semantic UV surface and the MCSMv2.0.1 integrity kernel.

## Added

- 32 concept suspension hardpoints per vehicle, with mirrored left/right evidence.
- Deterministic front MacPherson and rear multi-link wheel-pose models.
- Steer, jounce, rebound, camber and toe state records.
- Elliptical-section tyre meshes transformed through sampled wheel poses.
- Convex-hull tyre sweep envelopes and independent final-body distance/parity checks.
- Four side-door hinges, bonnet hinge approximation and rear-hatch hinge system.
- Sampled closure sweep validation against the independently retained fixed-body surface.
- Parent-relative helical/barrel side-glass motion for four moving windows.
- Door-cavity containment, fixed-body distance and intersection checks for moving glass.
- Closed, open, suspension and engineering GLB scenes for every variant.
- V3 concept-kinematic assurance and an explicit V4/V5 boundary.

## Carried forward from MCSMv2.1

- persistent semantic UV coordinates;
- radial projection to the implicit outer scaffold;
- independent bidirectional semantic/implicit alignment;
- exclusive surface-domain ownership;
- 16 named panel domains, a fixed-body ledger and six glass apertures.

## Changed

- The wheel gate now includes actual transformed tyre geometry in addition to the older analytic wheel-envelope check.
- The final scene separates fixed body, movable closure surfaces, glass support surfaces, suspension hardpoints and sweep hulls.
- Assurance is promoted from V2 to V3 only when suspension, tyre, closure and glass checks all pass.

## Deferred

- measured production suspension hardpoints;
- exact multi-link constraint solving and compliance;
- production four-bar bonnet geometry;
- inner closure panels, hems, reinforcements, hinges, latches and seals;
- production glass guides and regulator hardware;
- manufacturing, structural FEA, CFD, crash, homologation and physical validation.
