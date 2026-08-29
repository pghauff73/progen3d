# MVPv2.5 Evidence-Constrained Automotive Geometry Verification

## Document Control

- **Date:** Saturday, August 22, 2026
- **Specification:** `/home/pamela/.codex/attachments/3306d5fd-631a-4b52-9e2f-c6b4019c7046/pasted-text-1.txt`
- **Implementation plan:** `PROGEN3D_MVPV25_EVIDENCE_CONSTRAINED_AUTOMOTIVE_GEOMETRY_IMPLEMENTATION_PLAN.md`
- **Architecture guide:** `docs/MVGv2_5.md`
- **Acceptance fixture:** `examples/MVPv25_Red_AWD_Hatchback_EvidenceConstrained.p3d`
- **Native acceptance gate:** `tests/run_mvpv25_hatchback_checks.sh`
- **Supervisor visual gate:** `tests/run_mvpv25_hatchback_visual_checks.sh`

## Verification Standard

This document maps every numbered section in the supplied MVPv2.5 design to
current source and executable evidence. A requirement is accepted only when the
authoritative object model exists, the red AWD hatchback fixture populates it,
validation fails closed on invalid state, and a focused test exercises the
required behavior.

The implementation separates two authorities:

1. Native C++ owns evidence, geometry relationships, kinematics, residuals,
   deterministic hashes, and fail-closed validation.
2. The `.p3d` fixture is a current-parser compatibility lowering that exposes
   the same hierarchy and evidence categories while retaining the proven FGKv1
   visible mesh. It does not pretend that the existing grammar parser directly
   evaluates a nonlinear Class-A or hardpoint solver.

## Canonical Aggregate Audit

| Term | Authority | Evidence |
|---|---|---|
| `E` Evidence | `VehicleObservationSet`, view/line/ellipse observations | Five complete calibrated view records |
| `R` Reference | `VehicleReferenceFrame`, `VehicleFiducialDatum` | Right-handed SAE frame and explicit datums |
| `P` Package | `VehiclePackageEvidence`, `OccupantPackage` | Hard dimensions, AWD evidence and two occupant packages |
| `W` Wireframe | `AutomotiveWireframe` | Landmark topology and soft shape prior |
| `C` Curves | `AutomotiveCharacterCurve` | Twenty five-view semantic character curves |
| `S` Surfaces | `ClassASurfaceGraph` | Patches, guides, connections and continuity evidence |
| `A` Apertures | Closure geometry model | Apertures, gaps, rolled edges, closures, glass and seals |
| `K` Kinematics | Chassis and closure services | Hinge, glass, suspension and wheel-pose functions with sweeps |
| `B` BIW | `BodyInWhite` | Variable-section members, holes, reinforcement and joints |
| `F` Functional assemblies | `ModernVehicleAssembly` | Preserved placed-assembly, joint and resolved-mesh graph plus tyre/rim/brake/wheel-house records |
| `V` Validation | Validation/residual/hash services | Fail-closed diagnostics, nine-term residuals and deterministic hashes |

## Requirement Matrix

