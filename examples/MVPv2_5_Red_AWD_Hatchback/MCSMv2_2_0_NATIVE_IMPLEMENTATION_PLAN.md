# ProGen3D MCSMv2.2 Native Implementation Plan

**Status:** Planned

**Date:** Monday, August 24, 2026

**Source release:** `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_RELEASE/`

**Primary objective:** Integrate the MCSMv2.2 source release into the native
ProGen3D C++ object model, executable grammar, generated-mesh system, editor
preview, and acceptance suite while preserving MCSMv2.0.1, MCSMv2.1,
MCP_OMv1, MVPv1, MVPv2, MVPv2.5, and MVPv2.6 compatibility.

This plan is intentionally stored adjacent to the immutable source release.
The source release remains the signed 109-file package identified by
`RELEASE_MANIFEST.json`; implementation planning and native evidence must not
silently change that release contract.

## 1. Current Baseline

### 1.1 Authoritative Source Release

The native implementation must begin from the exact MCSMv2.2 source contract:

- version: `2.2.0`;
- model schema: `MCSMv2.2-uv-semantic-closure-suspension-kinematics`;
- release manifest schema: `MCSMv2.2.ReleaseManifest.v1`;
- release manifest SHA-256:
  `9053996bc0e8ba0017ad8eeffb616ab801b0b709dbe0fef93708daa93518882b`;
- signed artifact count: `109`;
- signed artifact size: `103068076` bytes;
- variants: `reference`, `track`, `aero`, and `crossover`;
- source frame: `+X` front, `+Y` right, `+Z` up;
- ProGen3D frame: `+X` right, `+Y` up, `+Z` forward;
- coordinate conversion:
  `progen3d(source_x, source_y, source_z) = (source_y, source_z, source_x)`.

The source package currently passes all 34 Python tests. The native source
contract gate must rerun those tests rather than trusting the recorded report.

### 1.2 Existing Native Foundation

MCSMv2.2 must extend, not replace, the following accepted implementations:

- `McsMv21SemanticFamilyDefinition` owns the four-variant registered-surface
  catalog and MCSMv2.1 source identity.
- `McsMv21SemanticGeometry` owns final-body geometry, registered semantic
  surface, exclusive domain partition, and static glass apertures.
- `McsMv21SemanticGeometryGenerationService` generates native MCSMv2.1
  semantic geometry.
- `McsMv21GeneratedMeshProvider` resolves body, fixed-body, panel-region, and
  glass mesh keys.
- `McsMv21SemanticGrammarLoweringService` emits parser-supported
  `GeneratedMeshReference` grammar.
- `SuspensionHardpointModel`, `WheelPoseState`, `SolvedWheelPose`,
  `WheelPoseFunction`, `TyreGeometry`, and `WheelSweptEnvelope` already provide
  reusable vehicle kinematic concepts.
- `VehicleClosureKinematicsService` already evaluates hinge, four-bar, and
  parent-relative drop-glass transforms.
- `VehicleJoint` and `VehicleJointGraph` already provide shared kinematic
  relationship ownership.
- `ProceduralMeshRepository` and `GeneratedMeshProviderRegistry` already
  provide generated-mesh caching and grammar resolution.

### 1.3 Existing Compatibility Boundary

MCSMv2.1 remains V2 static concept geometry. MCSMv2.2 is the additive V3 layer
for sampled suspension, actual tyre geometry, moving closures, and moving side
glass. Existing MCSMv2.1 classes must not be retroactively redefined as V3
objects.

The inherited MCP_OMv1 side-view visual exceptions remain a separate baseline
issue. MCSMv2.2 must not worsen them, but implementation completion must not
misrepresent that inherited exception as newly certified.

## 2. Scope

### 2.1 In Scope

- Exact MCSMv2.2 source-contract verification.
- Deterministic generated C++ catalog data for all four variants.
- Explicit native ownership of 32 suspension hardpoints per variant.
- Deterministic front and rear sampled wheel-pose evaluation.
- Exact 36-pose cardinality per variant.
- Native elliptical-section tyre geometry.
- Transformed tyre samples and conservative sweep geometry.
- Native binding of six semantic closure surfaces to six hinge systems.
- Native binding of four side-glass surfaces to their parent doors.
- Parent-relative helical/barrel glass motion.
- Independent fixed-body distance, parity, containment, and intersection
  validation.
- Native V3 sampled concept-kinematic assurance aggregation.
- Executable closed, open, suspension, and family preview grammars.
- Bounded-LOD GUI preview and selected-state inspection.
- Deterministic hashes, source parity evidence, and compatibility evidence.

### 2.2 Explicitly Deferred

- Measured production hardpoints.
- Exact constrained multi-link suspension solving.
- Compliance, bushing, force, caster-trail, scrub-radius, or load analysis.
- Continuous interval proof for tyre or closure motion.
- Production inner closure panels, hems, reinforcements, hinges, latches,
  seals, or gas struts.
- Production glass guides, regulators, channels, carriers, motors, and
  weatherstrip deformation.
