# ProGen3D Building Object Image Matching V1 Implementation Plan

## Document Control

- **Date:** August 27, 2026
- **Program name:** `Building Object Image Matching V1`
- **Canonical building package:** `SMBv3 SimpleModernBuilding Version 3`
- **Reference source:** six-view ImageGen object cards
- **Render source:** object-local ProGen3D camera views
- **Comparison views:** front, back, left, right, top, and bottom
- **Minimum acceptance:** every required view scores at least 80%
- **Construction target:** every required view scores at least 84%
- **Current scope:** 108 renderable SMBv3 taxonomy objects

## 1. Objective

Create a repeatable image-matching program in which each building object is
rendered independently, compared with its corresponding ImageGen reference in
six object-local camera views, and improved through grammar changes that are
owned by an explicit visual object model.

The program must improve real agreement rather than optimize one blended score.
Camera errors, missing references, invisible views, wrong silhouettes, missing
parts, material differences, and connection mistakes remain separate facts.
No strong metric may conceal a failed independent test.

## 2. Current Evidence Baseline

The current deterministic freeform six-view baseline records:

- 18 expected ImageGen atlases;
- 18 available atlases;
- 108 renderable taxonomy objects;
- 108 objects with available ImageGen references;
- 648 front/back/left/right/top/bottom comparison slots;
- 600 measurable comparisons;
- 48 isolated observability failures;
- 0 objects passing every required view at 80%.

The highest current model-family means are `couch` at 66.84%, `canopy` at
63.62%, `site_assembly` at 63.04%, `toilet` at 61.37%, `interior_wall` at
59.91%, and `downpipe` at 59.70%. The weakest families include `stair` at
27.79%, `chair` at 33.61%, `privacy_screen` at 34.87%, and `television` at
37.24%.

This baseline is diagnostic evidence, not a certification result. Reference
coverage is complete; the next six-view priority is to correct view
observability and improve grammar geometry without averaging away failures.

An additive constrained front/right/top program now records 108 of 108 objects
passing 324 comparisons with a 96.63% minimum. It preserves deterministic
ProGen3D silhouettes and transfers ImageGen chroma inside them, so it is useful
construction evidence but not a substitute for the freeform six-view gate.

## 3. Independent Test Design

Each test produces one `BuildingImageMatchingObservation` for one object, one
camera view, and one test kind. An observation records its state, optional
numeric score, and evidence. Tests do not write another test's result.

### 3.1 Hard-Gate Tests

| Test | Purpose | Failure Meaning | State |
|---|---|---|---|
| Source provenance | Verify exact taxonomy, catalog, reference, grammar, and camera hashes. | The comparison cannot be reproduced. | Implemented |
| Camera calibration | Verify orthographic projection, local axes, framing, background, and scale policy. | Geometry and reference are not being viewed under the same contract. | Implemented |
| Object isolation | Verify one taxonomy object, one reference crop, and one object-local render set. | The test contains another object, duplicate reference, or ambiguous ownership. | Implemented |
| View observability | Classify a view as measurable, accepted-degenerate, or failed. | A transparent or edge-on object was silently discarded or an expected view is absent. | Implemented |
| Landmark topology | Verify named part counts, adjacency, hierarchy, and relative placement. | The render may have a similar mass but depicts the wrong object structure. | Planned |
| Cross-view consistency | Reconstruct dimensions and part counts across all six views. | The ImageGen card or grammar renders incompatible objects between views. | Planned |
| Connection-point alignment | Compare supports, mounts, openings, services, and host contacts. | The isolated shape cannot connect correctly in the building. | Planned |
| Extent agreement | Compare visual, collision, access, motion, growth, and aggregate extents. | The visible object does not agree with its spatial contract. | Planned |
| Human semantic review | Confirm identity, material intent, and acceptable visual equivalence. | Deterministic similarity is high but the object is semantically wrong. | Planned |

### 3.2 Independent Numeric Tests

