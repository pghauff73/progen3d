# ProGen3D MCSMv2.0.1 Native Integration Implementation Plan

**Date:** Monday, August 24, 2026
**Status:** Proposed implementation plan
**Source release:** `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_0_1_Modern_Car_Complete/`
**Native integration target:** `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_Modern_Car_Complete/`

## 1. Purpose

Integrate the MCSMv2.0.1 reference generator, semantic data, engineering data,
and integrity evidence into the active native ProGen3D MCSMv2 object model.

The implementation must:

1. preserve the signed MCSMv2.0.1 release as immutable source evidence;
2. reproduce its four vehicle variants through purpose-driven native C++
   model and service classes;
3. retain one canonical owner for every parameter, station, curve, engineering
   relationship, and assurance fact;
4. make every supported vehicle executable through current ProGen3D grammar;
5. maintain deterministic direct-service and grammar-generated geometry;
6. require native, grammar, GUI, artifact, and three-view visual acceptance;
7. preserve MVPv2.5, MCP_OMv1, and existing MCSMv2 compatibility; and
8. fail closed for deferred MCSMv2.1, kinematic, structural, manufacturing, and
   physical-validation claims.

This plan does not modify the source release directory because adding or
changing files there would invalidate its stored file count and hashes.

## 2. Current Baseline

### 2.1 MCSMv2.0.1 reference release

The reference package already provides:

- four variants: reference, track, aero, and crossover;
- an explicit X-front, Y-right, Z-up source frame;
- a source-to-ProGen3D coordinate adapter;
- parameter-specific evidence, units, confidence, and uncertainty;
- a typed causal parameter dependency graph;
- eleven semantic section stations;
- fourteen named character curves;
- semantic surface and implicit body representations;
- wheel-motion envelope approximations;
- panel, BIW, occupant, and functional-package graphs;
- final-mesh integrity and package checks;
- stored V2 assurance records and release manifests; and
- high-resolution STL, GLB, CSV, JSON, and preview artifacts.

The package's `.p3d` file is a target grammar specification. Its conceptual
operators are not compatible with the current parser and must not be treated as
an executable acceptance artifact.

### 2.2 Existing native implementation

The repository already contains a native MCSMv2 object model and should be
extended rather than replaced. Important existing abstractions include:

- `ModernCarSemanticFamily`;
- `ModernCarSemanticVariant`;
- `VehicleSemanticSectionField`;
- `VehicleCharacterCurveNetwork`;
- `VehicleSemanticSurface`;
- `ImplicitScalarField` implementations;
- `VehicleWheelEnvelopeSet`;
- `VehiclePanelPatchGraph`;
- `VehicleBodyInWhiteAssembly`;
- `VehicleOccupantEnvelopeSystem`;
- `VehicleFunctionalPackageSystem`;
- `ModernCarSemanticCatalogFactory`;
- `ModernCarSemanticBodyFieldFactory`;
- `ModernCarSemanticIsoSurfaceGenerationService`;
- `McsMv2GeneratedMeshProvider`; and
- `ModernCarSemanticGrammarLoweringService`.

The integration must first compare this implementation to MCSMv2.0.1 and add
only missing source-contract, integrity, parity, or MCSMv2.1 responsibilities.

## 3. Architectural Decisions

### 3.1 Immutable reference authority

`MCSMv2_0_1_Modern_Car_Complete` remains the authoritative historical source
release. It is read-only input to catalog generation and parity validation.

No implementation step may:

- rewrite its Python source;
- remove its signed bytecode files in place;
- regenerate its stored models over existing artifacts;
- update its manifest without producing a separately versioned release; or
- reinterpret a deferred assurance item as complete.

### 3.2 Canonical native catalog

The generated native catalog is the runtime owner of imported source facts.
Grammar files, services, tests, and preview tools consume the catalog instead of
duplicating vehicle dimensions or styling constants.