- Manufacturing and structural V4 assurance.
- CFD, crash, homologation, and physical V5 assurance.

Deferred features must remain explicit in the native assurance report. They
must never be inferred from successful sampled V3 checks.

## 3. Delivery Strategy

Use a reference-variant vertical slice, then expand through generated catalog
data. Do not implement four variant-specific procedural branches.

The first usable native milestone is:

1. Verify the exact MCSMv2.2 source release.
2. Generate the reference kinematic catalog entry.
3. Reuse the accepted MCSMv2.1 reference semantic geometry.
4. Reproduce the 32 reference hardpoints and 36 reference wheel poses.
5. Render the reference nominal tyre and its four sweep envelopes.
6. Render the reference vehicle with all closures closed and fully open.
7. Validate the four parent-relative side-glass systems.
8. Emit a parser-compatible reference kinematics preview grammar.

Only after that slice passes should the same architecture be populated for
`track`, `aero`, and `crossover`.

### Efficiency Rules

- Reuse existing generic vehicle models whenever their semantics match.
- Extend existing enums and constructors compatibly rather than introducing a
  second `SuspensionHardpoint` or `VehicleJoint` hierarchy.
- Preserve MCSMv2.1 as the canonical owner of static surface geometry.
- Store one immutable base mesh plus transforms; do not duplicate a tyre,
  closure, or glass mesh for every state.
- Generate catalog data from JSON; do not hand-copy 128 hardpoints or 144 pose
  records into C++.
- Use `glm::dvec3` and `glm::dmat4` for source-parity calculations. Convert to
  float only at the renderer or mesh-buffer boundary.
- Run reference-only math and mesh tests during development.
- Run all-four-variant and visual checks only at milestone boundaries.
- Keep preview geometry below the 16-bit ImGui draw-list vertex limit per draw
  list by using bounded LOD and deterministic mesh splitting.
- Preserve the last valid scene when parsing or generated-mesh resolution
  fails.
- Fail closed on missing source records, unmatched semantic domains, invalid
  transforms, incomplete collision checks, or unsupported assurance claims.

## 4. Canonical Ownership Model

| Concept | Canonical owner | Required action |
|---|---|---|
| Static body and registered surface | `McsMv21SemanticGeometry` | Reuse unchanged |
| Panel and aperture ownership | `VehicleSurfaceDomainPartition` | Reuse and bind 2.2 closure names |
| Source release identity | `McsMv220SourceRelease` | Add |
| Variant V3 definitions | `McsMv22KinematicVariantDefinition` | Add |
| Hardpoint facts | existing `SuspensionHardpoint` | Extend compatibly |
| Corner mechanism definition | existing `SuspensionHardpointModel` | Reuse with explicit policies |
| Wheel state | existing `WheelPoseState` | Reuse |
| Solved wheel pose | existing `SolvedWheelPose` | Extend with full transform if required |
| Actual tyre state sample | `VehicleTyrePoseSample` | Add |
| Actual tyre sweep | `VehicleTyrePoseSweep` | Add |
| Closure-domain association | `VehicleClosureSurfaceBinding` | Add |
| Closure joint | existing `VehicleJoint` plus closure relationship | Reuse |
| Moving side-glass association | `VehicleGlassSurfaceBinding` | Add |
| Generated V3 geometry | `McsMv22KinematicGeometry` | Add |
| V3 assurance | `McsMv22KinematicAssuranceReport` | Add |

Every source fact must have one native owner. Validation reports may reference
facts, but must not become a second parameter store.

## 5. Required Object Model

### 5.1 MCSMv2.2 Source and Family Models

Place MCSMv2.2-specific models under `include/vehicle/mcsmv2/model/`.

#### `McsMv220SourceRelease`

Owns:

- release version;
- model schema;
- manifest schema;
- release manifest SHA-256;
- signed artifact count and total bytes;
- source reference-frame schema;
- source-to-ProGen3D matrix;
- recorded Python acceptance count;
- source assurance boundary.

This class is the MCSMv2.2 equivalent of `McsMv210SourceRelease`. It is not a
subclass because the releases are immutable records rather than substitutable
runtime polymorphic objects.

#### `McsMv22KinematicFamilyDefinition`

Composes:

- one `McsMv220SourceRelease`;
- four `McsMv22KinematicVariantDefinition` objects.

#### `McsMv22KinematicVariantDefinition`

Composes:

- variant identifier and display name;
- expected MCSMv2.1 semantic variant identifier;
- suspension corner definitions;
- sampled wheel-state grid;
- tyre definition and declared clearance;
- six closure surface bindings;
- six hinge or rising-revolute definitions;
- four glass surface bindings;
- four helical glass motion definitions;
- accepted V3 thresholds and source evidence;
- source release gate and achieved assurance level.

It must not duplicate package, static body, UV surface, or domain meshes already
owned by MCSMv2.1.

### 5.2 Suspension and Wheel Models