| Test | Measurement | Primary Grammar Signal | State |
|---|---|---|---|
| Silhouette overlap | Intersection-over-union after object-local normalization. | Primary volumes, profile, protrusions, cut-outs. | Implemented |
| Edge alignment | Tolerant F1 score over extracted foreground boundaries. | Part boundaries, frame lines, tread edges, seams. | Implemented |
| Projection distribution | Row and column occupancy distributions. | Where mass is located within the object extent. | Implemented |
| Aspect ratio | Width-to-height agreement per view. | Authoritative dimensions and local orientation. | Implemented |
| Foreground occupancy | Normalized filled-area agreement. | Thickness, voids, openings, foliage density. | Implemented |
| Component count | Connected visible component agreement. | Part counts, gaps, disconnected supports, repeated elements. | Implemented |
| Symmetry | Horizontal and vertical reflection agreement. | Balanced layouts and intentional asymmetry. | Implemented |
| Repetition spacing | Count, pitch, run length, and terminal spacing. | Slats, pickets, treads, chairs, mullions, rods. | Planned |
| Transparency boundary | Boundary agreement separate from transparent interior. | Glazing, shower glass, pool water, translucent curtains. | Planned |
| Material region | Semantic region segmentation independent of lighting. | Wood, metal, glass, stone, fabric, water, foliage. | Planned |
| Color palette | Illumination-normalized region color comparison. | Material color parameters rather than geometry. | Planned |
| Texture frequency | Direction, module, scale, and spatial-frequency comparison. | Paving joints, timber grain, fabric pleats, foliage scale. | Planned |

### 3.3 Independence Rules

1. A missing observation fails only its own object view; it does not abort the
   remaining 647 comparison slots.
2. Hard gates never contribute a weighted score and cannot be averaged away.
3. Every implemented weighted test must meet its own minimum, initially 60%.
4. The weighted composite must also meet the camera-view minimum of 80%.
5. An object passes only when every required view passes.
6. An optional-degenerate view passes only when both reference and render are
   unobservable under the same declared view contract.
7. A required unobservable view fails even if the other five views are perfect.
8. Duplicate object IDs, reference crops, view contracts, test specifications,
   or observations fail closed.
9. Planned tests have zero score weight until an implementation and synthetic
   validation fixture exist.
10. Whenever a planned weighted test becomes active, the category profile is
    rebalanced to sum to exactly one.

## 4. Acceptance Formula

For each measurable required view:

```text
independent_test_pass = every implemented test meets its own threshold

view_score =
    sum(test_score * category_test_weight)
    / sum(category_test_weight)

view_pass =
    provenance_pass
    and camera_calibration_pass
    and object_isolation_pass
    and observability_pass
    and independent_test_pass
    and view_score >= 80

object_pass = every required view_pass
```

The construction target is 84%, not 80%, so minor renderer, anti-aliasing, or
segmentation differences do not immediately regress a previously accepted
object below the release gate.

Certification additionally requires the complete 18-atlas reference set, all
108 renderable objects, deterministic rerun equality, active topology and
cross-view gates, and recorded human review.

## 5. Object-Local Camera Contract

Every comparison uses the object's canonical local frame:

- front and back follow the local forward axis;
- left and right follow the local right axis;
- top and bottom follow the local up axis;
- the camera frames the declared visual extent with a fixed margin;
- orthographic scale derives from the same extent used by collision and
  spatial inspection;
- the object pivot is its declared visual center, not the building origin;
- hierarchy objects frame the deterministic descendant extent;
- spatial areas frame their occupancy boundary;
- planar and linear objects may declare specific optional-degenerate views;
- transparent objects retain a visible boundary pass independent of opacity;
- rotational symmetry declares equivalent views without deleting evidence.

Perspective, arbitrary auto-framing, environment-dependent exposure, and
building-global camera axes are prohibited in the deterministic comparison
pass. They may be used in a separate presentation render, but not in the
acceptance score.

## 6. Native Object Model

The C++ design is compositional and UML-readable:

```text
BuildingVisualObjectModelCatalog
    aggregates 1..* BuildingVisualObjectModel

BuildingVisualObjectModel
    composes 1..* BuildingVisualFeatureRequirement
    composes 6 BuildingCameraViewMatchContract
    composes 1..* BuildingImageMatchingTestSpecification
    composes 1..* BuildingGrammarImageConstructionDirective

BuildingImageMatchingObservation
    associates 1 BuildingCameraViewName
    associates 1 BuildingImageMatchingTestKind

BuildingObjectImageMatchEvaluationService
    consumes 1 BuildingVisualObjectModel
    consumes 0..* BuildingImageMatchingObservation
    creates 1 BuildingObjectImageMatchReport

BuildingObjectImageMatchReport
    composes 6 BuildingCameraViewMatchResult

BuildingVisualObjectModelValidationService
    validates 1 BuildingVisualObjectModel
    creates 1 BuildingVisualObjectModelValidationReport
```

