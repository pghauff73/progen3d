# MVGv2.5 Evidence-Constrained Automotive Geometry

MVGv2.5 promotes the ProGen3D vehicle model from semantic closure fitting to an
evidence-constrained automotive geometry architecture. It preserves the MVGv1
rendering builders and the MVPv2 `VehicleFittingArchitecture`, while adding an
immutable native model for reference geometry, package evidence, five-view
observations, semantic wireframes, Class-A patches, rolled gaps, apertures,
closure and glass kinematics, body-in-white structure, suspension hardpoints,
wheel envelopes, aerodynamics, and validation residuals.

## Canonical Model

```text
MVPv2.5 = (E, R, P, W, C, S, A, K, B, F, V)
```

- `E`: calibrated view observations and image-derived evidence;
- `R`: SAE-oriented vehicle reference frame and fiducial datums;
- `P`: hard package dimensions and occupant packages;
- `W`: semantic automotive landmark wireframe and soft shape prior;
- `C`: automotive character-curve network;
- `S`: Class-A surface patch graph and edge-specific continuity;
- `A`: apertures, rolled gaps, closures, helical glass, and seals;
- `K`: suspension hardpoints, wheel-pose functions, and swept envelopes;
- `B`: variable-section body-in-white members and explicit joints;
- `F`: preserved `VehiclePlacedAssembly`/`VehicleJoint` functional graph plus tyres,
  rims, brakes, wheel houses, closures, and aerodynamic assemblies;
- `V`: named hard/soft residuals, validation reports, and deterministic hashes.

The construction order is deliberately automotive-native:

```text
views
→ semantic landmarks
→ automotive wireframe
→ character curves
→ Class-A patch graph
→ rolled gaps and apertures
→ closures

hardpoints
→ solved wheel poses
→ swept tyre envelopes
```

## Reference and Package

`VehicleReferenceFrame` owns the longitudinal, lateral, and vertical axes,
origin planes, ground plane, and fiducial datums. The acceptance hatchback uses
hard design-intent evidence:

```text
length      4.350 m
width       1.800 m
height      1.430 m
wheelbase   2.650 m
front track 1.500 m
rear track  1.500 m
drive       AWD
```

`OccupantPackage` stores the H-point, eye point, heel point, knee envelope,
head envelope, torso line, and reach envelope independently for front and rear
occupants. Package evidence is hard; it cannot be overridden by a stylistic
shape prior.

## Evidence and Projection

The acceptance architecture contains orthographic front, rear, left, right,
and top `VehicleCameraModel` records. Each `VehicleViewObservation` owns its
image identifier, projected silhouette, landmark references, character-line
observations, panel lines, glazing lines, and wheel-ellipse evidence. The
acceptance fixture populates every category in all five views; validation treats
a missing category as invalid view evidence.

`VehicleProjectionService`:

- projects finite world points through orthographic or perspective cameras;
- calculates landmark reprojection residuals;
- calculates nearest-polyline silhouette and character-line residuals;
- consumes no random values.

The red hatchback contains 31 `VehicleLandmark` records. Their fixed semantic
topology forms an `AutomotiveWireframe`; `VehicleShapePrior` remains explicitly
soft evidence.

## Character Curves and Class-A Patches

`AutomotiveCharacterCurve` represents roof, belt, shoulder, rocker, hood,
fender, rear-haunch, and glazing design intent. `ClassASurfacePatch` records its
four boundary curves, guide curves, degrees, spans, and control grid.

The 17-patch acceptance graph uses explicit `PatchConnection` records with:

```text
G0
G1
G2
G3
Crease
PanelGap
Trimmed
```

`HighlightFlowValidator` samples connected patch edges and reports position,
tangent, curvature, and normal-flow residuals. Patch, span, and control-point
counts contribute to a complexity penalty so fitting does not improve by adding
unbounded surface structure.

## Gaps, Apertures, and Closures

`AutomotivePanelGap` stores variable gap width, primary radius, secondary
radius, two flanges, continuity targets, and closeout policy. `RolledEdge`
represents the curved material boundary rather than a drawn seam.

Each `BodySideAperture` owns pillar references, roof rail, rocker, dogleg,
B-line, J-surface, side-glass surface, flange, and seal seat. Doors are derived
from the Class-A outer skin, the aperture, J-surface, and side glass rather than
being independent boxes.

`AutomotiveClosure` is shared by the four side doors, bonnet, and rear hatch.
`ClosureHingeStudy` derives its axis from two hinge points, records the opening
range and rise curve, and owns deterministic swept bounds. The hatch owns rear
glass, spoiler, rear wiper, and high-mounted brake lamp child transforms.

## Helical Glass and Variable Seals

Side glass does not translate straight down. `HelicalGlassDrop` combines:

- a `SideGlassSurface`;
- a `BarrelSurface`;
- a helical axis, pitch, and rotation rate;
- generated front and rear `GlassChannel` records;
- upper and lower limits;
- a door-cavity containment boundary.