Reuse `SuspensionHardpoint` and extend it without breaking existing MVPv2.5
callers:

- retain identifier, role, corner, position, classification, and tolerance;
- add source role name where the generic enum is broader than the release role;
- add confidence with a backwards-compatible default;
- preserve `ConceptDerived` as evidence, mapped explicitly to the repository's
  evidence classification vocabulary.

Extend `SuspensionHardpointRole` only where distinct MCSMv2.2 roles cannot be
represented accurately. Required distinctions include:

- `StrutTop`;
- `LowerArmFrontInner`;
- `LowerArmRearInner`;
- `UpperLinkInner`;
- `UpperLinkOuter`;
- `LowerLinkOuter`;
- `ToeLinkInner`;
- `ToeLinkOuter`;
- `DamperTop`.

Do not collapse front/rear inner link locations into one vague mount role.

Add `VehicleWheelPoseSolutionPolicy` as a value model containing the explicit
travel, camber, toe, and steering gains used by the concept solver. The policy
must be composed by `SuspensionHardpointModel`; it must not be hidden as
hard-coded constants inside `WheelPoseSolver`.

Add:

- `VehicleTyrePoseSample`: associates one solved pose with one rigid transform;
- `VehicleTyrePoseSweep`: composes the immutable tyre mesh, 3 or 15 pose
  samples, transformed sample evidence, conservative hull mesh, and validation;
- `VehicleTyreSweepSet`: composes four corner sweeps and the exact total count
  of 36 poses.

### 5.3 Closure and Glass Models

Reuse generic hinge and glass-motion relationships from
`VehicleClosureAssembly.h` where their meaning matches the source release.

Add:

- `VehicleClosureSurfaceBinding`: associates one MCSMv2.1 semantic closure
  domain with one closure identifier and joint relationship;
- `VehicleClosurePoseSample`: owns normalized state, angle, rise, transform,
  minimum non-hinge distance, intersection result, and pass state;
- `VehicleClosureSweep`: composes one surface binding and seven sampled poses;
- `VehicleClosureSweepSet`: composes the six closure sweeps;
- `VehicleGlassSurfaceBinding`: associates one aperture domain with its parent
  door and motion definition;
- `VehicleGlassPoseSample`: owns normalized state, drop, rotation, composed
  world transform, cavity containment, fixed-body distance, support-pose error,
  intersection result, and pass state;
- `VehicleGlassMotionSet`: composes four glass systems with six states each.

World glass motion must always be:

```text
parent door world transform * door-local helical glass transform
```

The glass system must not store an independent world transform that can drift
from its parent closure.

### 5.4 Generated Geometry and Assurance Models

#### `McsMv22KinematicGeometry`

Composes:

- a shared immutable `McsMv21SemanticGeometry` reference;
- four suspension corner models;
- four tyre pose sweeps;
- six closure surface bindings and evaluated sweeps;
- four moving glass surface bindings and evaluated motions;
- generated preview meshes for hardpoints, links, sweep hulls, closure states,
  and glass states;
- one `McsMv22KinematicAssuranceReport`.

#### `McsMv22KinematicAssuranceReport`

Owns explicit V0-V5 status and diagnostics. V3 passes only when:

```text
V2 static concept geometry passes
and hardpoint validation passes
and actual tyre pose sweep validation passes
and closure sweep validation passes
and helical glass validation passes
```

V4 and V5 must remain false with their source-declared boundaries.

Do not reuse a legacy aggregate V2 flag blindly. MCSMv2.2 uses the registered
open semantic exterior surface, so native V2 composition must explicitly reuse
the accepted package, section, character-curve, body-in-white, projection,
partition, and semantic/implicit alignment gates.

## 6. Required Services

### 6.1 Source and Catalog Services

- `McsMv22SourceContractVerificationService`
  - verifies the exact release manifest and all signed files;
  - detects missing, changed, and unexpected release files;
  - records the known verification-report self-accounting discrepancy without
    weakening file hashes;
  - invokes the packaged 34-test Python suite.

- `McsMv22KinematicCatalogFactory`
  - constructs the four generated variant definitions;
  - joins each V3 definition to the matching MCSMv2.1 variant identifier;
  - rejects missing or duplicate variants.

- `McsMv22SemanticKinematicBindingService`
  - binds six closure names and six aperture names to the exact MCSMv2.1
    partition owners;
  - requires 17 panel records, six aperture records, 23 exclusive owners, zero
    unassigned faces, and zero unresolved multiple owners;
  - rejects a V3 catalog if static semantic ownership differs from the source
    release.

### 6.2 Suspension and Tyre Services

- `VehicleSuspensionKinematicEvaluationService`
  - evaluates the explicit policy associated with each corner;
  - returns `glm::dmat4` transforms;
  - preserves zero rear steer;
  - validates finite centres, camber, toe, and rigid rotation determinants.

- `VehicleTyreMeshGenerationService`
  - creates the elliptical-section torus used by MCSMv2.2;
  - produces deterministic vertex ordering and topology;
  - validates watertightness and outer radius.

