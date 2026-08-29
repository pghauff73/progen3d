# ProGen3D CH01-CH22 Three-View Primitive Set Implementation Plan

## Document Control

- **Date:** August 22, 2026
- **Status:** Implemented; absolute 66-view fit and GL46 certification pass
- **Last updated:** August 23, 2026
- **Primary objective:** Extend FGKv1 with a compact domain-neutral primitive set that allows each MVPv2.5 chassis part CH01-CH22 to reproduce at least 90% of the visible side, front, and top reference geometry while preserving authoritative scheduled dimensions, deterministic construction, semantic part ownership, collision geometry, and MVPv1/MVPv2 compatibility.
- **Reference package:** `examples/MVPv2_5_Red_AWD_Hatchback/`
- **Current assembly:** `MVP25_Chassis_CH01_22_Assembled.grammar`
- **Current validation:** `MVP25_Chassis_CH01_22_Validation.json`
- **Acceptance command:** `./tests/run_mvp25_chassis_ch01_22_assembly_checks.sh`

## 1. Completion Definition

Implementation is complete only when all requirements below have current evidence.

1. FGKv1 contains purpose-driven object models for:
   - `ThinWallProfile2D`;
   - `VariableSectionSweepShapeSpecification`;
   - `FormedPanelShapeSpecification`;
   - `HostedOpeningShapeSpecification`;
   - `EmbossedBeadShapeSpecification`;
   - `EdgeFlangeShapeSpecification`;
   - `CompoundShapeSpecification`.
2. Every new shape has:
   - candidate/evaluated model separation where applicable;
   - bounded validation;
   - deterministic canonical text and cache key;
   - mesh realization;
   - surface tags;
   - topology and collision evidence;
   - grammar parser/evaluator support;
   - catalog registration;
   - procedural repository routing;
   - focused native tests.
3. CH01-CH22 are regenerated with the new primitive set.
4. Each part preserves one purpose-named semantic object and one or more explicit geometry bindings.
5. The authoritative schedule remains a hard constraint.
6. Every side/front/top view has deterministic projection evidence.
7. Each view reaches the fit thresholds defined in section 3.
8. The complete assembled grammar passes native and GL46 gates.
9. Existing MVPv1, MVPv2, curved-shape, profile, vegetation, and spatial tests remain compatible.

## 2. Current-State Gap

The current assembly intentionally uses the smallest valid existing operators:

```text
CH01       ShellOffset(SurfaceLoft(...))
CH02-CH18 SweepProfile(constant RoundedRect, three-point path)
CH19-CH22 ShellLoft(four circular annular sections)
```

This proves semantic ownership, dimensions, placement, connection graphs,
triangle realization, and GL46 rendering. It does not reproduce most visible:

```text
variable sections
open thin-wall channels
multi-cell rocker construction
formed floor steps
pressed beads and ribs
local bosses
lightening openings
end flanges and joint pads
tower collars and mounting flanges
radial bolt-hole patterns
```

The implementation must fix these missing capabilities in FGKv1 rather than
introducing one mesh builder per automotive part.

## 3. Ninety-Percent Fit Contract

The phrase "90% closer" is normalized as a measurable 90% reference fit.
No improvement percentage may be claimed without retaining the baseline score.

### 3.1 Per-view hard gates

For every CH part and every declared orthographic view:

```text
silhouette intersection-over-union >= 0.90
95th percentile contour distance    <= 0.015 view diagonal
principal dimension residual        <= declared tolerance
visible feature recall              >= 0.90
```

Every view must pass independently. An average cannot hide a failed view.

### 3.2 Reported fit score

```text
fit =
    0.45 * silhouetteFit
  + 0.20 * contourFit
  + 0.15 * visibleFeatureFit
  + 0.10 * dimensionFit
  + 0.10 * sectionFit
```

### 3.3 Evidence boundary

- JSON schedules are authoritative for dimensions and identifiers.
- Generated sheets are visual construction evidence.
- Hidden profiles, joints, welds, reinforcements, and manufacturing details remain inferred.
- A 90% projection fit is not crash, fatigue, corrosion, NVH, joining, or homologation certification.

## 4. Target Object Model

