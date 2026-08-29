# ProGen3D AxialProfilev1 Core Primitive Implementation Plan

## Document Control

- **Date:** August 21, 2026
- **Status:** Implementation plan; no AxialProfile runtime implementation is included in this document.
- **Primary objective:** Promote `AxialProfile` to a first-class procedural shape family that produces deterministic, validated, immutable triangle geometry.
- **Release authority:** Deterministic source tests, mesh evidence, quantitative Component 24 comparison, the complete editor release gate, and an exact-binary supervisor restart.
- **External prerequisite:** The Coruscant `Component_24 / ID3050` COLLADA source is not present in this checkout. Core promotion remains evidence-blocked until an approved source artifact and provenance record are supplied.

## 1. Executive Decision

The original architectural hinge has already been completed by the curved-primitive work. ProGen3D now has:

- immutable evaluated `ShapeSpecification` objects;
- `ScenePrimitiveInstance::shape_specification` and `resolved_geometry` ownership;
- a generic triangle-mesh preview and export path;
- actual mesh bounds and triangle picking;
- topology analysis, volume evidence, collision policy, and per-generation procedural caching;
- expansion-time shape-expression evaluation;
- procedural catalog and shape-option autocomplete support.

Evidence:

- `include/geometry/model/ShapeSpecification.h:13`
- `include/Context.h:118`
- `src/Context.cpp:895`
- `src/Context.cpp:2094`
- `src/Context.cpp:2195`
- `src/geometry/service/ProceduralMeshRepository.cpp:117`
- `src/Grammar.cpp:3178`
- `src/Grammar.cpp:4104`

Therefore, AxialProfilev1 should **extend and generalize the existing procedural-shape architecture**, not introduce a second `ProceduralMeshFactory`, a second mesh cache, or another instance rendering path.

The true first implementation boundary is now:

> Refactor the radial assumptions out of the common `ShapeSpecification` and procedural-generation interfaces, while preserving Cylinder and Sphere behavior exactly.

## 2. Current Repository Baseline

### 2.1 Already Implemented and Reusable

| Capability | Current implementation | AxialProfile action |
|---|---|---|
| Evaluated shape ownership | `ScenePrimitiveInstance` stores immutable shape and resolved geometry | Reuse unchanged |
| Generic mesh rendering | Triangle-mesh instances use `append_mesh_vertices()` | Reuse unchanged |
| Export | Preview instance buffers feed PLY export | Reuse unchanged |
| Bounds and picking | Resolved triangle mesh drives bounds and collision geometry | Reuse unchanged |
| Density volume | Mesh/analytic volume evidence is transformed by matrix determinant | Reuse mesh-derived path |
| Static mesh collision | `StaticTriangleMesh` is accepted for immovable instances | Reuse for v1 |
| Procedural cache | `ProceduralMeshRepository` caches by evaluated shape key | Extend family dispatch |
| Expression timing | Shape expressions evaluate during expansion | Extend with AxialProfile AST |
| Topology evidence | `MeshTopologyAnalyzer` checks degeneracy, boundary edges, and hash | Reuse and extend tests |
| Editor catalog | Procedural entries provide tooltips and option completions | Add structured AxialProfile entry |

### 2.2 Radial Assumptions That Must Be Removed

The current abstract `ShapeSpecification` owns radial domains, curved topology, closure masks, tessellation, mapping, and clips directly. Those are valid common properties for Cylinder and Sphere but not for an axial profile.

Evidence: `include/geometry/model/ShapeSpecification.h:33`.

`ProceduralMeshRepository` also assumes every non-Cylinder specification is a Sphere and performs family-specific static casts when deriving volume and collision policy.

Evidence: `src/geometry/service/ProceduralMeshRepository.cpp:16`, `src/geometry/service/ProceduralMeshRepository.cpp:39`, and `src/geometry/service/ProceduralMeshRepository.cpp:117`.

The current syntax model is also flat: `ShapeDescriptorSyntax` owns a vector of named options whose arguments are all `GeometryExpression` values. That is insufficient for nested named profile declarations and ordered transition statements.

Evidence: `include/grammar/model/ShapeDescriptorSyntax.h:9` and `include/grammar/model/ShapeOptionSyntax.h:8`.

### 2.3 Important Scope Correction

Cube and its split-transform variants should not be migrated into the procedural shape hierarchy during AxialProfilev1. Their specialized dynamic preview behavior remains intentional.

`AxialProfile` reaches core-primitive status by participating in parsing, catalog discovery, scene instances, resolved geometry, preview, export, bounds, picking, mass, diagnostics, and static collision. It does not require Cube to become a procedural specification.

## 3. P0 Scope

AxialProfilev1 supports:

- local Y axis only;
- one simple closed outer polygon per named profile;
- one explicit initial `at(...)` statement;
- absolute, ordered axial coordinates;
- `hold`, `linear`, and same-level `step` transitions;
- per-level center offset, nonzero orientation-preserving scale, and rotation;
- independent bottom and top cap policy;
- deterministic simple-polygon cap triangulation;
- deterministic nested-polygon step-ring triangulation;
- triplanar texture mapping;
- exact mesh bounds, picking, PLY export, and mesh-derived closed volume;
- immovable triangle-mesh collision;
- editor diagnostics, catalog completion, inspector evidence, and optional construction overlays.

