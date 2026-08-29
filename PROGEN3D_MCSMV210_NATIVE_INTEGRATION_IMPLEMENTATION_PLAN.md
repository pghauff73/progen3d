# ProGen3D MCSMv2.1.0 Native Integration Implementation Plan

**Date:** Monday, August 24, 2026
**Status:** Implemented; cross-version release certification remains blocked by the inherited MCP_OMv1 side-view baseline
**Source release:** `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_1_0_Modern_Car_Complete/`
**Native integration target:** `examples/MVPv2_5_Red_AWD_Hatchback/MCSMv2_Modern_Car_Complete/`

## 1. Purpose

Integrate the MCSMv2.1 registered semantic surface, bidirectional
semantic/implicit correspondence, periodic surface domains, exclusive final-body
ownership, and aperture-derived static glass into the native ProGen3D MCSMv2
object model.

The implementation must:

1. preserve MCSMv2.0.1 package, parameter, frame, engineering, and compatibility
   behavior;
2. make MCSMv2.1 source evidence reproducible and fail closed when its source
   contract changes;
3. give the registered semantic surface one explicit native owner;
4. represent periodic UV domains as model objects rather than triangle-mask
   conventions;
5. evaluate semantic/implicit agreement as a native release gate;
6. assign every final body face exactly one named owner while separately proving
   declared-domain coverage;
7. derive panel adjacency from shared domain boundaries;
8. construct static glass from aperture support surfaces and validate its
   relationship to the body;
9. lower all four variants to current executable ProGen3D grammar;
10. preserve deterministic native and grammar-generated geometry; and
11. rerun native, grammar, GUI, visual, MVPv2.5, MCP_OMv1, and MVP2.6
    compatibility gates before claiming completion.

MCSMv2.1 remains V2 concept geometry. Closure motion, helical glass motion,
exact suspension kinematics, physical panel construction, manufacturing,
structural analysis, CFD, crash, homologation, and production Class-A approval
remain outside this plan.

## 2. Current Baseline

### 2.1 Accepted MCSMv2.0.1 native implementation

The existing native implementation already owns:

- the source frame and source-to-ProGen3D transformation;
- four typed vehicle variants and their 74-node dependency graphs;
- ordered semantic section fields and character curves;
- calibrated implicit body fields with no post-mesh affine correction;
- deterministic body and semantic-surface generation;
- panel, BIW, occupant, functional-package, and wheel-envelope models;
- current-parser executable grammars; and
- native, GUI, visual, MVPv2.5, MCP_OMv1, and MVP2.6 compatibility evidence.

MCSMv2.1 extends this implementation. It does not replace or fork the existing
package and body-generation owners.

### 2.2 MCSMv2.1 source release

The source package provides:

- an independently tessellated no-wheelhouse implicit scaffold;
- a registered semantic surface with persistent periodic UV coordinates;
- bidirectional distance and normal correspondence evidence;
- fifteen panel-domain definitions;
- nine static glazing-aperture definitions;
- exclusive final-body face ownership with fixed-body fallbacks;
- aperture support and glass meshes; and
- V2 source validation for reference, track, aero, and crossover variants.

The supplied `.p3d` file is target syntax and is not current-parser compatible.
The native integration must generate executable grammars instead of teaching the
parser release-specific conceptual operators.

## 3. Architectural Decisions

### 3.1 Canonical surface ownership

The native model has three named surfaces with different responsibilities:

| Surface | Native owner | Responsibility |
|---|---|---|
| Raw semantic surface | `VehicleSemanticSurface` | Ordered section topology and persistent parameterization basis |
| Registered semantic surface | `RegisteredVehicleSemanticSurface` | Authoritative semantic-domain and correspondence surface |
| Final body surface | `GeneratedModernCarSemanticBody` | Rendered, exported, wheelhouse-resolved body geometry |

No service may silently substitute one surface for another.

### 3.2 Periodic surface-domain model

The common semantic domain is `[0,1] x S1`:

- `u=0` is rear and `u=1` is front;
- `v=0` and `v=1` are the same roof-centre seam;
- every domain boundary is represented by one or more closed polygon loops;
- aperture domains take ownership before panel domains; and
- uncovered declared exterior-domain samples fail the domain-coverage gate.

`fixed_body` and `fixed_body_internal` remain explicit final-body ownership
classes, but they must not be used to disguise gaps in the declared exterior UV
domain.

### 3.3 Registration and correspondence

Registration retains the raw semantic mesh topology and UV coordinates while
moving vertices to the independently generated scaffold. Native acceptance
requires:

- deterministic nearest-triangle projection;
- terminal-ring protection against longitudinal folding;
- finite, manifold registered geometry;
- bidirectional mean, RMS, p95, and maximum distance;
- bidirectional p95 normal-angle evaluation;
- global and rear/cabin/front longitudinal-third residual records; and
- explicit source tolerances that fail generation when exceeded.

Native normal agreement uses angle-weighted vertex normals interpolated at each
closest point. This preserves a mesh-resolution-independent smooth-surface
comparison while retaining nearest-triangle distance and deterministic closest
point checksums. Static concept glass additionally enforces a `0.050 m` maximum
body separation ceiling so aperture offsets cannot drift away from their support
surface.

The source release publishes global correspondence tolerances only. Native
regional reports therefore remain mandatory nonempty evidence partitions, while
certification continues to use the published global thresholds rather than
inventing unapproved per-region limits.

### 3.4 Glass scope

MCSMv2.1 glass is static aperture-derived concept geometry. Each glass object
must aggregate:

- its aperture domain;
- its support-surface face set;
- its outward offset distance;
- its generated mesh; and
- containment, finite geometry, nonempty area, and body-separation evidence.

It must not expose moving-window or closure-kinematic claims.

## 4. Native Object Model

### 4.1 Model classes

| Class | Responsibility |
|---|---|
| `VehicleSurfaceCoordinate` | One normalized periodic `(u,v)` coordinate |
| `VehicleSurfaceDomainLoop` | One closed polygonal loop in the periodic surface domain |
| `VehicleSurfaceDomain` | Named panel or aperture domain and its semantic relationships |
| `VehicleSurfaceDomainCatalog` | The complete declared panel/aperture domain set for one variant |
| `RegisteredVehicleSemanticSurface` | Raw, scaffold, and registered mesh relationship with persistent UV coordinates |
| `SurfaceCorrespondenceDirectionReport` | One direction of distance and normal residual evidence |
| `SemanticImplicitCorrespondenceReport` | Bidirectional agreement and tolerance result |
| `VehicleSurfaceOwnership` | One owner label for each final-body face |
| `VehicleSurfaceDomainPartition` | Regions, coverage, overlaps, adjacency, and exclusive ownership |
| `VehicleGlassAperture` | Aperture domain, support mesh, glass mesh, and validation evidence |
| `McsMv21SemanticGeometry` | Aggregate root for registration, partition, glass, and V2 validation |

### 4.2 Service classes

| Class | Responsibility |
|---|---|
| `VehicleSemanticSurfaceRegistrationService` | Register semantic vertices to the scaffold while preserving topology and UV |
| `SemanticImplicitAgreementEvaluationService` | Produce global and region-specific bidirectional residual reports |
| `VehicleSurfaceDomainPartitionService` | Map final-body faces into periodic UV, assign owners, prove coverage, derive adjacency |
| `VehicleGlassApertureConstructionService` | Extract supports, offset glass, and validate body relationships |
| `McsMv21SemanticGeometryGenerationService` | Coordinate the complete fail-closed MCSMv2.1 realization |

## 5. Implementation Phases

## Phase 0 — Repair and Lock the Source Contract

1. Correct the source package metadata to version `2.1.0`.
2. Correct the changelog panel-domain count to fifteen.
3. Exclude interpreter bytecode from future release manifests.
4. Make the release builder generate every distributed report and validation
   artifact or classify it as external evidence.
5. Add an MCSMv2.1 source-contract verifier.
6. Verify all manifest entries, JSON schemas, expected counts, and source hashes.
7. Run all twelve Python tests without reading or writing stored bytecode.
8. Regenerate to a temporary directory and compare deterministic core artifacts.

### Acceptance

- source metadata is internally consistent;
- no source-contract mismatch is tolerated;
- all twelve tests pass;
- all declared manifest artifacts match; and
- the regeneration comparison clearly distinguishes deterministic core
  artifacts from external verification records.

## Phase 1 — Extend the Generated Native Catalog

1. Extend `generate_native_catalog.py` with an explicit MCSMv2.1 source input.
2. Import source release version, schema, hashes, and assurance.
3. Import all panel and aperture loops with parent relationships.
4. Import correspondence tolerances and accepted source metrics.
5. Import the registered surface topology and source validation facts.
6. Generate deterministic MCSMv2.1 records alongside the existing 2.0.1
   records.
7. Extend `native_catalog_manifest.json` to record both source releases.

### Acceptance

- four variants each own fifteen panels and nine apertures;
- every imported fact identifies its source artifact;
- two generations are byte-identical; and
- MCSMv2.0.1 records remain byte-for-byte semantically unchanged.

## Phase 2 — Implement Registration and Correspondence

