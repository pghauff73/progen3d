# ProGen3D Full Geometry Kernel FGKv1 Completion Roadmap

Date: 2026-08-22

## Decision

FGKv1 should become the single geometry realization boundary:

```text
Grammar or domain object
        -> ShapeSpecification
        -> ShapeRealizationService
        -> MeshFinishingPipeline
        -> ValidatedGeometrySnapshot
```

RBGv1, MVGv1, vegetation, furniture, fixtures, mechanical assemblies, and
imported geometry should describe shapes and relationships. They should not
construct preview/GPU vertices directly.

The implementation should extend the existing kernel rather than replace it.
ProGen3D already has validated profiles with holes, extrusions, sweeps, revolved
geometry, lofts, shell lofts, folded profiles, tapered sweeps, arrays, topology
evidence, collision policies, detail levels, deterministic shape keys, and
domain assembly builders. The unfinished work is chiefly integration,
surface/shell capability, mesh semantics, and a uniform transaction pipeline.

## Current Evidence Baseline

The current source tree contains these working FGKv1 foundations:

- `Profile2D` with one outer loop, multiple holes, winding normalization,
  containment validation, and complexity ceilings.
- profile factories for rectangles, rounded rectangles, circles, ellipses,
  chamfered rectangles, and polygons.
- `Curve3D` for line, polyline, Bezier, and Catmull-Rom evaluation, first
  derivative evaluation, and deterministic sampling.
- `ExtrudeProfile`, profile holes, `SweepProfile`, `SweepDisk`, `Revolve`,
  `Loft`, holed `ShellLoft`, `FoldedProfile`, and `TaperedSweep` mesh builders.
- linear, grid, and radial `InstanceArraySpecification` construction.
- `LayerSet`, `PanelBox`, `PanelArray`, hosted-window assembly, glazing,
  stairs, sanitary basins, pools, and grid surface tiling in the architecture
  layer.
- vehicle body shell construction using corresponding outer/inner loft
  sections, plus semantic vehicle guide curves, wheel arches, and panel seams.
- vegetation assembly using `BranchGraph` above tapered sweeps and botanical
  blade geometry.
- `GeometryBuildResult`, bounded complexity, topology analysis, deterministic
  `ShapeSpecificationKey` caching, volume evidence, collision policy selection,
  and semantic face-role tags.
- AABB, OBB, convex-hull, and triangle-mesh collision structures in the spatial
  and physics layers.
- detail-level ranges used meaningfully by architecture, vehicle, and
  vegetation assemblies.

Focused baseline suites:

```text
tests/run_profile_extrusion_checks.sh
tests/run_mvg_vehicle_checks.sh
tests/run_vegetation_geometry_assembly_checks.sh
tests/run_curved_geometry_pipeline_checks.sh
tests/run_axial_profile_geometry_checks.sh
```

The profile/hosted-assembly, MVGv1 vehicle, and vegetation assembly suites pass
on the August 22, 2026 source state. Their current deterministic topology hashes
include `10740567470808396435` for the representative vehicle assembly and
`865635542578743181` for the vegetation assembly.

## Critical Incompleteness

### 1. Shape-family declarations overstate runtime support

`ShapeFamily` names more families than `ProceduralMeshRepository` can realize.
The repository currently routes Cylinder, Sphere, AxialProfile, ExtrudeProfile,
SweepProfile, SweepDisk, TaperedSweep, botanical blades, Plant, Revolve, Loft,
ShellLoft, FoldedProfile, and nested `InstanceArray` specifications. `LayerSet`,
`PanelBox`, `PanelArray`, `GlazingPanel`, and `SurfaceTiling` are not repository
routes.

Some of those capabilities exist in domain builders, but they do not yet lower
through one canonical kernel facade.

### 2. P0A grammar exposure is substantially implemented

The grammar now exposes canonical descriptors for `SweepProfile`, `SweepDisk`,
`Revolve`, `Loft`, `ShellLoft`, `FoldedProfile`, and recursively nested
`InstanceArray`, in addition to the earlier Cylinder, Sphere, AxialProfile,
Extrude, TaperedSweep, botanical, vegetation, and curved-alias paths.

The MVPv1 bridge also exposes `VehicleBodyShell`, which accepts semantic
transverse body stations and lowers them to a validated `ShellLoft` rather than
constructing GPU geometry in grammar code. The advanced red hatchback example
uses this path together with revolved wheels, swept seams, and radial spoke
arrays.

