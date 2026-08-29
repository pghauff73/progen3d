# MVPv2 Hatchback Fitting Verification

Verification date: **August 22, 2026**

## Implemented Scope

- semantic five-view envelope and weighted silhouette observations;
- semantic vehicle cross-sections and surface landmarks;
- character-curve and surface-patch relationship graphs;
- projection-constrained patch records;
- seam loops, extracted panels, and apertures;
- four side-door closures, bonnet closure, and rear-hatch closure;
- hinge pairs, four-bar relationships, guide rails, belt seals, and telescoping links;
- door-local drop-glass dependency;
- normalized closure state and deterministic swept AABB evidence;
- fail-closed semantic validation and deterministic fitting hash;
- executable editor grammar lowering and five-view supervisor evidence.

## Build Gate

```bash
make -j2
```

Result: **passed**. Rebuilt `progen3d-editor-gui` with the MVPv2 vehicle sources.

Binary SHA-256 after the verified build:

```text
521addbaffb467f4846922f25cd1708eb57a71f67ac13d28086cff74b2f290fa
```

## Native MVPv2 Gate

```bash
./tests/run_mvpv2_hatchback_fitting_checks.sh
```

Result:

```text
MVPv2 hatchback fitting passed:
views=5
sections=7
landmarks=14
curves=18
patches=13
closures=6
drop_glass=4
fitting_hash=7109851553845307852
```

The harness also verifies:

- repeat builds have the same fitting hash;
- hinge axes derive from two points;
- closed door state is identity;
- open door state moves the closure;
- glass world motion composes through the owning door;
- swept bounds use 17 deterministic samples;
- invalid closure state fails validation.

## MVGv1 Compatibility Gate

```bash
./tests/run_mvg_vehicle_checks.sh
```

Result:

```text
MVGv1 acceptance vehicle passed:
assemblies=9
joints=16
vertices=54042
triangles=18014
hash=10740567470808396435
```

The MVPv1 vehicle remains source-compatible and passes its existing acceptance path.

## Supervisor Visual Gate

```bash
./tests/run_mvpv2_hatchback_visual_checks.sh
```

Result: **passed** for explicit front, back/rear, left, right, and top camera orientations.

The script requires both:

```text
PROGEN3D_GUI_VIEW_PASSED
PROGEN3D_GUI_VISUAL_CAPTURE_PASSED
```

for every view and rejects grammar execution or parser errors.

Evidence:

```text
tests/evidence/mvpv2_red_awd_hatchback/front.ppm
tests/evidence/mvpv2_red_awd_hatchback/rear.ppm
tests/evidence/mvpv2_red_awd_hatchback/left.ppm
tests/evidence/mvpv2_red_awd_hatchback/right.ppm
tests/evidence/mvpv2_red_awd_hatchback/top.ppm
tests/evidence/mvpv2_red_awd_hatchback/contact-sheet.png
tests/evidence/mvpv2_red_awd_hatchback/sha256sums.txt
```

Contact-sheet SHA-256:

```text
0f905d78d7558976f60e29f0b3c7aaf1c2e99f86c767ccf27a11f7a79949ffae
```

Visual inspection confirms:

- vehicle up is consistently `+Y`;
- wheels contact the ground plane;
- front and rear views are distinct;
- left and right views are distinct;
- top view is correctly oriented;
- no upside-down transform is present;
- no grammar hierarchy or container errors remain.

## Source Artifact Hashes

```text
MVPv2 grammar:
4b32fd2e2cbb6c12005535d9c587f4bc9f4c6759d9dcc84f1ceecc32e394476d

Implementation plan:
ad9182652bca08c59309031c3662a3eb33f9548b11ea4da37561ddd09b9778d9

MVGv2 documentation:
7fe97442d9c36f753fdbc226af7636e57b94485fbcfbc5c258ebd532c6432384
```

## Explicit P0 Limits

- Projection constraints are stored and validated but do not yet optimize mesh control points.
- Four-bar motion is a deterministic engineering approximation, not synthesized hardware.
- Swept volumes are sampled broad-phase AABBs, not exact swept solids.
- Current grammar window joints are editor compatibility projections; native motion uses `GuideRailJoint`.
- Exact hidden mechanisms remain classified as engineering inference.