`AutomotiveClosureGeometryService` evaluates ordered transforms and swept
bounds throughout the complete state interval. Four glass drops and eight
channels are validated in the acceptance model.

`VariableSealSweep` stores a path, variable section stations, material, and
contact targets. The acceptance fixture has variable seals for all four doors,
the bonnet, and the hatch.

## Body-in-White

`BodyInWhite` contains variable-section `AutomotiveStructuralMember` records
for front rails, A/B/C pillars, roof rails, rockers, firewall, toe pan, floor,
crossmembers, and rear structure. Section stations carry width, height,
thickness, and flange width. Members also retain explicit hole bounds, local
reinforcement identifiers, material, and provenance.

`BIWJoint` references the joined members and overlap surfaces, joining method,
reinforcement, and joint position. Validation rejects missing member references
or a disconnected structural graph. The acceptance model contains 18 members
and 17 explicit joints.

## Suspension, Wheels, and Aerodynamics

Each corner has a `SuspensionHardpointModel` with wheel centre, upper and lower
body mounts, inner and outer tie-rod points, and fixed-length `KinematicLink`
records. `VehicleChassisKinematicsService` is the authoritative
`WheelPoseSolver` and evaluates:

```text
wheelPose = F(travel, steering)
```

Front corners sample five travel values by five steering values. Rear corners
sample five travel values. `WheelSweptEnvelope` must contain every sampled tyre
bound, and each `WheelHouse` must contain that envelope plus clearance.

`TyreGeometry` records section dimensions, radii, shoulder, tread, and sidewall
parameters. Every corner also owns an `AutomotiveRimGeometry` with bead-seat and
barrel radii, width, spoke count, material, and corner identity, plus an
`AutomotiveBrakeGeometry` with rotor dimensions, caliper bounds, piston count,
material, and corner identity. Validation requires each rotor to fit inside its
rim barrel and each caliper to remain inside the corresponding wheel house.
`AeroGeometry` validates upper body, wheels, wheel houses, underbody, splitter,
cooling opening, diffuser, and spoiler as separate roles.

## Fit Objective and Hard/Soft Semantics

`VehicleFitObjective` uses the complete nine-term objective:

```text
landmark
silhouette
character curve
package
gap and closure
kinematic
Class-A
shape prior
complexity
```

Hard package failures invalidate the architecture regardless of the weighted
soft score. Soft prior improvements can never hide a failed hard constraint.
`VehicleFitObjectiveService` returns named residual evidence and a deterministic
weighted total.

## Determinism and Validation

`VehicleMvp25ValidationService` fails closed on invalid frames, missing
references, non-finite evidence, broken topology, unsupported continuity,
invalid closure or glass intervals, disconnected BIW structure, invalid
hardpoints, failed swept-envelope containment, missing aero roles, and invalid
fit terms.

`VehicleMvp25DeterministicHashService` hashes evaluated values and ordered
relationships. It excludes addresses and unordered-container iteration.
Repeated builds of the acceptance model must produce identical architecture
and residual hashes.

## Executable Grammar Lowering

`examples/MVPv25_Red_AWD_Hatchback_EvidenceConstrained.p3d` preserves the
proven visible FGKv1 hatchback geometry and adds executable current-grammar
objects and interfaces for:

- vehicle axes, ground, axle datums, and occupant points;
- five camera observations and silhouette evidence;
- all 31 semantic vehicle landmarks;
- 17 Class-A patch regions and all seven continuity categories;
- body-side apertures, B-lines, J-surfaces, rolled gaps, and seals;
- helical glass barrel axes and eight generated channels;
- 18 BIW member interfaces and representative joint evidence;
- 20 suspension hardpoints and four wheel-house limits;
- eight aerodynamic roles and nine objective terms.

The grammar `Joint` declarations are editor compatibility projections. Native
C++ remains authoritative for helical glass motion, Class-A residuals, closure
rise and swept bounds, BIW connectivity, solved wheel poses, tyre sweeps, and
the deterministic evidence hash.

## Acceptance Commands

Run the native architecture and deterministic validation gate:

```bash
./tests/run_mvpv25_hatchback_checks.sh
```

Run the five-view editor/supervisor visual matrix:

```bash
./tests/run_mvpv25_hatchback_visual_checks.sh
```

Run compatibility gates:

```bash
./tests/run_mvpv2_hatchback_fitting_checks.sh
./tests/run_mvpv2_hatchback_visual_checks.sh
./tests/run_mvpv1_red_hatchback_grammar_checks.sh
```

Build the complete application:

```bash
make -j2
```

## Current Boundary

MVGv2.5 introduces an authoritative automotive object model and deterministic
validation architecture. The editor fixture is a compatibility lowering, not a
claim that current render meshes are fitted by a production nonlinear Class-A
optimizer. Exact image optimization, trimmed NURBS panel extraction, structural
finite-element analysis, and exact moving collision solids remain separate
future capabilities.