| Section | Requirement | Authoritative implementation | Acceptance evidence | Status |
|---:|---|---|---|---|
| 1 | Automotive reference system | `VehicleReferenceFrame`, `VehicleReferenceDatum` in `include/vehicle/model/VehicleMvp25Reference.h` | Right-handed axes, ground plane, front/rear axle datums and centre plane validated by `VehicleMvp25ValidationService` and harness | Verified |
| 2 | Occupant/package primitives | `VehiclePackageEvidence`, `OccupantPackage` | 4.350 m length, 1.800 m width, 1.430 m height, 2.650 m wheelbase, AWD, front/rear H/eye/heel/envelope records; package residual exactly zero | Verified |
| 3 | Semantic wireframe before surfaces | `VehicleLandmark`, `AutomotiveWireframeEdge`, `VehicleShapePrior` | 31 landmarks, typed topology edges, soft hatchback prior, zero generated landmark reprojection error | Verified |
| 4 | `AutomotiveWireframe` | `include/vehicle/model/AutomotiveWireframe.h` | Stable identity, landmark collection, topology graph and prior all present and hash-covered | Verified |
| 5 | Images as first-class evidence | `VehicleCameraModel`, `VehicleViewObservation`, `CharacterLineObservation` | Five invertible cameras; every view contains silhouette, landmarks, 20 character lines, 6 panel lines, 6 glazing lines and 4 wheel ellipses with uncertainty/confidence | Verified |
| 6 | Projection/reprojection | `VehicleProjectionService` | Orthographic and perspective projection paths; landmark, silhouette and character-line residual functions; zero generated landmark/character reprojection error | Verified |
| 7 | Silhouette constraints | `SilhouetteConstraint` | Five view-specific constraints with model boundary, observed silhouette, weight, strength and tolerance | Verified |
| 8 | Character lines as geometry | `AutomotiveCharacterCurve` | 20 semantic curves with degree, control points and five observation references each | Verified |
| 9 | Class-A patch graph | `ClassASurfaceGraph` | 17 patches, curve graph and 16 explicit patch connections | Verified |
| 10 | `ClassASurfacePatch` | `ClassASurfacePatch` | Four resolved boundaries, two resolved internal guides, degrees, spans, dimensions and complete control grids for every patch | Verified |
| 11 | Prefer simple patches | `HighlightFlowReport` and `VehicleClassAValidationService::calculateComplexityPenalty` | Patch, control-point and span penalties contribute to the ninth objective term | Verified |
| 12 | Edge-specific continuity | `PatchContinuityLevel`, `PatchConnection` | G0, G1, G2, G3, Crease, PanelGap and Trimmed are all exercised | Verified |
| 13 | Highlight validation | `HighlightFlowValidator` | Position, tangent, curvature and normal-flow residuals generated for every patch connection | Verified |
| 14 | Rolled panel gaps | `AutomotivePanelGap` | Six seams with variable gap width, primary/secondary radius stations, dual flanges, G2/G1 continuity and closeouts | Verified |
| 15 | `RolledEdge` | `RolledEdge` | Six valid contact curves with radius, flange length/angle and continuity | Verified |
| 16 | Aperture-first doors | `BodySideAperture`, `DoorEgressSurface`, `AutomotiveClosure` | Four apertures precede four side-door closure records and provide their derivation references | Verified |
| 17 | `BodySideAperture` | `BodySideAperture` | Hinge/A/B/C pillars, roof rail, rocker, dogleg, B-line, J-surface, glass, flange and seal seat retained | Verified |
| 18 | B-line and J-surface | `Curve3D bLine`, J-surface identifiers, `DoorEgressSurface` | All four aperture B-lines validate and every egress surface retains B-line/J-surface/glass/belt references | Verified |
| 19 | Door derived from four references | `AutomotiveClosure` | Side-door records reference outer Class-A skin, aperture, J-surface and side glass | Verified |
| 20 | Hinge rise and swept volume | `ClosureHingeStudy`, `AutomotiveClosureGeometryService` | Six closures have five rise samples, deterministic open transforms and swept bounds containing closed geometry | Verified |
| 21 | Helical window drop | `HelicalGlassDrop` | Four side-glass motions combine translation and rotation and remain inside declared door cavities | Verified |
| 22 | `HelicalGlassDrop` data model | `SideGlassSurface`, `BarrelSurface`, helix axis/pitch/rate/limits/cavity | Positive barrel radius/length and deterministic transforms validated for all four doors | Verified |
| 23 | Generated window channels | `GlassChannel`, `AutomotiveClosureGeometryService::generateChannel` | Eight channels with valid centre lines, width, depth and clearance | Verified |
| 24 | Variable door seals | `VariableSealSweep` | Six EPDM seals with three varying section stations and seat/rolled-edge contact targets | Verified |
| 25 | Common closure framework | `AutomotiveClosureType`, `AutomotiveClosure` | Four doors, bonnet and rear hatch use one closure/hinge representation | Verified |
| 26 | Hatch child geometry | `AutomotiveClosure::childObjectIdentifiers` | Rear hatch owns rear glass, spoiler, rear wiper and high-mounted brake lamp | Verified |
| 27 | Body-in-white layer | `BodyInWhite` | Separate structural model with 18 members and 17 explicit joints | Verified |
| 28 | Variable-section BIW members | `AutomotiveStructuralMember` | Every member has three positive section stations, explicit hole bounds, reinforcement, DP600 material and provenance | Verified |
| 29 | Explicit structural joints | `BIWJoint` | Every joint references two existing members, two overlap surfaces, joining method, local reinforcement and position; graph connectivity validated | Verified |
| 30 | Suspension hardpoints | `SuspensionHardpointModel` | Four corners, each with five hardpoints and four measured links; references and link lengths validated | Verified |
| 31 | Wheel pose solved from travel/steering | `WheelPoseSolver`, `VehicleChassisKinematicsService` | Front functions sample 5 x 5 travel/steer states; rear functions sample 5 x 1; steering and travel both change solved pose | Verified |
| 32 | Wheel arch from swept tyre | `WheelSweptEnvelope`, `WheelHouse` | Every sample bound is contained by its swept envelope and every envelope is contained by its wheel house plus clearance | Verified |
| 33 | Improved wheel primitives | `TyreGeometry`, `AutomotiveRimGeometry`, `AutomotiveBrakeGeometry` | Four tyres retain section/radius/tread evidence; four rims retain corner, radii, width, spokes and material; four brakes retain packaged rotor/caliper/piston/material evidence | Verified |
| 34 | Wheels and underbody in shape validation | `AeroGeometry` | Eight unique roles include upper body, wheels, wheel houses, underbody, splitter, cooling opening, diffuser and spoiler | Verified |
| 35 | Complete fit objective | `VehicleFitObjective`, `VehicleFitResidualReport` | Nine unique terms and finite raw/weighted residuals; changing silhouette evidence changes recomputed objective | Verified |
| 36 | Hard and soft constraints | `VehicleFitConstraint`, `VehicleConstraintStrength` | Hard package failures invalidate a report regardless of soft score; hatchback prior remains soft | Verified |
| 37 | Primitive hierarchy | Model headers under `include/vehicle/model` and services under `include/vehicle/service` | The wheel branch explicitly contains `TyreGeometry`, `AutomotiveRimGeometry`, `AutomotiveBrakeGeometry`, and `WheelHouse`; all other evidence/reference/Class-A/closure/glass/BIW/chassis/validation responsibilities remain separated | Verified |
| 38 | Grammar direction | `examples/MVPv25_Red_AWD_Hatchback_EvidenceConstrained.p3d` | Executable evidence hierarchy exposes reference/package/views/wireframe/Class-A/closures/glass/BIW/chassis/aero/objective categories | Verified as compatibility lowering |
| 39 | Door grammar example | Same fixture: aperture, B-line, J-surface, gap, seal and closure interfaces | Current parser projection is executable; native closure service remains kinematic authority | Verified as compatibility lowering |
| 40 | Window grammar example | Same fixture: barrel axes and front/rear channel interfaces | Current parser projection is executable; native `HelicalGlassDrop` owns the actual helical transform | Verified as compatibility lowering |
| 41 | MVPv2 to MVPv2.5 difference | `docs/MVGv2_5.md` and implementation plan | Evidence-constrained reference/package/wireframe/Class-A/closure/BIW/kinematic layers coexist with the preserved MVPv2 compatibility architecture | Verified |
| 42 | Implementation order | Implementation plan, model/service file separation and focused gates | Reference/evidence precede surfaces, closures, BIW and chassis in construction; validation/hash are final publication gates | Verified |