P0 deliberately excludes:

- holes inside a section profile;
- multiple disconnected loops at one level;
- arbitrary intersecting step contours;
- topology changes during `linear` transitions;
- automatic perimeter resampling;
- curved or spline interpolation;
- direct COLLADA runtime dependency;
- dynamic AxialProfile collision or generalized inertia;
- per-surface materials;
- continuous perimeter/height UVs;
- general polygon Boolean operations or CSG.

## 4. Locked Language Contract

### 4.1 Canonical Syntax

```p3d
I(
    AxialProfile(
        axis(y)

        profile(Base polygon(
            -0.5 -0.5
             0.5 -0.5
             0.5  0.5
            -0.5  0.5
        ))

        profile(Shaft polygon(
            -0.45 -0.45
             0.45 -0.45
             0.45  0.45
            -0.45  0.45
        ))

        at(0 Base)
        hold(1.4)
        step(Shaft)
        hold(20)
        linear(24 Shaft center(0 0) scale(1 1) rotate(0))
        cap(all)
    )
    material(darkslatesmoothmetal)
    1
    0.3
)
```

### 4.2 EBNF

```ebnf
shape-specification =
      primitive-name
    | curved-shape-specification
    | axial-profile-specification ;

axial-profile-specification =
    "AxialProfile", "(",
        axis-statement,
        profile-statement,
        { profile-statement },
        initial-level-statement,
        axial-transition-statement,
        { axial-transition-statement },
        [ cap-statement ],
    ")" ;

axis-statement =
    "axis", "(", "y", ")" ;

profile-statement =
    "profile", "(", identifier,
        "polygon", "(",
            expression, expression,
            expression, expression,
            expression, expression,
            { expression, expression },
        ")",
    ")" ;

initial-level-statement =
    "at", "(", expression, identifier,
        { level-transform },
    ")" ;

axial-transition-statement =
      hold-statement
    | linear-statement
    | step-statement ;

hold-statement =
    "hold", "(", expression, ")" ;

linear-statement =
    "linear", "(", expression, identifier,
        { level-transform },
    ")" ;

step-statement =
    "step", "(", identifier,
        { level-transform },
    ")" ;

level-transform =
      "center", "(", expression, expression, ")"
    | "scale", "(", expression, expression, ")"
    | "rotate", "(", expression, ")" ;

cap-statement =
    "cap", "(", cap-component, { cap-component }, ")" ;

cap-component =
      "none"
    | "all"
    | "bottom"
    | "top" ;
```

### 4.3 Ordering and Coordinate Semantics

The parser and validator enforce this order:

1. exactly one `axis(y)` statement;
2. one or more named profile declarations;
3. exactly one initial `at(...)` statement;
4. one or more transition statements;
5. zero or one final `cap(...)` statement.

`at`, `hold`, and `linear` coordinates are **absolute local-axis positions**, not deltas. This interpretation matches the proposed sequence `at(0)`, `hold(1.4)`, `hold(20)`, and `linear(24 ...)`.

`step(...)` creates a new axial state at the current coordinate. It is the only intentional equal-coordinate event.

### 4.4 Level Transform Semantics

Each evaluated section state is:

```text
position
 profile reference
 center offset
 scale
 rotation
 transition from previous state
 source range
```

Defaults:

```text
center(0 0)
scale(1 1)
rotate(0)
```

`hold(y)` copies the current profile, center, scale, and rotation exactly at target coordinate `y`.

`linear(y Profile ...)` establishes a new transformed ring and connects it to the previous ring by semantic vertex index.

`step(Profile ...)` replaces the section at the current coordinate and creates a horizontal material boundary between the old and new transformed contours.

### 4.5 Cap Semantics

```text
cap(all)        bottom and top
cap(none)       neither boundary
cap(bottom)     bottom only
cap(top)        top only
cap(bottom top) both boundaries
```

`all` and `none` cannot be combined with other components. Omitted `cap(...)` defaults to `all`.

## 5. Locked Validation Invariants

### 5.1 Profile Invariants

Every profile must:

- have a unique identifier;
- contain at least three distinct vertices after optional duplicate-terminal removal;
- contain only finite coordinates;
- contain no zero-length edge;
- contain no consecutive zero-turn triple in P0;
- have nonzero signed area;
- be a simple non-self-intersecting polygon;
- be normalized to positive signed area in the local XZ coordinate pair;
- preserve semantic vertex order apart from a complete winding reversal;
- remain immutable after validation.

Removing one terminal vertex that exactly repeats the first vertex is the only accepted normalization that changes vertex count. All other invalidity produces a diagnostic.

### 5.2 Level Invariants

- All evaluated values are finite.
- `axis(y)` is the only supported axis.
- There is exactly one initial `at(...)` state.
- At least one `hold` or `linear` transition creates positive axial height.
- Every `hold` and `linear` target is strictly greater than the current coordinate.
- `step` preserves the current coordinate and must change the transformed contour.
- Center offsets are finite.
- Both scale components are nonzero.
- P0 accepts only orientation-preserving scale pairs where `scale_x * scale_z > 0`.
- Rotation is canonicalized to a stable degree interval before key generation.