The remaining P0A exposure work is a direct `RoundedBox` descriptor, common
domain-family lowering through one realization facade, and imported-mesh
participation in the same versioned key and transaction model.

### 3. There is no common surface object model

Loft and sweep builders currently emit triangles directly. There is no shared
`Surface`, parameter domain, derivative, normal, curvature, trim boundary, or
adaptive tessellation abstraction. Vehicle guide curves are validated and
rendered as separate details; they do not constrain the current body loft.

### 4. Shell support is narrower than the name suggests

Current `ShellLoft` is a loft between corresponding outer and inner profile
loops. It is useful and watertight, but it is not a general offset shell. There
is no `ShellOffset`, `ShellExtrude`, `CurvedPanel`, or thickness policy for an
arbitrary surface.

### 5. Mesh finishing is fragmented

`Mesh` has separate position, face, normal, face-normal, and texture-coordinate
arrays. It has no tangent stream, normal-generation policy, UV strategy object,
material groups, or object-part identifiers. `MeshSurfaceTag` preserves useful
roles and boundary indices, but not stable `SurfaceId`, `MaterialId`, or
`ObjectPartId` values.

### 6. Arrays currently duplicate mesh data

`InstanceArrayGeometryBuilder` composes transformed copies into one larger mesh.
It validates array counts, but it does not preserve the optimal representation:

```text
one immutable mesh + N transforms
```

Path, surface, and scatter arrays also remain outside the common kernel.

### 7. Imported geometry bypasses the same contract

STL loading enters through `Mesh::getInstance` and a separate STL cache. It does
not produce a `ShapeSpecification`, common diagnostics, semantic surfaces,
normal/UV/tangent finishing evidence, or a versioned geometry cache key.

### 8. Validation is useful but incomplete

Current topology analysis measures boundary edges, nonmanifold edges,
degenerate triangles, signed volume, and a topology hash. It does not yet report
duplicate triangles, inconsistent winding, self-intersection, isolated
vertices, material/tag mismatches beyond face count, or an explicit
`TopologyExpectation` contract.

## Layer Status

| FGKv1 Layer | Current State | Completion Required |
|---|---|---|
| Mathematical primitives | Partial | Add purpose-driven `GeometryFrame`, `AxisLine`, `PlaneEquation`, `OrientedBounds`, and `SphereBounds`; reuse GLM for storage. |
| Curves | Partial | Add Curve2D, arc, circle, ellipse, helix, second derivative, length, closest-point, frame transport, and bounded tessellation. |
| Profiles | Strong partial | Add Bezier loops, compound profiles, stable loop IDs, and reusable offset/inset operations. |
| Surfaces | Missing common layer | Add parametric surfaces, guide-constrained lofting, ruled/sweep/boundary/height/offset surfaces, trims, and surface sampling. |
| Solids and shells | Partial | Add true surface shells, offset/extrude shell policies, thin sheets, curved panels, and shell diagnostics. |
| Construction operations | Strong partial | Expose existing operators to grammar; add mirror, compound, surface loft, shell offset, and hosted opening lowering. |
| Repetition and instancing | Partial | Preserve source mesh plus transforms; add path, surface, and scatter arrays. |
| Deformation | Missing kernel layer | Introduce explicit deformation specifications and compatibility adapters for DSX/DSY/DSZ/DTX/DTY/DTZ. |
| Topology | Partial | Add complete expectation-driven mesh validation and self-intersection checks. |
| Interfaces | Strong domain layer, weak kernel binding | Bind interface definitions to shape surfaces/edges/volumes and preserve them through realization. |
| Mesh realization | Partial | Add one finishing pipeline for vertices, normals, UVs, tangents, groups, bounds, and tags. |
| Collision geometry | Strong partial | Add analytic shape variants and per-shape collision-realization policy; keep render and collision results distinct. |
| Materials and UVs | Partial | Add material bindings, canonical UV strategies, seams, tangent generation, and stable material groups. |
| LOD | Partial | Make resolution settings affect every generator; separate semantic LOD from tessellation quality. |
| Validation | Strong partial | Extend statuses, diagnostics, topology expectations, and transactional commit/rollback. |
| Caching and determinism | Strong partial | Add explicit `GeometryVersion`, dependency hashes, imported-file hashes, and instance-preserving cache entries. |

## Correct Domain Boundary

The target dependency direction should be:

```text
RBGv1 Building objects ─┐
MVGv1 Vehicle objects ──┼─> ShapeSpecification/ShapeGraph ─> FGKv1
Vegetation reasoning ───┤
Furniture/fixtures ─────┤
Imported geometry ──────┘
```

`Plant` should not remain a long-term kernel shape family. `BranchGraph`, plant
species, growth state, phyllotaxis, vehicle packaging, rooms, kitchens,
circuits, and suspension types are domain objects. They should lower to generic
kernel operations such as tapered sweeps, ribbon surfaces, lofts, arrays,
compounds, and interfaces.

Compatibility adapters may keep existing grammar names while producing the new
generic specifications.

## Completion Sequence

### FGKv1-P0A: Unify existing capabilities

Implement this before adding new geometry algorithms.

August 22, 2026 implementation delta:

- complete: direct grammar/evaluator paths for `SweepProfile`, `SweepDisk`,
  `Revolve`, `Loft`, `ShellLoft`, `FoldedProfile`, and nested `InstanceArray`;
- complete: recursive `InstanceArray` repository realization;
- complete: semantic `VehicleBodyShell -> ShellLoft` MVPv1 lowering;
- remaining: direct `RoundedBox`, compound/domain lowering,
  `ShapeRealizationService`, imported-mesh specifications, and geometry-version
  participation in cache keys.

1. Add grammar syntax and evaluator models for:
   - `SweepProfile`
   - `SweepDisk`
   - `Revolve`
   - `Loft`
   - `ShellLoft`
   - `FoldedProfile`
   - `RoundedBox`
   - `InstanceArray`
2. Add repository routes for `InstanceArray` and compound/domain-lowered shapes.
3. Introduce `ShapeRealizationService` as the only public shape-to-geometry
   service. It delegates to existing purpose-specific builders.
4. Make imported STL geometry enter through `ImportedMeshShapeSpecification`.
5. Add a versioned key:

```text
hash(canonical specification, detail, tessellation, geometry version,
     imported source hash)
```

Acceptance gate: every existing builder is grammar- or domain-reachable through
the same shape-realization facade and produces the same deterministic topology
hash on repeated builds.

### FGKv1-P0B: Common surface and shell kernel

Add a UML-readable surface model:

```text
ParametricSurface
├── PlaneSurface
├── ExtrudedSurface
├── RevolvedSurface
├── RuledSurface
├── SweepSurface
├── LoftSurface
├── BoundarySurface
├── HeightFieldSurface
└── OffsetSurface
```

Core supporting classes:

```text
SurfaceDomain
SurfaceSample
SurfaceDerivativeFrame
SurfaceTrimLoop
GuideCurveConstraint
SurfaceTessellationSettings
```

Add:

```text
SurfaceLoftShapeSpecification
ShellOffsetShapeSpecification
ShellExtrudeShapeSpecification
CurvedPanelShapeSpecification
ThinSheetShapeSpecification
```

For MVGv1 P0, guide curves must influence the loft solution rather than exist
only as independently rendered curves. C1 need only be an approximate tangent
constraint initially; C2 remains later work.

Acceptance gate: one vehicle body, one chair shell, one bathtub, and one curved
architectural panel are built from surfaces/shells without Cube approximation.

### FGKv1-P0C: Canonical mesh finishing

Introduce:

```text
MeshVertex
MeshTriangle
MeshSurfaceGroup
MeshMaterialGroup
MeshFinishingRequest
MeshFinishingPipeline
```

`MeshVertex` should carry position, normal, UV, and tangent. Triangle or surface
group records should carry stable surface, material, and object-part IDs.

Normal policies:

```text
Flat
Smooth
AngleWeighted
HardEdgeTagged
```

UV policies:

```text
Planar
Box
Cylindrical
Spherical
ProfileLength
SweepLength
LoftParameter
Triplanar
Imported
```

Acceptance gate: architecture hard edges, furniture rounded edges, vehicle
smooth panels, normal maps, and glass tangents all use the same finishing path.

### FGKv1-P0D: Compound shapes and real instancing

Add:

```text
CompoundShapeSpecification
ShapeGraph
ShapeNode
TransformShapeNode
OperationShapeNode
InstanceShapeNode
```

Keep semantic parts until export or explicit flattening. Replace array mesh
duplication with a `RealizedInstanceSet` containing one cached source mesh and
bounded transforms.

Acceptance gate: forty identical windows, wheel spokes, lights, leaves, and
tiles each use one source mesh with repeated transforms while retaining stable
part IDs.