No inheritance is used merely to share data. The visual families classify
behavior and metric profiles, while each concrete object model owns its named
features and construction directives.

### 6.1 Model Responsibilities

- `BuildingVisualObjectModel` owns the visual contract for one object category.
- `BuildingVisualFeatureRequirement` owns named landmark counts and required
  views.
- `BuildingCameraViewMatchContract` owns observability and minimum similarity
  for one canonical view.
- `BuildingImageMatchingTestSpecification` owns criticality, implementation
  state, score weight, and threshold.
- `BuildingGrammarImageConstructionDirective` owns one ordered grammar change.
- `BuildingImageMatchingObservation` owns one independent measured fact.
- `BuildingCameraViewMatchResult` owns one view's composite and failed tests.
- `BuildingObjectImageMatchReport` owns the all-required-views decision.
- `BuildingVisualObjectModelCatalog` owns model lookup, not taxonomy identity.
- `BuildingObjectImageMatchEvaluationService` applies acceptance policy but does
  not generate images or mutate grammar.
- `BuildingVisualObjectModelValidationService` rejects malformed contracts.

## 7. Visual Building Object Catalog

The generated catalog contains 49 visual object models: 38 specialist models
covering the requested object families and 11 generic models covering the
remaining renderable SMBv3 taxonomy classes. The generated coverage map assigns
all 108 renderable taxonomy objects exactly once.

### 7.1 Organic and Exterior Objects

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Vegetation | Build trunk and primary branches first; replace spherical crowns with overlapping asymmetric foliage clusters; separate bark, leaf, soil, and planter regions; constrain growth extent. | Silhouette, occupancy, component count, texture frequency, extent agreement |
| Paving | Use a bounded profile; expose module size and direction; generate joints and edge restraints; encode drainage fall and hosted drains. | Projection, repetition, texture frequency, edge alignment |
| Privacy screen | Model posts, rails, slats or panels, mounts, exact count, pitch, and viewing orientation. | Repetition, silhouette, edge alignment, component count |
| Fencing | Build from an authored path; parameterize posts, infill, terminals, gates, ground contacts, and exact spacing. | Repetition, connection alignment, projection, silhouette |
| Guttering | Sweep an authored channel profile; add fall, brackets, outlets, corners, and end caps. | Silhouette, edge alignment, connection alignment, extent agreement |
| Drain downpipes | Sweep a segmented pipe path; add bends, offsets, brackets, gutter inlet, and drain outlet. | Silhouette, component count, connection alignment, aspect ratio |
| Ground drains | Cut a hosted paving recess; model drain body, grate bars or slots, and outlet invert. | Repetition, edge alignment, connection alignment, topology |

### 7.2 Structural and Architectural Objects

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Exterior walls | Use solids with explicit thickness and returns; subtract hosted door/window openings before frames; separate substrate, cladding, reveal, and trim. | Silhouette, aspect ratio, topology, material region |
| Structural floors | Use bounded profiles with thickness; subtract stair/service openings; model slab edge, finish layer, direction, and perimeter trim. | Aspect ratio, occupancy, topology, texture frequency |
| Inner walls | Use a partition profile with explicit thickness; subtract openings; create wall-end and junction connection points. | Silhouette, edge alignment, connection alignment, extent agreement |
| Windows and glazing | Separate outer frame, sash, mullions, sill, reveal, and panes; bind to a hosted opening; retain deterministic transparent boundaries. | Transparency boundary, repetition, topology, edge alignment |
| Balconies | Model platform, fascia, soffit, guard, posts, panels, supports, door threshold, and drainage fall as connected parts. | Topology, repetition, connection alignment, silhouette |
| Doors | Separate leaf, frame, handle, threshold, hinge axis, swing state, opening, and reveal depth. | Topology, edge alignment, aspect ratio, connection alignment |
| Stairs | Derive exact tread and riser count from rise/run; model stringers, landings, nosing, guards, and local stair axis. | Repetition, topology, projection, component count |