| Fact | Canonical owner | Derived consumers |
|---|---|---|
| Source artifact hashes | native catalog manifest | contract tests, release report |
| Coordinate frame | variant reference-frame record | field, curve, grammar placement |
| Package dimensions | `ModernCarSemanticVariant` composition | body field, wheels, validation |
| Station values | `VehicleSemanticSectionField` | curves, surface, body field |
| Character curves | `VehicleCharacterCurveNetwork` | previews, surface, parity checks |
| Body mesh | procedural mesh repository entry | grammar, exporters, GUI |
| Panel relationships | `VehiclePanelPatchGraph` | engineering validation and views |
| BIW relationships | `VehicleBodyInWhiteAssembly` | engineering validation and views |
| Occupant envelopes | `VehicleOccupantEnvelopeSystem` | containment validation |
| Functional envelopes | `VehicleFunctionalPackageSystem` | package validation |
| Assurance result | `ModernCarSemanticValidationReport` | evidence and release gates |

### 3.3 Grammar is a lowering target

The initial integration will not add every conceptual MCSMv2.0.1 Python
operation as a grammar primitive. Current grammar should express object
identity, orientation, material, detail level, and immutable mesh references.

Detailed geometry remains native C++ behavior reached through:

```text
ModernCarSemanticGrammarLoweringService
    -> GeneratedMeshReference
    -> GeneratedMeshReferenceResolutionService
    -> GeneratedMeshProviderRegistry
    -> McsMv2GeneratedMeshProvider
    -> ProceduralMeshRepository
```

### 3.4 Explicit placement contract

Every vehicle object must carry:

- a named reference frame;
- a ground-contact plane;
- a local origin;
- an explicit forward direction;
- an explicit up direction;
- a source-to-ProGen3D transform; and
- a verified ground-clearance relationship.

Vehicle orientation must never be inferred from mesh bounds or camera view.

### 3.5 Assurance boundaries

The first integration target remains **V2 concept geometry**. MCSMv2.1 status
may be assigned only after its separately listed gates pass.

The implementation must not claim:

- production Class-A surface quality;
- continuous closure or suspension kinematics;
- geometric BIW load-path adequacy;
- manufacturability;
- CFD, crash, thermal, durability, or NVH validation; or
- homologation or physical validation.

## 4. Target Object Model

### 4.1 Existing model classes to retain

| Class | Category | Responsibility |
|---|---|---|
| `ModernCarSemanticFamily` | model | Composes the supported vehicle variants |
| `ModernCarSemanticVariant` | model | Owns one coherent vehicle definition |
| `VehicleSemanticSectionField` | model | Owns ordered semantic section stations |
| `VehicleCharacterCurveNetwork` | model | Owns the fourteen named character curves |
| `VehicleSemanticSurface` | model | Owns the sampled semantic design surface |
| `VehicleWheelEnvelopeSet` | model | Owns side-specific wheel motion envelopes |
| `VehiclePanelPatchGraph` | relationship model | Owns panel patches and adjacency relationships |
| `VehicleBodyInWhiteAssembly` | assembly model | Owns BIW members and joint relationships |
| `VehicleOccupantEnvelopeSystem` | system model | Owns occupant package envelopes |
| `VehicleFunctionalPackageSystem` | system model | Owns functional volume envelopes |
| `ModernCarSemanticValidationReport` | evidence model | Owns measured validation and assurance results |

### 4.2 New classes required only when absent

| Class | Category | Responsibility |
|---|---|---|
| `McsMv201SourceRelease` | source model | Identifies version, schema, root, and immutable manifest |
| `McsMv201SourceArtifact` | source model | Identifies one source artifact and expected hash |
| `McsMv201SourceContractValidationService` | validation service | Verifies schemas, files, hashes, and required records |
| `McsMv201CatalogGenerationService` | generation service | Converts validated source data into deterministic native records |
| `VehicleReferenceFrameDefinition` | model | Owns axes, units, handedness, and adapter transform |
| `VehicleCoordinateTransformationService` | service | Converts points and directions between source and ProGen3D frames |
| `McsMv201ParityEvaluationService` | validation service | Compares source and native semantic outputs |
| `SemanticImplicitAgreementReport` | evidence model | Records semantic-to-implicit surface distance metrics |
| `SemanticImplicitAgreementEvaluationService` | validation service | Implements the MCSMv2.1 authoritative-surface convergence gate |
| `VehiclePanelDomainPartition` | relationship model | Owns an exclusive panel-domain assignment for MCSMv2.1 |
| `VehiclePanelDomainValidationService` | validation service | Detects overlaps, gaps, and invalid panel boundaries |
| `VehicleGlassApertureSystem` | system model | Owns explicit glass apertures and supporting surfaces |