### 5.3 Linear Transition Invariants

- Previous and target profiles contain the same number of vertices.
- Vertex index `j` maps only to vertex index `j`.
- Corresponding side quads must have nonzero area.
- Generated side triangles must not introduce non-adjacent self-intersections.
- Profile name changes are allowed when correspondence remains valid.

P0 never silently resamples or rotates the starting vertex index to improve a fit.

### 5.4 Step Invariants

At the shared axial coordinate, the transformed old and new contours must satisfy exactly one relationship:

```text
old strictly contains new
or
new strictly contains old
```

Reject:

- intersecting boundaries;
- touching boundaries;
- coincident contours;
- partial overlap;
- disconnected Boolean results.

Required diagnostic for crossing contours:

```text
AxialProfile step profiles intersect and cannot be resolved by v1.
```

### 5.5 Shape Safety Ceilings

Initial hard ceilings:

```text
profiles per shape               <= 128
vertices per profile             <= 256
total declared profile vertices  <= 32768
axial states                     <= 512
step events                      <= 256
generated vertices               <= 131072
generated triangles              <= 262144
```

The validator must calculate conservative output bounds before allocation. The generator must also enforce actual output counts while building.

## 6. Purpose-Driven Object Model

### 6.1 General Shape Hierarchy

Refactor the current radial-heavy base into a true conceptual hierarchy:

```text
ShapeSpecification
├── RadialShapeSpecification
│   ├── CylinderShapeSpecification
│   └── SphereShapeSpecification
└── AxialProfileShapeSpecification
```

`ShapeSpecification` owns only properties shared by every evaluated procedural shape:

```cpp
class ShapeSpecification {
public:
    ShapeFamily family() const;
    const ShapeSpecificationKey &key() const;
    virtual std::string canonicalText() const = 0;
    virtual bool isDefaultFamilyShape() const = 0;
    virtual bool requestsClosedGeometry() const = 0;
};
```

`RadialShapeSpecification` owns:

```text
radial domain
topology
closure policy
tessellation
mapping
clip planes
```

Cylinder and Sphere retain their current behavior through this intermediate abstraction.

`AxialProfileShapeSpecification` owns:

```text
axis
validated profile library
ordered axial states
cap policy
mapping policy
canonical specification key
```

### 6.2 Axial Model Classes

```cpp
enum class AxialProfileAxis {
    Y
};

enum class AxialTransitionKind {
    Initial,
    Hold,
    Linear,
    Step
};

class AxialProfilePolygon {
public:
    const std::string &name() const;
    const std::vector<glm::vec2> &vertices() const;
    float signedArea() const;
};

class AxialProfileLevel {
public:
    float axialPosition() const;
    std::size_t profileIndex() const;
    const glm::vec2 &centerOffset() const;
    const glm::vec2 &scale() const;
    float rotationDegrees() const;
    AxialTransitionKind transitionKind() const;
};

class AxialProfileCapPolicy {
public:
    bool closesBottom() const;
    bool closesTop() const;
};
```

Profile references are resolved to stable indices during evaluation. The evaluated specification does not perform name lookup during mesh generation.

### 6.3 Generic Generated Geometry

Rename radial-specific interfaces:

```text
CurvedShapeMeshGenerator -> ProceduralShapeMeshGenerator
GeneratedCurvedMesh      -> GeneratedPrimitiveMesh
```

The generated result continues to contain:

```text
mutable construction mesh
face surface tags
optional family construction evidence
```

Add AxialProfile surface roles:

```text
AxialSide
AxialStep
AxialBottomCap
AxialTopCap
```

Do not overload `Outer`, `AxialStart`, or `Rim` with ambiguous AxialProfile meanings.

### 6.4 Construction Evidence

Add immutable optional construction evidence to `ResolvedPrimitiveGeometry`:

```text
ShapeConstructionEvidence
└── AxialProfileConstructionEvidence
```

Axial evidence contains:

- transformed section rings;
- section centers;
- transition kinds;
- profile, level, hold, linear, and step counts;
- generated surface counts.

The scene inspector and debug overlay consume this evidence rather than regenerating geometry.

## 7. Dedicated Syntax Model

The flat `ShapeOptionSyntax` representation remains appropriate for Cylinder and Sphere but must not be stretched into a nested mini-language.

Introduce:

```text
AxialProfileDescriptorSyntax
AxialProfileAxisSyntax
AxialProfileDefinitionSyntax
AxialProfilePolygonVertexSyntax
AxialProfileInitialLevelSyntax
AxialProfileHoldSyntax
AxialProfileLinearSyntax
AxialProfileStepSyntax
AxialProfileLevelTransformSyntax
AxialProfileCapSyntax
```

Each nested syntax object owns a `SourceRange` or equivalent token span.

Extend `GeometryExpression` to retain its source span, or introduce `GeometryExpressionSyntax` containing:

```text
source text
line
start column
end column
```

This is required to produce diagnostics against `CrownB`, a scale expression, or one polygon coordinate rather than highlighting the complete `I(...)` action.

## 8. Parser Architecture

### 8.1 Token Cursor

Create a `ShapeSyntaxTokenCursor` that wraps the existing parallel arrays:

```text
raw token strings
SourceTokenSpan values
current index
```

It provides explicit operations:

```text
peek
consume
consumeIdentifier
consumeOpeningParenthesis
consumeClosingParenthesis
parseExpressionUntilBoundary
currentSourceRange
```

This avoids introducing a second lexer and removes raw-index manipulation from the AxialProfile parser.

### 8.2 Parser Services

```text
ShapeSpecificationParser
└── dispatches to AxialProfileSyntaxParser

AxialProfileSyntaxParser
├── parseAxis
├── parseProfileDefinition
├── parsePolygon
├── parseInitialLevel
├── parseHold
├── parseLinear
├── parseStep
├── parseLevelTransforms
└── parseCapPolicy
```

`Grammar.cpp` should only:

1. identify the shape name;
2. create the token cursor;
3. call the shape parser;
4. attach returned syntax or a diagnostic to the action token.

No polygon or transition construction logic belongs in `Grammar.cpp`.

## 9. Semantic and Runtime Evaluation

### 9.1 Semantic Validation

Replace the current flat `validate_shape_descriptor_expressions()` special cases with visitor-based traversal:

```text
ShapeSyntaxSemanticAnalyzer
├── CurvedShapeSyntaxSemanticAnalyzer
└── AxialProfileSyntaxSemanticAnalyzer
```

The Axial analyzer validates:

- numeric expression symbols and supported functions;
- immutable `t` usage;
- duplicate profile names;
- profile-reference existence;
- statement ordering;
- duplicate center/scale/rotate modifiers;
- supported axis and cap keywords;
- profile identifiers as identifiers, not variable expressions.

### 9.2 Expansion-Time Evaluator

```text
AxialProfileDescriptorSyntax
    -> AxialProfileSpecificationEvaluator
    -> AxialProfileSpecificationCandidate
    -> AxialProfileSpecificationValidator
    -> immutable AxialProfileShapeSpecification
```

The evaluator uses the existing checked expression callback so grammar variables, `R`, rerolls, `&`, `t`, `sin`, `swing`, and other supported functions retain current behavior.

The geometry generator consumes no random stream.

### 9.3 Canonical Key

The key contains only normalized evaluated values in stable order:

```text
family
axis
profile count
for each profile: name, vertex count, exact canonical float bits
level count
for each level: position, profile index, center, scale, rotation, transition
cap mask
mapping mode
```

Canonicalization includes negative-zero removal and stable rotation wrapping. Expression source text is excluded.

## 10. Polygon Geometry Kernel

Create independently testable services:

```text
SimplePolygonValidator
SimplePolygonTriangulator
PolygonContainmentAnalyzer
NestedPolygonRingTriangulator
TriangleSelfIntersectionAnalyzer
```

### 10.1 Simple Polygon Validation

Use robust epsilon-aware segment intersection and signed-area tests. Adjacent edges may meet only at their shared endpoint. Non-adjacent edges may not touch or cross.

### 10.2 Cap Triangulation

`SimplePolygonTriangulator` uses deterministic ear clipping:

1. canonical positive-area loop;
2. candidate ears visited in ascending semantic vertex order;
3. strictly convex ear test;
4. no remaining vertex inside the ear;
5. deterministic tie handling;
6. no zero-area output triangle.

Bottom-cap winding points toward local `-Y`; top-cap winding points toward local `+Y`.

### 10.3 Step Ring Triangulation

A step surface is a polygon with one hole, so a simple cap ear clipper alone is insufficient.

`NestedPolygonRingTriangulator` shall:

1. classify which transformed contour is outer and which is inner;
2. reject touching, crossing, or coincident contours;
3. select the inner contour's rightmost vertex with deterministic tie-breaking;
4. select a visible outer bridge vertex deterministically;
5. create one bridged simple loop without changing source contours;
6. ear-clip the bridged loop;
7. orient inward steps toward `+Y` and outward steps toward `-Y`;
8. tag every triangle as `AxialStep` with the step-event index.

This is constrained nested-polygon triangulation, not a general polygon Boolean engine.

## 11. AxialProfile Mesh Generation

### 11.1 Ring Construction

For each level and profile vertex `q = (qx, qz)`:

```text
scaled = (sx * qx, sz * qz)
rotated = rotate2D(rotation_degrees, scaled)
offset = rotated + center
embedded = (offset.x, axial_position, offset.y)
```

Keep the transformed rings in semantic vertex order as construction evidence.

### 11.2 Hold Transition

`hold(target)` creates an exact copy of the previous ring at the new axial coordinate and connects matching indices.

The specification key and tests must confirm that all XZ coordinates and profile identity remain unchanged.

### 11.3 Linear Transition

For each index `j`:

```cpp
addQuad(
    previous[j],
    previous[next],
    target[next],
    target[j]
);
```

No intermediate rings are generated. The result is a planar ruled strip between corresponding polygon edges.

### 11.4 Step Transition

At one axial coordinate:

```text
old transformed ring
new transformed ring
nested-polygon transition face
```

The previous positive-height loft terminates on the old ring. The next positive-height loft starts from the new ring. The step triangulator fills the material difference between them.

### 11.5 Vertex Duplication and Normals

Maintain explicit hard surface boundaries:

- side vertices are not shared with caps;
- side vertices are not shared with step faces;
- step vertices are not shared with adjacent side strips;
- cap and step normals are constant axis normals;
- side normals derive from triangle geometry and point outward.

P0 uses deterministic face/strip normals and triplanar UV fallback. Continuous perimeter smoothing and perimeter/height UVs remain post-v1 improvements.

### 11.6 Finalization

The generator:

1. enforces actual output budgets;
2. rejects zero-area triangles;
3. builds collision acceleration;
4. returns mesh, surface tags, and construction evidence;
5. consumes no random values.

## 12. Procedural Repository Integration

Extend `ProceduralMeshRepository` with an explicit exhaustive family dispatch:

```cpp
switch (specification.family()) {
case ShapeFamily::Cylinder:
    generated = cylinder_mesh_generator_->generate(...);
    break;
case ShapeFamily::Sphere:
    generated = sphere_mesh_generator_->generate(...);
    break;
case ShapeFamily::AxialProfile:
    generated = axial_profile_mesh_generator_->generate(...);
    break;
}
```

Do not retain the current Cylinder-versus-everything-else ternary.

Extract family-specific evidence decisions from `ProceduralMeshRepository` into:

```text
ProceduralGeometryEvidenceService
```

It resolves:

- whether the specification requests watertight geometry;
- analytic versus mesh-derived volume;
- collision policy;
- actionable topology diagnostics.

AxialProfile policy:

```text
both caps + valid steps + watertight mesh -> MeshDerived volume
one or both end caps omitted            -> Undefined volume
all AxialProfile forms                  -> StaticTriangleMesh collision in v1
```

`Context.cpp` already permits a `StaticTriangleMesh` only when the instance is immovable. Dynamic AxialProfile instances may render and animate transforms, but they do not participate as dynamic collision bodies in v1.

## 13. Cache Policy

The current `PrimitiveGeometryResolver` belongs to `SceneGenerationContext`, and each successful compilation creates a fresh context. The existing cache is therefore generation-local and naturally bounded by the generated scene.

Evidence: `src/Grammar.cpp:4633` and `src/editor/service/GrammarCompilationService.cpp:35`.

P0 correctness uses this existing generation-local cache.

An optional reusable LRU cache may be added only after benchmarks demonstrate a material benefit. If added, it must be bounded by both:

```text
maximum entry count
maximum retained mesh bytes
```

and expose hit, miss, eviction, and retained-byte evidence. Time-varying expressions must never create an unbounded mesh attic.

## 14. Scene, Export, Bounds, Mass, and Collision

No AxialProfile-specific render or export branch is allowed.

Required path:

```text
evaluated AxialProfileShapeSpecification
-> PrimitiveGeometryResolver
-> ProceduralMeshRepository
-> ResolvedPrimitiveGeometry
-> ScenePrimitiveInstance
-> generic preview/export/picking/bounds path
```

The existing resolved-geometry path already provides:

- actual local mesh bounds;
- transformed world bounds;
- inverse-transpose normal transformation;
- triangle picking;
- PLY export;
- determinant-scaled density mass;
- static triangle collision for immovable geometry.

Regression tests must prove AxialProfile uses these paths without adding a type-name exception.

Dynamic collision and AxialProfile inertia remain disabled. The implementation must not reuse box inertia as certified AxialProfile evidence.

## 15. Editor Integration

### 15.1 Catalog and Syntax

Add `AxialProfile` as a canonical procedural catalog entry with a multiline template rather than a misleading flat option signature.

Syntax highlighting recognizes:

```text
AxialProfile
axis
profile
polygon
at
hold
linear
step
center
scale
rotate
cap
```

### 15.2 Context-Sensitive Completion

The current autocomplete only offers options at descriptor depth one and explicitly returns when deeper parentheses are active.

Evidence: `src/imgui_main.cpp:7343`.

Replace the single Boolean `shape_option_context` with a purpose-revealing context model:

```text
ShapeCompletionContext
├── ShapeStatement
├── ProfileBody
├── LevelTransform
├── ProfileReference
└── CapComponent
```

Suggested completions:

```text
inside AxialProfile(...) -> profile, at, hold, linear, step, cap
inside profile(...)      -> polygon
inside at/linear/step    -> center, scale, rotate, known profile names
inside cap(...)          -> all, none, bottom, top
```

### 15.3 Scene Inspector

Extend `ScenePrimitiveInspection` with optional AxialProfile fields:

```text
family
profile count
level count
hold count
linear count
step count
generated vertex count
generated triangle count
cap policy
watertightness
volume evidence
topology hash
shape key
```

The existing generic shape text remains available.

### 15.4 Construction Overlay

Use the existing preview overlay-line renderer to add an AxialProfile overlay mode:

- section rings;
- section centers;
- axial centerline;
- step planes;
- optional normal lines;
- transition colors and `H`, `L`, `S` legend in the inspector.

Overlay geometry comes from `AxialProfileConstructionEvidence` and never mutates the scene mesh.

## 16. Component 24 Evidence Gate

### 16.1 Current Blocker

No file matching `Component_24`, `ID3050`, Coruscant, `.dae`, or COLLADA exists in the repository as of August 21, 2026.