### 7.3 Furniture and Interior Objects

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Chairs | Parameterize seat, back, supports, arms, caster or pedestal topology; use rounded upholstered profiles; generate explicit set count and spacing. | Topology, component count, symmetry, repetition |
| Couches | Generate counted seat, back, and accent cushions; separate arms and base; use rounded profiles and compression rather than boxes. | Silhouette, topology, component count, symmetry |
| Tables | Separate top and support topology; match leg or pedestal count, edge profile, overhang, and authoritative dimensions. | Component count, topology, aspect ratio, symmetry |
| Beds | Layer frame, mattress, bedding, pillows, and headboard; use rounded soft profiles and explicit paired symmetry. | Silhouette, topology, symmetry, occupancy |
| Cushions | Use rounded volumes, seams, controlled compression, host contact, and deterministic rotation. | Silhouette, edge alignment, material region, connection alignment |
| Vases | Revolve an authored hollow profile with body, neck, rim, base, wall thickness, and optional contents. | Silhouette, aspect ratio, symmetry, material region |
| Cabinets | Build carcass modules, doors, drawers, gaps, handles, top, plinth or legs; expose exact bay count. | Topology, repetition, edge alignment, component count |
| Wardrobes | Build explicit carcass bays, door leaves, panel gaps, handles, plinth, and deterministic open/closed state. | Repetition, topology, symmetry, edge alignment |
| Study desks | Separate worktop, supports, drawers, cabinets, cable opening, knee clearance, and service points. | Topology, aspect ratio, connection alignment, extent agreement |

### 7.4 Rooms and Interior Assemblies

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Kitchens | Compose named cabinet runs, benchtops, island, sink, taps, appliances, splashback, and service bays on one dimensional grid. | Topology, cross-view consistency, repetition, connection alignment |
| Study | Compose desk, chair, storage, window, light, clearances, and object-owned sub-cameras; do not flatten the room into one mesh. | Topology, extent agreement, cross-view consistency, connection alignment |
| Generic rooms | Derive the room camera from occupancy extent; preserve doors, windows, major furniture, and circulation voids as named descendants. | Occupancy, topology, extent agreement, cross-view consistency |

### 7.5 Plumbing and Bathroom Objects

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Toilets | Use rounded bowl and cistern profiles; model seat, base, waste, inlet, wall offset, and floor contact separately. | Silhouette, aspect ratio, topology, connection alignment |
| Bathroom vanities | Layer cabinet, top, basin, tap, mirror, cut-out, waste, and hot/cold service points. | Topology, edge alignment, connection alignment, material region |
| Bathroom showers | Model tray or floor fall, glass screens, door, head, controls, drain, seals, and enclosure opening state. | Transparency boundary, topology, connection alignment, extent agreement |
| Sinks | Use a rounded basin with depth and wall thickness; bind rim or under-mount flange to the worktop cut-out; expose waste and overflow. | Silhouette, topology, connection alignment, component count |
| Taps | Sweep the spout path; separate body, controls, outlet, mount, aerator, and hot/cold connections. | Silhouette, topology, connection alignment, symmetry |

### 7.6 Fixtures, Media, and Decoration

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Lights | Separate emitter, housing, mount, stem or cable, shade, and repeated fixture count; compare emission independently from shape. | Component count, topology, material region, repetition |
| Television | Model screen, bezel, housing, mount, cable zone, and exact screen ratio; avoid a single unframed rectangle. | Aspect ratio, edge alignment, topology, material region |
| Artwork | Separate panel, frame, content region, thickness, and wall mount; keep texture comparison out of silhouette scoring. | Aspect ratio, material region, color palette, texture frequency |
| Fireplace | Model firebox opening, surround, hearth, flue or vent, layered depth, and flame/emissive region. | Topology, occupancy, edge alignment, material region |
| Curtains | Generate a wave or pleat profile, hem, heading, opening state, overlap, and fabric material rather than a flat sheet. | Projection, texture frequency, silhouette, material region |
| Curtain rods or tracks | Sweep the rod or track; add brackets, finials or stops, rings or gliders, and curtain connection points. | Repetition, component count, connection alignment, edge alignment |

### 7.7 Pool Objects

| Object Model | Grammar Changes | Leading Tests |
|---|---|---|
| Pool shell | Build one authored boundary with depth profile, coping, steps, benches, skimmers, inlets, and drains. | Silhouette, topology, connection alignment, extent agreement |
| Pool water | Derive water boundary from the pool shell; model waterline and transparent volume separately; prohibit an unrelated duplicate pool outline. | Transparency boundary, extent agreement, cross-view consistency, material region |

### 7.8 Remaining Generic Taxonomy Models

