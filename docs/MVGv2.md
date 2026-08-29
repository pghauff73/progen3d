# MVGv2 Semantic Hatchback Fitting

MVGv2 adds an evidence-bearing vehicle fitting and closure model beside the existing MVGv1 geometry builders.

## Construction Model

```text
Reference views
→ MultiViewEnvelope
→ VehicleCrossSection and SurfaceLandmark
→ CharacterCurveNetwork
→ SurfacePatchGraph
→ PanelSeamLoop and ExtractedSurfacePanel
→ Aperture and ClosureAssembly
→ VehicleClosureState and swept-volume evidence
```

The current implementation intentionally separates semantic fitting from rendering geometry. Existing FGKv1 shapes remain the executable lowering used by the editor preview and export path.

## Native Model Classes

The native C++ object model includes:

- `MultiViewEnvelope` and `VehicleViewEnvelope`;
- `SurfaceLandmark` with per-view observations and evidence classification;
- `VehicleCrossSection` with named roof, glass, belt, shoulder, door, rocker, underbody, and floor landmarks;
- `CharacterCurveNetwork` and typed curve relationships;
- `BoundaryPatch`, `ProjectionConstrainedPatch`, and `SurfacePatchGraph`;
- `PanelSeamLoop`, `ExtractedSurfacePanel`, and `Aperture`;
- `ClosureAssembly` for doors, bonnet, hatch, and fuel flap;
- `HingePair`, `FourBarJoint`, `GuideRailJoint`, and `TelescopingLink`;
- `DropGlassAssembly`, `BeltSeal`, and door-inner-volume evidence;
- `VehicleClosureState` and sampled `SweptVolume` bounds.

## Evidence Classification

Every mechanism or landmark can be classified as:

```text
Observed
Triangulated
EngineeringInference
```

Hidden four-bar hinges, guide rails, regulators, and support links must not be classified as directly observed. The validator fails closed when hidden hardware is given unsupported provenance.

## Kinematic Semantics

Door hinge axes derive from lower and upper three-dimensional hinge points. Bonnet and hatch assemblies use deterministic four-bar approximations. Moving glass evaluates front and rear guide rails in door-local coordinates:

```text
T_world_glass = T_world_door * T_door_glass(q)
```

Every state value is normalized to `[0,1]`. Swept volumes are deterministic broad-phase AABBs sampled over an ordered state sequence; they are not exact swept solids.

## Acceptance Fixture

`RedAwdHatchbackMvpv2Builder` attaches an immutable `VehicleFittingArchitecture` to the existing acceptance vehicle. It contains:

- 5 reference views;
- 7 semantic cross-sections;
- 14 surface landmarks;
- 18 character curves;
- 13 projection-constrained surface patches;
- 6 seam loops and extracted outer panels;
- 6 apertures;
- 4 side-door closures;
- 1 bonnet closure;
- 1 rear-hatch closure;
- 4 door-owned drop-glass assemblies.

Run the semantic and kinematic gate:

```bash
./tests/run_mvpv2_hatchback_fitting_checks.sh
```

Run the five-view editor/supervisor visual matrix:

```bash
./tests/run_mvpv2_hatchback_visual_checks.sh
```

## Grammar Lowering

`examples/MVPv2_Red_AWD_Hatchback_SemanticClosures.p3d` is the current executable lowering. It uses existing `VehicleBodyShell`, `SurfaceLoft`, `ShellOffset`, `CurvedPanel`, `PanelCut`, `SweepDisk`, `Revolve`, `InstanceArray`, `Object`, `Interface`, `Position`, and `Joint` grammar constructs.

The grammar declares:

- two hinge-point interfaces for every side door;
- body and closure mounts for bonnet and hatch evidence;
- door-contained glass objects with front and rear rail interfaces;
- compatibility `Joint` records for current editor inspection.

The prismatic window records in the grammar are compatibility state projections only. Native MVGv2 glass motion is calculated by `GuideRailJoint` from two curved rails.

## Current Limit

P0 stores and validates projection constraints but does not yet optimize render-mesh control points against image silhouettes. Production Class-A patch fitting, UV-domain panel extraction, exact moving collision meshes, and mechanism synthesis remain later milestones.