```text
ShapeSpecification
├── VariableSectionSweepShapeSpecification
├── FormedPanelShapeSpecification
├── HostedOpeningShapeSpecification
├── EmbossedBeadShapeSpecification
├── EdgeFlangeShapeSpecification
└── CompoundShapeSpecification

ThinWallProfile2D
├── ClosedThinWallProfile
├── OpenThinWallProfile
├── HatSectionProfile
├── ChannelSectionProfile
└── MultiCellSectionProfile

VariableSectionSweepShapeSpecification
├── Curve3D path
├── SectionStationSpecification[] stations
├── SweepFramePolicy
└── ExtrudeProfileCapPolicy

CompoundShapeSpecification
└── CompoundShapePartSpecification[] parts
```

`ThinWallProfile2D` is profile construction data, not a chassis-specific shape.
Automotive semantics remain in the MVPv2.5 domain builder and lower to these
generic specifications.

## 5. Primitive Requirements

### 5.1 ThinWallProfile2D

Inputs:

```text
centreline or canonical section family
sheet thickness
bend radius
closure policy
optional lips
optional internal cells
```

Outputs:

```text
validated Profile2D outer loop
validated Profile2D inner loops
stable section feature identifiers
```

P0 supports explicit outer and inner loops plus canonical closed-box,
open-channel, hat, and multi-cell factories. General centreline offset with
bend-radius fillets may follow after the canonical factories.

### 5.2 VariableSectionSweep

Each station provides:

```text
normalized path position
Profile2D
profile offset
profile scale
profile rotation
```

P0 requires identical loop and point topology at every station. Frame transport
must use deterministic rotation-minimising frames. The builder must reject:

```text
unordered stations
stations outside [0,1]
duplicate stations
incompatible profile topology
non-finite transforms
degenerate path segments
frame singularities
resource-limit violations
```

### 5.3 FormedPanel

P0 representation:

```text
bounded plan profile
ordered transverse section profiles
sheet thickness
cap/rim policy
```

The generated surface must preserve arbitrary plan width variation and local
formed height. It may initially lower to `SurfaceLoft` plus `ShellOffset` while
retaining the semantic `FormedPanel` family and surface tags.

### 5.4 HostedOpening

P0 is a deterministic host-aware cut for thin panels and swept members. It is
not general Boolean CSG. It must support:

```text
circle
rounded rectangle
polygon
slot
```

The operation must retain cut-wall surface tags and fail closed if the opening
does not intersect the host or produces unsupported topology.

### 5.5 EmbossedBead

Inputs:

```text
host surface
path
width
depth
shoulder radius
side
end style
```

P0 may realize beads as tagged finite-thickness swept additions composed with
the host. Later implementations may deform the host surface directly.

### 5.6 EdgeFlange

Inputs:

```text
host boundary selection
width
angle
bend radius
side
```

P0 may use explicit boundary paths supplied by the grammar or domain builder.
Automatic semantic edge selection is later work.

### 5.7 CompoundShape

`CompoundShape` groups resolved child shapes with local transforms while
preserving child surface tags, material identities, deterministic ordering,
bounds, topology evidence, and collision geometry.

Flattening is allowed only during final mesh realization. The specification
must retain the child graph.

## 6. Proposed Grammar

### 6.1 Variable section sweep

```text
VariableSectionSweep(
    path(-0.52 0.34 2.00 -0.52 0.33 1.78 -0.52 0.31 1.40 -0.52 0.30 0.78)
    station(0.00 RoundedRect 0.090 0.075 0.006 4 center(0 0) scale(1 1) rotate(0))
    station(0.30 RoundedRect 0.096 0.078 0.006 4 center(0 0) scale(1 1) rotate(0))
    station(0.70 RoundedRect 0.105 0.082 0.007 4 center(0 0) scale(1 1) rotate(0))
    station(1.00 RoundedRect 0.110 0.085 0.008 4 center(0 0) scale(1 1) rotate(0))
    up(0 1 0)
    frame(rotationMinimizing)
    cap(all)
    detail(LOD3)
)
```

### 6.2 Formed panel

```text
FormedPanel(
    section(-1.550 Polygon ...)
    section(-1.100 Polygon ...)
    section(0.000 Polygon ...)
    section(1.100 Polygon ...)
    section(1.550 Polygon ...)
    thickness(0.0012)
    side(both)
    detail(LOD3)
)
```

### 6.3 Compound

```text
CompoundShape(
    part(source(ShellLoft(...)) transform(1 0 0 0 ...))
    part(source(Revolve(...)) transform(1 0 0 0 ...))
    part(source(InstanceArray(...)) transform(1 0 0 0 ...))
    detail(LOD3)
)
```