`building_assembly`, `site_assembly`, `structural_assembly`, `room_assembly`,
`service_equipment`, `system_aggregate`, `appliance`, `safety_equipment`,
`canopy`, `ground_surface`, and `solar_equipment` preserve exact coverage for
taxonomy objects that do not yet have a specialist model. Each generic model
requires decomposition into named visible masses before material refinement.
Repeated use or a persistent score below 80% is a trigger to replace a generic
model with a purpose-specific visual object model.

## 8. Grammar Image Construction Loop

Each object is improved by one controlled grammar-image iteration:

1. Freeze the object taxonomy record, ImageGen crop, camera contract, grammar,
   renderer settings, and source hashes.
2. Confirm object isolation and classify all six views as required,
   optional-degenerate, or not applicable.
3. Extract the reference silhouette, bounding aspect, foreground occupancy,
   connected components, symmetry, repeated-element counts, and named
   landmarks.
4. Define authoritative local dimensions and orientation in the object model.
5. Construct primary masses and voids first.
6. Construct named structural parts and enforce feature counts.
7. Add repeated elements from count and spacing parameters rather than copies.
8. Add host openings, support contacts, service points, and connection graph
   edges.
9. Add rounded profiles, sweeps, revolutions, extrusions, subtractive openings,
   and assemblies according to the category directive.
10. Add semantic material regions before color or texture tuning.
11. Render all six canonical views with the deterministic object-local camera.
12. Run every implemented independent test and record per-view evidence.
13. Modify only the parameter group owned by the weakest failed test.
14. Rerender all six views so a local improvement cannot regress another view.
15. Stop geometry tuning only when every required view reaches at least 84% and
    every independent minimum passes.
16. Run a clean deterministic rerun and a human semantic review before
    accepting the object at the 80% release gate.

### 8.1 Metric-to-Parameter Ownership

| Weakest Test | Permitted First Changes | Changes Deferred |
|---|---|---|
| Silhouette | Primary profile, dimensions, protrusions, cut-outs, orientation. | Color and texture |
| Edge alignment | Frame widths, seams, reveals, tread edges, part boundaries. | Global rescaling |
| Projection distribution | Relative part placement and mass distribution. | Material color |
| Aspect ratio | Authoritative width, height, depth, and local axis. | Fine detail |
| Occupancy | Thickness, holes, open space, foliage density, cushion fullness. | Lighting |
| Component count | Explicit part count, gaps, supports, repeated elements. | Smoothing |
| Symmetry | Mirrored parameter relationships or deliberate asymmetry. | Texture noise |
| Repetition | Count, pitch, phase, terminal spacing, path length. | Random scatter |
| Transparency | Boundary mesh, thickness, opacity model, depth sorting. | Opaque silhouette substitution |
| Material region | Region ownership and material assignment. | Geometry unless region boundary is wrong |
| Connection alignment | Connection point position, host cut-out, support contact. | Decorative detail |
| Extent agreement | Visual/collision/access/motion/growth extent owners. | Camera compensation |

## 9. Delivery Phases

### Phase 0 — Matching Architecture

Status: implemented and deterministically tested.

- define native visual object, view, test, directive, observation, and report
  classes;
- generate 49 object models and exact assignments for 108 renderable objects;
- isolate failures per object view;
- enforce hard gates, independent minimums, and 80% per-view composites;
- preserve optional-degenerate view semantics;
- publish the current baseline and focused acceptance runner.

### Phase 1 — Reference and Camera Completion

Status: reference coverage completed; six-view observability repair remains.

- generated and ingested ImageGen atlases 16, 17, and 18;
- verified exactly one crop for every renderable taxonomy object;
- recorded 108/108 references and all 648 comparison slots;
- added a constrained front/right/top gate with 108/108 objects above 84%;
- retain 48 freeform six-view observability failures for explicit repair;
- freeze remaining transparent-boundary behavior without hiding failed views.

Exit gate: complete provenance, object isolation, and camera calibration are
met. Phase closure still requires no failed six-view observation and no silent
comparison.

### Phase 2 — Planar, Framed, and Linear Objects

Prioritize walls, floors, windows, doors, television, artwork, paving, canopy,
gutters, downpipes, rods, screens, fences, and drains. These objects expose
camera, thickness, profile, frame, and repetition errors with comparatively low
organic-shape uncertainty.

Exit gate: each selected object reaches an 84% construction target in every
required view and passes topology/repetition tests relevant to its category.