- `VehicleTyrePoseTransformationService`
  - applies every accepted pose transform to the immutable tyre mesh;
  - records transformed point counts and deterministic hashes;
  - does not retain 36 duplicate base meshes in long-lived runtime state.

- `ConvexHullMeshGenerationService`
  - is a generic geometry service, not an MCSMv2-only procedural function;
  - builds a deterministic conservative hull from transformed tyre vertices;
  - uses deterministic quantization and tie-breaking;
  - exposes topology and volume validation.

- `VehicleTyreSweepValidationService`
  - uses final MCSMv2.1 body triangles rather than the generating field;
  - computes nearest-surface distance;
  - performs sparse parity checks;
  - requires the declared clearance after tessellation tolerance;
  - reports every corner separately and the family minimum.

### 6.3 Closure and Glass Services

- `VehicleClosurePoseEvaluationService`
  - delegates generic hinge and rising-revolute math to
    `VehicleClosureKinematicsService`;
  - evaluates the exact seven source states;
  - keeps the closed state as the identity transform.

- `VehicleClosureSweepValidationService`
  - evaluates moving semantic surface triangles against retained fixed-body
    triangles;
  - excludes only explicitly declared hinge/support adjacency;
  - performs broad-phase candidate filtering and complete candidate
    intersection screening;
  - reports minimum non-hinge distance and intersection completeness.

- `VehicleHelicalGlassMotionService`
  - evaluates downward travel, inward shift, longitudinal shift, and
    barrel-following rotation;
  - composes the result with the parent-door transform;
  - evaluates the exact six source states.

- `VehicleGlassMotionValidationService`
  - checks door-cavity containment;
  - checks minimum distance to fixed-body triangles;
  - checks support-pose composition error;
  - checks complete broad-phase intersection candidates;
  - rejects any system that loses its parent closure relationship.

### 6.4 Shared Geometry Validation Services

Add or extract reusable services rather than embedding collision logic in the
MCSMv2.2 generator:

- `MeshSurfaceDistanceEvaluationService`;
- `MeshRayParityClassificationService`;
- `MeshIntersectionScreeningService`;
- `RigidTransformValidationService`.

These services should reuse `Mesh` collision BVH data. They must be independent
of the source generating field.

### 6.5 Integration Services

- `McsMv22KinematicGeometryGenerationService`
  - accepts one matching MCSMv2.1 geometry and MCSMv2.2 definition;
  - generates all V3 systems through the services above;
  - returns no geometry when any blocking diagnostic exists.

- `McsMv22GeneratedMeshProvider`
  - resolves bounded-LOD state and engineering mesh keys;
  - caches immutable base geometry and derived state snapshots;
  - never regenerates expensive geometry inside the render loop.

- `McsMv22KinematicGrammarLoweringService`
  - emits only current parser-supported syntax;
  - always supplies a material for geometric instances;
  - emits family, closed, open, suspension, and selected-state previews;
  - emits generated mesh references rather than source target-grammar
    declarations that the current parser does not support.

- `McsMv22DeterministicHashService`
  - hashes source identity, variant definitions, hardpoints, policies, sampled
    states, transforms, topology, semantic bindings, validation results, and
    assurance boundaries;
  - uses stable ordering and explicit numeric quantization.

## 7. Generated Catalog Pipeline

Add `tools/generate_mcsmv22_catalog.py`.

Inputs:

- `VERSION.json`;
- `RELEASE_MANIFEST.json`;
- `validation.json`;
- `partitions/*_surface_partition.json`;
- `kinematics/*_suspension_hardpoints.json`;
- `kinematics/*_wheel_pose_sweeps.json`;
- `kinematics/*_closure_system.json`;
- the matching MCSMv2.1 generated catalog identity.

Outputs:

- `include/vehicle/mcsmv2/generated/GeneratedMcsMv22Catalog.h`;
- `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_NATIVE_CATALOG_MANIFEST.json`.

The generated header should contain compact source facts and accepted sample
records, not millions of mesh vertices. Runtime services generate mesh data
from the accepted definitions.

Required generation checks:

1. exact four-variant order;
2. exactly 32 hardpoints per variant;
3. exactly 36 accepted wheel poses per variant;
4. front pose cardinality of 15 per side;
5. rear pose cardinality of 3 per side;
6. exactly six closure definitions per variant;
7. exactly four glass systems per variant;
8. seven accepted states per closure;
9. six accepted states per moving glass;
10. exact closure-to-domain and glass-to-parent associations;
11. finite source-to-ProGen3D conversion;
12. two generated runs produce byte-identical outputs.

## 8. Phased Implementation

### Phase 0 - Freeze and Verify the Source Contract

**Goal:** Prevent native work from beginning on a changed or incomplete release.

Tasks:

1. Add `tools/verify_mcsmv220_source_contract.py`.
2. Add `tests/run_mcsmv220_source_contract_checks.sh`.
3. Verify all 109 signed file sizes and SHA-256 values.
4. Reject unexpected files inside the signed release root.
5. Run the packaged 34-test suite with bytecode generation disabled.
6. Record Python, NumPy, scikit-image, and trimesh versions.
7. Record the 45-byte `RELEASE_VERIFICATION.json` self-accounting difference
   as a known metadata defect while retaining the manifest as authoritative.
8. Add an adjacent source-contract classification record; do not modify the
   source release to store native planning or evidence.

Exit gate:

- Exact manifest SHA and 109-file package pass.
- All 34 packaged tests pass.
- No `.pyc`, `__pycache__`, temporary regeneration, or editor files are added
  to the release directory.

### Phase 1 - Generate the Native Catalog

**Goal:** Create deterministic native source facts before adding runtime math.

Tasks:

1. Add `McsMv220SourceRelease`.
2. Add the family and variant definition classes.
3. Implement `tools/generate_mcsmv22_catalog.py`.
4. Generate `GeneratedMcsMv22Catalog.h`.
5. Implement `McsMv22KinematicCatalogFactory`.
6. Add catalog and source-identity harnesses.
7. Compare two generated catalog runs byte-for-byte.

Exit gate:

- Four deterministic variants exist.
- Every cardinality and schema listed in Section 7 passes.
- The MCSMv2.1 semantic variant identifier matches each MCSMv2.2 variant.

### Phase 2 - Implement Reference Suspension Parity

**Goal:** Reproduce the source reference hardpoints and wheel transforms.

Tasks:

1. Extend generic hardpoint roles and evidence fields compatibly.
2. Introduce the explicit wheel-pose solution policy.
3. Implement double-precision source-frame and ProGen3D-frame evaluation.
4. Reproduce all 32 reference hardpoints.
5. Reproduce all 36 reference wheel poses.
6. Compare centres, steer, travel, camber, toe, and matrices against source
   records.
7. Validate left/right mirroring and rotation determinants.

Exit gate:

- Hardpoint positions agree within `1e-9 m` in double-precision source space.
- Pose scalar values agree within `1e-9` before renderer conversion.
- Transform rotations have determinant within `1e-9` of one.
- Source-to-ProGen3D conversion preserves grounding and orientation.

### Phase 3 - Implement Actual Tyre Pose Sweeps

**Goal:** Replace analytic bounds-only wheel evidence with actual tyre geometry.

Tasks:

1. Implement deterministic elliptical tyre mesh generation.
2. Validate watertight topology and expected outer radius.
3. Transform the tyre through all reference states.
4. Implement the deterministic convex-hull service.
5. Construct four reference sweep hulls.
6. Implement final-body distance and parity checks.
7. Compare native minimum clearances with accepted source values.
8. Add preview LOD for nominal tyres and sweep hulls.

Exit gate:

- Exactly 36 transformed poses are evaluated.
- Every transformed tyre sample is finite and outside the final body.
- Every corner passes its declared clearance and tessellation tolerance.
- Repeated runs produce identical topology and hashes.

### Phase 4 - Implement Reference Closure Motion

**Goal:** Bind the six source closures to native semantic surfaces and joints.

Tasks:

1. Bind four doors, bonnet, and rear hatch to exact partition domains.
2. Reuse generic hinge and rising-revolute relationships.
3. Evaluate seven states per closure.
4. Generate closed and fully open snapshot meshes.
5. Validate fixed-body distance and complete candidate intersections.
6. Preserve the compound rear-hatch limitation explicitly.

Exit gate:

- Six unique surface bindings and six unique joint relationships exist.
- Closed transforms are identity transforms.
- Fully open transforms move every closure.
- All 42 sampled closure states pass complete intersection screening.

### Phase 5 - Implement Reference Helical Glass Motion

**Goal:** Add four parent-relative moving side-glass systems.

Tasks:

1. Bind front and rear side glass on both sides to their parent doors.
2. Evaluate six glass states per system.
3. Compose local glass motion with the evaluated parent-door transform.
4. Validate drop direction, barrel rotation, cavity containment, fixed-body
   distance, support-pose error, and intersections.
5. Generate closed and fully dropped snapshot meshes.

Exit gate:

- Four moving glass systems and 24 sampled states exist.
- State zero is identity relative to the parent door.
- Full state moves every glass downward.
- Parent-child transform composition error is within `1e-9` in double
  precision.
- All cavity and collision gates pass.

### Phase 6 - Compose Native V3 Assurance

**Goal:** Make the assurance claim an explicit, fail-closed object model.

Tasks:

1. Implement `McsMv22KinematicAssuranceReport`.
2. Reuse accepted MCSMv2.1 V2 semantic evidence.
3. Add hardpoint, tyre, closure, and glass reports.
4. Require every component gate before V3 passes.
5. Preserve V4 and V5 as false with exact limitations.
6. Include maximum alignment outliers as diagnostics even though the accepted
   source gate uses p95 distance and normal thresholds.

Exit gate:

- Reference reaches V3 only through explicit component evidence.
- Removing or failing any component demotes the result and emits a blocking
  diagnostic.