Before adding any new class, audit the current native model for an existing
class with the same semantic responsibility. Extend the existing owner when the
relationship is truly the same; do not create aliases or parallel owners.

## 5. Implementation Phases

## Phase 0 — Reproduce and Freeze the Source Contract

### Tasks

1. Add a source-contract checker outside the immutable release directory.
2. Verify all `RELEASE_MANIFEST.json` file sizes and SHA-256 hashes.
3. Verify the declared schema and version are exactly MCSMv2.0.1.
4. Create a clean Python environment from `source/requirements.txt`.
5. Install NumPy, SciPy, scikit-image, trimesh, and Pillow.
6. Run the complete Python unit-test suite without importing stored `.pyc`
   files.
7. Regenerate all four variants to a temporary output directory.
8. Compare regenerated semantic and validation artifacts to stored evidence.
9. Record runtime, dependency versions, commands, and source hashes.

### Commands

```bash
python3 -m venv /tmp/mcsmv201-venv
/tmp/mcsmv201-venv/bin/python -m pip install --upgrade pip
/tmp/mcsmv201-venv/bin/python -m pip install \
  -r examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_0_1_Modern_Car_Complete/source/requirements.txt

cd examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_0_1_Modern_Car_Complete/source
PYTHONDONTWRITEBYTECODE=1 /tmp/mcsmv201-venv/bin/python -B \
  -m unittest discover -s tests -v
```

### Acceptance

- all 25 archived tests reproduce in the clean environment;
- the manifest has no missing, modified, or unexpected source artifacts;
- regeneration writes only to a temporary directory;
- dependency versions and Python version are captured; and
- any source/artifact mismatch blocks catalog generation.

## Phase 1 — Generate the Canonical Native Catalog

### Tasks

1. Extend
   `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_Modern_Car_Complete/source/generate_native_catalog.py`
   to consume the MCSMv2.0.1 source release explicitly.
2. Validate JSON schemas before reading values.
3. Validate CSV station roles, order, units, and finite values.
4. Import the source reference frame and adapter matrix.
5. Import all four parameter dependency graphs.
6. Import the eleven stations and eight semantic landmarks per variant.
7. Import the fourteen character curves per variant.
8. Import panel, BIW, occupant, and functional-package records.
9. Import evidence status, confidence, uncertainty, and assurance data.
10. Store source artifact hashes in `native_catalog_manifest.json`.
11. Generate `include/vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h`.
12. Run two independent generations and require byte-identical output.

### Acceptance

- exactly four variants are generated;
- every generated record identifies its source artifact and schema;
- all station and curve roles are preserved by name;
- no grammar file owns duplicated dimensions or style constants;
- generated output contains no timestamps, absolute paths, `.pyc`, or
  `__pycache__` data; and
- two generations produce identical header and manifest hashes.

## Phase 2 — Establish Native Semantic Parity

### Tasks

1. Construct each variant through `ModernCarSemanticCatalogFactory`.
2. Validate family composition and variant identity.
3. Validate package, platform, style, wheel, powertrain, and closure values.
4. Evaluate the complete dependency graph in deterministic topological order.
5. Construct the source and ProGen3D reference frames explicitly.
6. Validate axis mapping for points, vectors, wheel centers, and ground plane.
7. Evaluate all semantic station landmarks over a dense longitudinal domain.
8. Require positive landmark gaps and non-crossing interpolation.
9. Extract all fourteen character curves.
10. Generate the semantic surface and record its deterministic hash.
11. Compare source and native values through
    `McsMv201ParityEvaluationService`.

### Acceptance

- the dependency graph is acyclic and complete;
- all source units and value types are retained;
- all semantic stations remain ordered over the dense audit domain;
- frame conversion is round-trip stable within the selected tolerance;
- curve and field residuals remain within the source-contract tolerance; and
- native semantic hashes are deterministic across repeated runs.

## Phase 3 — Establish Native Body and Wheel-Envelope Parity

### Tasks

1. Build each semantic body field through
   `ModernCarSemanticBodyFieldFactory`.
2. Preserve lower-body, greenhouse, fender, wheelhouse, and package-field
   composition as explicit scalar-field objects.
