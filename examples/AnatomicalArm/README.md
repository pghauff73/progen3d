# Milestone A: Anatomical Arm Geometry and Kinematics

This milestone defines a reproducible, dependency-free reference implementation for a parametric humerus, radius, ulna, muscle bellies and a **single continuous arm skin surface** through elbow flexion. It is an interchange/reference generator, **not** a native ProGen3D grammar operator or a physics/skin FEM solver.

## Run
```sh
python3 examples/AnatomicalArm/arm_milestone_a.py --angle 90 --out /tmp/arm90.obj
python3 examples/AnatomicalArm/arm_milestone_a.py --angle 145 --out /tmp/arm145.obj
python3 -m unittest discover -s examples/AnatomicalArm -p 'test_*.py'
```

Coordinate convention: metres; shoulder at (0, 0, 0); elbow at (0, -0.30, 0); flexion occurs about local Z. At zero flexion, forearm extends down along negative Y; at positive flexion the wrist moves toward +X. Elbow and wrist landmarks are computed from the same transform.

The skin is sampled as one longitudinal swept mesh, with interpolation of ring frames across the elbow and an anatomically motivated bulge/fold profile. Muscle surfaces are separate and adjust volume approximately with contraction. Bone geometry is simplified elliptical shafts and **not** anatomically faithful joint articular surfaces.

## Validation
- Verify bone lengths remain invariant under flexion.
- Verify skin vertex count, triangle indices, seam continuity, and positive radius.
- Verify 0, 45, 90, 120 and 145 degree poses.
- Verify continuous elbow ring topology without a wrist/forearm gap.

## Next
Milestone B: skin sliding, tissue elasticity and attachment maps. Milestone C: close-fitting garment with cloth contact and wrinkles. Neither is implemented here.

## Milestones B/C: reduced-order skin and sleeve prototype

The `arm_soft_sleeve.py` reference implementation extends the original arm with a denser continuous skin surface, a separate fitted sleeve mesh, elbow-local skin creasing, and compression-side sleeve folds. It preserves the Milestone A bones and muscle geometry. Sleeve and skin are independent objects with a bounded radial gap; this is **not** a physically solved cloth/contact simulation.

Generate six poses and a machine-readable validation report:

```sh
python3 examples/AnatomicalArm/arm_soft_sleeve.py --out build/anatomical-arm-sleeve
python3 -m unittest discover -s examples/AnatomicalArm -p 'test_*.py' -v
```

Each OBJ includes named objects for skin, sleeve, humerus, radius, ulna, biceps and triceps. The validation checks landmark lengths, finite vertices, triangle indices, ring counts and radial skin-to-sleeve clearance. The nominal 5% garment circumferential prestrain is metadata; it is not yet enforced by a strain-energy solver.

**Limitations:** the elbow centerline remains piecewise linear; interpolation of normals is smoothed, but this does not guarantee a globally non-self-intersecting surface at extreme flexion. Tissue material laws, frictional sliding, external contact, shell bending energy, volume preservation, collision response, mesh interchange to the native renderer and anatomical accuracy require further work. The tests are committed but must be executed in a Python environment before claiming a validated release.

## Experimental XPBD cloth constraints

`xpbd_cloth.py` now supplies a dependency-free iterative distance-constraint solver, pinning, damping and a generic collision-projection hook. Its tests cover edge deduplication, anchored vertices, constraint convergence and a spherical obstacle. Run:

```sh
python3 -m unittest discover -s examples/AnatomicalArm -p 'test_*.py' -v
```

This is a **physics foundation**, not the completed coupled sleeve. The garment currently uses prescribed surface geometry. Integration with elbow-driven skin contact, moving attachment constraints, bending resistance, calibrated cloth material, continuous-time simulation and native ProGen3D rendering remains outstanding. Do not interpret the presence of the XPBD module as evidence that the sleeve already simulates frictional sliding or skin contact.
