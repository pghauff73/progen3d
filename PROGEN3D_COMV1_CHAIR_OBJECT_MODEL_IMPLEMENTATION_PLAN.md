# ProGen3D COMv1 Chair Object Model and FGKv1 Curved Geometry Implementation Plan

Date: 2026-08-24

Implementation status: complete. Authoritative verification is recorded in
`COMV1_CHAIR_OBJECT_MODEL_VERIFICATION.md` and
`tests/evidence/comv1_chair_acceptance_2026-08-24.json`.

## Document Control

- Contract: `COMv1`
- Building integration target: `SMB-OMv2.1`
- Geometry kernel target: `FGKv1`
- Chair catalog size: exactly 15 canonical styles
- Reference views: front, side, and top
- Silhouette acceptance: every individual view must have raw IoU greater than `0.80`
- Compatibility rule: preserve all 124 SMB-OMv2.1 spatial object identities

## 1. Completion Definition

This plan is complete only when current source and generated evidence prove all of
the following:

1. `Curve3D`, `MirrorCurve3D`, `SweepDisk`, `CurveNetworkSurface`, and
   `ShellOffset` resolve through the generic procedural `Mesh` pipeline.
2. Curves support line, polyline, Bezier, and Catmull-Rom evaluation, first and
   second derivatives, deterministic arc-length lookup, and bounded uniform
   arc-distance sampling.
3. Mirrored curves preserve their source curve and transformation relationship
   without silently changing source control points.
4. `SweepDisk` samples a `Curve3D`, supports constant and linearly varying
   radius, uses rotation-minimizing frames, and produces deterministic UVs,
   normals, caps, and topology.
5. `CurveNetworkSurface` validates rectangular U/V curve topology, validates
   every intersection against a scale-aware tolerance, and produces a
   deterministic parameterized surface and tessellated mesh.
6. `ShellOffset` produces a closed shell from a smooth open source surface,
   closes source boundaries, rejects collapsed output, and respects configured
   generated-mesh limits.
7. COMv1 contains exactly 15 stable chair design definitions covering dining,
   kitchen, study, living, and patio use contexts.
8. Every chair object is represented by explicit dimensions, components,
   support topology, geometry parameters, material variants, context
   applicability, three-view reference evidence, an acceptance state, and a
   `ChairPlacement` that owns its ground-contact position and orientation.
9. The ProGen3D chair builder can generate a deterministic mesh for every
   catalog design.
10. All 45 front/side/top comparisons pass raw silhouette IoU greater than
    `0.80`; an average cannot compensate for a failed view.
11. Dimensional validation separately proves width, depth, overall height, and
    seat height because normalized silhouettes cannot prove physical scale.
12. `SMB_001_Ground_Dining_Chairs` binds four COMv1 chair instances and
    `SMB_001_Upper_Study_Chair` binds one COMv1 chair instance without adding new
    SMB spatial identities.
13. Kitchen, living, and patio chair placements remain explicit proposals until
    a later SMB topology revision supplies stable spatial object identities.
14. Existing geometry, vehicle, vegetation, architecture, SMB-OMv2.1, and
    editor compatibility gates remain green.

## 2. Current State and Required Delta

### 2.1 Already implemented

The repository already provides:

- generic resolved triangle-mesh rendering, export, picking, collision, bounds,
  material, and evidence routing;
- deterministic `ShapeSpecificationKey` procedural mesh caching;
- a concrete `Curve3D` value model for line, polyline, Bezier, and Catmull-Rom
  evaluation and first derivatives;
- grammar-visible `SweepDisk` using a polyline path;
- grammar-visible `ShellOffset` using an already generated source mesh;
- `SurfaceLoft`, `ShellLoft`, `MirrorShape`, `CompoundShape`, and procedural
  assembly composition;
- deterministic front, side, and top binary silhouette projection;
- SMB-OMv2.1 with 124 stable spatial objects, including one dining `ChairSet`
  aggregate and one study `ChairSet` aggregate.

### 2.2 Missing or incomplete

- `Curve3D` has no second derivative, arc-length table, length query,
  parameter-at-distance query, transformed-curve relationship, or explicit
  zero-length/resource-limit validation.
- Existing sweeps construct an independently up-aligned frame at every path
  sample rather than transporting the previous frame. Nearly straight and
  inflected paths can therefore rotate unexpectedly.