## Deterministic Acceptance Fixture

The native red AWD hatchback currently proves:

```text
views                         5
landmarks                    31
semantic character curves   20
Class-A patches              17
panel gaps                    6
rolled edges                  6
body-side apertures           4
closures                      6
helical glass drops           4
glass channels                8
variable seals                6
BIW members                  18
BIW joints                   17
suspension hardpoints        20
rim geometries                4
brake geometries              4
wheel pose states (front)    25 per corner
wheel pose states (rear)      5 per corner
aero roles                    8
fit objective terms           9
```

Current deterministic evidence from Saturday, August 22, 2026:

```text
native architecture hash     14359227217057828916
front visual sha256          904495b9754a...
rear visual sha256           cd2dfb30d904...
left visual sha256           e95deeeb34ac...
right visual sha256          9280969287ee...
top visual sha256            dd753bd6b675...
```

## Release Gates

Required commands:

```bash
./tests/run_mvpv25_hatchback_checks.sh
./tests/run_mvpv25_hatchback_visual_checks.sh
./tests/run_mvpv2_hatchback_fitting_checks.sh
./tests/run_mvpv2_hatchback_visual_checks.sh
./tests/run_mvpv1_red_hatchback_grammar_checks.sh
./tests/run_mvpv1_red_hatchback_visual_checks.sh
./tests/run_mvpv1_awd_hatchback_grammar_checks.sh
./tests/run_mvpv1_awd_hatchback_visual_checks.sh
make -j2
```

Passing the focused native gate proves object-model, relationship, kinematic,
residual and deterministic-hash behavior. Passing the five-view supervisor gate
proves that the executable compatibility grammar remains visible, upright and
view-distinct. Legacy gates prove that the MVPv2.5 promotion has not replaced or
regressed the MVPv2 and MVPv1 acceptance fixtures.

## Explicit Boundary

This implementation promotes MVPv2.5 as a deterministic, evidence-constrained
automotive object model and acceptance fixture. It does not claim that the
current editor performs production nonlinear Class-A optimization directly from
raster pixels, nor that the proposed section 38–40 syntax has replaced the
existing grammar parser. The fixture exposes those concepts through executable
current-language objects and interfaces, while the native services remain the
authority for projection, residuals, continuity, closure motion, glass motion,
BIW connectivity and wheel sweeps.