3. Construct side-specific wheel motion envelopes.
4. Generate editor-safe native bodies through
   `ModernCarSemanticIsoSurfaceGenerationService`.
5. Orient closed surfaces through the existing face-orientation service.
6. Reject non-finite vertices, degenerate triangles, boundary edges,
   nonmanifold edges, negative volume, and unstable topology hashes.
7. Validate package dimensions, wheelbase, tracks, ground clearance, occupant
   containment, and final-mesh tyre clearance.
8. Retain the high-resolution Python body as the reference silhouette, not as
   the editor runtime mesh.

### Resolution policy

- **Editor preview:** use the established editor-safe native grid.
- **Visual acceptance:** use the accepted watertight comparison grid.
- **Offline reference:** retain the high-resolution Python artifacts.
- **Family preview:** avoid dense debug-point and curve overlays that can exceed
  the ImGui 16-bit draw-list vertex limit.

### Acceptance

- all four native bodies are watertight and consistently wound;
- signed volume is positive;
- each body is grounded in the declared frame;
- wheelhouse clearance meets the declared tolerance;
- repeated generation produces identical topology hashes;
- direct-service and repository-resolved meshes are identical; and
- the GUI preview completes without assertion failure or `SIGABRT`.

## Phase 4 — Integrate Engineering Relationship Models

### Tasks

1. Construct panel patches with `VehiclePanelPatchExtractionService`.
2. Construct panel adjacency through explicit relationship objects.
3. Construct BIW members and joints through
   `VehicleBodyInWhiteAssemblyService`.
4. Construct occupant envelopes through
   `VehicleOccupantEnvelopeConstructionService`.
5. Construct functional envelopes through
   `VehicleFunctionalPackageConstructionService`.
6. Run the corresponding validation service for every system.
7. Generate deterministic engineering hashes.
8. Keep graph connectivity claims separate from geometric contact or load-path
   claims.

### Acceptance

- panel and BIW identifiers are unique and source-traceable;
- every relationship references existing endpoints;
- BIW connectivity passes without claiming structural adequacy;
- occupant and functional envelopes remain inside their required package;
- engineering hashes are deterministic; and
- visual engineering scenes agree with the native relationship models.

## Phase 5 — Produce Current-Parser Executable Grammars

### Tasks

1. Generate grammars through
   `tools/generate_mcsmv2_grammars.cpp` and
   `ModernCarSemanticGrammarLoweringService`.
2. Generate one preview grammar for each variant.
3. Generate an executable four-variant family preview.
4. Generate the current executable `MCSMv2_Modern_Car_Family.p3d`.
5. Put `Start` first in every grammar.
6. Use `GeneratedMeshReference` with a registered immutable mesh key.
7. Supply a valid material argument to every `!I(...)` instance.
8. Emit explicit object class, identity, frame, position, and orientation
   metadata.
9. Keep conceptual target grammar text as documentation only.
10. Do not embed generated vertices or high-resolution mesh data in grammar.

### Example executable grammar shape

```text
Start ->
[
    Object(
        name(MCSMv2Reference)
        class(ModernCarSemanticVariant)
        frame(MCSMv2VehicleFrame)
        position(0 0 0)
        forward(0 0 1)
        up(0 1 0)
        grounded(true))

    !I(GeneratedMeshReference(
          meshKey(MCSMv2ReferenceBodyMesh)
          detail(LOD2))
       material(redmetallicpaint)
       alpha(1)
       texscale(0.16))
]
```

The exact object metadata syntax must use only syntax already supported by the
current parser. If a metadata field is unsupported, store it in the native
object model and lower only the supported geometry instance rather than adding
unvalidated grammar syntax.

### Acceptance

- all generated grammars pass parser and semantic validation;
- no undefined conceptual operator remains;
- every grammar regenerates byte-identically;
- direct-service and grammar-generated topology hashes match;
- all grammars render under the GUI smoke test; and
- the family preview remains within the editor-safe geometry budget.

## Phase 6 — Three-View Silhouette Acceptance

### Tasks

1. Define canonical orthographic front, side, and top cameras in the explicit
   ProGen3D vehicle frame.