- `SweepDisk` accepts only evaluated polyline points and one constant radius.
- There is no common `Surface3D` model.
- There is no `CurveNetworkSurface` shape family, validator, runtime surface,
  tessellator, grammar evaluator, procedural repository route, or evidence
  record.
- Current `ShellOffset` offsets a source mesh but does not explicitly reject
  flipped/collapsed triangles or record scale-aware thickness validity.
- No COMv1 chair model, catalog, geometry builder, SMB binding model, or
  three-view chair fit report exists.
- Existing SMB knowledge maps every `ChairSet` to dining activity, including
  the study chair.

## 3. Architectural Boundary

```text
Grammar or COMv1 design
        -> Curve / Surface / Shape specification
        -> Validation service
        -> Procedural geometry service
        -> Mesh
        -> ResolvedPrimitiveGeometry
        -> generic preview/export/picking/collision pipeline
```

No new primitive may append GPU vertices directly or branch rendering by shape
name. Curves and surfaces are mathematical models; tessellators and builders
are services; only resolved meshes enter the instance pipeline.

## 4. FGKv1 Curve Object Model

### 4.1 Curve models

- `Curve3D`: immutable control-point curve with a declared `Curve3DType`.
- `CurveArcLengthTable`: immutable ordered parameter/distance samples.
- `TransformedCurve3D`: association between a source curve and one explicit
  transformation matrix.
- `MirrorCurve3D`: purpose-specific reflected curve implemented through a
  `TransformedCurve3D` relationship.

`Curve3D` retains value semantics for compatibility with existing vehicle
models. Transformation wrappers compose a source curve instead of rewriting its
control-point evidence.

### 4.2 Curve API

```cpp
glm::dvec3 evaluate(double parameter) const;
glm::dvec3 evaluateDerivative(double parameter) const;
glm::dvec3 evaluateSecondDerivative(double parameter) const;
double length() const;
double parameterAtDistance(double distance) const;
glm::dvec3 evaluateByArcFraction(double arc_fraction) const;
std::vector<glm::dvec3> sampleByArcLength(std::size_t sample_count) const;
```

Existing float-returning compatibility methods remain available while geometry
builders use double-precision evaluation internally.

### 4.3 Curve validation

- finite control points;
- family-specific minimum control-point count;
- no more than the configured control-point limit;
- nonzero scale-aware total length;
- finite first and second derivatives at sampled validation parameters;
- requested arc-length sample count within the configured curve sample limit.

Diagnostics use stable codes:

- `P3D-GEO-CURVE-001`: insufficient control points;
- `P3D-GEO-CURVE-002`: nonfinite control point;
- `P3D-GEO-CURVE-003`: zero-length curve;
- `P3D-GEO-CURVE-004`: invalid parameter or distance;
- `P3D-GEO-CURVE-005`: configured resource limit exceeded.

## 5. FGKv1 SweepDisk

`SweepDisk` owns:

- one `Curve3D` centre path;
- initial up hint;
- start radius and end radius;
- longitudinal sample count;
- circumferential sample count;
- start/end cap policy;
- geometry detail level.

Legacy `path(...) radius(...) segments(...)` syntax remains valid and lowers to
a polyline `Curve3D` with equal start and end radii.

Extended syntax:

```p3d
SweepDisk(
    curve(bezier x0 y0 z0 x1 y1 z1 x2 y2 z2 x3 y3 z3)
    radiusStart(0.025)
    radiusEnd(0.018)
    longitudinalSegments(24)
    radialSegments(16)
    up(0 1 0)
    cap(all)
)
```

The mesh builder samples by arc distance and constructs rotation-minimizing
frames. UV `u` follows normalized path distance; UV `v` follows disk angle.

## 6. FGKv1 Surface Object Model

### 6.1 Surface models

- `Surface3D`: conceptual parameterized surface category.
- `CurveNetworkSurface`: rectangular U/V curve-network surface.
- `CurveNetworkIntersection`: one validated U/V relationship and its resolved
  parameter pair.
- `CurveNetworkParameterLattice`: ordered intersection lattice.
- `SurfaceTessellationSpecification`: U/V sample counts and closure policy.

`Surface3D` exposes:

```cpp
glm::dvec3 evaluate(double u, double v) const;
glm::dvec3 derivativeU(double u, double v) const;
glm::dvec3 derivativeV(double u, double v) const;
glm::dvec3 normal(double u, double v) const;
```

### 6.2 CurveNetworkSurface P0

P0 requires:

- at least two U curves and two V curves;
- every U curve intersects every V curve exactly once in the supplied parameter
  lattice;
- identical ordered lattice dimensions;
- intersection residual within a scale-aware tolerance;
- no silent reconciliation unless `allowFitting(true)` is explicit;
- bounded curve, surface, vertex, and triangle counts.

P0 uses an interpolating tensor patch over the validated lattice. Gordon
interpolation remains a compatible later method after the P0 lattice and
surface diagnostics are stable.

Grammar shape:

```p3d
CurveNetworkSurface(
    uCurve(bezier ...)
    uCurve(bezier ...)
    vCurve(bezier ...)
    vCurve(bezier ...)
    samplesU(48)
    samplesV(48)
    tolerance(0.002)
    method(interpolatingPatchGrid)
)
```

## 7. FGKv1 ShellOffset

P0 supports:

- a smooth open or closed source mesh;
- finite positive constant thickness;
- inward, outward, or symmetric offset;
- boundary-wall closure;
- deterministic winding and surface tags;
- collapsed-triangle rejection;
- normal-reversal diagnostics;
- configured vertex and triangle ceilings.

`ShellOffset` does not claim CAD-grade self-intersection repair. If the requested
thickness creates collapsed or reversed output, validation fails closed.

## 8. COMv1 Chair Object Model

```text
ChairCatalog
  *-- ChairDesignDefinition [15]

ChairDesignDefinition
  *-- ChairDimensionSpecification
  *-- ChairErgonomicSpecification
  *-- ChairComponentComposition
  *-- ChairGeometryParameterSet
  *-- ChairMaterialVariantSet
  *-- ChairReferenceViewSet
  *-- ChairDesignAcceptanceRecord
  --> ChairContextApplicabilityRelationship

ChairSetInstance
  o-- ChairInstance [1..*]

ChairInstance
  --> ChairDesignDefinition
  --> ChairMaterialVariant
  *-- ChairPlacement

ChairObjectModel
  --> ChairDesignDefinition
  *-- ChairPlacement

ChairPlacement
  *-- ChairOrientation

ChairBuildingObjectBinding
  --> ChairSetInstance
  --> SMB spatial object identifier
```

### 8.1 Placement and orientation

COMv1 chair geometry is authored in a local right-handed frame:

- local `+X`: chair right;
- local `-Y`: chair forward;
- local `+Z`: chair up;
- local ground plane: `Z = local_ground_height`.

ProGen3D scenes are world-Y-up. `ChairOrientation` owns the fixed local-to-world
pitch of `-90` degrees about X and the instance yaw about world Y.
`ChairPlacement` owns the world-space ground-contact position and produces the
deterministic transform:

```text
T(ground_x ground_y ground_z)
A(yaw_degrees 1)
A(-90 0)
T(0 0 -local_ground_height)
```

The final local translation is omitted when `local_ground_height` is zero. The
world transform maps the authored local ground reference to the requested world
ground-contact point. Invalid nonfinite positions, orientations, or ground
heights fail closed in `ChairAssemblyBuilder`.

### 8.2 Component categories

- `ChairSeatComponent`
- `ChairBackrestComponent`
- `ChairArmrestComponent`
- `ChairCushionComponent`
- `ChairFootrestComponent`
- `ChairSupportAssembly`

Support assembly specializations:

- `FourLegSupportAssembly`
- `CantileverSupportAssembly`
- `SledSupportAssembly`
- `PedestalSupportAssembly`
- `SwivelCasterSupportAssembly`

Chair styles are data definitions, not subclasses.

### 8.3 Canonical style catalog

| ID | Style | Contexts |
|---|---|---|
| `COMv1.Windsor` | Windsor spindle-back | Dining, kitchen |
| `COMv1.ShakerLadderBack` | Shaker ladder-back | Dining, kitchen |
| `COMv1.BentwoodCafe` | Bentwood cafe | Dining, kitchen |
| `COMv1.Parsons` | Upholstered Parsons | Dining, living |
| `COMv1.ScandinavianShell` | Molded shell | Dining, kitchen, study, patio |
| `COMv1.Wishbone` | Wishbone Y-chair | Dining, living |
| `COMv1.Cantilever` | Tubular cantilever | Dining, study |
| `COMv1.BackedCounterStool` | Backed counter stool | Kitchen |
| `COMv1.SaddleStool` | Saddle stool | Kitchen |
| `COMv1.MeshTask` | Mesh task chair | Study |
| `COMv1.ExecutiveHighBack` | Executive high-back | Study |
| `COMv1.LoungeArmchair` | Lounge armchair | Living |
| `COMv1.MidCenturyAccent` | Mid-century accent | Living, study |
| `COMv1.Adirondack` | Adirondack | Patio |
| `COMv1.MetalBistro` | Metal bistro | Patio, kitchen |