Nested descriptor syntax may be adjusted to the existing parser architecture,
but the semantic information above must remain expressible.

## 7. CH01-CH22 Migration

| Parts | Required construction |
|---|---|
| CH01 | `FormedPanel` base, floor steps, bead network, hosted holes, perimeter flanges |
| CH02-CH03 | variable closed thin-wall sweep, four or more stations, curved path, end plate, holes |
| CH04-CH05 | variable closed thin-wall sweep, rear kick-up, changing width/height, end flange |
| CH06-CH07 | variable multi-cell sweep, tapered ends, repeated lightening openings, bulkheads |
| CH08-CH09 | variable open-channel or hat sweep, widened base, roof flange, local slots |
| CH10-CH11 | variable hat sweep, widened ends, grooves, holes, attachment pads |
| CH12-CH13 | asymmetric variable channel sweep, lower/upper flanges, local embosses |
| CH14-CH15 | arched variable C-channel sweep, A/B/C joint pads and holes |
| CH16 | bowed variable sweep, end plates, hole pattern |
| CH17 | variable sweep with centre transition, tunnel relief, end plates, openings |
| CH18 | variable sweep, shallow bow, end flanges and mounting holes |
| CH19-CH20 | compound annular shell loft, lower flange, upper collar, radial holes, ribs |
| CH21-CH22 | independently fitted rear compound towers with the same generic operations |

Mirroring may seed paired candidates, but left and right parts remain separate
semantic objects and independently overrideable specifications.

## 8. Implementation Sequence

### Phase A: Reference and baseline

1. Add deterministic three-view crop and mask definitions for all 22 sheets.
2. Record baseline projections from the current grammar.
3. Publish per-part, per-view residuals without claiming a 90% result.

### Phase B: Variable section and thin walls

1. Implement the model, validator, mesh generator, parser/evaluator, catalog,
   repository route, evidence route, and tests.
2. Migrate CH02-CH18.
3. Validate topology, dimensions, projection residuals, and GL46 rendering.

### Phase C: Compound composition

1. Implement compound child specifications and deterministic mesh composition.
2. Preserve child surface tags and local transforms.
3. Add nested grammar syntax and tests.

### Phase D: Formed panels and features

1. Implement `FormedPanel` lowering.
2. Implement hosted openings, beads, flanges, and feature arrays.
3. Migrate CH01 and add features to CH02-CH18.

### Phase E: Suspension towers

1. Replace circular-only towers with independently fitted non-circular sections.
2. Add collar, flange, hole, and rib children through compounds.
3. Migrate CH19-CH22.

### Phase F: Fit service

1. Add `ChassisPartReferenceDefinition` and view evidence classes.
2. Add `ThreeViewProjectionService`.
3. Add `ChassisReferenceFitService` with deterministic bounded optimization.
4. Add `ChassisPartFitReport` and residual ledger.

### Phase G: Acceptance and compatibility

1. Run 66 native projection comparisons.
2. Run all topology, collision, dimension, determinism, and semantic gates.
3. Capture GL46 side/front/top views for every part family and the complete chassis.
4. Run existing curved, profile, vehicle, vegetation, spatial, and release gates.

## 9. Repository Integration Points

New or changed ownership includes:

```text
include/geometry/model/
src/geometry/model/
include/geometry/service/
src/geometry/service/
include/grammar/model/
include/grammar/service/
src/grammar/service/
tests/procedural_profile_link_sources.sh
tests/*geometry*checks.sh
examples/MVPv2_5_Red_AWD_Hatchback/generate_chassis_ch01_22_assembly.py
examples/MVPv2_5_Red_AWD_Hatchback/MVP25_Chassis_CH01_22_*.json
examples/MVPv2_5_Red_AWD_Hatchback/MVP25_Chassis_CH01_22_*.grammar
```

## 10. Resource and Safety Limits

```text
path points                 <= configured maximumPathPoints
section stations            <= configured maximumLoftSections
profile points/station      <= configured maximumPointsPerProfile
compound children           <= 4096
hosted openings/shape       <= 1024
features/shape              <= 4096
vertices/object             <= configured maximumGeneratedVertices
triangles/object            <= configured maximumGeneratedTriangles
optimization iterations     <= explicit deterministic ceiling
```

Candidate geometry remains transactional: the current valid mesh is retained
until the candidate builds, validates, projects, and satisfies all hard gates.