2. Render the high-resolution MCSMv2.0.1 reference bodies into binary masks.
3. Render direct native bodies using the same cameras and framing.
4. Render grammar-generated bodies using the same cameras and framing.
5. Normalize image size, crop, ground plane, and mask threshold.
6. Measure intersection-over-union independently for each view.
7. Produce side-by-side comparison images and machine-readable results.
8. Track direct-native versus source and grammar-native versus source results.

### Acceptance

For every variant:

- front-view silhouette IoU is at least `0.80`;
- side-view silhouette IoU is at least `0.80`;
- top-view silhouette IoU is at least `0.80`;
- grammar and direct-native masks are pixel-identical or explainably equivalent;
- no average score may hide a failed individual view; and
- failed MCP_OMv1 compatibility baselines remain separately visible and block
  cross-version certification when applicable.

## Phase 7 — Implement the MCSMv2.1 Geometry Gates

This phase begins only after MCSMv2.0.1 native parity is accepted.

### 7.1 Semantic and implicit convergence

1. Select the authoritative design-surface contract.
2. Sample semantic and implicit surfaces in the same reference frame.
3. Measure mean, RMS, p95, and maximum bidirectional distance.
4. Record station- and region-specific residuals.
5. Define explicit release tolerances.
6. Fail release when the authoritative convergence gate is exceeded.

### 7.2 Exclusive panel-domain partition

1. Define a common semantic surface domain.
2. Assign every valid domain location to exactly one exterior panel or one
   explicitly named opening.
3. Reject panel overlaps, uncovered regions, and invalid boundaries.
4. Derive panel adjacency from shared boundaries.
5. Validate closure seams independently from visual triangle masks.

### 7.3 Explicit glass apertures

1. Define windscreen, backlight, and side-glass apertures.
2. Define supporting greenhouse surfaces.
3. Derive glass geometry from the aperture/support relationship.
4. Validate containment, boundary closure, and body intersection.

### MCSMv2.1 acceptance

- semantic/implicit convergence is a release gate rather than a diagnostic;
- panel coverage is exclusive and gap-free over the declared domain;
- glass is aperture-derived; and
- all MCSMv2.0.1 native, grammar, visual, and compatibility gates still pass.

MCSMv2.1 must not absorb closure kinematics, suspension solving, structural
analysis, manufacturing, or physical validation without separate named plans
and acceptance evidence.

## Phase 8 — Release Evidence and Packaging

### Tasks

1. Create a separately versioned native integration release.
2. Do not overwrite or rename the historical MCSMv2.0.1 release.
3. Generate a native source-contract manifest.
4. Generate direct and grammar topology hashes.
5. Generate semantic, curve, surface, and engineering hashes.
6. Generate three-view silhouette evidence.
7. Capture all commands, exit codes, compiler versions, Python versions, and
   dependency versions.
8. Exclude `.pyc`, `__pycache__`, temporary models, GUI state, and transient
   logs from the release.
9. Run additive compatibility checks for MVPv2.5, MCP_OMv1, and current MCSMv2.
10. Produce a requirement-to-evidence audit before declaring completion.

### Required evidence artifacts

- source-contract verification JSON;
- native catalog manifest;
- native object-model test report;
- body-integrity and wheel-clearance report;
- engineering-relationship report;
- grammar validation report;
- GUI smoke-test report;
- three-view silhouette acceptance JSON;
- visual comparison images;
- compatibility report;
- release manifest with hashes; and
- known-limitations document.

## 6. Repository Work Map

### Reuse and extend

- `include/vehicle/mcsmv2/model/`
- `include/vehicle/mcsmv2/service/`
- `src/vehicle/mcsmv2/model/`
- `src/vehicle/mcsmv2/service/`
- `include/vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h`
- `include/geometry/service/GeneratedMeshProviderRegistry.h`
- `include/geometry/service/GeneratedMeshReferenceResolutionService.h`
- `include/geometry/service/ProceduralMeshRepository.h`
- `tools/generate_mcsmv2_grammars.cpp`
- `tools/export_mcsmv2_native_bodies.cpp`
- `tests/mcsmv2_*_harness.cpp`
- `tests/run_mcsmv2_*_checks.sh`
- `tests/mcsmv2_visual_iou.py`

### Add when required by the parity audit