## 9. COMv1 Geometry Strategy

Every chair lowers through a common `ChairAssemblyBuilder`:

```text
ChairAssembly
  -> SeatAssembly
  -> BackrestAssembly
  -> SupportAssembly
  -> optional ArmrestAssembly
  -> optional FootrestAssembly
  -> optional CushionAssembly
```

The highest-gain FGKv1 acceptance design is `COMv1.ScandinavianShell`:

```text
left shell edge       Curve3D
right shell edge      MirrorCurve3D
seat/back centre      Curve3D
transverse sections   Curve3D set
        -> CurveNetworkSurface
        -> ShellOffset
        -> chair shell

four leg paths
        -> SweepDisk
        -> chair support assembly
```

Other catalog designs may use rounded boxes, extrusions, revolutions, sweeps,
arrays, and compound composition while preserving the same COMv1 component
contract.

## 10. Three-View Reference and Fit Contract

Each style owns one approved three-view reference set:

- front;
- side;
- top.

Generated and reference views use the same fixed physical projection envelope.
The reference package stores source classification, dimensions, silhouette
mask, source hash, mask hash, and orientation metadata.

For view `v`:

```text
IoU(v) = intersection(reference(v), generated(v))
         / union(reference(v), generated(v))
```

Hard acceptance:

```text
front IoU > 0.80
side  IoU > 0.80
top   IoU > 0.80
minimum IoU > 0.80
```

Additional hard gates:

- width, depth, and overall-height relative error no greater than `0.03`;
- seat-height relative error no greater than `0.02`;
- required component topology present;
- no nonfinite geometry;
- no generated-mesh resource-limit violation;
- repeated generation has identical mesh and silhouette hashes.

Parameter fitting uses deterministic bounded coordinate descent. Models may
propose a candidate, but deterministic metrics choose and accept it.

## 11. SMB-OMv2.1 Integration

Immediate bindings:

```text
Binding.COMv1.SMB001.DiningChairs
  -> SMB_001_Ground_Dining_Chairs
  -> four COMv1 dining chair instances

Binding.COMv1.SMB001.StudyChair
  -> SMB_001_Upper_Study_Chair
  -> one COMv1 study chair instance
```

COM instance identifiers do not become SMB spatial object identities. SMB owns
containment, the supporting floor reference, and collision layer. Each COMv1
chair object owns its resolved ground-contact pose relative to the containing
`ChairSet`, together with chair style, dimensions, components, material variants,
geometry, and fit evidence.

Object-specific functional allocation replaces the current class-only chair
mapping:

- dining binding: `Function.SupportDiningActivity`;
- study binding: `Function.SupportStudyActivity`;
- all chairs: `Function.SupportSeatedOccupant` and `Role.SeatingElement`.

Kitchen, living, and patio placements are proposal records only in
SMB-OMv2.1. Promoting them to stable spatial objects requires a later topology
revision.

Every bound chair instance publishes a `placement` record containing its
ground-contact position, yaw, local ground height, local axes, world up axis, and
grounded state. Legacy `relative_translation_m` and `yaw_degrees` fields remain
derived compatibility projections of that canonical placement record.

## 12. Implementation Sequence

### Phase A: Baseline and plan

- record current focused test status;
- preserve unrelated worktree state;
- publish this integrated plan.

### Phase B: Curve kernel

- extend `GeometryComplexityLimits` with curve and surface ceilings;
- add second derivatives and deterministic arc-length tables to `Curve3D`;
- add transformed and mirrored curve models;
- add deterministic curve tests and diagnostics.

### Phase C: SweepDisk extension

- add curve family and control-point specification;
- retain legacy path syntax;
- add start/end radius and longitudinal sampling;
- implement rotation-minimizing frames;
- add straight, bent, S-curve, tapered, and nearly-straight fixtures.

### Phase D: Surface kernel

- add `Surface3D`;
- add `CurveNetworkSurface` and intersection lattice models;
- add validator and uniform tessellator;
- add grammar parsing/evaluation and procedural repository routing;
- add flat, dome, saddle, and chair-shell fixtures.