The implementation may reach **candidate complete** using synthetic fixtures, but it may not claim **core promotion complete** until Component 24 evidence is available and approved.

### 16.2 Evidence Intake

Create:

```text
tests/fixtures/axialprofile/component24/
    README.md
    source_manifest.json
    expected_metrics.json
    approved_profile.p3d
```

`source_manifest.json` records:

```text
source filename or external path
SHA-256
license or use authority
COLLADA node identifier
coordinate-system conversion
unit conversion
extraction tool version
approval status
```

Do not commit the original COLLADA asset unless its license and repository policy permit it.

### 16.3 Offline Tools

Keep source analysis separate from runtime:

```text
tools/axialprofile/collada_axial_profile_analyzer.py
tools/axialprofile/collada_axial_profile_fit.py
tools/axialprofile/compare_axial_profile_mesh.py
```

The analyzer must resolve COLLADA node transforms and units before measuring sections. Outputs include source hashes and deterministic JSON evidence.

### 16.4 Comparison Metrics

At agreed axial sample levels, calculate:

```text
cross-section bidirectional RMS boundary distance
maximum bidirectional section deviation
closed volume difference
source/generated triangle counts
source/generated axial event counts
profile parameter count
unexpected boundary edges
degenerate triangles
nonmanifold edges
```

Initial promotion thresholds:

```text
cross-section RMS error       <= 2%
maximum section error         <= 5%
volume error                  <= 3%
unexpected boundary edges     = 0
degenerate triangles          = 0
nonmanifold edges              = 0
deterministic mesh hash        stable
preview/export geometry        identical
```

Normalization for percentage distances must be declared in `expected_metrics.json`; use the source component's maximum cross-section diameter unless evidence review selects another measure.

## 17. Test Matrix

### 17.1 Specification and Polygon Tests

```text
three-vertex minimum profile
duplicate terminal point normalization
clockwise winding reversal
zero-length edge rejection
zero-area polygon rejection
self-intersection rejection
touching non-adjacent edges rejection
duplicate profile name rejection
undefined profile reference rejection
unsupported axis rejection
non-finite expression rejection
zero scale rejection
orientation-reversing scale rejection
safety budget rejection
```

### 17.2 Transition Geometry Tests

```text
rectangle hold extrusion
triangle hold extrusion
octagonal hold extrusion
linear square taper
center drift
scale drift
rotation drift
profile-name change with preserved correspondence
linear vertex-count mismatch rejection
inward step
outward step
different-count nested step
intersecting step rejection
touching step rejection
coincident step rejection
multiple ordered steps
```

### 17.3 Cap and Topology Tests

```text
cap(all)
cap(none)
cap(bottom)
cap(top)
concave asymmetric bottom cap
concave asymmetric top cap
step annulus winding
every closed edge incident exactly twice
open boundary edges match omitted caps
zero degenerate triangles
zero nonmanifold edges
finite nonzero signed volume for closed forms
```

### 17.4 Analytic Volume Tests

Rectangle extrusion:

```text
expected volume = width * depth * height
```

Linear square frustum:

```text
expected volume = h / 3 * (A1 + A2 + sqrt(A1 * A2))
```

Compare against mesh-derived tetrahedral volume with tessellation-independent tolerances, since AxialProfile side surfaces are already planar.

### 17.5 Determinism Tests

Identical evaluated specifications must produce identical:

- shape key;
- canonical text;
- vertex count and order;
- face count and order;
- normals and texture coordinates;
- surface-tag order;
- topology hash;
- bounds;
- signed volume;
- preview buffer hash;
- PLY geometry payload hash.

### 17.6 Runtime Integration Tests

Verify:

- grammar variables in profile coordinates and levels;
- `t`-driven center, scale, rotation, and height;
- failed time samples preserve the last valid scene;
- no random values are consumed by mesh generation;
- one specification pointer reaches the stored instance;
- one resolved geometry object drives preview, export, bounds, and picking;
- density mass uses mesh volume and transform determinant;
- open forms reject density-derived mass;
- immovable AxialProfile uses triangle collision;
- dynamic AxialProfile does not claim supported collision;
- legacy Cube/Cylinder/Sphere grammar remains unchanged.

### 17.7 GUI Tests

Add separate startup examples and smoke tests:

```text
examples/axial_profile_showcase.p3d
examples/axial_profile_temporal.p3d
tests/run_axial_profile_gui_smoke_check.sh
tests/run_axial_profile_temporal_gui_smoke_check.sh
```

Do not replace the existing curved-primitive smoke fixtures.

## 18. Performance and Cache Evidence

Create a deterministic benchmark harness for:

```text
1 unique AxialProfile
100 cached identical instances in one scene
100 unique profiles
1000 cached identical instances
time-varying evaluated profiles
Component 24 candidate
```

Record separately:

```text
syntax parse time
expression evaluation time
validation time
polygon triangulation time
mesh construction time
topology analysis time
BVH construction time
cache lookup count and hit rate
preview buffer construction time
generated and retained bytes
```

Correctness gates use cache build counts and deterministic outputs, not fragile machine-specific timing thresholds. Timing results are recorded as evidence and compared to an approved baseline.

## 19. File-Level Change Plan