- No V4 or V5 claim can be produced by the V3 service.

### Phase 7 - Expand Through the Four-Variant Family

**Goal:** Prove that the implementation is generated and parametric.

Tasks:

1. Populate `track`, `aero`, and `crossover` through the generated catalog.
2. Use the same service graph for every variant.
3. Generate all hardpoint, pose, tyre, closure, and glass systems.
4. Compare native accepted sample records with the source package.
5. Produce family-level minimum clearance and assurance summaries.

Exit gate:

- Four variants each have 32 hardpoints, 36 tyre poses, six closures, and four
  moving glass systems.
- Every variant reaches V3.
- No variant-specific procedural branch exists outside explicit policy data.

### Phase 8 - Add Generated Mesh Resolution and Executable Grammar

**Goal:** Make the V3 model usable in the current editor without extending the
grammar with unsupported source-target constructs.

Tasks:

1. Implement `McsMv22GeneratedMeshProvider`.
2. Add bounded keys for fixed body, closure state snapshots, glass state
   snapshots, suspension hardpoints, nominal tyres, and sweep hulls.
3. Implement `McsMv22KinematicGrammarLoweringService`.
4. Generate one reference kinematics preview first.
5. Generate closed and open previews for every variant.
6. Generate a family closed preview and a family engineering preview.
7. Run every grammar through the current parser and headless GUI smoke test.
8. Check per-draw-list vertex counts and split generated meshes when required.

Exit gate:

- Every grammar starts with `Start ->`.
- No fallback entry rule is used.
- No unsupported rule reference or function is emitted.
- Every `!I(...)` geometry instance has a material argument.
- No scene-generation failure or ImGui vertex-index assertion occurs.
- Direct-service and grammar-resolved component hashes agree.

### Phase 9 - Add Selected-State Editor Inspection

**Goal:** Inspect kinematic state without duplicating geometry or bypassing
existing relationship owners.

Tasks:

1. Add `McsMv22KinematicState` as a value model containing selected wheel,
   closure, and glass states.
2. Add `McsMv22KinematicPoseEvaluationService`.
3. Route closure state through `VehicleJointGraph` and existing kinematic
   services.
4. Update preview transforms in place rather than rebuilding unrelated body
   geometry.
5. Keep expensive tyre-hull construction cached and state-independent.
6. Add closed, half-state, and full-state deterministic screenshots.

Exit gate:

- Changing state does not mutate canonical source definitions.
- Returning to a prior state restores the same transform and rendered hash.
- Parent-relative glass remains synchronized with its door.
- Scene vertex counts remain bounded across repeated state changes.

### Phase 10 - Native, Visual, and Compatibility Audit

**Goal:** Complete a requirement-to-evidence audit without overstating the V3
boundary.

Run focused MCSMv2.2 gates first, then all inherited compatibility gates.

Required new scripts:

- `tests/run_mcsmv220_source_contract_checks.sh`;
- `tests/run_mcsmv22_catalog_checks.sh`;
- `tests/run_mcsmv22_suspension_checks.sh`;
- `tests/run_mcsmv22_tyre_sweep_checks.sh`;
- `tests/run_mcsmv22_closure_checks.sh`;
- `tests/run_mcsmv22_glass_checks.sh`;
- `tests/run_mcsmv22_assurance_checks.sh`;
- `tests/run_mcsmv22_grammar_checks.sh`;
- `tests/run_mcsmv22_visual_checks.sh`;
- `tests/run_mcsmv22_compatibility_checks.sh`.

Required inherited gates:

- `tests/run_mcsmv210_source_contract_checks.sh`;
- `tests/run_mcsmv21_catalog_checks.sh`;
- `tests/run_mcsmv21_registration_checks.sh`;
- `tests/run_mcsmv21_semantic_geometry_checks.sh`;
- `tests/run_mcsmv21_grammar_checks.sh`;
- `tests/run_mcsmv21_silhouette_checks.sh`;
- `tests/run_mcsmv201_source_contract_checks.sh`;
- `tests/run_mcsmv2_source_contract_checks.sh`;
- `tests/run_mcsmv2_semantic_section_checks.sh`;
- `tests/run_mcsmv2_implicit_body_checks.sh`;
- `tests/run_mcsmv2_semantic_surface_checks.sh`;
- `tests/run_mcsmv2_engineering_checks.sh`;
- `tests/run_mcsmv2_grammar_checks.sh`;
- `tests/run_mcsmv2_visual_checks.sh`;
- `tests/run_mvpv25_hatchback_checks.sh`;
- `tests/run_mvpv25_hatchback_visual_checks.sh`;
- `tests/run_mvpv25_reference_collision_grammar_checks.sh`;
- `tests/run_mvp26_parametric_integration_checks.sh`;
- MCP_OMv1 native, grammar, and visual gates;
- MVPv1 and MVPv2 native, grammar, fitting, and visual gates;
- `make -j2 progen3d-editor-gui`;
- full `make -j2` after focused acceptance passes.

