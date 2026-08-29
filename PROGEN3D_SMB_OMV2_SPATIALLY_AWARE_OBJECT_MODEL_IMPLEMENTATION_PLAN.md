# ProGen3D SMB-OMv2 Spatially Aware Object Model Implementation Plan

## Document Control

- **Date:** Friday, August 21, 2026
- **Status:** Implementation plan only; no SMB-OMv2 syntax or runtime behavior is promoted by this document.
- **Objective:** Add a first-class spatial building model whose transforms are derived from parent frames, interfaces, connections, constraints, collision queries, clearances, and immutable resolution evidence.
- **Compatibility authority:** SMB-OMv1, existing grammar syntax, primitive transforms, procedural geometry, preview, export, and physics remain unchanged until each SMB-OMv2 release gate passes.
- **Release authority:** Deterministic focused tests, evidence hashes, the full SMB-OMv2 fixture, visual overlays, the complete editor release gate, and a collected supervisor-mode run.

## 1. Executive Decision

SMB-OMv2 should be a semantic and spatial layer above the existing primitive scene. It should not be implemented as unrelated fields added to `ScenePrimitiveInstance`, and it should not use the dynamic physics loop as its assembly solver.

```text
SpatialBuildingModel
├── SpatialBuildingObject[]
├── SpatialContainmentTree
├── SpatialConnectionGraph
├── SpatialConstraintGraph
├── SpatialObjectGeometryBinding[]
└── SpatialResolutionRecord[]
        │
        └── references ScenePrimitiveInstance[]
```

The core rule is:

> A building object may own zero, one, or many rendered primitives. A primitive is geometry evidence, not the object's permanent identity.

This is required because:

- SMB containers and semantic relationships may have no geometry;
- one SMB object may emit several L5 primitives;
- object identity must survive geometry replacement;
- constraints must move an entire object assembly coherently;
- connections join interfaces rather than primitive indices;
- source selection currently identifies primitive occurrences, not stable building objects.

The target snapshot relationship is:

```text
GeneratedSceneSnapshot
├── GrammarDocument
│   └── SceneGenerationContext
│       └── ScenePrimitiveInstance[]
└── SpatialBuildingModel
```

During construction, `SceneGenerationContext` may own a mutable spatial construction context. After successful validation and resolution, the published `SpatialBuildingModel` is immutable.

## 2. Current Repository Baseline

### 2.1 Scene Instances

`ScenePrimitiveInstance` already owns primary and secondary transforms, authored initial transforms, dual transforms, motion state, mass, source range, immutable shape specifications, resolved geometry, and collision geometry cache state.

Evidence: `include/Context.h:109`.

`SceneGenerationContext::addPrimitive()` resolves geometry and copies the current transform scope into the primitive instance.

Evidence: `src/Context.cpp:2581`.

This is a strong geometry-instance layer, but it does not model stable building identity, containment, object aggregation, interfaces, connection endpoints, constraint dependencies, or placement evidence.

### 2.2 Transform Authority

`SpatialTransformScope` already provides matrix-authoritative translation, primary scaling, secondary scaling, dual-axis transforms, rotation, cloning, and restoration.

Evidence: `include/Scope.h:9` and `src/Scope.cpp:142`.

SMB-OMv2 must preserve the meaning of `T`, `S`, `D`, `DS*`, `DT*`, and `A`. It adds an object resolution transform rather than changing authored transform semantics.

### 2.3 Procedural Geometry

The repository already provides:

- immutable Cylinder, Sphere, and AxialProfile shape specifications;
- generic resolved triangle geometry;
- actual local bounds;
- topology hashes and watertightness;
- volume evidence and collision policy;
- semantic mesh surface tags;
- generic preview, export, picking, and collision geometry generation.

Evidence:

- `include/geometry/model/ShapeSpecification.h:11`
- `include/geometry/model/ResolvedPrimitiveGeometry.h:16`
- `include/geometry/model/MeshSurfaceTag.h:5`
- `src/geometry/service/PrimitiveGeometryResolver.cpp:42`

SMB-OMv2 must reuse these results rather than creating another mesh model.

### 2.4 Collision Infrastructure

The current runtime already has AABB bounds, OBB data, convex hulls, triangle meshes, BVH acceleration, SAT-style overlap, GJK/EPA convex collision, triangle-mesh overlap, penetration normals, depths, contact points, and mass-weighted dynamic correction.

Evidence:

- `include/Context.h:25`
- `include/physics/model/CollisionContact.h:5`
- `include/physics/service/ConvexCollisionDetector.h:6`
- `src/Context.cpp:1800`
- `tests/collision_positioning_harness.cpp:94`

The dynamic physics loop answers what happens over time. SMB-OMv2 needs a deterministic query-and-commit solver that reuses collision geometry without using gravity, velocity, sleep thresholds, or frame stepping as placement authority.

### 2.5 Connection Metadata and Overlays

The STL catalog already contains connection points, surfaces, detected features, centers, axes, dimensions, diameters, threads, planes, and evidence. The preview already draws catalog connection overlays.

Evidence: `include/StlCatalog.h:28`, `src/StlCatalog.cpp:622`, and `src/imgui_main.cpp:3342`.

This metadata is useful input, but it is catalog-specific. A later adapter should translate it into validated core `SpatialInterface` objects.

### 2.6 SMB-OMv1 Compatibility Fixture

SMB-OMv1 already provides:

- 124 unique SMB object rules;
- 48 complete L1-L5 templates;
- fixed structure and collision-active loose contents;
- fail-closed configuration validation;
- zero fake geometry for semantic and relationship leaves;
- finite scene bounds and supervisor-mode GUI evidence.

Evidence:

- `examples/SMB_OMv1_Instance_SMB_001_FiveDeep_Positioned.p3d:1`
- `docs/SMB_OMv1_SMALL_MODERN_BUILDING.md:1`
- `tests/run_smb_omv1_checks.sh:1`

SMB-OMv1 remains append-only compatibility evidence. SMB-OMv2 receives a new fixture.

### 2.7 Missing Capabilities

The current runtime does not yet provide:

```text
stable building object IDs
object-local frames relative to semantic containers
object-to-primitive aggregation
validated containment tree
object-owned interface frames
interface-to-interface connections
collision layers and masks per object
constraint dependency graph
translation-to-contact query service
atomic placement transactions
clearance validation
cycle diagnostics
resolution evidence in the snapshot
object-level selection and inspection
```

## 3. Architectural Principles

### 3.1 Identity Is Not Geometry

`SpatialBuildingObject` must not inherit from `ScenePrimitiveInstance`.

```text
SpatialBuildingObject
    1
    ├── 0..* SpatialObjectGeometryBinding
    │          └── 1 ScenePrimitiveInstance
    └── 0..* SpatialInterface
```

### 3.2 Authored and Resolved Placement Are Separate

Every object stores:

```text
authored_local_transform
resolution_local_transform
resolved_world_transform
```

The solver never destroys authored placement.

### 3.3 Three Independent Graphs

```text
Containment tree
    What contains this object?

Connection graph
    What interfaces are functionally or physically connected?

Constraint graph
    What placement dependencies determine the transform?
```

### 3.4 Placement Is Transactional

A placement operation must:

1. validate references and compatibility;
2. evaluate candidate transforms without mutating the scene;
3. record search and residual evidence;
4. validate required contact and forbidden collision;
5. commit all affected primitives together;
6. publish no partial model on failure.

### 3.5 Interfaces Are Frames

A P0 interface stores an origin, normal, tangent axis, type, region, compatibility profile, tolerance, and clearance policy. The tangent axis is required so later orientation alignment can resolve roll ambiguity.

### 3.6 Fail Closed

Duplicate IDs, missing containers, graph cycles, missing interfaces, incompatible connections, zero directions, non-finite values, impossible travel, forbidden collision, and unresolved required constraints abort spatial publication.

## 4. Target Object Model

### 4.1 Aggregate Root

```cpp
class SpatialBuildingModel
{
public:
    const SpatialObjectRegistry &objects() const;
    const SpatialContainmentTree &containmentTree() const;
    const SpatialConnectionGraph &connectionGraph() const;
    const SpatialConstraintGraph &constraintGraph() const;
    const std::vector<SpatialObjectGeometryBinding> &geometryBindings() const;
    const std::vector<SpatialResolutionRecord> &resolutionRecords() const;
};
```

### 4.2 Identity

```cpp
class SpatialObjectIdentity
{
public:
    const SpatialObjectId &objectId() const;
    const std::string &objectName() const;
    const SpatialObjectClass &objectClass() const;
    const SpatialTaxonomyPath &taxonomyPath() const;
    std::size_t instanceIndex() const;
    std::uint64_t revision() const;
    const SpatialObjectProvenance &provenance() const;
};
```

Supporting purpose classes:

```text
SpatialObjectId
SpatialObjectClass
SpatialTaxonomyPath
SpatialObjectProvenance
```

P0 identity rules:

- grammar IDs use safe identifiers such as `SMB001_Ground_Kitchen_Sink01`;
- explicit IDs are unique in one expanded model;
- repeated declarations require an explicit deterministic instance policy;
- primitive indices are never permanent object IDs;
- identical source, seed, time, and configuration produce the same ID set.

### 4.3 Spatial Object

```cpp
class SpatialBuildingObject
{
public:
    const SpatialObjectIdentity &identity() const;
    const SpatialFrameState &frameState() const;
    const SpatialBoundaryModel &boundaryModel() const;
    const std::vector<SpatialInterface> &interfaces() const;
    const CollisionParticipationPolicy &collisionPolicy() const;
    SpatialObjectState state() const;
};
```

The class composes purpose-specific models. It should not become a large mutable struct.

### 4.4 Frame State

```cpp
class SpatialFrameState
{
public:
    const glm::mat4 &authoredLocalTransform() const;
    const glm::mat4 &resolutionLocalTransform() const;
    const glm::mat4 &resolvedWorldTransform() const;
    const SpatialFrameReference &parentFrame() const;
    const SpatialPoseUncertainty &uncertainty() const;
};
```

P0 uncertainty is explicitly zero. Nonzero uncertainty is deferred, but the frame model should not require redesign later.

### 4.5 Containment

```cpp
class SpatialContainmentRelationship
{
public:
    const SpatialObjectId &containerId() const;
    const SpatialObjectId &containedObjectId() const;
};
```

```cpp
class SpatialContainmentTree
{
public:
    const SpatialObjectId &rootObjectId() const;
    const SpatialObjectId *parentOf(const SpatialObjectId &object_id) const;
    const std::vector<SpatialObjectId> &childrenOf(
        const SpatialObjectId &object_id) const;
};
```

Validation:

```text
exactly one root
every non-root object has one container
all containers exist
no self containment
no containment cycle
parent world frame resolves before child
geometry-free containers are valid
```

### 4.6 Geometry Binding

```cpp
class SpatialObjectGeometryBinding
{
public:
    const SpatialObjectId &objectId() const;
    std::size_t primitiveInstanceIndex() const;
    const glm::mat4 &primitiveLocalToObjectTransform() const;
    const std::vector<MeshSurfaceTag> &boundSurfaceTags() const;
};
```

```cpp
class SpatialObjectGeometryBindingService
{
public:
    bool applyResolvedObjectTransform(
        const SpatialBuildingObject &object,
        const std::vector<SpatialObjectGeometryBinding> &bindings,
        SceneGenerationContext *scene_context,
        std::string *diagnostic) const;
};
```

The service updates all bound primitives coherently, preserves initial transforms, updates metadata, and marks collision geometry dirty.

### 4.7 Boundary Representations

Use inheritance only for real representation categories:

```cpp
class BoundaryRepresentation
{
public:
    virtual ~BoundaryRepresentation() = default;
    virtual BoundaryRepresentationKind kind() const = 0;
};

class AxisAlignedBoundingBoundary : public BoundaryRepresentation { ... };
class OrientedBoundingBoundary : public BoundaryRepresentation { ... };
class ConvexHullBoundary : public BoundaryRepresentation { ... };
class TriangleMeshBoundary : public BoundaryRepresentation { ... };
class AnalyticSurfaceBoundary : public BoundaryRepresentation { ... };
class CompoundBoundary : public BoundaryRepresentation { ... };
```

P0 authorizes:

```text
Broad phase       AxisAlignedBoundingBoundary
Positioning       AxisAlignedBoundingBoundary
Exact query       existing CollisionGeometry adapter when available
```

P1 promotes OBB, convex, triangle mesh, analytic surface, and surface-tag boundaries.

### 4.8 Interface Model

```cpp
enum class SpatialInterfaceType
{
    Support,
    Bearing,
    Mate,
    Seat,
    Insert,
    Socket,
    Shaft,
    Seal,
    Fastener,
    Anchor,
    Hinge,
    Slide,
    PipePort,
    DuctPort,
    ElectricalPort,
    DataPort,
    ControlPort,
    DrainPort,
    ThermalInterface,
    InspectionInterface
};
```

```cpp
class SpatialInterfaceFrame
{
public:
    const glm::vec3 &localOrigin() const;
    const glm::vec3 &localNormal() const;
    const glm::vec3 &localTangent() const;
    glm::vec3 localBitangent() const;
};
```

```cpp
class SpatialInterface
{
public:
    const SpatialInterfaceId &interfaceId() const;
    const SpatialObjectId &ownerObjectId() const;
    SpatialInterfaceType type() const;
    const SpatialInterfaceFrame &localFrame() const;
    const SpatialInterfaceRegion &region() const;
    const InterfaceCompatibilityProfile &compatibility() const;
    const SpatialClearanceRequirement &clearance() const;
    SpatialInterfaceState state() const;
};
```