### 19.1 New Model Files

```text
include/geometry/model/RadialShapeSpecification.h
include/geometry/model/AxialProfileAxis.h
include/geometry/model/AxialTransitionKind.h
include/geometry/model/AxialProfilePolygon.h
include/geometry/model/AxialProfileLevel.h
include/geometry/model/AxialProfileCapPolicy.h
include/geometry/model/AxialProfileShapeSpecification.h
include/geometry/model/AxialProfileConstructionEvidence.h
include/geometry/model/GeneratedPrimitiveMesh.h
```

### 19.2 New Geometry Services

```text
include/geometry/service/ProceduralShapeMeshGenerator.h
include/geometry/service/AxialProfileSpecificationValidator.h
include/geometry/service/AxialProfileMeshGenerator.h
include/geometry/service/SimplePolygonValidator.h
include/geometry/service/SimplePolygonTriangulator.h
include/geometry/service/PolygonContainmentAnalyzer.h
include/geometry/service/NestedPolygonRingTriangulator.h
include/geometry/service/TriangleSelfIntersectionAnalyzer.h
include/geometry/service/ProceduralGeometryEvidenceService.h

src/geometry/service/*.cpp
```

### 19.3 New Grammar Model and Services

```text
include/grammar/model/AxialProfileDescriptorSyntax.h
include/grammar/model/AxialProfileDefinitionSyntax.h
include/grammar/model/AxialProfileTransitionSyntax.h
include/grammar/model/AxialProfileLevelTransformSyntax.h
include/grammar/model/ShapeSyntaxTokenCursor.h
include/grammar/service/AxialProfileSyntaxParser.h
include/grammar/service/AxialProfileSyntaxSemanticAnalyzer.h
include/grammar/service/AxialProfileSpecificationEvaluator.h

src/grammar/service/*.cpp
```

### 19.4 Existing Files to Modify

```text
include/geometry/model/ShapeSpecification.h
include/geometry/model/CylinderShapeSpecification.h
include/geometry/model/SphereShapeSpecification.h
include/geometry/model/MeshSurfaceTag.h
include/geometry/model/ResolvedPrimitiveGeometry.h
include/geometry/service/ProceduralMeshRepository.h
src/geometry/service/ProceduralMeshRepository.cpp
src/geometry/service/PrimitiveGeometryResolver.cpp
include/grammar/model/GeometryExpression.h
include/grammar/service/ShapeSpecificationParser.h
src/grammar/service/ShapeSpecificationParser.cpp
include/grammar/service/ShapeSpecificationEvaluator.h
src/grammar/service/ShapeSpecificationEvaluator.cpp
include/grammar.h
src/Grammar.cpp
include/Context.h
src/Context.cpp
src/geometry/service/ProceduralShapeCatalogRepository.cpp
include/editor/model/GrammarEditorInteractionState.h
include/editor/model/ScenePrimitiveInspection.h
src/editor/presentation/GrammarAutocompletePresentation.cpp
src/editor/presentation/GrammarSyntaxPresentation.cpp
src/editor/presentation/SceneInspectorPanel.cpp
src/imgui_main.cpp
Makefile
tests/run_editor_architecture_checks.sh
tests/run_release_checks.sh
tests/README.md
```

`Mesh.cpp`, `PLYWriter.cpp`, and the renderer should require no AxialProfile-specific geometry branch. Changes there require explicit justification and a source gate.

## 20. Milestone and Merge Sequence

### Milestone 0: Evidence and Contract Lock

Deliver:

- this semantic contract;
- synthetic fixture definitions;
- Component 24 provenance placeholder;
- source gates that protect the existing generic mesh path.

Gate:

- no code assumes the Component 24 asset is present;
- all existing release checks remain green.

### Milestone 1: General Shape Abstraction

Deliver:

- lean `ShapeSpecification` base;
- `RadialShapeSpecification` intermediate class;
- renamed generic generated-mesh interfaces;
- exhaustive procedural family dispatch;
- extracted geometry evidence service.

Gate:

- Cylinder and Sphere shape keys, canonical text, meshes, topology hashes, preview/export payloads, mass, and collision policies remain unchanged;
- complete curved and release suites pass before AxialProfile is added.

### Milestone 2: Syntax-Independent Axial Geometry Kernel

Deliver:

- Axial model classes;
- profile validator;
- simple cap triangulator;
- nested step-ring triangulator;
- Axial mesh generator;
- construction evidence;
- programmatic specification tests.

Gate:

- rectangle, taper, drift, rotation, inward step, outward step, asymmetric concave caps, topology, volume, and determinism tests pass;
- generator contains no grammar or random dependencies.

### Milestone 3: Procedural Repository and Scene Integration

Deliver:

- `ShapeFamily::AxialProfile` dispatch;
- cached resolved geometry;
- mesh-derived volume evidence;
- static collision policy;
- inspector-ready construction evidence.

Gate:

- programmatically constructed AxialProfile uses the same preview, export, bounds, picking, mass, and collision paths as other resolved meshes;
- no `instance.type == "AxialProfile"` geometry branch exists in `Context.cpp`.

### Milestone 4: Grammar AST, Semantics, and Evaluation

Deliver:

- dedicated nested syntax hierarchy;
- token cursor with source spans;
- parser, semantic analyzer, evaluator, and validator diagnostics;
- backwards-compatible `I(...)` appearance parsing.

Gate:

- all valid canonical syntax forms expand;
- every invalid invariant produces a source-located blocking diagnostic;
- failed expansion publishes no partial scene;
- existing Cube/Cylinder/Sphere grammar tests remain green.

### Milestone 5: Editor and Debug Evidence

Deliver:

- procedural catalog entry;
- nested syntax coloring;
- context-aware statement, transform, cap, and profile-name completion;
- inspector metrics;
- optional construction overlay.

Gate:

- editor harnesses cover each completion context;
- selected AxialProfile reports correct immutable geometry evidence;
- overlay generation is deterministic and does not affect exported mesh.

### Milestone 6: Fixtures, Performance, and GUI Validation

Deliver:

- synthetic fixture suite;
- showcase and temporal examples;
- benchmark evidence;
- ordinary and temporal GUI smoke checks.

Gate:

- architecture suite passes;
- both AxialProfile GUI smoke checks pass;
- cached identical instances build one topology per scene;
- safety ceilings reject pathological inputs before allocation.

### Milestone 7: Component 24 Certification

Deliver after approved asset intake:

- source manifest and exact hash;
- deterministic extraction evidence;
- approved AxialProfile reconstruction;
- comparison metrics and plots/data;
- explicit pass/fail report against thresholds.

Gate:

- all Component 24 thresholds pass against the approved exact source hash;
- unresolved deviations are disclosed rather than silently accepted.

### Milestone 8: Release and Supervisor Promotion

Run:

```bash
./tests/run_release_checks.sh
```

Only after a clean pass:

1. record the built binary SHA-256;
2. restart `progen3d-editor-gui.service`;
3. verify a new PID;
4. verify `ActiveState=active` and `SubState=running`;
5. verify `NRestarts=0` after observation;
6. verify `/proc/<pid>/exe` matches the validated binary hash;
7. inspect recent supervisor logs for runtime geometry errors.

## 21. Source Gates

Add deterministic guards that fail when:

- `ShapeSpecification` regains radial-only fields;
- procedural dispatch treats every non-Cylinder family as Sphere;
- AxialProfile generation appears in `Grammar.cpp`;
- AxialProfile geometry is selected by `instance.type` in `Context.cpp`;
- AxialProfile bypasses `ResolvedPrimitiveGeometry`;
- preview and export resolve different meshes;
- density mass uses box volume for AxialProfile;
- dynamic AxialProfile collision is advertised as supported;
- a generator reads `grammar_rng`, `effects_rng`, `rand`, or another random source;
- shape keys contain expression source text;
- a closed generated mesh bypasses topology analysis;
- cap or step triangulation emits untagged faces;
- generated budgets are checked only after allocation;
- runtime code imports or parses COLLADA.

## 22. Completion States

Use three explicit states:

### Candidate Complete

- geometry kernel, grammar, scene integration, editor support, synthetic tests, performance evidence, and GUI smoke checks pass;
- Component 24 source is unavailable or not yet approved.

### Validation Complete

- approved Component 24 artifact is available;
- reconstruction and quantitative thresholds pass;
- exact source and candidate hashes are recorded.

### Core Promotion Complete

- validation is complete;
- full clean release gate passes;
- supervised GUI runs the exact validated binary without restart churn or runtime geometry errors.

## 23. Definition of AxialProfilev1 Complete

AxialProfilev1 is complete only when:

- the common shape hierarchy contains no false radial relationship;
- syntax and evaluated specification models are separate and immutable;
- all nested expressions evaluate through the existing lexical environment;
- profiles are validated simple polygons with canonical winding;
- hold, linear, inward step, and outward step behave deterministically;
- center, scale, and rotation transforms work;
- caps and step rings triangulate without degeneracy;
- closed forms are watertight and have mesh-derived volume;
- open forms reject density-derived mass;
- preview, export, bounds, Fit, picking, and static collision use one resolved geometry object;
- editor diagnostics identify the nested failing construct;
- catalog, autocomplete, inspector, and debug evidence are present;
- existing Cube, CubeX/Y/Z, Cylinder, Sphere, curved aliases, material, alpha, texscale, physics, and temporal behavior remain compatible;
- Component 24 passes the approved quantitative evidence gate;
- `./tests/run_release_checks.sh` passes from a clean build;
- the supervised GUI runs the exact validated executable hash.

Dynamic collision, generalized inertia, inner profile holes, topology-changing interpolation, curved transitions, automatic profile resampling, and direct COLLADA conversion remain post-v1 work.

## 24. Highest-Gain Implementation Order

```text
generalize ShapeSpecification without regressions
        ->
syntax-independent AxialProfile geometry kernel
        ->
procedural repository and resolved-geometry integration
        ->
dedicated nested grammar AST and evaluator
        ->
editor evidence and GUI fixtures
        ->
Component 24 exact-hash certification
        ->
clean release and supervisor promotion
```

The generic mesh-instance refactor is no longer the critical path because it is already implemented. The highest-risk work is now the correctness boundary around nested syntax, polygon validation, deterministic cap triangulation, and constrained step-ring construction.
