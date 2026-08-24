# MCSMv2.2 Known Limitations

## Suspension

- Hardpoints are concept-derived from package relationships, not measured production coordinates.
- The wheel-pose solver uses deterministic camber/toe gains rather than a full constrained linkage solution.
- Compliance, bushing deflection, steering-axis geometry, scrub radius, caster trail and force analysis are not implemented.

## Tyre sweeps

- Tyre poses are sampled discretely: five front steering positions and three travel positions, plus three rear travel positions per side.
- Convex hulls are conservative visual sweep envelopes and are not continuous interval proofs.
- The tyre is an elliptical-section torus without loaded contact-patch deformation or tread blocks.

## Closures

- Doors, bonnet and hatch are semantic exterior skins without production inner panels, hems, reinforcements, hinge hardware, latches, seals or gas-strut force models.
- Bonnet motion is a rising-revolute approximation, not a solved four-bar mechanism.
- Rear-hatch outer semantic skin may be multi-component; a manufactured inner frame remains deferred.
- Fixed-body apertures are open semantic surface regions, not thickness-controlled body-in-white apertures.

## Glass

- Side-glass motion approximates a helical/barrel path with drop, inward shift, longitudinal shift and small rotation.
- No regulator, carrier, motor, cable, scissor mechanism, channel compliance or weatherstrip deformation is modeled.

## Surfaces and manufacturing

- Semantic/implicit alignment is tessellation-level and is not production Class-A NURBS approval.
- Panel ownership is semantic; tooling split, draw direction, draft, springback, hemming and stamping feasibility are not evaluated.

## Higher assurance

- V4 manufacturing and structural engineering are not claimed.
- V5 CFD, cooling, crash, thermal, ergonomic, homologation and physical testing are not claimed.