P0 regions:

```text
Point
PlaneRectangle
AxisSegment
ObjectBoundaryFace
```

Frame validation requires finite values, nonzero normal and tangent, nonparallel axes, and deterministic orthonormalization.

### 4.9 Compatibility

```cpp
class InterfaceCompatibilityProfile
{
public:
    InterfaceShape shape() const;
    InterfaceGender gender() const;
    float nominalDiameter() const;
    float nominalWidth() const;
    float nominalHeight() const;
    const std::string &threadType() const;
    float threadPitch() const;
    const std::string &flowType() const;
    float voltage() const;
    const std::string &pressureClass() const;
};
```

P0 enforcement covers interface type pairing, shape, diameter or rectangular size tolerance, optional gender, and finite values. Thread, pressure, voltage, material, fire, and load enforcement are later releases.

### 4.10 Connections

```cpp
enum class SpatialConnectionType
{
    Contains,
    ConnectedTo,
    DrainsTo,
    Powers,
    ServedBy,
    Monitors,
    SupportedBy,
    SeatedIn,
    InsertedIn,
    SealedTo,
    FixedTo,
    AlignedWith
};
```

```cpp
class SpatialConnection
{
public:
    const SpatialConnectionId &connectionId() const;
    const SpatialInterfaceReference &sourceInterface() const;
    const SpatialInterfaceReference &targetInterface() const;
    SpatialConnectionType connectionType() const;
    const InterfaceAlignmentRequirement &alignmentRequirement() const;
    const SpatialClearanceRequirement &clearance() const;
    float insertionDepth() const;
    const ConstraintDegreeOfFreedomState &lockedDegreesOfFreedom() const;
    SpatialConnectionState state() const;
};
```

Connections state relationships. They may generate constraints, but they are not themselves transform commands.

### 4.11 Constraints

Use a real constraint hierarchy:

```cpp
class SpatialConstraint
{
public:
    virtual ~SpatialConstraint() = default;
    virtual SpatialConstraintKind kind() const = 0;
    virtual const SpatialConstraintId &constraintId() const = 0;
    virtual int priority() const = 0;
};

class CoincidentConstraint : public SpatialConstraint { ... };
class ParallelConstraint : public SpatialConstraint { ... };
class PerpendicularConstraint : public SpatialConstraint { ... };
class AxisAlignedConstraint : public SpatialConstraint { ... };
class CenteredConstraint : public SpatialConstraint { ... };
class ContactConstraint : public SpatialConstraint { ... };
class ClearanceConstraint : public SpatialConstraint { ... };
class InsertionConstraint : public SpatialConstraint { ... };
class SeatedConstraint : public SpatialConstraint { ... };
class SupportedConstraint : public SpatialConstraint { ... };
class ContainmentConstraint : public SpatialConstraint { ... };
class CollisionPositionConstraint : public SpatialConstraint { ... };
class AvoidCollisionConstraint : public SpatialConstraint { ... };
```

P0 gives solving authority only to translation collision positioning, clearance, and containment validation. Unimplemented modes fail explicitly.

### 4.12 Collision Position Constraint

```cpp
enum class CollisionPositionMode
{
    Touch,
    Gap,
    Drop,
    Seat,
    Insert,
    Tangent,
    Between,
    CenterContact
};
```

```cpp
class CollisionPositionConstraint : public SpatialConstraint
{
public:
    const SpatialObjectId &movingObjectId() const;
    const SpatialInterfaceId &movingInterfaceId() const;
    const SpatialObjectId &targetObjectId() const;
    const SpatialInterfaceId &targetInterfaceId() const;
    const SpatialDirection &direction() const;
    CollisionPositionMode mode() const;
    const SpatialClearanceRequirement &clearance() const;
    float seatingDepth() const;
    float maximumDistance() const;
    float tolerance() const;
    const CollisionLayerMask &collisionMask() const;
};
```

P0 modes:

```text
Touch
Gap
Drop
```

P2 modes:

```text
Seat
Insert
Tangent
Between
CenterContact
```

### 4.13 Collision Layers

```cpp
enum class CollisionLayer
{
    Structure,
    Envelope,
    Interior,
    Furniture,
    Plumbing,
    Hvac,
    Electrical,
    Equipment,
    Terrain,
    Temporary
};
```

A pair blocks only when both masks permit the other layer.

### 4.14 Query Result

```cpp
class SpatialRelationResult
{
public:
    const SpatialObjectId &firstObjectId() const;
    const SpatialObjectId &secondObjectId() const;
    SpatialRelationKind relation() const;
    float distance() const;
    float penetration() const;
    const glm::vec3 &contactPoint() const;
    const glm::vec3 &contactNormal() const;
    const glm::vec3 &closestPointOnFirst() const;
    const glm::vec3 &closestPointOnSecond() const;
    float confidence() const;
};
```

P0 guarantees AABB distance, overlap, containment, above, below, and adjacency. Exact closest surfaces are P1.

### 4.15 State Models

```cpp
enum class SpatialObjectState
{
    Unplaced,
    Approximate,
    Positioned,
    ContactResolved,
    Connected,
    Constrained,
    Validated,
    Invalid
};
```

```cpp
enum class SpatialConnectionState
{
    Proposed,
    Compatible,
    Positioned,
    Engaged,
    Seated,
    Locked,
    Verified,
    Broken,
    Invalid
};
```

State transition services validate legal progression. Public code should not assign arbitrary state values.

### 4.16 Resolution Evidence

```cpp
class SpatialResolutionRecord
{
public:
    const SpatialConstraintId &constraintId() const;
    const SpatialObjectId &sourceObjectId() const;
    const SpatialObjectId &targetObjectId() const;
    const glm::mat4 &initialWorldTransform() const;
    const glm::mat4 &finalWorldTransform() const;
    SpatialResolutionAlgorithm algorithm() const;
    int broadPhaseStepCount() const;
    int refinementIterationCount() const;
    float tolerance() const;
    const glm::vec3 &contactPoint() const;
    const glm::vec3 &contactNormal() const;
    float resultingClearance() const;
    float residualError() const;
    SpatialResolutionStatus status() const;
    const std::vector<std::string> &warnings() const;
    std::uint64_t evidenceHash() const;
};
```

Resolution evidence becomes immutable after snapshot publication.

## 5. Transform and Frame Authority

### 5.1 Composition

```text
object_world =
    parent_world
    * authored_object_local
    * resolution_object_local

primitive_world =
    object_world
    * primitive_local_to_object
```

At binding time:

```text
primitive_local_to_object =
    inverse(authored_object_world)
    * authored_primitive_world
```

Spatial object frames must be finite and invertible.

### 5.2 Direction Frames

```cpp
enum class SpatialDirectionFrame
{
    World,
    Parent,
    MovingObject,
    MovingInterface,
    TargetObject,
    TargetInterface
};
```

