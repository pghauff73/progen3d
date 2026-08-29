# MCSMv2.0.1 Known Limitations

MCSMv2.0.1 is an integrity release for concept-level geometry. The following
boundaries are deliberately visible rather than varnished over.

## Deferred to MCSMv2.1

- The explicit semantic surface and implicit scaffold are both generated, but
  their agreement is diagnostic rather than constrained by a release gate.
- Panel regions remain semantic triangle masks. They are not yet an exclusive,
  gap-free partition of a common UV surface domain.
- Glass regions are not yet derived from explicit apertures and support
  surfaces.

## Deferred to the kinematic stage

- Door, bonnet and rear-hatch meshes do not yet execute full opening sweeps.
- Side glass does not yet execute a helical barrel-surface drop.
- Suspension is represented by conservative wheel-pose envelopes rather than a
  solved hardpoint linkage.
- The tyre sweep is sampled, not a continuous exact swept-volume proof.

## Deferred to structural and manufacturing stages

- The BIW graph proves declared graph connectivity, not geometric joint contact,
  flange overlap, weld accessibility, stiffness or load-path adequacy.
- Sheet thicknesses, materials, stamping draw limits, hems, flanges and joining
  tolerances are not production-validated.
- The exterior-envelope volume is not vehicle mass, cabin volume or sheet-metal
  volume.

## Deferred to physical validation

- No CFD-derived drag/lift claim.
- No crash, thermal, durability or noise-vibration-harshness analysis.
- No regulated visibility, ergonomics, homologation or physical prototype test.

## Inverse fitting

The included inverse fit is a synthetic semantic-metric demonstration. A near
zero metric residual does not prove parameter identifiability. Image-calibrated
multi-view fitting, Jacobian conditioning, uncertainty intervals and held-out
observations remain future work.

## Legacy boundary

MCSMv2.0.1 still reuses selected MCSMv1 utilities through an explicit legacy
module. Replacement with fully native MCSMv2 services is staged, not silently
claimed complete.
