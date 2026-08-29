# Modern Luxury Townhouse Camera and Taxonomy Design

## Purpose

`ModernLuxuryTownhousev1` demonstrates how ProGen3D can combine a spatially
aware building object model, fail-closed placement, object-isolated camera views,
and deterministic walkthrough scripting. ImageGen output is retained as a design
reference; generated grammar geometry and runtime spatial evidence remain the
authoritative implementation record.

## Object Model

The grammar declares 23 spatial objects in
`examples/SimpleModernBuilding/ModernLuxuryTownhousev1.grammar`. The taxonomy manifest in
`examples/SimpleModernBuilding/ModernLuxuryTownhousev1.taxonomy.json` names each object's class,
parent object, taxonomy path, and building area.

Twenty-one objects are renderable comparison subjects. The root townhouse is
excluded because object cameras must not silently fall back to the whole scene.
The primitive coverage study is excluded because it validates language coverage,
not a building object. These exclusions make the individual-object contract
explicit and fail closed.

The townhouse taxonomy is complete for the objects authored by this grammar. It
is not a claim that the 23 nodes are a universal inventory of every building
object type. The repository's broader building-object catalog remains the owner
for cross-building taxonomy coverage.

## Spatial Awareness

Spatial declarations separate building identity from geometry. `Object`
declarations establish containment, class, taxonomy, and area. `Interface`
declarations expose support surfaces. `Connect` declarations express intended
relationships. `Position` constraints perform collision-aware placement against
named support objects and interfaces.

The townhouse uses four support constraints. Each constraint must resolve before
the scene is accepted. Missing supports, forbidden collisions, excessive travel,
or unresolved placement remain errors rather than inferred placements.

## Camera View System

`PreviewObjectCameraViewService` isolates geometry directly bound to one spatial
object, hides every other active primitive, selects a retained primitive, and
fits the camera to the isolated bounds. It does not include descendants or the
whole scene implicitly. This keeps object identity and camera evidence aligned.

The named orthographic views are front, back, left, right, top, and bottom.
`--preview-hide-grid` provides clean comparison output. `--preview-fit-extents`
derives framing from the isolated authored geometry bounds.

`CameraWalkthroughScript` owns the ordered keyframes and frame rate.
`CameraWalkthroughScriptParser` validates syntax and monotonic time.
`CameraWalkthroughPlaybackService` samples position and lens state, including
shortest-path angle interpolation. The renderer reuses the canonical preview
camera and projection model rather than creating a second camera authority.

## Comparison Workflow

Each of the 21 renderable objects has an ImageGen six-view reference card in
`examples/reference_images/ModernLuxuryTownhousev1_imagegen/taxonomy`. The
camera pipeline renders the same six ordered views and writes a side-by-side
comparison sheet per object.

The automated metric is normalized RMSE for grayscale appearance and extracted
edges. The values are advisory because ImageGen references contain different
materials, lighting, and detail interpretation. They identify review priorities;
they do not override deterministic grammar, spatial resolution, or camera tests.

## Implementation Plan Status

1. Define deterministic camera keyframes and parser contracts: complete.
2. Reuse the canonical preview camera during playback: complete.
3. Isolate exact spatial-object geometry for camera framing: complete.
4. Rewrite the townhouse with explicit taxonomy and supports: complete.
5. Exercise every available grammar geometry primitive: complete.
6. Generate ImageGen references for all comparison objects: complete.
7. Render and compare 21 objects across six views: complete.
8. Promote visual findings into geometry refinements: next iteration.

## Next Iterations

- Refine vegetation primitives so orthographic extents match the denser reference
  canopies and planted beds without changing the taxonomy identity.
- Refine facade depth, glazing subdivisions, and side-elevation massing where the
  comparison cards show under-modeled silhouettes.
- Add optional descendant-inclusive camera framing as a separate explicit mode;
  preserve direct-geometry isolation as the default.
- Add collision-aware camera path clearance and focus-target tracking for indoor
  walkthroughs.
- Connect townhouse classes to the repository-wide building-object catalog and
  produce an exact coverage matrix before claiming universal taxonomy coverage.