Every direction declares its frame. P0 grammar should default collision approach directions to `TargetInterface`, not guess world space.

### 5.3 Object Scope Binding

While an object scope is active:

1. capture the authored object world matrix;
2. execute existing transform actions normally;
3. when `I()` emits geometry, calculate primitive-local-to-object;
4. append a geometry binding;
5. preserve primitive source ranges;
6. bind child object primitives only to the nearest object scope.

### 5.4 Reset Semantics

`resetSimulation()` continues restoring authored dynamic state.

Add a distinct `SpatialAssemblyResolutionService::resetResolvedPlacement()` operation that restores resolution transforms to identity and reapplies authored object and primitive transforms.

## 6. Canonical P0 Grammar Direction

Use a dedicated composite object scope, not pending global object state.

```p3d
SMB_001_Ground_Kitchen_CabinetRun ->
Object(
    id(SMB001_Ground_Kitchen_CabinetRun)
    class(Cabinet)
    taxonomy(Building GroundFloor Kitchen Furniture Cabinet)
    container(SMB001_Ground_Kitchen_01)
    layer(Furniture)
    mask(Structure Interior Furniture)
)
[
    Interface(
        id(bottom)
        type(Support)
        origin(0 -0.5 0)
        normal(0 -1 0)
        tangent(1 0 0)
        region(PlaneRectangle 3 0.7)
        tolerance(0.001)
    )

    Interface(
        id(back)
        type(Mate)
        origin(0 0 0.35)
        normal(0 0 1)
        tangent(1 0 0)
        region(PlaneRectangle 3 1)
        tolerance(0.001)
    )

    Position(
        id(DropCabinetToFloor)
        moving(bottom)
        target(SMB001_GroundFloor upperSurface)
        mode(Drop)
        direction(TargetInterface 0 0 -1)
        clearance(0)
        maxDistance(3)
        tolerance(0.0005)
        priority(10)
    )

    Position(
        id(BackCabinetFromWall)
        moving(back)
        target(SMB001_Ground_Kitchen_Wall internalFace)
        mode(Gap)
        direction(TargetInterface 0 0 -1)
        clearance(0.01)
        maxDistance(2)
        tolerance(0.0005)
        priority(20)
    )

    S(3 1 0.7)
    I(Cube material(offwhiteceramic) 1 0.3)
]
```

The exact syntax is new and requires dedicated parser tests. The structural invariant is that `Object(...)` owns its body token list, so object state cannot leak into a later unrelated `I()`.

### 6.1 Dedicated Syntax Models

```text
SpatialObjectDescriptorSyntax
SpatialTaxonomySyntax
SpatialInterfaceDeclarationSyntax
SpatialConnectionDeclarationSyntax
SpatialConstraintDeclarationSyntax
CollisionParticipationSyntax
SpatialClearanceSyntax
SpatialDirectionSyntax
SpatialObjectScopeToken
```

Do not encode spatial metadata in generic `GrammarActionToken::arguments` or `var_names`.

### 6.2 Expression Preservation

Numeric transforms, interface coordinates, dimensions, tolerances, clearances, travel limits, and priorities remain `GeometryExpression` values until expansion. IDs, classes, interface names, enum names, and reference paths are identifiers.

This preserves grammar variables, rule arguments, `t`, deterministic random declarations, and checked finite expression evaluation.

### 6.3 Two-Pass Construction

```text
Pass 1
    expand grammar
    create evaluated objects
    capture primitive bindings
    collect interfaces, connections, and constraints

Pass 2
    resolve references
    validate containment
    validate interface compatibility
    build connection and constraint graphs
    detect cycles
    resolve placement
    publish immutable model
```

Constraint behavior must not depend on source order.

### 6.4 Semantic Relationship Rules

Existing zero-geometry relationship rules should create connections without fake primitives.

```p3d
SMB_REL_KitchenSink_CONNECTED_TO_ColdWater ->
Connect(
    id(KitchenSinkColdWater)
    source(SMB001_Ground_Kitchen_Sink coldWaterInlet)
    target(SMB001_Plumbing_ColdWaterRiser outlet)
    type(ConnectedTo)
)
```

## 7. Spatial Query Architecture

### 7.1 Boundary Derivation

```cpp
class SpatialObjectBoundaryDerivationService
{
public:
    SpatialBoundaryModel derive(
        const SpatialBuildingObject &object,
        const std::vector<SpatialObjectGeometryBinding> &bindings,
        const SceneGenerationContext &scene_context) const;
};
```

P0 uses the union AABB of all bound primitives after applying a candidate object transform. Geometry-free objects require explicit interface regions to participate in positioning.

### 7.2 Query Services

```text
SpatialAabbQueryService
SpatialContainmentQueryService
SpatialDistanceQueryService
SpatialCollisionQueryService
SpatialInterfaceQueryService
```

Do not introduce a vague `SpatialManager` or `SpatialUtil`.

### 7.3 Shared Collision Models

Extract collision models currently declared in `Context.h` so physics and spatial services can share them without depending on the complete scene context.

```text
include/geometry/model/AxisAlignedBounds.h
include/physics/model/CollisionTriangle.h
include/physics/model/CollisionOrientedBox.h
include/physics/model/CollisionGeometry.h
```

Keep temporary compatibility aliases if needed. Do not duplicate GJK, SAT, triangle overlap, or BVH code.

## 8. Deterministic Collision Positioning

### 8.1 Solver Interface

```cpp
class CollisionPositioningSolver
{
public:
    SpatialConstraintResolutionResult solve(
        const CollisionPositionConstraint &constraint,
        const SpatialBuildingModel &model,
        const SceneGenerationContext &scene_context) const;
};
```

The solver returns a candidate result and never mutates the scene.

### 8.2 Validation

Before search:

```text
moving and target objects exist
moving differs from target
interfaces exist and belong to their owners
interfaces are compatible
direction is finite and nonzero
direction frame resolves
maximum distance is finite and positive
tolerance is finite and positive
clearance is finite and supported
object frames are invertible
moving and target boundaries exist
starting overlap policy is supported
collision masks permit target contact
```

### 8.3 Candidate Evaluation

```text
candidate_world(s) = initial_world * translation_world(s)
```

Candidate transforms are evaluated against derived boundaries without updating primitive instances.

### 8.4 Bracketing

Find deterministic samples:

```text
s_separated
s_intersecting
```

such that the first is separated and the second intersects the target.

The step is derived from tolerance and projected extent, capped by maximum travel and solver ceilings.

Suggested limits:

```text
broad-phase steps       <= 4096
binary refinements      <= 64
minimum tolerance       >= 0.000001 scene units
```

### 8.5 Refinement

```text
s_mid = (s_separated + s_intersecting) / 2
```

Repeat until the bracket width is within tolerance.

### 8.6 Clearance

For `Touch` and `Drop`:

```text
s_final = s_separated
```

For `Gap` with nonnegative clearance `c`:

```text
s_final = max(0, s_separated - c)
```

Clearance sign and direction are fixed by the contract and tested. They are not inferred after search.

### 8.7 Final Validation

```text
required contact or gap is present
clearance is within tolerance
moving interface overlaps target region
no forbidden collision exceeds slop
higher-priority constraints remain satisfied
final transform is finite
no primitive is removed or out of bounds
```

### 8.8 Sequential P0 Constraints

P0 may resolve multiple translation constraints in priority order when the graph is acyclic, each constraint removes one translational degree of freedom, orientation remains unchanged, and later constraints preserve earlier residuals.

This supports:

```text
Drop cabinet to floor
then
move cabinet to wall with 10 mm gap
```

P0 does not perform global numerical optimization.

### 8.9 Placement Transaction

```cpp
class SpatialPlacementTransaction
{
public:
    void stageObjectTransform(const SpatialObjectId &object_id,
                              const glm::mat4 &resolution_local_transform);
    void stageResolutionRecord(SpatialResolutionRecord record);
    bool validate(...);
    bool commit(...);
    void discard();
};
```

Commit order:

1. stage object transforms;
2. derive affected primitive transforms;
3. rebuild candidate object boundaries;
4. validate residuals and forbidden collisions;
5. update the construction model;
6. update bound primitives;
7. append evidence;
8. publish only after every constraint succeeds.

## 9. Dependency and Assembly Resolution

A positioning dependency creates an edge:

```text
target object -> moving object
```

```cpp
class SpatialConstraintDependencyAnalyzer
{
public:
    SpatialConstraintDependencyReport analyze(
        const SpatialConstraintGraph &constraint_graph) const;
};
```

The report contains topological object order, topological constraint order, unresolved references, and an exact cycle path.

Cycle diagnostic:

```text
Spatial constraint cycle:
Cabinet01
-> Benchtop01
-> Sink01
-> Cabinet01
```

No source-order fallback is allowed.

```cpp
class SpatialAssemblyResolutionService
{
public:
    SpatialAssemblyResolutionResult resolve(
        SpatialBuildingModel construction_model,
        SceneGenerationContext *scene_context) const;
};
```

Responsibilities:

```text
validate containment and frames
validate interfaces and connections
build dependency order
execute deterministic solvers
advance object and connection states
revalidate prior constraints
create immutable resolution records
return a complete result or fail closed
```

## 10. Grammar Integration

### 10.1 Parser Services

```text
SpatialObjectSpecificationParser
SpatialInterfaceDeclarationParser
SpatialConnectionDeclarationParser
SpatialConstraintDeclarationParser
```

Do not extend the `I()` parser with unrelated spatial syntax.

### 10.2 Evaluators

```text
SpatialObjectSpecificationEvaluator
SpatialInterfaceDeclarationEvaluator
SpatialConnectionDeclarationEvaluator
SpatialConstraintDeclarationEvaluator
```

They use the existing checked expression callback.

### 10.3 Construction Context

```cpp
class SpatialBuildingModelConstructionContext
{
public:
    void beginObject(...);
    void bindPrimitive(...);
    void addInterface(...);
    void addConnection(...);
    void addConstraint(...);
    void endObject(...);
    SpatialBuildingModel finalize(...);
};
```

Construction-time state is mutable. Published model classes are immutable.

### 10.4 Diagnostics

Every nested spatial syntax node stores a source range.

Required errors:

```text
duplicate object ID
undefined container
containment cycle
duplicate interface ID
undefined object or interface reference
incompatible interfaces
invalid direction frame
zero direction
invalid clearance
unsupported positioning mode
constraint cycle
no contact within maximum distance
forbidden collision
residual violation
non-invertible frame
safety ceiling exceeded
```

A failed spatial expansion publishes neither a partial model nor a partially positioned scene.

## 11. Scene and Snapshot Integration

### 11.1 Construction Ownership

Extend `SceneGenerationContext` by composition with a `SpatialBuildingModelConstructionContext`. Expose narrow operations rather than public mutable collections.

### 11.2 Snapshot Publication

Add:

```cpp
const SpatialBuildingModel *GeneratedSceneSnapshot::spatialBuildingModel() const;
```

The result may be null for legacy grammars. Legacy scenes remain valid without synthetic spatial objects.

### 11.3 Source and Primitive Associations

Keep the current primitive source association index and add:

```text
SpatialObjectSourceAssociation
SpatialObjectSourceAssociationIndex
SpatialObjectPrimitiveAssociationIndex
```

This supports primitive-to-object lookup, object-wide highlighting, navigation to object declarations, and inspection of geometry-free semantic objects.

## 12. Editor Integration

### 12.1 Spatial Object Inspector

Add:

```text
SpatialObjectInspection
SpatialObjectInspectionService
SpatialObjectInspectorPanel
```

Display:

```text
object ID, class, taxonomy, container, state
authored local, resolution, and world transforms
uncertainty
primitive bindings
boundary representations
collision layer and mask
interfaces
connections
constraints
resolution records and residuals
evidence hash
source range
```

Do not continue enlarging `ScenePrimitiveInspection` with the full object model.

### 12.2 Object and Graph Views

Staged views:

```text
Containment Tree
Connection Graph
Constraint Dependency Graph
Resolution Timeline
```

P0 may use hierarchical lists, but the underlying graph models remain authoritative.

### 12.3 Overlays

Extend preview overlays with:

```text
object frames
interface frames and regions
connection links
constraint directions
sweep paths
contact points and normals
clearance segments
forbidden collision highlights
object AABBs
resolution status labels
```

Stable colors:

```text
green     validated
yellow    approximate or pending
cyan      interface
blue      connection
orange    clearance
magenta   constraint direction
red       invalid or forbidden collision
```

### 12.4 Visual Evidence

Capture authored, resolved, interface, constraint, contact, clearance, cycle-failure, and collision-failure views. Repeated valid captures must be byte-identical for the same source, seed, and time.

## 13. Progressive Fixtures

### 13.1 Cabinet Fixture

```text
Building
└── GroundFloor
    └── Kitchen
        ├── KitchenWall
        └── Cabinet01
```

Requirements:

```text
bottom support interface
back mate interface
Drop to floor
Gap 10 mm from wall
two resolution records
zero forbidden collision
```

### 13.2 Sink and Benchtop Fixture

Objects:

```text
Cabinet01
Benchtop01
Sink01
WastePipe01
ColdWaterPipe01
HotWaterPipe01
```

Requirements include interface compatibility, support placement, and the required plumbing connection graph. Centering and orientation become P2 solving gates.

### 13.3 Window Fixture

Objects:

```text
ExternalWall01
WindowOpening01
Window01
```

P0 proves containment, sill support, perimeter clearance, and collision validation. Full normal/tangent alignment is P2.

