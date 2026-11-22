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