### Phase 3 — Furniture and Sanitary Objects

Prioritize couch, chairs, tables, beds, cabinets, wardrobe, desk, toilet,
vanity, shower, sink, taps, cushions, and vases. Replace box-only approximations
with explicit parts and rounded profiles.

Exit gate: named landmark counts and support/service connections pass in
addition to the image scores.

### Phase 4 — Repeated, Transparent, and Organic Objects

Prioritize stairs, vegetation, privacy screens, fencing, curtains, glazing,
lights, pool shell, and pool water. Activate repetition, transparency,
material-region, color, and texture tests only after synthetic fixtures prove
that each metric is stable.

Exit gate: transparent boundaries, repeated counts, growth extents, and
cross-view part consistency pass.

### Phase 5 — Assemblies and Rooms

Prioritize kitchen, study, balconies, bathroom and bedroom assemblies, building
storeys, envelope, roof, services, and site. Score named descendants and their
relationships; do not compare only the outer aggregate silhouette.

Exit gate: descendant identities, clearances, connection graph edges, and
aggregate extents agree across all six views.

### Phase 6 — Certification Candidate

- run all 108 objects and all 648 comparison slots from a clean build;
- require every required view to score at least 80%;
- require the object construction target of at least 84% before promotion;
- require all active independent tests to pass;
- require no missing, duplicate, or unclassified observation;
- verify deterministic evidence files byte-for-byte;
- record grammar, renderer, taxonomy, reference, and report hashes;
- perform human semantic review and record any accepted equivalence;
- retain rollback to the prior accepted grammar artifact.

## 10. Independent Test Fixtures

The matching metrics require synthetic tests in addition to building images:

- identical masks score 100% for every implemented numeric metric;
- component-count changes affect the component observation independently;
- aspect-ratio changes affect the aspect observation independently;
- occupancy changes affect the occupancy observation independently;
- symmetry changes affect the symmetry observation independently;
- spatial shifts reduce silhouette, edge, and projection agreement without
  changing declared aspect ratio;
- background-only images are classified as unobservable;
- duplicate object records fail the isolation gate;
- an independent score below its minimum fails even when the weighted composite
  exceeds 90%;
- a required degenerate view fails;
- an explicitly optional degenerate view passes;
- invalid model weights or missing six-view contracts fail validation;
- a regenerated baseline must byte-match the committed evidence.

## 11. Canonical Artifacts

- `include/building/model/BuildingVisualObjectModel.h`
- `include/building/model/BuildingVisualFeatureRequirement.h`
- `include/building/model/BuildingCameraViewMatchContract.h`
- `include/building/model/BuildingImageMatchingTestSpecification.h`
- `include/building/model/BuildingGrammarImageConstructionDirective.h`
- `include/building/model/BuildingImageMatchingObservation.h`
- `include/building/model/BuildingCameraViewMatchResult.h`
- `include/building/model/BuildingObjectImageMatchReport.h`
- `include/building/model/BuildingVisualObjectModelCatalog.h`
- `include/building/service/BuildingVisualObjectModelValidationService.h`
- `include/building/service/BuildingObjectImageMatchEvaluationService.h`
- `examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/source/SMBv3_Building_Object_Visual_Matching_Models.json`
- `examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/generated/SMBv3_Building_Object_Image_Matching_Coverage.csv`
- `examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/evidence/SMBv3_Building_Object_Image_Matching_Baseline/`
- `tools/analyze_building_object_image_matches.py`
- `tests/test_building_object_image_match_metrics.py`
- `tests/building_object_image_matching_model_harness.cpp`
- `tests/run_building_object_image_matching_model_checks.sh`

## 12. Acceptance Command

```bash
./tests/run_building_object_image_matching_model_checks.sh
```

The command checks generated catalog drift, exact 108-object coverage,
six-view contracts, metric weights, construction directives, deterministic
baseline replay, synthetic metric independence, C++ model validation, hard-gate
behavior, optional-degenerate handling, and the no-averaging acceptance rule.

## 13. Current Limit

The matching architecture and construction plan are implemented, but no object
is currently certified above 80% in every required view. Fifteen of eighteen
ImageGen atlases are present, and the planned topology, repetition,
transparency, material, color, texture, cross-view, connection, extent, and
human-review gates are not yet all executable. Those limits remain explicit so
the current baseline cannot be mistaken for completion of the image-fitting
program.