### 13.4 Pipe and Elbow Fixture

Proves axis interface compatibility and diameter tolerance. Insertion and internal-stop seating are P2.

### 13.5 Full Fixture

Add without modifying v1:

```text
examples/SMB_OMv2_Instance_SMB_001_SpatiallyAware.p3d
docs/SMB_OMv2_SPATIALLY_AWARE_BUILDING.md
tests/smb_omv2_grammar_scene_harness.cpp
tests/run_smb_omv2_checks.sh
```

## 14. Implementation Milestones

### Milestone 0: Freeze the Contract

Tasks:

```text
identity repeat policy
transform composition
direction frame semantics
Touch/Gap/Drop clearance signs
default collision layer matrix
legal state transitions
evidence hash serialization
safety ceilings
```

Gate: every invariant has positive and negative programmatic tests; no grammar change exists yet.

### Milestone 1: Extract Shared Boundary Models

Tasks:

```text
extract bounds, triangles, OBB data, and CollisionGeometry
preserve compatibility aliases
remove model responsibilities from Context.h
```

Gate: all existing collision, transform, and curved geometry suites pass unchanged.

### Milestone 2: Spatial Domain Kernel

Tasks:

```text
identity and taxonomy
frame state and zero uncertainty
containment tree
interfaces and compatibility
connections and graph
constraint hierarchy
collision layers
geometry bindings
resolution evidence hashing
immutable model builders
```

Gate: duplicate IDs, missing parents, cycles, invalid frames, and missing endpoints fail deterministically.

### Milestone 3: Spatial Queries

Tasks:

```text
compound object AABBs
candidate-transform queries
containment and distance queries
collision query adapter
interface world frames and region projection
```

Gate: nested and multi-primitive object queries pass without scene mutation.

### Milestone 4: Translation Positioning

Tasks:

```text
validation and compatibility
bounded bracketing
binary refinement
clearance
forbidden collision validation
residual checks
transactions
sequential orthogonal constraints
cabinet programmatic fixture
```

Gate: cabinet floor and wall placement passes with identical transforms, iterations, and evidence hashes across runs.

### Milestone 5: Dependency Resolution

Tasks:

```text
topological ordering
cycle paths
stable equal-priority ordering
higher-priority residual revalidation
full rollback
```

Gate: source order does not affect results and failed assemblies leave no mutations.

### Milestone 6: Grammar Syntax

Tasks:

```text
Object composite scope
Interface declarations
Connect declarations
Position declarations
layer and mask
two-pass semantic construction
source diagnostics
syntax highlighting and autocomplete
canonical printing
```

Gate: grammar fixture matches programmatic fixture evidence exactly and legacy grammar remains unchanged.

### Milestone 7: Snapshot and Editor Evidence

Tasks:

```text
optional spatial snapshot
object source and primitive indices
object selection
spatial inspector
hierarchy lists
overlays
visual capture
```

Gate: objects with zero, one, and many primitives are selectable and evidence matches runtime values.

### Milestone 8: Progressive Real Assemblies

Tasks:

```text
cabinet
sink and benchtop
window opening
pipe and elbow
invalid compatibility, collision, reference, and cycle fixtures
```

Gate: every valid fixture has deterministic JSON and visual evidence; invalid fixtures fail with exact diagnostics.

### Milestone 9: Full 124-Object SMB-OMv2

Tasks:

```text
stable IDs and classes for all 124 objects
one containment tree
interface-level conversion of relationship rules
interfaces for structure, fixtures, services, and equipment
derived placement constraints where reliable
explicit authored placement where no solver exists
collision layers and masks
priority validity predicates
full visual and inspector evidence
```

Gate: all full-model acceptance conditions pass while SMB-OMv1 remains green.

### Milestone 10: P1 Exact Boundaries

Tasks:

```text
OBB, convex, and triangle boundaries
surface-tag interfaces
AxialProfile interfaces
catalog interface adapter
exact nearest surface and interface queries
surface IDs in evidence
```

Gate: exact results are identified as exact, AABB fallback remains visible, and surface bindings survive procedural regeneration.

### Milestone 11: P2 Orientation and Insertion

Tasks:

```text
normal alignment
tangent alignment
DOF tracking
centering
insertion
seating depth
contact patches
supported simultaneous constraints
```

Gate: window and pipe fixtures resolve orientation and insertion without roll ambiguity or forbidden penetration.

### Milestone 12: P3 Uncertainty and Adaptation

Tasks:

```text
pose covariance
uncertainty propagation
tolerance versus uncertainty validation
automatic interface discovery
governed geometry adaptation proposals
```

Gate: no placement claims validated status when accumulated uncertainty exceeds tolerance; every adaptive change retains provenance and rollback.

## 15. Proposed Source Layout

### 15.1 Spatial Models

```text
include/spatial/model/
    SpatialObjectId.h
    SpatialObjectClass.h
    SpatialTaxonomyPath.h
    SpatialObjectProvenance.h
    SpatialObjectIdentity.h
    SpatialFrameReference.h
    SpatialPoseUncertainty.h
    SpatialFrameState.h
    SpatialObjectState.h
    SpatialBuildingObject.h
    SpatialBuildingModel.h
    BoundaryRepresentation.h
    AxisAlignedBoundingBoundary.h
    OrientedBoundingBoundary.h
    ConvexHullBoundary.h
    TriangleMeshBoundary.h
    AnalyticSurfaceBoundary.h
    CompoundBoundary.h
    SpatialBoundaryModel.h
    SpatialInterfaceId.h
    SpatialInterfaceType.h
    SpatialInterfaceFrame.h
    SpatialInterfaceRegion.h
    InterfaceCompatibilityProfile.h
    SpatialInterfaceState.h
    SpatialInterface.h
    SpatialConnectionId.h
    SpatialConnectionType.h
    SpatialConnectionState.h
    SpatialConnection.h
    SpatialConstraintId.h
    SpatialConstraint.h
    CollisionPositionConstraint.h
    ClearanceConstraint.h
    ContainmentConstraint.h
    SpatialClearanceRequirement.h
    SpatialDirection.h
    CollisionLayer.h
    CollisionLayerMask.h
    CollisionParticipationPolicy.h
    SpatialRelationResult.h
    SpatialResolutionRecord.h
    SpatialResolutionStatus.h
    ConstraintDegreeOfFreedomState.h
```

### 15.2 Relationships and Graphs

```text
include/spatial/relationship/
    SpatialContainmentRelationship.h
    SpatialObjectGeometryBinding.h
    SpatialInterfaceReference.h
    SpatialObjectSourceAssociation.h
    SpatialObjectPrimitiveAssociation.h

include/spatial/graph/
    SpatialObjectRegistry.h
    SpatialContainmentTree.h
    SpatialConnectionGraph.h
    SpatialConstraintGraph.h
```

### 15.3 Services

