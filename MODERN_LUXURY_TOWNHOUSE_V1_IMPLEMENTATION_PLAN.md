# Modern Luxury Townhouse V1 Implementation Plan

## Objective

Replace the geometry-only townhouse draft with an executable, spatially aware
building grammar whose static camera views and scripted walkthrough can be
compared reproducibly with the ImageGen building and object references.

The implementation must preserve the current grammar language, use the complete
live primitive inventory, keep semantic objects separate from geometry, resolve
supported collision-positioning constraints transactionally, and produce
deterministic visual evidence.

## Authoritative Inputs

- Grammar: `examples/ModernLuxuryTownhousev1.grammar`
- Building views: `examples/reference_images/ModernLuxuryTownhousev1_imagegen/building_views_v2.png`
- Building objects: `examples/reference_images/ModernLuxuryTownhousev1_imagegen/building_objects_v2.png`
- Baseline camera captures: `examples/reference_images/ModernLuxuryTownhousev1_camera_baseline/`
- Spatial grammar contract: `Object`, `Interface`, `Connect`, and `Position`
- Supported positioning modes: `Touch`, `Gap`, and `Drop`
- Camera presets: front, back, left, right, top, bottom, and isometric

## Current Baseline

- The grammar parses 27 rules and emits 55 primitive instances.
- It exercises all 45 live geometry primitive names.
- It declares only one spatial object and one inspection interface.
- It declares no spatial connections or collision-positioning constraints.
- Camera preset captures are available for front, back, left, right, top, and
  isometric views.
- The current render communicates massing but does not match the reference
  facade hierarchy, object density, vegetation, furniture, or roof-terrace
  composition closely enough.

## Deliverables

### 1. Camera Walkthrough Scripting

Add a purpose-driven camera scripting model:

- `CameraWalkthroughKeyframe` owns time, camera target, orbit angles, distance,
  scale, and vertical field of view.
- `CameraWalkthroughScript` owns an ordered non-empty keyframe sequence.
- `CameraWalkthroughSample` is an immutable resolved camera state.
- `CameraWalkthroughScriptParser` reads a deterministic text format and reports
  line-specific failures.
- `CameraWalkthroughPlaybackService` interpolates a script at a requested time,
  including shortest-path angular interpolation.

Add CLI integration:

```text
--camera-walkthrough <path>
--camera-time <seconds>
--camera-frame-directory <path>
--preview-object <spatial-object-id>
```

The smoke workflow applies the sampled state to the existing
`PreviewCameraController` and `PreviewProjectionConfiguration`. It must not
introduce a second camera owner or modify document/generation state.

`--preview-object` isolates the directly bound geometry of one spatial object,
selects a retained primitive, refreshes preview resources, and fits the existing
camera to the resulting visible bounds. It is intentionally separate from the
semantic inspector selection option.

Add `tools/render_camera_walkthrough.sh` to render deterministic numbered PNG
frames and optionally assemble an MP4 when `ffmpeg` is available.

### 2. Spatially Aware Grammar Rewrite

Organize the townhouse as explicit semantic objects:

- property and site
- structural slabs and support surfaces
- front, rear, side, and roof envelope systems
- street entrance and stair-glazing assembly
- street balconies and privacy fins
- courtyard paving and pool
- roof terrace and pergola
- dining tables, chairs, and lounge furniture
- planters, hedges, trees, palm, grasses, shrubs, and vines
- plumbing/service objects represented by visible reference features

Every physical assembly must declare a collision layer and mask. Geometry-free
semantic containers remain valid objects and must not receive placeholder
meshes.

### 3. Collision Positioning

Use only currently supported executable modes:

- `Drop` dining tables and chairs onto courtyard or roof support surfaces.
- `Drop` lounge chairs and planters onto their terrace support surfaces.
- `Gap` facade-mounted fins or screens where the current solver can prove the
  requested separation without initial penetration.
- `Touch` service or fence components only when the initial boundary relation is
  valid for the axis-aligned solver.

Every positioning declaration must have compatible interfaces, bounded travel,
explicit tolerance, explicit collision mask, and a corresponding connection.
Unsupported modes must remain documented future work rather than silently
falling back.

### 4. Primitive Coverage

Retain executable coverage for all 45 available geometry primitives:

- `Cube`, `CubeX`, `CubeY`, `CubeZ`
- 32 canonical procedural primitives
- 9 curved aliases

Primitives should be attached to meaningful architectural, furnishing,
landscape, or controlled design-study objects. Coverage may not be satisfied by
unowned geometry outside the spatial hierarchy.

### 5. Camera and Image Comparison

Render at least these deterministic camera outputs after the rewrite:

- front
- back
- left
- right
- top
- isometric
- walkthrough frames at declared keyframe and midpoint times

Comparison is evidence-based but not presented as photogrammetric truth. The
report must record:

- grammar and reference image SHA-256 hashes
- camera parameters and pixel hashes
- silhouette occupancy and non-background bounding boxes
- visual feature coverage for facade, roof, courtyard, furniture, vegetation,
  pool, and boundary objects
- known mismatches caused by perspective-only projection, primitive limits, or
  generated-image inconsistency

## Validation Gates

1. Camera model and parser tests pass with warnings enabled.
2. Launch-option tests cover both walkthrough flags.
3. The walkthrough parser rejects malformed, non-finite, duplicate-time, and
   unordered keyframes.
4. Playback is deterministic and clamps outside the script duration.
5. The rewritten grammar parses and regenerates without diagnostics.
6. All declared collision positions resolve or fail closed with explicit
   evidence; no constraint is silently ignored.
7. Object, interface, connection, position, and primitive counts match the
   checked-in validation contract.
8. Two captures of the same view and walkthrough time are byte-identical.
9. Static and walkthrough camera operations do not dirty or regenerate the
   document.
10. The comparison report identifies remaining mismatches instead of treating a
    successful render as proof of architectural equivalence.

## Implementation Order

1. Freeze baseline captures and hashes.
2. Implement and test walkthrough models, parser, and playback.
3. Integrate CLI sampling into the existing smoke workflow.
4. Add the reusable walkthrough frame renderer.
5. Rewrite the grammar into the spatial object hierarchy.
6. Add supported positioning constraints and verify resolution.
7. Capture static and walkthrough views twice.
8. Compute visual metrics and inspect the contact sheets.
9. Iterate geometry and camera keyframes from comparison evidence.
10. Publish validation evidence and unresolved next steps.

## Completion Standard

Completion requires the implementation, grammar, reference assets, walkthrough
script, deterministic captures, comparison report, and focused validation to be
present and current-hash consistent. A parsing pass or one attractive image is
not sufficient evidence of completion.