1. Generate a capped semantic surface with persistent periodic UV coordinates.
2. Generate the no-wheelhouse scaffold through the existing calibrated field
   pipeline.
3. Register semantic vertices using deterministic closest-triangle projection.
4. protect front/rear terminal rings and cap centres;
5. validate topology and orientation;
6. measure bidirectional global residuals;
7. measure rear, cabin, and front longitudinal-region residuals; and
8. fail generation when any source tolerance is exceeded.

### Acceptance

- registered topology is deterministic and manifold;
- UV cardinality equals vertex cardinality;
- global p95 distance is at most `0.008 m`;
- global maximum distance is at most `0.120 m`;
- global p95 normal angle is at most `20 deg`; and
- region evidence is present for every variant.

## Phase 3 — Implement Exclusive Surface Domains

1. Construct periodic panel and aperture domain objects from the catalog.
2. validate closed loops and normalized coordinates;
3. reject panel-panel and aperture-aperture overlap;
4. compute declared exterior-domain coverage and reject gaps;
5. map final-body face centres to registered-surface triangles with
   barycentric UV interpolation;
6. assign aperture, panel, fixed-body, and internal-body ownership;
7. require exactly one owner for every final-body face; and
8. derive adjacency from shared UV boundary segments.

### Acceptance

- all loops are closed;
- declared-domain overlap is zero;
- declared exterior-domain coverage is gap-free under the declared coverage
  policy;
- every final-body face has one owner;
- no adjacency edge is hand-authored; and
- partition hashes are deterministic.

## Phase 4 — Implement Static Aperture-Derived Glass

1. Extract aperture supports from registered semantic-surface faces.
2. Generate glass by the catalogued outward normal offset.
3. validate nonempty area, finite vertices, and consistent normals.
4. validate aperture containment and support correspondence.
5. validate minimum glass/body separation and reject body penetration.
6. preserve static-only V2 assurance wording.

### Acceptance

- all nine glass apertures generate for all four variants;
- every glass mesh has a matching support mesh;
- containment and body-separation gates pass; and
- no V3 moving-glass claim is introduced.

## Phase 5 — Executable Grammar and Editor Integration

1. Extend `ModernCarSemanticGrammarLoweringService` with MCSMv2.1 family and
   variant preview grammars.
2. Reuse current `Object`, `GeneratedMesh`, transform, instance, and material
   syntax.
3. Register deterministic body, panel-region, and glass mesh keys with
   `McsMv2GeneratedMeshProvider`.
4. add four variant previews and one family preview;
5. keep generated mesh sizes below the editor draw-list index limit; and
6. run grammar regeneration and headless GUI smoke tests.

### Acceptance

- every grammar parses with the current parser;
- direct-service and grammar mesh hashes match;
- GUI regeneration succeeds without assertions; and
- the source target grammar remains documentation, not executable authority.

## Phase 6 — Visual, Compatibility, and Release Audit

1. Produce front, side, and top binary silhouettes for each variant.
2. Compare native MCSMv2.1 views to the accepted 2.1 source models.
3. Require IoU at least `0.80` for all twelve views.
4. Rerun every MCSMv2.0.1 native and grammar gate.
5. Rerun MVPv2.5, MCP_OMv1, and MVP2.6 integration gates.
6. Generate machine-readable and human-readable requirement evidence.
7. Record exact source, generator, catalog, grammar, body, partition, glass,
   visual, and report hashes.

### Completion definition

MCSMv2.1 native integration is complete only when:

1. its repaired source contract validates;
2. the generated catalog is deterministic;
3. all four variants generate registered semantic geometry;
4. correspondence is a native fail-closed gate;
5. domain coverage, overlap, ownership, and adjacency pass;
6. all nine static glass apertures pass;
7. current-parser grammars and GUI regeneration pass;
8. all twelve silhouette scores are at least `0.80`;
9. all MCSMv2.0.1 and downstream compatibility gates pass; and
10. the requirement-to-evidence audit contains no missing or indirect item.

### Audit result

Items 1 through 8 and 10 are satisfied by direct source, native, grammar, GUI,
and twelve-view evidence. The MCSMv2.0.1, MVPv2.5, MCP_OMv1 native/grammar,
and MVP2.6 compatibility gates pass. Item 9 remains partially blocked only by
the pre-existing MCP_OMv1 visual baseline for `track:side`, `aero:side`, and
`crossover:side`; the independently rerun scores are `0.778319`, `0.773697`,
and `0.658835` against the inherited `0.80` threshold. MCSMv2.1 does not alter
MCP_OMv1 geometry or its reference images, and its own twelve silhouettes all
pass with a minimum IoU of `0.95026155`.