Release evidence:

- `tests/evidence/mcsmv22_acceptance_2026-08-24.json`;
- `tests/evidence/mcsmv22_state_parity/`;
- `tests/evidence/mcsmv22_three_view_silhouettes/`;
- `tests/evidence/mcsmv22_headless_grammar_logs/`;
- `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_2_0_NATIVE_IMPLEMENTATION_VERIFICATION.md`.

Exit gate:

- Every MCSMv2.2 checklist requirement maps to native and source evidence.
- Every new visualized feature also has a native source-level assertion.
- All compatibility changes are additive.
- Inherited failures are reported separately and are not promoted to passes.
- Exact source, generated catalog, grammar, mesh, image, and evidence hashes are
  recorded.

## 9. Example Executable Grammar

The signed release grammar is a target-language description and is explicitly
not compatible with the current parser. Native lowering should emit compact,
current syntax such as:

```text
# MCSMv2.2 executable reference kinematics preview.
# +X right, +Y up, +Z forward; origin is ground package centre.

Start ->
    MCSMv22ReferenceKinematicsPreview

MCSMv22ReferenceKinematicsPreview ->
[
    Object(id(MCSMV22_REFERENCE)
           name(ResearchMedianAWDHotHatchV22)
           class(ModernCarKinematicVariantV22)
           taxonomy(Vehicle MCSMv2_2 RegisteredSurface SampledKinematics V3)
           layer(Equipment) mask(Equipment Terrain Temporary))
    [
        Interface(id(packageCentre) type(InspectionInterface)
                  origin(0 0 0) normal(0 1 0) tangent(0 0 1)
                  region(Point) tolerance(0.001))
        MCSMv22ReferenceFixedBody
        MCSMv22ReferenceClosuresOpen
        MCSMv22ReferenceGlassOpen
        MCSMv22ReferenceSuspension
        MCSMv22ReferenceTyreSweeps
    ]
]

MCSMv22ReferenceFixedBody ->
[
    !I(GeneratedMeshReference(
          meshKey(MCSMv22ReferenceFixedBodyMesh)
          detail(LOD2)
          topology(surface))
       material(redmetallicpaint) alpha(1) texscale(0.16))
]

MCSMv22ReferenceClosuresOpen ->
[
    !I(GeneratedMeshReference(
          meshKey(MCSMv22ReferenceClosuresOpenMesh)
          detail(LOD2)
          topology(surface))
       material(redmetallicpaint) alpha(1) texscale(0.16))
]

MCSMv22ReferenceGlassOpen ->
[
    !I(GeneratedMeshReference(
          meshKey(MCSMv22ReferenceGlassOpenMesh)
          detail(LOD2)
          topology(surface))
       material(smokytransparentglass) alpha(0.56) texscale(0.10))
]

MCSMv22ReferenceSuspension ->
[
    !I(GeneratedMeshReference(
          meshKey(MCSMv22ReferenceSuspensionMesh)
          detail(LOD1)
          topology(surface))
       material(charcoalroughmetal) alpha(1) texscale(0.08))
]

MCSMv22ReferenceTyreSweeps ->
[
    !I(GeneratedMeshReference(
          meshKey(MCSMv22ReferenceTyreSweepMesh)
          detail(LOD1)
          topology(surface))
       material(charcoalroughrubber) alpha(0.45) texscale(0.10))
]
```

The first grammar milestone is a deterministic snapshot, not a new grammar
language for arbitrary kinematic expressions. Interactive state belongs to the
native state and service model.

## 10. Acceptance Matrix

| Release requirement | Native owner | Required evidence |
|---|---|---|
| Persistent UV surface | MCSMv2.1 geometry | existing registration and silhouette gates |
| Implicit scaffold projection | MCSMv2.1 geometry | existing projection and alignment gates |
| Exclusive surface partition | MCSMv2.1 partition plus 2.2 binding | 23-owner binding harness |
| Six glass apertures | MCSMv2.1 partition | exact aperture-name assertions |
| Six movable closures | closure binding and sweep set | source parity, native motion, grammar views |
| 32 hardpoints | suspension models | catalog and transform parity |
| Front/rear pose solver | explicit solution policy | 36-state parity harness |
| Actual tyre sweeps | tyre pose sweep set | topology, clearance, hull, visual evidence |
| Independent tyre clearance | tyre validation service | final-triangle distance and parity report |
| Closure hinge transforms | closure pose service | identity, rigidity, seven-state parity |
| Independent closure validation | closure validation service | complete intersection report |
| Parent-relative helical glass | glass motion service | parent composition parity |
| Glass cavity and collision | glass validation service | containment, distance, intersection report |
| V3 assurance | assurance report | fail-closed component matrix |
| Four variants | generated family catalog | all-four-variant acceptance summary |
| Automated tests | test scripts and harnesses | complete command transcript |
| Release manifest and hashes | source contract service | exact 109-file verification |

## 11. Performance and Editor Safety