- `include/vehicle/mcsmv2/model/VehicleReferenceFrameDefinition.h`
- `include/vehicle/mcsmv2/model/McsMv201SourceRelease.h`
- `include/vehicle/mcsmv2/model/McsMv201SourceArtifact.h`
- `include/vehicle/mcsmv2/model/SemanticImplicitAgreementReport.h`
- `include/vehicle/mcsmv2/model/VehiclePanelDomainPartition.h`
- `include/vehicle/mcsmv2/model/VehicleGlassApertureSystem.h`
- corresponding purpose-matched service headers and source files;
- `tests/mcsmv201_source_contract_harness.cpp`;
- `tests/mcsmv201_parity_harness.cpp`;
- `tests/run_mcsmv201_source_contract_checks.sh`; and
- `tests/run_mcsmv201_parity_checks.sh`.

Do not add a proposed file when the current implementation already has a class
that owns the same responsibility.

## 7. Acceptance Matrix

| Gate | Reference | Track | Aero | Crossover | Blocking |
|---|---:|---:|---:|---:|---:|
| Source files and hashes valid | required | required | required | required | yes |
| Python reference tests pass | required | required | required | required | yes |
| Native catalog deterministic | required | required | required | required | yes |
| Frame and ground placement valid | required | required | required | required | yes |
| Semantic stations non-crossing | required | required | required | required | yes |
| Fourteen curves present | required | required | required | required | yes |
| Body watertight and outward | required | required | required | required | yes |
| Wheel clearance passes | required | required | required | required | yes |
| Occupant containment passes | required | required | required | required | yes |
| Engineering graphs valid | required | required | required | required | yes |
| Direct and grammar hashes match | required | required | required | required | yes |
| Grammar GUI smoke passes | required | required | required | required | yes |
| Front silhouette IoU >= 0.80 | required | required | required | required | yes |
| Side silhouette IoU >= 0.80 | required | required | required | required | yes |
| Top silhouette IoU >= 0.80 | required | required | required | required | yes |
| Semantic/implicit convergence | MCSMv2.1 | MCSMv2.1 | MCSMv2.1 | MCSMv2.1 | 2.1 only |
| Exclusive panel partition | MCSMv2.1 | MCSMv2.1 | MCSMv2.1 | MCSMv2.1 | 2.1 only |
| Explicit glass apertures | MCSMv2.1 | MCSMv2.1 | MCSMv2.1 | MCSMv2.1 | 2.1 only |

## 8. Fastest Safe Critical Path

1. Restore the complete Python dependency set, especially `scikit-image`.
2. Reproduce the 25 reference tests in a clean environment.
3. Verify the immutable release manifest.
4. Diff MCSMv2.0.1 source data against the existing generated native catalog.
5. Add only missing catalog fields and provenance.
6. Run deterministic catalog generation twice.
7. Run semantic-section, surface, implicit-body, and engineering harnesses.
8. Regenerate executable grammars through the existing lowering service.
9. Run grammar and GUI smoke tests at editor-safe LOD.
10. Run all twelve three-view comparisons and require IoU >= `0.80` per view.
11. Run MVPv2.5 and MCP_OMv1 additive compatibility checks.
12. Produce the requirement-to-evidence audit.
13. Begin MCSMv2.1 convergence and panel work only after parity acceptance.

## 9. Completion Definition

MCSMv2.0.1 native integration is complete only when:

1. the historical source release remains byte-for-byte unchanged;
2. its source contract reproduces in a clean Python environment;
3. the generated native catalog is deterministic and provenance-complete;
4. all four variants are represented by the native object model;
5. source and native semantic outputs pass the defined parity tolerances;
6. native bodies pass topology, package, occupant, and wheel-clearance gates;
7. engineering models pass their relationship and containment gates;
8. current-parser grammars regenerate, resolve, and render successfully;
9. direct-service and grammar-generated topology hashes match;
10. every front, side, and top silhouette score is at least `0.80`;
11. compatibility failures remain explicit rather than being averaged away;
12. release artifacts contain no bytecode or transient files;
13. evidence identifies exact source and generated hashes; and
14. the final requirement-to-evidence audit has no unsupported completion claim.

MCSMv2.1 is complete only after the additional convergence, exclusive panel
partition, and explicit glass-aperture gates pass for all four variants without
regressing the MCSMv2.0.1 integration contract.