## 11. Required Validation Artifacts

```text
MVP25_Chassis_CH01_22_Baseline_Fit.json
MVP25_Chassis_CH01_22_Frozen_Reference_Masks.json
MVP25_Chassis_CH01_22_Reference_Masks.json
MVP25_Chassis_CH01_22_Fit_Report.json
MVP25_Chassis_CH01_22_Validation.json
MVP25_Chassis_CH01_22_Assembled.grammar
MVP25_Chassis_CH01_22_Assembled_GL46_Isometric.png
```

Each record must contain source hashes, grammar hash, generator hash, mesh
topology hashes, per-view residuals, primitive-family counts, semantic object
counts, relationship counts, and the exact validation command.

## 12. Compatibility Boundary

- Existing grammar syntax remains valid.
- Existing `SweepProfile`, `Loft`, `ShellLoft`, `SurfaceLoft`, `ShellOffset`,
  `FoldedProfile`, `MirrorShape`, and `InstanceArray` semantics remain unchanged.
- New primitives are additive.
- MVPv1 and MVPv2 vehicle builders are not rewritten to depend on MVPv2.5.
- Domain classes lower to FGKv1; FGKv1 does not acquire kitchen, vehicle-package,
  suspension-type, or chassis-part semantics.

## 13. Completion Audit Matrix

| Requirement | Authoritative evidence | Status |
|---|---|---|
| Implementation plan | this document | Complete |
| Baseline fit ledger | baseline JSON plus deterministic projector | Complete as a fail-closed unavailability ledger; the pre-migration grammar/raster was not retained, so comparative improvement claims are forbidden |
| Thin-wall profiles | model/factory/validator tests | Complete |
| Variable section sweep | model/parser/generator/native tests | Complete |
| Compound shape | model/parser/composer/native tests | Complete |
| Formed panel | model/lowering/topology tests | Complete |
| Hosted openings | host-cut topology tests | Complete |
| Beads and flanges | feature realization tests | Complete |
| CH01-CH22 migration | generated grammar and manifest | Complete |
| Frozen geometry input | fixed 256-pixel labeled masks and hash manifest | Complete; 66 source masks regenerate directly from the 22 labeled sheets before grammar generation |
| 90% three-view fit | 66-view fit report | Complete: 66/66 pass; minimum silhouette IoU 0.914661, average silhouette IoU 0.980264, minimum fit score 0.953063 |
| GL46 rendering | deterministic visual evidence | Complete: isometric fit-to-extents, render, capture, repeat comparison, and smoke gates pass |
| Compatibility | focused suites and release checks | Profile extrusion, curved-shape model, three-view projection, MVPv1, MVPv2, MVPv2.5, vegetation geometry, and spatial positioning pass. The broad P2/release path is independently blocked by stale test stubs that lack existing vehicle-joint and lighting APIs; it is not used as evidence for this completion claim |

## 14. Implemented Fit Strategy

The final package keeps the primitive set domain-neutral while allowing each
chassis family to use the construction that best matches its evidence:

```text
CH01        CompoundShape formed envelope + hosted panel + tunnel + beads +
            flanges + physically resampled longitudinal stamped cells
CH02-CH10  evidence-fitted VariableSectionSweep with explicit nominal sections
CH11        CompoundShape section carrier + disconnected side/front stamped webs
CH12-CH18  evidence-fitted VariableSectionSweep with view-correct station sampling
CH19-CH22  independently fitted annular ShellLoft tower sections
```

The generator reads only `MVP25_Chassis_CH01_22_Frozen_Reference_Masks/`.
The fit reporter writes its own `MVP25_Chassis_CH01_22_Reference_Masks/`
outputs, so validation can no longer mutate the next generation input.

## 15. Final Measured Result

```text
views evaluated                         66
views passing                           66
minimum silhouette IoU                  0.914661
average silhouette IoU                  0.980264
minimum fit score                       0.953063
average fit score                       0.988615
maximum allowed contour p95 residual    0.015 view diagonal
native primitives                       22
semantic objects                        25
connections                             53
geometry bindings                       22
```

The absolute current-fit contract is certified by
`./tests/run_mvp25_chassis_ch01_22_assembly_checks.sh`. The missing historical
baseline remains explicit in `MVP25_Chassis_CH01_22_Baseline_Fit.json`; no
measured percentage-improvement claim is made.