### FGKv1-P0E: Validation and transactions

Extend `GeometryBuildStatus` with:

```text
InvalidSpecification
InvalidCurve
ResourceLimit
UnexpectedOpenBoundary
NonmanifoldTopology
InconsistentWinding
DuplicateTriangle
SelfIntersection
MaterialBindingFailure
InterfaceBindingFailure
InternalFailure
```

Add `TopologyExpectation` and `GeometryTransaction`:

```text
previous valid snapshot
        -> build candidate
        -> finish mesh
        -> validate topology/materials/interfaces/collision
        -> commit or retain previous snapshot
```

Acceptance gate: every invalid candidate leaves the previous valid scene mesh,
collision representation, material groups, and interfaces unchanged.

## FGKv1-P1

### Hosted openings

Create first-class:

```text
HostedOpeningSpecification
OpeningBoundary
OpeningHostReference
OpeningFillingReference
HostedOpeningRealizationService
```

Lower profile-aligned wall, slab, roof, and panel openings deterministically
before attempting general CSG. Reuse existing hosted-window positioning and
opening-boundary evidence, but move the void creation into FGKv1.

### Layer and panel lowering

Keep `LayerSetSpecification`, `PanelBoxSpecification`, and architectural intent
in the architecture layer. Lower their geometry to compounds, profiles,
extrusions, and instance sets through FGKv1. Do not duplicate their algorithms
inside the grammar interpreter.

### Repetition

Add `PathArray`, `SurfaceArray`, and deterministic `ScatterArray` with explicit
orientation, seed, spacing, and collision policies.

### Interface binding

Add kernel-level bindings from semantic interfaces to stable surface groups:

```text
PointGeometryInterface
AxisGeometryInterface
PlaneGeometryInterface
SurfaceGeometryInterface
EdgeGeometryInterface
VolumeGeometryInterface
```

These should reuse `SpatialInterfaceFrame`, compatibility, tolerance, and
clearance models instead of introducing a competing connection system.

## FGKv1-P2

Add the high-visual-value operators:

```text
EdgeTreatmentSpecification
├── ChamferTreatment
└── FilletTreatment

RoundedPanel
RibbonSweep
HeightField
AdaptiveTessellation
BranchJunctionBlend
SimpleSolidDifference
```

The current RoundedBox is a rounded 2D profile extrusion. It does not round all
twelve 3D edges. A true edge-treatment stage must operate on selected semantic
edges and preserve surface/material tags.

`LeafBlade` and `PetalBlade` should become domain adapters over a generic ribbon
surface plus width, camber, and tip/base functions.

## FGKv1-P3

Only after P0-P2 acceptance:

```text
General Boolean CSG
Implicit/SDF geometry
NURBS
Subdivision surfaces
Cage deformation
High-order C2 curvature constraints
```

These features must not delay profile holes, hosted openings, surface lofting,
mesh semantics, or real instancing.

## Recommended First Implementation Slice

The highest-value next code slice is not a new primitive. It is:

1. `ShapeRealizationService` facade.
2. grammar descriptors for the existing dormant builders.
3. `CompoundShapeSpecification` and instance-preserving realization.
4. canonical `MeshFinishingPipeline` with normal, UV, tangent, surface, material,
   and part groups.
5. imported-mesh normalization through the same result and validation contract.

This slice makes the existing kernel coherent and exposes current investment.
Only then should `ParametricSurface` and guide-constrained `SurfaceLoft` become
the next major geometry algorithm.

## Definition of FGKv1 Complete

FGKv1 is complete when all of these are true:

- Grammar and every domain layer submit `ShapeSpecification` or `ShapeGraph`
  objects, never GPU vertices.
- Every declared `ShapeFamily` has a validator, realizer, tests, deterministic
  key, bounds, topology expectation, collision policy, and evidence record.
- Imported geometry passes the same finishing and validation pipeline.
- Surface, material, object-part, and interface identities survive
  triangulation.
- Rendering and collision representations are distinct outputs of one validated
  realization.
- Arrays preserve one source mesh plus transforms unless explicitly flattened.
- Invalid rebuilds retain the previous valid geometry transactionally.
- Resource ceilings guarantee deterministic termination.
- Architecture, vehicle, vegetation, furniture, fixture, and mechanical
  acceptance scenes no longer depend on Cube/Cylinder/Sphere approximations for
  their defining forms.