### Phase E: ShellOffset hardening

- add scale-aware thickness checks;
- reject collapsed and reversed output;
- preserve boundary closure and stable surface tags;
- add flat-sheet, bowl, and chair-shell fixtures.

### Phase F: COMv1 catalog and geometry

- add chair model, relationship, context, and service classes;
- define exactly 15 styles;
- build deterministic component meshes;
- create the Scandinavian shell vertical slice using all five FGKv1 primitives;
- produce one grammar example for each style.

### Phase G: Three-view evidence

- add fixed-envelope projection;
- add binary silhouette IoU and contour comparison;
- generate 45 reference masks and 45 ProGen3D masks;
- run deterministic bounded fitting;
- publish per-style JSON and contact-sheet evidence.

### Phase H: SMB integration

- add COMv1 binding sidecar generated from the authoritative SMB generators;
- bind four dining and one study instance;
- correct object-specific chair functions;
- preserve all 124 SMB identities and current default grammar compatibility.

### Phase I: Acceptance audit

- run focused curve, surface, chair, fit, grammar, and SMB gates;
- run existing geometry and release compatibility gates;
- verify generated artifacts and hashes;
- audit every completion requirement against authoritative evidence.

## 13. Repository Integration Points

Planned source areas:

```text
include/geometry/model/
include/geometry/relationship/
include/geometry/service/
src/geometry/model/
src/geometry/service/
include/chair/model/
include/chair/relationship/
include/chair/service/
src/chair/model/
src/chair/service/
examples/COMv1_Chair_Catalog/
examples/SMB_OMv2_Instance_SMB_001/
tests/
```

Generated SMB files must continue to be produced by the existing SMB generators;
generated JSON, grammar, and C++ catalogs are never hand-edited as canonical
source.

## 14. Required Gates and Artifacts

```text
tests/run_fgkv1_curve_surface_checks.sh
tests/run_comv1_chair_catalog_checks.sh
tests/run_comv1_three_view_fit_checks.sh
tests/run_comv1_smb_omv21_integration_checks.sh
tests/run_three_view_projection_checks.sh
tests/run_profile_extrusion_checks.sh
tests/run_curved_geometry_pipeline_checks.sh
tests/run_smb_omv2_checks.sh
tests/run_smb_omv21_all_objects_checks.sh
```

Required generated evidence:

```text
examples/COMv1_Chair_Catalog/COMv1_Chair_Catalog.json
examples/COMv1_Chair_Catalog/COMv1_Chair_Catalog.p3d
examples/COMv1_Chair_Catalog/reference_views/
examples/COMv1_Chair_Catalog/generated_views/
examples/COMv1_Chair_Catalog/fit_reports/
examples/SMB_OMv2_Instance_SMB_001/COMv1_Chair_Bindings.json
tests/evidence/comv1_chair_acceptance_2026-08-24.json
```

## 15. Compatibility Boundary

- Existing grammar syntax remains valid.
- Existing `Curve3D` call sites remain source-compatible.
- Existing `SweepDisk(path(...) radius(...) segments(...))` remains valid.
- Existing `ShellOffset` grammar remains valid.
- No renderer path branches on COMv1 or the new surface family.
- Existing SMB spatial object identifiers and containment remain unchanged.
- The accepted SMB default chair geometry changes only after COMv1 evidence
  passes; until then the COMv1 binding remains an explicit opt-in variant.

## 16. Completion Audit Matrix

| Requirement | Authoritative evidence |
|---|---|
| Five named FGKv1 primitives | model/service sources plus curve/surface harness |
| Generic mesh route | resolved geometry integration and curved pipeline gate |
| Curve derivatives and arc length | deterministic unit harness |
| Rotation-minimizing SweepDisk | nearly-straight and S-curve fixtures |
| Rectangular curve network | validator tests and surface topology evidence |
| Shell closure and failure behavior | topology and invalid-thickness fixtures |
| Exactly 15 styles | catalog validator and generated JSON |
| Five use contexts | context coverage matrix |
| 45 views | reference/generated manifest counts |
| Every view IoU greater than 0.80 | fit report hard-gate records |
| Physical dimensions | dimension residual report |
| Dining/study SMB bindings | generated binding JSON and integration harness |
| 124 SMB identities preserved | existing SMB native and all-object gates |
| Determinism | repeated generation and artifact hash comparison |
| Compatibility | focused existing suites plus full release checks |