```text
include/spatial/service/
src/spatial/service/
    SpatialBuildingModelConstructionService
    SpatialContainmentValidationService
    SpatialWorldFrameResolutionService
    SpatialObjectBoundaryDerivationService
    SpatialAabbQueryService
    SpatialContainmentQueryService
    SpatialDistanceQueryService
    SpatialCollisionQueryService
    SpatialInterfaceQueryService
    SpatialInterfaceCompatibilityService
    SpatialConstraintDependencyAnalyzer
    CollisionPositioningSolver
    SpatialPlacementTransaction
    SpatialAssemblyResolutionService
    SpatialObjectGeometryBindingService
    SpatialValidityEvaluationService
    SpatialResolutionEvidenceHashService
    CatalogSpatialInterfaceImportService
```

### 15.4 Grammar

```text
include/grammar/model/
    SpatialObjectDescriptorSyntax.h
    SpatialTaxonomySyntax.h
    SpatialInterfaceDeclarationSyntax.h
    SpatialConnectionDeclarationSyntax.h
    SpatialConstraintDeclarationSyntax.h
    SpatialClearanceSyntax.h
    SpatialDirectionSyntax.h
    SpatialObjectScopeToken.h

include/grammar/service/
src/grammar/service/
    SpatialObjectSpecificationParser
    SpatialInterfaceDeclarationParser
    SpatialConnectionDeclarationParser
    SpatialConstraintDeclarationParser
    SpatialObjectSpecificationEvaluator
    SpatialInterfaceDeclarationEvaluator
    SpatialConnectionDeclarationEvaluator
    SpatialConstraintDeclarationEvaluator
```

### 15.5 Editor

```text
include/editor/model/
    SpatialObjectInspection.h
    SpatialGraphInspection.h
    SpatialResolutionInspection.h

include/editor/relationship/
    SpatialObjectSourceAssociationIndex.h
    SpatialObjectPrimitiveAssociationIndex.h

include/editor/service/
    SpatialObjectInspectionService.h
    SpatialOverlayEvidenceService.h

include/editor/presentation/
    SpatialObjectInspectorPanel.h
    SpatialHierarchyPanel.h
    SpatialConnectionPanel.h
    SpatialConstraintPanel.h
```

### 15.6 Existing Files Expected to Change

```text
include/Context.h
src/Context.cpp
include/grammar.h
src/Grammar.cpp
include/editor/model/GeneratedSceneSnapshot.h
include/editor/model/ScenePreviewSession.h
include/editor/model/SceneOverlayConfiguration.h
src/imgui_main.cpp
src/editor/presentation/GrammarSyntaxPresentation.cpp
src/editor/presentation/GrammarAutocompletePresentation.cpp
Makefile
tests/run_editor_architecture_checks.sh
tests/run_release_checks.sh
.github/workflows/ci.yml
tests/README.md
```

## 16. Test Plan

### 16.1 Focused Harnesses

```text
tests/spatial_object_model_harness.cpp
tests/spatial_containment_tree_harness.cpp
tests/spatial_interface_compatibility_harness.cpp
tests/spatial_boundary_query_harness.cpp
tests/collision_positioning_solver_harness.cpp
tests/spatial_constraint_graph_harness.cpp
tests/spatial_assembly_resolution_harness.cpp
tests/spatial_grammar_scene_harness.cpp
tests/spatial_editor_inspection_harness.cpp
tests/smb_omv2_grammar_scene_harness.cpp
```

Runner scripts:

```text
tests/run_spatial_object_model_checks.sh
tests/run_spatial_query_checks.sh
tests/run_spatial_positioning_checks.sh
tests/run_spatial_grammar_scene_checks.sh
tests/run_spatial_editor_checks.sh
tests/run_smb_omv2_checks.sh
tests/run_smb_omv2_visual_checks.sh
tests/run_smb_omv2_supervisor_matrix.sh
```

### 16.2 Required Matrices

Identity:

```text
unique and duplicate IDs
deterministic repeated IDs
identity survives geometry replacement
zero-geometry object
one object with several primitives
```

Containment:

```text
single root
missing parent
self containment
two parents
short and long cycles
nested world transforms
moving storey moves descendants
```

Interfaces:

```text
frame normalization
zero normal
parallel tangent
world transformation
point, plane, axis, and boundary-face regions
duplicate interface ID
```

Compatibility:

```text
allowed and disallowed types
diameter match and mismatch
rectangular size tolerance
gender match and mismatch
missing optional dimensions
non-finite values
```

Collision layers:

```text
Furniture versus Structure
Furniture versus Interior
Furniture ignoring Electrical
Plumbing versus Plumbing
Temporary policy
asymmetric masks do not collide
```

Positioning:

```text
Cube, Sphere, and Cylinder drop
positive and negative approach
requested gap
multi-primitive assembly
maximum travel failure
starting overlap failure
zero direction
forbidden obstacle
target layer excluded
transaction rollback
sequential floor and wall constraints
conflicting constraints
```

### 16.3 Numerical Gates

```text
abs(resulting_clearance - requested_clearance) <= tolerance
penetration <= contact_slop
residual_error <= tolerance
broad_phase_steps <= ceiling
refinement_iterations <= ceiling
final transforms finite
evidence hash stable
```

### 16.4 Determinism

Identical evaluated specifications produce identical:

```text
object and graph ordering
dependency order
candidate sequence
iteration counts
final matrices
resolution record order
evidence hashes
preview and export geometry
```

### 16.5 Transform Tests

Under nested translation, rotation, and nonuniform scale, verify parent-to-child frames, interface world frames, direction conversion, compound bounds, primitive binding transforms, clearance, resolution-local transform, final primitive world transform, and reset behavior.

### 16.6 Failure Atomicity

Every failure leaves primitive transforms, object transforms, collision caches, resolution records, and the previous valid editor snapshot unchanged. Diagnostics point to the responsible nested source declaration.

### 16.7 Visual Evidence

Capture object axes, container axes, interface normals and tangents, constraint vectors, contacts, clearances, AABBs, selected object groups, valid state, collision failure, and cycle failure. Deterministic captures are byte-identical.

## 17. Full SMB-OMv2 Acceptance

Machine-readable evidence reports:

```text
object, containment, connection, constraint, and interface counts
geometry binding count
resolved and validated object counts
invalid object count
collision query count
broad-phase and refinement totals
maximum residual
maximum forbidden penetration
minimum and maximum measured clearance
resolution evidence hash
aggregate topology hash
```

Required gates:

```text
124 object IDs exactly once
one containment root
zero containment cycles
zero constraint cycles
zero unresolved references
zero incompatible required connections
zero fake geometry on semantic leaves
zero forbidden collision above tolerance
zero unresolved required P0 constraints
all transforms finite
all evidence hashes stable
```

Priority validity:

```text
cabinet supported and wall-cleared
sink supported and plumbing-connected
water heater connected to hot-water riser
HVAC indoor units served by outdoor unit
solar array connected to electrical system
smoke alarms connected to monitored floors
windows contained by openings
loose furniture supported by correct floors
```

