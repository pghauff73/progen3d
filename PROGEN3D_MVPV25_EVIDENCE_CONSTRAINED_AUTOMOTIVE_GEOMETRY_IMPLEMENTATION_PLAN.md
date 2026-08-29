# ProGen3D MVPv2.5 Evidence-Constrained Automotive Geometry Implementation Plan

## 1. Objective

Promote the red AWD hatchback from the MVPv2 semantic fitting model to an evidence-constrained automotive geometry model:

```text
calibrated reference views
→ automotive reference and package
→ semantic vehicle landmarks
→ topology-preserving automotive wireframe
→ observed character curves
→ Class-A patch graph
→ rolled panel gaps, apertures, and closures
→ helical side-glass motion
→ body-in-white structure
→ suspension hardpoints and solved wheel poses
→ swept tyre and closure envelopes
→ weighted hard/soft validation objective
```

The canonical aggregate is:

```text
MVPv2.5 = (E, R, P, W, C, S, A, K, B, F, V)
```

where evidence, reference, package, wireframe, curves, surfaces, apertures, kinematics, BIW, functional assemblies, and residual validation remain explicit object-model responsibilities.

## 2. Compatibility Boundary

MVPv2.5 extends rather than deletes the current implementation.

- MVGv1 body and assembly builders remain legal.
- MVPv2 `VehicleFittingArchitecture` remains available as the compatibility semantic layer.
- `ModernVehicleAssembly` receives an optional immutable MVPv2.5 architecture and evidence hash.
- Existing preview/export geometry remains the current visual lowering.
- New automotive-native model objects become the authoritative fitting, kinematic, package, and validation evidence.

## 3. Requirement Map

### Reference and package

Implement:

- `VehicleReferenceFrame`;
- longitudinal, lateral, and vertical axes;
- zero planes and ground plane;
- fiducial datums;
- hard package dimensions for length, width, height, wheelbase, track, overhang, wheel centres, and drive layout;
- `OccupantPackage` with H-point, eye point, heel point, knee, head, torso, and reach geometry.

### Evidence and fitting

Implement:

- calibrated `VehicleCameraModel`;
- `VehicleViewObservation` for front, rear, left, right, and top;
- `VehicleLandmark` with observations, confidence, symmetry, topology role, and provenance;
- projection/reprojection services;
- `SilhouetteConstraint` and deterministic projected silhouette residuals;
- observed character-line samples and residuals.

### Automotive wireframe and prior

Implement:

- `AutomotiveWireframe` as a fixed semantic topology graph;
- typed wireframe edges for centre spine, roof, belt, shoulder, rocker, wheel, arch, glazing, aperture, and bumper relationships;
- `VehicleShapePrior` with class and soft weight;
- hard/soft constraint classification that prevents prior evidence overriding package evidence.

### Class-A surface model

Implement:

- `AutomotiveCharacterCurve` with degree, control points, observations, and continuity targets;
- `ClassASurfacePatch` with four boundaries, guides, degrees, spans, and control grid;
- `ClassASurfaceGraph`;
- edge-specific `PatchConnection` values for G0, G1, G2, G3, crease, panel gap, and trimmed boundaries;
- patch complexity counts and penalties;
- `HighlightFlowValidator` using sampled boundary position, tangent, curvature, and normal-flow residuals.

### Closure geometry

Implement:

- `AutomotivePanelGap` with variable width and primary/secondary radii;
- primary and secondary flanges, continuity, and optional closeout;
- `RolledEdge`;
- `BodySideAperture` with pillars, roof rail, rocker, dogleg, B-line, J-surface, glass surface, flange, and seal seat;
- `DoorEgressSurface`;
- `AutomotiveClosure` shared by door, bonnet, hatch, and fuel flap;
- `ClosureHingeStudy` with derived axis, rise curve, opening range, and swept volume;
- hatch child-transform ownership for rear glass, spoiler, wiper, and brake lamp.

### Glass and seals

Implement:

- `SideGlassSurface`;
- `BarrelSurface`;
- `HelicalGlassDrop` with axis, pitch, rotation rate, limits, channels, and door cavity;
- generated `GlassChannel` geometry specifications;
- deterministic helical transforms;
- cavity/collision sampling across the full state interval;
- `VariableSealSweep` with variable section stations and compression/contact targets.

### Body-in-white

Implement:

- complete `BodyInWhite` member catalog for rails, pillars, firewall, toe pan, floor, rockers, crossmembers, and rear structure;
- `AutomotiveStructuralMember` as a variable-section/thickness sweep specification;
- holes, reinforcements, material, and provenance;
- explicit `BIWJoint` with member references, overlap surfaces, joining method, and local reinforcement;
- structural connectivity validation.

### Chassis, wheels, and aero

Implement:

- `SuspensionHardpoint` and typed hardpoint roles;
- `KinematicLink` between hardpoints;
- per-corner hardpoint model;
- `WheelPoseSolver` for travel and steering state;
- deterministic `WheelSweptEnvelope` over state samples;
- `TyreGeometry` with section, aspect ratio, crown, shoulder, sidewall bulge, tread, and loaded/unloaded radii;
- `AutomotiveRimGeometry` with corner identity, bead-seat/barrel radii, width, spokes, and material;
- `AutomotiveBrakeGeometry` with corner identity, rotor dimensions, caliper bounds, piston count, and material;
- `WheelHouse` clearance validation against the swept tyre envelope;
- external `AeroGeometry` catalog including upper body, wheels, wheel houses, underbody, splitter, cooling openings, diffuser, and spoiler.

### Fit objective and evidence residuals

Implement:

- `VehicleFitObjective`;
- weighted landmark, silhouette, character-line, package, gap, kinematic, Class-A, prior, and complexity terms;
- explicit hard and soft constraints;
- finite deterministic residual report;
- failure when any hard constraint exceeds tolerance;
- objective hash and reproducibility evidence.

## 4. Object-Model Structure

```text
VehicleMvp25Architecture
├── VehicleReferenceFrame
├── VehiclePackageEvidence
├── OccupantPackage
├── VehicleObservationSet
├── AutomotiveWireframe
├── VehicleShapePrior
├── AutomotiveCharacterCurveNetwork
├── ClassASurfaceGraph
├── AutomotiveClosureGeometry
├── BodyInWhite
├── VehicleChassisGeometry
├── AeroGeometry
├── VehicleFitObjective
└── VehicleFitResidualReport
```

The model uses composition for owned subsystems and association identifiers for relationships. Inheritance is not required for unrelated geometry types.

## 5. Deterministic Service Layer

### `VehicleProjectionService`

- projects world points through orthographic or perspective camera models;
- calculates per-observation reprojection residuals;
- calculates nearest-polyline silhouette and character-line residuals;
- never consumes random values.

### `VehicleClassAValidationService`

- validates patch references and degrees;
- samples connected patch edges;
- reports G0/G1/G2/G3 residuals;
- reports highlight/normal-flow residuals;
- reports patch, control-point, and span complexity.

### `AutomotiveClosureGeometryService`

- derives hinge axes from hinge points;
- evaluates closure transform and rise;
- evaluates helical glass translation and rotation;
- calculates deterministic closure and glass swept bounds;
- validates glass against its door cavity;
- evaluates variable seal sections.

### `VehicleChassisKinematicsService`

- resolves wheel pose from hardpoint geometry, travel, and steering;
- samples complete wheel swept envelopes;
- validates wheel-house clearance;
- keeps state sampling order fixed.

### `VehicleFitObjectiveService`

- aggregates all residual terms;
- applies hard/soft semantics;
- returns total objective and named component evidence;
- does not allow soft prior improvements to hide hard failures.

### `VehicleMvp25ValidationService`

- validates every model and relationship;
- rejects duplicate IDs, missing references, invalid topology, non-finite geometry, invalid intervals, unsupported evidence claims, disconnected BIW structure, invalid hardpoints, failed closure/glass/wheel clearances, and non-finite objective terms.

### `VehicleMvp25DeterministicHashService`

- hashes all evaluated values and ordered relationships;
- excludes addresses and unordered-container iteration;
- produces stable architecture and residual hashes.

## 6. Red AWD Hatchback Acceptance Model

Use hard package evidence:

```text
length      4.350 m
width       1.800 m
height      1.430 m
wheelbase   2.650 m
drive       AWD
```

The acceptance model shall contain:

- one SAE-oriented vehicle reference frame;
- front and rear occupant packages;
- five calibrated observations;
- at least 25 semantic landmarks;
- one five-door hatch topology wireframe;
- roof, hood, belt, shoulder, rocker, fender, haunch, and glazing character curves;
- a minimum 15-patch Class-A graph;
- six rolled closure gaps;
- four body-side apertures;
- four side-door hinge studies;
- bonnet and hatch closures;
- four helical side-glass drops and eight generated channels;
- variable door, bonnet, and hatch seals;
- a connected BIW member/joint graph;
- four suspension hardpoint models;
- four solved wheel-pose functions and swept envelopes;
- four tyre, rim, brake, and wheel-house models;
- upper-body, wheel, underbody, splitter, cooling, diffuser, and spoiler aero components;
- a complete fit objective and residual report.

## 7. Grammar and Editor Lowering

Create an executable current-grammar fixture that mirrors the MVPv2.5 hierarchy through:

- `Object` taxonomy and containment;
- `Interface` evidence for datums, landmarks, hinge points, hardpoints, channels, BIW joints, and wheel-house limits;
- current geometry primitives for visible lowering;
- current `Joint` declarations as editor compatibility projections;
- comments that distinguish native helical/four-bar/hardpoint behavior from compatibility records.

The source must load without parser or execution errors in supervisor mode.

## 8. Validation Matrix

### Reference/package tests

- orthonormal frame;
- correct handedness;
- positive package dimensions;
- wheelbase and track consistency;
- hard package residuals equal zero in the acceptance model.

### Evidence tests

- all five views present;
- cameras finite and invertible;
- landmarks have valid observations;
- projection residuals deterministic;
- changed observation changes objective/hash.

### Class-A tests

- all boundaries resolve;
- patch degrees and grids are valid;
- G0/G1/G2/G3 residuals are finite;
- highlight-flow report is finite;
- complexity penalty matches patch/CV/span counts.

### Closure/glass tests

- doors close at state zero;
- hinge rise is finite;
- all closure swept bounds contain every sample;
- helical glass translates and rotates;
- glass remains within the declared cavity;
- hatch children compose through hatch transform;
- seal sections and compression are valid.

### BIW/chassis tests

- BIW graph is connected;
- all joints reference members;
- all links reference hardpoints;
- wheel pose changes with travel/steering;
- swept envelope contains every sampled tyre bound;
- rims fit the corresponding tyre bead-seat radius;
- brake rotors fit inside rim barrels and calipers remain inside wheel houses;
- wheel house contains swept envelope plus clearance.

### Objective tests

- every term is finite;
- hard failures invalidate the model;
- soft terms cannot override hard failures;
- repeated build produces identical architecture and residual hashes.

### Compatibility and visual tests

- MVGv1 suite passes;
- MVPv2 suite passes;
- MVPv2.5 suite passes;
- full application builds;
- front, back, left, right, and top captures are distinct and upright;
- grammar execution logs contain no errors.

## 9. File-Level Implementation

```text
include/vehicle/model/
    VehicleMvp25Reference.h
    VehicleMvp25Evidence.h
    AutomotiveWireframe.h
    VehicleClassASurface.h
    VehicleClosureGeometry25.h
    VehicleBodyInWhite.h
    VehicleChassisMvp25.h
    VehicleFitObjective.h
    VehicleMvp25Architecture.h

include/vehicle/service/
    VehicleProjectionService.h
    VehicleClassAValidationService.h
    AutomotiveClosureGeometryService.h
    VehicleChassisKinematicsService.h
    VehicleFitObjectiveService.h
    VehicleMvp25ValidationService.h
    VehicleMvp25DeterministicHashService.h
    RedAwdHatchbackMvp25Builder.h

src/vehicle/service/
    corresponding implementations

docs/MVGv2_5.md
examples/MVPv25_Red_AWD_Hatchback_EvidenceConstrained.p3d
tests/mvpv25_hatchback_harness.cpp
tests/run_mvpv25_hatchback_checks.sh
tests/run_mvpv25_hatchback_visual_checks.sh
```

## 10. Definition of Complete

MVPv2.5 is complete when every named primitive in the specification exists as a purpose-revealing model class, every required relationship is validated, deterministic projection/Class-A/closure/glass/BIW/chassis/objective services operate on the red hatchback acceptance fixture, existing MVGv1 and MVPv2 gates remain green, the current-grammar lowering executes in supervisor mode, and five-view evidence proves a correctly oriented vehicle.

The implementation must preserve the central rule:

```text
Evidence and hard package constraints determine the vehicle.
Shape priors and aesthetic preferences may regularize it, but may not contradict it.
```
