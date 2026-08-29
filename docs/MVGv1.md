# MVGv1 Modern Motor Vehicle Grammar

MVGv1 introduces a purpose-driven vehicle object model on top of Progen3D's
shared procedural geometry kernel. The implemented vertical slice follows the
canonical coordinate convention:

```text
+X vehicle right
+Y vehicle up
+Z vehicle forward
origin at the ground projection of the front axle centre
rear axle Z = -wheelbase
```

## Implemented Vertical Slice

- `VehicleIntent`, `VehiclePackage`, package envelopes, and fail-closed package validation.
- Fourteen derived vehicle datums, including axle planes and wheel centres.
- Deterministic `Curve3D` evaluation for line, polyline, Bezier, and Catmull-Rom curves.
- Seven mirrored body sections and three semantic longitudinal guide curves.
- A finite-thickness body generated through the existing `ShellLoft` geometry family.
- Open `SurfaceLoft` patches with transactional `ShellOffset` realization for thin trim and roof forms.
- `CurvedPanel` glazing, semantic `PanelCut` reveals, and reusable wheel-arch edge sweeps.
- Recursive `MirrorShape` reflection with corrected winding for bilateral assemblies.
- Reusable wheel assemblies with revolved tire, rim, hub, brake disc, radial spokes, caliper, and optional fasteners.
- Four semantic suspension corners with upright, control arm, damper, spring, and front tie rod geometry.
- A high-level `Joint` declaration parsed into a validated `VehicleJointGraph`, with revolute, prismatic, fixed, and ball joint contracts.
- Revolute and prismatic joints for wheel rotation, steering, suspension travel, doors, hood, and tailgate.
- LOD-aware geometry while semantic sections, panel cuts, arches, and joints remain present.
- Whole-vehicle validation, deterministic hashing, and PLY export.

The implementation deliberately uses:

```text
ShapeSpecification
-> Loft / ShellLoft / SurfaceLoft / ShellOffset / MirrorShape
-> CurvedPanel / PanelCut / SweepProfile / SweepDisk / Revolve / ExtrudeProfile
-> GeneratedPrimitiveMesh
-> VehicleAssemblyGeometry
```

It does not introduce a second vehicle-only mesh engine.

## Executable MVPv1 Grammar Bridge

The grammar exposes a high-level `VehicleBodyShell` descriptor for authored
vehicle transverse stations:

```text
VehicleBodyShell(
    thickness(0.045)
    section(stationZ halfWidth bottomY shoulderY beltY roofY greenhouseHalfWidth)
    section(...)
    cap(all)
    detail(LOD3))
```

The evaluator converts every semantic station into matching symmetric outer
and inner `Profile2D` loops, validates the complete candidate, and returns a
canonical `ShellLoftShapeSpecification`. Grammar code therefore declares the
vehicle form but never constructs mesh vertices.

`examples/MVPv1_Red_Sporty_Hatchback_Advanced.p3d` exercises this bridge with:

- `VehicleBodyShell -> ShellLoft` for the principal painted body;
- `SurfaceLoft -> ShellOffset` for the thin finite-thickness roof trim;
- `CurvedPanel` for independently tessellated front and rear glazing;
- `PanelCut` and `SweepDisk` for semantic panel seams and trim paths;
- `MirrorShape` for one-source bilateral mirror housings;
- `Revolve` for tire, rim, brake-disc, and hub annuli;
- `InstanceArray` for deterministic radial wheel spokes;
- `Joint` for four wheel rotations plus hood and tailgate hinges;
- separate side glazing, semantic object ownership, and four GL46 scene lights.

Run its focused source, placement, export, and determinism gate with:

```bash
./tests/run_mvpv1_red_hatchback_grammar_checks.sh
```

## Acceptance Vehicle

`MVG_Test_EV_001` is a generic four-door, five-seat, all-wheel-drive EV
fastback. It is not based on a specific production vehicle.

```text
length              4.75 m
width               1.90 m
height              1.45 m
wheelbase           2.90 m
front track         1.64 m
rear track          1.63 m
ground clearance    0.15 m
wheel diameter      0.72 m
```

Run its acceptance gate with:

```bash
./tests/run_mvg_vehicle_checks.sh
```

The harness verifies geometry finiteness, bilateral body symmetry, datum
alignment, nominal wheel-arch clearance, steering at plus/minus 30 degrees,
suspension travel, stable hierarchy and mesh hashes, LOD reduction, and PLY
export.

## Deferred MVG Work

The completed vertical slice intentionally keeps the following as later milestones:

- physically extracted door, hood, and tailgate skins;
- detailed occupant, seat, dashboard, battery, e-drive, and underbody assemblies;
- exact swept-volume collision against the body triangle mesh;
- editor panels for package, section, guide, continuity, and joint inspection;
- automatic extraction of authored body-surface patches into independently movable closures;
- broader semantic grammars for complete interior and mechanical assemblies.

These extensions can reuse the datums, interfaces, joints, LOD policy, and
procedural geometry services introduced by this implementation.