## 18. Safety and Performance

Suggested ceilings:

```text
objects per model                <= 4096
interfaces per object            <= 64
connections per model            <= 8192
constraints per model            <= 8192
primitive bindings per object    <= 1024
containment depth                 <= 128
constraint dependency depth      <= 512
broad-phase steps per constraint <= 4096
binary refinements               <= 64
resolution records per object    <= 256
cycle diagnostic length          <= 512
```

Benchmark separately:

```text
syntax parse
expression evaluation
model construction
containment validation
world frame resolution
compatibility
constraint graph and cycle detection
boundary derivation
collision queries
positioning search
transaction validation
primitive transform application
overlay construction
snapshot publication
```

Benchmark one cabinet, ten cabinets, 100 objects, the 124-object SMB, 1000 semantic objects, and a time sample with unchanged topology. Establish thresholds from measured evidence rather than guessing them in advance.

## 19. Backward Compatibility Gates

Before every merge:

```text
SMB-OMv1 source and runtime gates pass
Cube, Cylinder, Sphere, and AxialProfile grammars pass
transform scope and grammar-scene gates pass
collision positioning passes
80-example positioning convention passes
preview and export geometry remain unchanged
legacy snapshots may omit a spatial model
legacy primitive selection remains functional
```

No existing grammar token changes meaning for SMB-OMv2.

## 20. Recommended Merge Sequence

### Merge 1: Shared Collision Models

Extract bounds and collision model classes. Gate on all existing transform and collision suites.

### Merge 2: Spatial Model Kernel

Add identity, frames, objects, containment, interfaces, connections, constraints, layers, bindings, and evidence. Gate on programmatic model tests.

### Merge 3: Spatial Queries

Add compound AABBs, candidate transforms, and collision adapters. Gate on transformed multi-primitive object queries without mutation.

### Merge 4: P0 Positioning

Add `Touch`, `Gap`, `Drop`, clearance, masks, transactions, and evidence. Gate on the programmatic cabinet fixture.

### Merge 5: Dependency Resolution

Add topological ordering, cycle detection, sequential constraints, and rollback. Gate on source-order independence.

### Merge 6: Grammar

Add object scopes, interfaces, connections, positions, layer/mask, two-pass validation, diagnostics, highlighting, and autocomplete. Gate on exact programmatic-versus-grammar evidence parity.

### Merge 7: Editor Evidence

Add spatial snapshot, object selection, inspector, graph lists, overlays, and visual capture. Gate on deterministic editor evidence.

### Merge 8: Full SMB-OMv2

Add the complete 124-object fixture, connection migration, reliable derived placements, validity predicates, documentation, and supervisor matrix. Gate on the full acceptance matrix with SMB-OMv1 still green.

## 21. Risks and Mitigations

### Primitive Instances Become the Object Model

Use `SpatialObjectGeometryBinding`; never inherit building objects from primitives.

### Object Scope Leaks

Parse `Object(...) [ ... ]` as a composite token with owned body tokens.

### Authored and Resolved Transforms Mix

Store and inspect authored local, resolution local, and resolved world transforms separately.

### Dynamic Physics Becomes Placement Authority

Reuse only collision queries. Never use gravity or frame stepping for assembly placement.

### Collision Algorithms Are Duplicated

Extract shared models and adapt existing SAT, GJK/EPA, mesh overlap, and BVH services.

### AABB Results Are Reported as Exact

Store representation kind and confidence in query and resolution evidence. Label AABB positioning as approximate.

### Multi-Primitive Objects Split Apart

Commit one object transform through the binding service to all owned primitives atomically.

### Forward References Depend on Source Order

Use two-pass model construction and graph resolution.

### Cycles Are Silently Relaxed

Fail before solving and report the exact cycle path.

### Later Constraints Break Earlier Constraints

Revalidate higher-priority residuals after every placement and roll back on failure.

### Interface Semantics Become Strings

Use typed enums and purpose classes for authoritative P0 semantics.

### Catalog Metadata Becomes Unvalidated Authority

Import through a validation adapter. Catalog text remains data, never executable instruction.

### Full SMB Conversion Hides Kernel Bugs

Require cabinet, sink, window, and pipe fixtures first.

### Visual Success Hides Semantic Failure

Visual evidence remains supporting evidence. Graph, residual, collision, and hash gates are authoritative.

## 22. P0 Non-Goals

```text
general 6-DOF optimization
automatic interface discovery
nonzero uncertainty propagation
mesh contact patches
deformable placement
automatic geometry adaptation
dynamic joints
general insertion or seating
thread engagement
electrical or hydraulic simulation
automatic conflict repair
source-order relaxation
silent collider approximation
```

Unimplemented modes fail explicitly; placeholder enum storage is not implementation.

## 23. P0 Completion Standard

SMB-OMv2 P0 is complete only when:

```text
stable object identity exists
containment is cycle-free
local and world frames exist
objects bind to zero or more primitives
manual interfaces exist
connection and constraint graphs exist
collision layers and masks exist
Touch, Gap, and Drop resolve deterministically
clearance is validated
placement commits atomically
resolution evidence is immutable and hashed
object source and primitive associations exist
spatial inspector and overlays exist
cabinet fixture passes
full 124-object SMB-OMv2 passes
invalid fixtures fail closed
SMB-OMv1 remains unchanged and green
full editor release checks pass
supervisor-mode evidence passes
```

P1, P2, and P3 capabilities are not required and must not be claimed by P0.

## 24. Recommended First Implementation Slice

The strongest first coding slice is the model/query/solver kernel for a floor-supported cabinet, with no grammar changes.

Implement:

```text
SpatialObjectIdentity
SpatialFrameState
SpatialBuildingObject
SpatialContainmentTree
SpatialObjectGeometryBinding
AxisAlignedBoundingBoundary
SpatialInterface
CollisionLayerMask
CollisionPositionConstraint
SpatialResolutionRecord
SpatialObjectBoundaryDerivationService
SpatialInterfaceCompatibilityService
CollisionPositioningSolver
SpatialPlacementTransaction
```

Construct programmatically:

```text
Building
└── GroundFloor
    └── Kitchen
        ├── Wall
        └── Cabinet
```

Resolve:

```text
Cabinet.bottom Drop to GroundFloor.upperSurface
Cabinet.back Gap 0.01 from Wall.internalFace
```

Required evidence:

```text
authored and resolved cabinet transforms
floor contact point and normal
wall clearance
zero forbidden penetration
two deterministic resolution records
stable evidence hash
all bound primitives updated together
full rollback after an injected conflicting obstacle
```

Do not modify grammar parsing until this programmatic kernel passes. That isolates spatial object and collision-positioning correctness from language risk and creates a UML-readable foundation for the full SMB-OMv2 building.