- Cache generated MCSMv2.1 geometry by semantic variant hash.
- Cache tyre base mesh by tyre-parameter hash.
- Cache pose transforms by variant, corner, and state-grid hash.
- Cache sweep hulls by tyre mesh and pose-transform hashes.
- Cache closure and glass state snapshots by base mesh and transform hash.
- Do not retain redundant transformed tyre meshes after hull and validation
  evidence are produced unless a selected state requires display.
- Keep `LOD1` engineering previews separate from evidence/export geometry.
- Split combined meshes deterministically before any draw list exceeds 65,535
  indexed vertices.
- Never put every full-resolution tyre pose, closure state, and glass state into
  one editor scene.
- Perform collision and hull generation outside the render loop.
- Make cache invalidation explicit through deterministic source hashes.

## 12. Recommended File Sequence

Implement in this order to minimize blocked work:

1. `tools/verify_mcsmv220_source_contract.py`
2. `tests/run_mcsmv220_source_contract_checks.sh`
3. `include/vehicle/mcsmv2/model/McsMv220SourceRelease.h`
4. MCSMv2.2 family and variant definition headers.
5. `tools/generate_mcsmv22_catalog.py`
6. `include/vehicle/mcsmv2/generated/GeneratedMcsMv22Catalog.h`
7. `McsMv22KinematicCatalogFactory`
8. Compatible extensions to generic hardpoint and wheel-pose models.
9. `VehicleSuspensionKinematicEvaluationService`
10. Suspension parity harness.
11. `VehicleTyreMeshGenerationService`
12. `VehicleTyrePoseTransformationService`
13. `ConvexHullMeshGenerationService`
14. Shared mesh distance, parity, and intersection services.
15. Tyre sweep generation and validation harnesses.
16. Closure and glass surface-binding models.
17. Closure and glass evaluation services.
18. Closure and glass validation harnesses.
19. `McsMv22KinematicAssuranceReport`
20. `McsMv22KinematicGeometryGenerationService`
21. `McsMv22GeneratedMeshProvider`
22. `McsMv22KinematicGrammarLoweringService`
23. Grammar generation tool and committed previews.
24. State inspection integration.
25. Four-variant visual and compatibility audit.
26. Final native verification and exact-hash evidence.

## 13. Planning Estimate

These estimates are sequencing guidance, not completion evidence.

| Milestone | Focused estimate | Result |
|---|---:|---|
| Source contract and catalog | `0.5-1 day` | deterministic native facts |
| Reference suspension parity | `0.5-1 day` | 32 hardpoints and 36 poses |
| Actual tyre geometry and hulls | `1-1.5 days` | V3 wheel sweep evidence |
| Closure and glass motion | `1-1.5 days` | six closures and four glass systems |
| Independent validation and assurance | `1 day` | native V3 report |
| Grammar and selected-state preview | `1 day` | executable editor views |
| Four variants and compatibility audit | `1-1.5 days` | final evidence package |

The fastest safe critical path is approximately six focused development days,
assuming the existing generic vehicle kinematic types require extension rather
than replacement.

## 14. Completion Definition

MCSMv2.2 native implementation is complete only when:

1. The exact 109-file source release and manifest hash pass current verification.
2. The packaged 34 Python tests pass in the implementation environment.
3. Generated native catalog output is deterministic.
4. All four variants bind to their accepted MCSMv2.1 semantic geometry.
5. Every variant owns exactly 32 hardpoints and 36 wheel poses.
6. Actual tyre meshes are watertight and transformed through every accepted
   state.
7. Every tyre sweep passes independent final-body clearance and parity checks.
8. Six closure systems and four parent-relative glass systems exist per
   variant.
9. Every sampled closure and glass state passes complete validation.
10. V3 passes only through explicit V2, hardpoint, tyre, closure, and glass
    evidence.
11. V4 and V5 remain explicitly unclaimed.
12. Closed, open, suspension, and family grammars parse and regenerate without
    fallback or blocking diagnostics.
13. GUI previews remain within draw-index limits and preserve the last valid
    scene on failure.
14. Direct-service and grammar-resolved hashes agree.
15. MCSMv2.0.1, MCSMv2.1, MCP_OMv1, MVPv1, MVPv2, MVPv2.5, and MVPv2.6
    compatibility results are recorded without hiding inherited exceptions.
16. Every requirement has source, native, visual where applicable, and exact
    artifact evidence.

## 15. Fastest Critical Path

```text
Immutable Source Contract
    -> Generated V3 Catalog
    -> Reference Suspension Parity
    -> Reference Actual Tyre Sweeps
    -> Reference Closures
    -> Reference Parent-Relative Glass
    -> Native V3 Assurance
    -> Generated Mesh Provider
    -> Executable Reference Grammar
    -> Four-Variant Expansion
    -> Selected-State GUI Inspection
    -> Compatibility and Exact-Hash Audit
```

This ordering produces visible, validated V3 behavior early while preserving
the accepted MCSMv2.1 static geometry and the wider ProGen3D vehicle stack.
