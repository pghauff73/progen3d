# Vegetation Renderer Integration Implementation Plan

Date: 2026-08-28

## Objective

Convert the vegetation triangle-distribution contract into executable geometry
representations that preserve biological structure, projected organ area,
deterministic evidence, runtime LOD selection, packed GPU ownership, and scalable
draw submission.

## Current Renderer Boundary

The renderer now preserves vegetation object and representation identity through
scene admission, selection, packed per-material GPU pages, CPU command planning,
direct indexed fallback, and multi-draw indirect submission. All LOD levels remain
resident together, so representation residency rather than command submission is
the next unresolved scaling boundary.

## Slice A: Offline Vegetation LOD Bundle

Status: Completed and validated

1. Classify every vegetation geometry part into a semantic triangle role.
2. Allocate per-part triangle budgets with
   `VegetationTriangleDistributionService`.
3. Decimate structural geometry with deterministic protected-edge collapse.
4. Remove complete leaf, petal, flower, fruit, or bud organs before simplifying
   surviving organ geometry.
5. Preserve represented organ area by scaling surviving organs around their own
   centroids within an explicit scale limit.
6. Construct deterministic semantic and spatial meshlets.
7. Package fine-to-coarse representations with object-space geometric error and
   stable evidence hashes.
8. Select representations at runtime from projected screen error, motion bias,
   silhouette bias, and hysteresis.

### Slice A Evidence

- `tests/run_vegetation_renderer_lod_pipeline_checks.sh` passes protected
  structural edge collapse, complete-organ removal, surface-area preservation,
  meshlet limits, deterministic bundle generation, projected-error selection,
  semantic bias, and hysteresis.
- `tests/run_building_vegetation_triangle_distribution_checks.sh` passes the
  preceding triangle-allocation and independent fidelity contracts.
- `tests/run_vegetation_geometry_assembly_checks.sh` passes the existing
  vegetation assembly topology and scale fixtures.
- `make -j2 progen3d-editor-gui` compiles and links the editor with the new
  services.
- Validated editor SHA-256:
  `489f3bd447361c28d94c9b4258eabe3422215d95c5d3ffea5f072bc2ad225338`.

### Slice A Limits At Completion

- Billboard cloud construction remains fail-closed and is not represented as
  executable geometry yet.
- At Slice A completion, the renderer did not yet consume
  `VegetationLodBundle`; Slice B subsequently replaced that limit with the
  generic resolved-representation path.
- No frame-time, upload-bandwidth, overdraw, or memory improvement is claimed
  before Slice B benchmarks the integrated renderer path.

## Slice B: Scene And Renderer Integration

Status: Completed and validated

1. Add a generic resolved-primitive representation set that can aggregate a
   vegetation LOD bundle without making scene primitives own vegetation logic.
2. Generate the bundle while resolving procedural Plant and Vine geometry, then
   associate it with the admitted scene primitive instance.
3. Extend `ScenePreviewGeometryBatches` with per-object vegetation upload records
   instead of appending those vertices to material-wide opaque buffers.
4. Upload all validated representations when scene resources change; do not
   re-upload geometry when only the camera changes.
5. Select one representation per visible vegetation object in the render frame,
   after camera matrices and frustum visibility are known.
6. Submit selected meshlet ranges through an indexed path while retaining the
   existing direct triangle-array path as fallback.
7. Record selected LOD, projected error, submitted meshlets, triangles,
   hysteresis decisions, upload bytes, and fallback use in preview performance
   evidence.
8. Add deterministic resource/batch tests, headless camera-distance tests, and
   a before/after frame-time and memory benchmark before enabling the path by
   default.

### Slice B Implemented Object Model

- `ResolvedPrimitiveRepresentationSet` gives admitted primitive geometry one
  generic owner for fine-to-coarse meshes, meshlets, geometric error, and
  deterministic evidence.
- `PlantGeometryAssemblyService` and `VineGeometryAssemblyService` retain
  semantic vegetation parts long enough to generate LOD bundles during
  procedural resolution.
- `PreviewVegetationLodObjectFactory` converts resolved representations into
  per-object world-space vertex/index uploads while preserving primitive,
  material, meshlet, bounds, error, and evidence identity.
- `OpenGlPreviewVegetationLodSelectionService` applies the shared projected-error
  policy after camera matrices exist and before shadow submission.
- `OpenGlPreviewVegetationLodObjectResource` owns one VAO/VBO/EBO set per
  representation and retains previous selection state for hysteresis.
- `OpenGlPreviewVegetationLodPass` owns visible indexed meshlet submission and
  selected-representation shadow submission; scene and shadow passes compose it
  instead of retaining vegetation-specific procedural blocks.
- `OpenGlPreviewMaterialResourceBindingService` keeps texture and material-record
  binding semantics shared by array, dynamic-instance, and vegetation paths.
- `PreviewRenderPerformanceEvidence` reports object, selection, hysteresis,
  meshlet, projected-error, upload, fallback, triangle, and culling evidence.

### Slice B Evidence

- `tests/run_vegetation_renderer_scene_integration_checks.sh` passes direct
  resolved-representation upload, object-level batch ownership, opaque and
  transparent fallback counting, near/far selection, hysteresis, fail-closed
  invalid ordering, evidence accounting, indexed draw ownership, and frame
  selection ordering.
- `tests/run_vegetation_renderer_lod_pipeline_checks.sh` continues to pass the
  shared offline decimation, organ-removal, meshlet, and runtime policy gates.
- `tests/run_scene_preview_resource_checks.sh` passes compatibility resource and
  batch ownership gates.
- `tests/run_scene_preview_rendering_workflow_checks.sh` passes render-setting
  propagation and preview workflow ownership after the new configuration was
  added.
- `tests/run_vegetation_geometry_assembly_checks.sh` continues to pass existing
  topology and assembly evidence.
- `tests/run_open_gl_preview_renderer_architecture_checks.sh` passes with the
  frame orchestration, general render-pass, vegetation pass, material binding,
  geometry upload, and render error publication responsibilities below their
  explicit ownership ceilings.
- `make -j2 progen3d-editor-gui` compiles and links the integrated renderer.
- Validated editor SHA-256:
  `89ae0efc37ca6ee2c99452da7d91a59eb8fd7cba01bc93836c144945dd4a5957`.

### Slice B Limits

- Vegetation LOD remains an opt-in render setting. Slice C now supplies the
  retained hardware A/B evidence, but default enablement remains blocked by
  representation residency and broader plant-shape coverage.
- Transparent Plant and Vine objects retain the existing sorted direct-array
  compatibility path; alpha-masked indexed vegetation is not yet separated from
  blended transparency.
- Meshlets are submitted with direct `glDrawElements` calls. GPU meshlet culling,
  multi-draw indirect command generation, and mesh shaders are not claimed.
- Billboard cloud construction remains fail-closed.
- Slice B deterministic and compile evidence does not independently establish a
  performance claim; Slice C owns the retained hardware measurements.

## Slice C: Hardware A/B Enablement And Submission Scaling

Status: Hardware A/B completed; default enablement deferred

1. Add an explicit A/B launch configuration that renders identical vegetation
   scenes with runtime LOD disabled and enabled.
2. Add a dense Plant/Vine/grass workload with near, transition, and far camera
   frames plus a fixed material and lighting configuration.
3. Record GPU pass medians, CPU frame medians, resident representation bytes,
   selected LOD distribution, visible meshlets, triangles, fallback count, and
   upload bytes for both paths.
4. Capture deterministic comparison images and reject silhouette, projected
   organ-area, topology, or alpha-coverage regressions before enabling by
   default.
5. Assess grouped multi-draw indirect submission from the measured direct draw
   count instead of assuming that additional command infrastructure is useful.
6. Decide default enablement from measured performance, visual fidelity,
   selection stability, and representation residency evidence.

### Slice C Feasible Reduction Repair

The first retained hardware A/B run exposed that every production vegetation
bundle had collapsed to `lod0_full`. The default representation factory retried
generation by removing coarse levels whenever any part failed. Two valid
semantic constraints made arbitrary distributed triangle counts unreachable:

- Protected petiole and stem endpoints can leave no legal structural edge
  collapse at the requested count.
- Complete-organ removal can require more leaf or flower triangles than the
  distributed count in order to preserve represented area within the configured
  linear scale.

The repaired contract treats a distributed triangle count as a requested budget,
not permission to violate botanical semantics:

- `VegetationStructuralMeshDecimationService` now returns the closest legal
  topology it can reach and records requested versus actual triangles, including
  unchanged protected parts.
- `VegetationOrganAreaPreservingReductionService` first retains complete organs
  within the request, then retains additional complete organs until the
  configured area-preservation scale is satisfied.
- `VegetationOfflineLodGenerationService` composes the actual feasible part
  results; representation triangle counts remain deterministic and decreasing at
  the assembly level.
- `tests/run_vegetation_default_lod_representation_checks.sh` proves four
  decreasing default representations for real `GrassClump`, `Shrub`, and
  `IvyVine` catalog geometry.

### Slice C Hardware Evidence

`tests/run_vegetation_renderer_lod_ab_checks.sh --refresh` runs three independent
process repetitions for each near, transition, and far camera state in both
`full` and `lod` modes. Each process reports the median of eight non-blocking GPU
timing samples and produces a deterministic capture. Retained evidence lives in
`tests/evidence/vegetation_renderer_lod_ab_2026-08-28.json`.

- Hardware: NVIDIA GeForce RTX 5060 Ti, direct OpenGL 4.6, 16,311 MB dedicated
  video memory.
- Near: runtime LOD submits 74.0% of full triangles and uses 85.8% of full GPU
  time; one-pixel-tolerant green silhouette F1 is 98.4% and represented green
  area is 101.2%.
- Transition: runtime LOD submits 66.8% of full triangles and uses 70.3% of full
  GPU time; silhouette F1 is 96.2%, represented green area is 98.7%, and PSNR is
  40.9 dB.
- Far: runtime LOD selects `0,0,6,20` objects across LOD0-LOD3, submits 60.1% of
  full triangles and 62.2% of full meshlets, and uses 63.2% of full GPU time;
  silhouette F1 is 94.0%, represented green area is 94.3%, and PSNR is 47.7 dB.
- All A/B modes use the same resources and report 224,159,696 resident vegetation
  representation bytes. Runtime selection therefore reduces submission work but
  does not yet reduce representation residency.
- Far runtime LOD still emits 25,148 direct scene draws, decisively exceeding the
  retained indirect-submission candidate threshold of 128 draws.

### Slice C Decision

- The integrated runtime LOD path is validated for the retained dense vegetation
  fixture and now has measured CPU, GPU, geometry, selection, and visual evidence.
- Runtime LOD remains disabled by default. The current all-level residency cost
  and limited catalog/view coverage do not justify global enablement yet.
- Multi-draw indirect submission is justified as the next renderer slice because
  direct per-meshlet submission remains the dominant scaling defect after LOD.
- Billboard clouds remain fail-closed until alpha coverage, cross-fade, and
  shadow behavior have independent evidence.

## Slice D: Packed Geometry And Indirect Submission

Status: Completed and validated

1. Introduce a purpose-driven `VegetationIndirectSubmissionBatch` model that
   groups selected opaque meshlets by compatible material, vertex layout, and
   shared index-buffer ownership.
2. Replace per-object VAO/EBO ownership where necessary with packed vegetation
   geometry pages so one `glMultiDrawElementsIndirect` call can submit many
   selected meshlets without changing material semantics.
3. Build indirect commands from the already selected representation and CPU
   frustum result first; add GPU command compaction only after the CPU-generated
   command path has deterministic evidence.
4. Preserve the direct indexed path as a fail-closed fallback and report command
   count, packed-buffer bytes, visible meshlets, rejected commands, and fallback
   use in `PreviewRenderPerformanceEvidence`.
5. Repeat the near/transition/far hardware A/B matrix against direct indexed
   submission and require unchanged captures, reduced CPU submission cost, and
   no GPU regression before adopting indirect submission.
6. Keep multi-draw indirect opt-in until additional plant architectures,
   alpha-masked foliage, blended transparent vegetation, animated wind, and
   broader material ordering are covered.

### Slice D Implementation

- `OpenGlPreviewVegetationPackedPageResource` owns one shared VAO, vertex buffer,
  index buffer, and preallocated indirect command buffer per vegetation material.
- Each representation owns explicit packed-page, first-index, base-vertex,
  vertex-count, and index-count metadata rather than private GL buffers.
- `OpenGlPreviewVegetationIndirectCommandService` preserves object order while
  producing contiguous compatible command batches from the selected LOD and CPU
  frustum result.
- `OpenGlPreviewVegetationSubmissionService` submits those exact commands through
  either `glDrawElementsBaseVertex` or `glMultiDrawElementsIndirect`; unavailable
  or over-capacity indirect storage falls back to direct indexed submission.
- `--vegetation-submission-mode direct|indirect` exposes an A/B control while the
  configured default remains direct indexed.
- Performance evidence reports packed pages, command capacity, indirect batches,
  command count, ABI upload bytes, rejected commands, fallback commands, and
  direct versus indirect scene draw calls.

### Slice D Hardware Evidence

`tests/run_vegetation_renderer_submission_ab_checks.sh --refresh` runs three
independent process repetitions for each near, transition, and far camera state
in direct and indirect modes. Retained evidence and six captures live in
`tests/evidence/vegetation_renderer_submission_ab_2026-08-28.json` and
`tests/evidence/vegetation_renderer_submission_ab_2026-08-28/`.

- Hardware: NVIDIA GeForce RTX 5060 Ti, direct OpenGL 4.6.
- Every direct/indirect capture pair is byte-identical.
- Every state reports one packed page, zero rejected commands, zero direct
  fallback commands, and one indirect batch when indirect mode is requested.
- Near: CPU ratio is 38.4%, total GPU ratio is 80.7%, and draw-call ratio is
  0.057% of direct submission.
- Transition: CPU ratio is 33.4%, total GPU ratio is 57.9%, and draw-call ratio is
  0.033% of direct submission.
- Far: CPU ratio is 35.0%, total GPU ratio is 53.6%, opaque GPU ratio is 51.8%,
  and draw-call ratio is 0.036% of direct submission.
- Far submission changes from 25,148 direct scene draws to two non-vegetation
  direct draws plus one vegetation indirect draw while preserving 459,039
  submitted triangles and 25,146 visible meshlet commands.
- Direct and indirect modes both retain 224,159,696 vegetation bytes. Indirect
  submission fixes command scaling but does not reduce representation residency.

### Slice D Decision

- The packed-page and CPU-generated multi-draw indirect path is validated for the
  retained dense opaque vegetation fixture.
- Multi-draw indirect remains disabled by default until alpha-masked, transparent,
  wind-deformed, multi-material, and broader plant coverage is retained.
- GPU command compaction is not justified: one CPU-built indirect batch already
  removes the dominant direct-call overhead for the retained fixture.
- Representation residency is now the next renderer integration slice.

## Slice E: Budgeted Representation Residency

Status: Next renderer integration slice

1. Introduce a `VegetationRepresentationResidencyPolicy` model with explicit
   always-resident, distance-window, and budgeted-working-set policies.
2. Split packed material pages into independently resident representation segments
   so unused LOD tiers can be uploaded, retained, and evicted without destroying
   selected-object or shared-material identity.
3. Keep CPU representation metadata and evidence hashes resident while GPU
   geometry residency changes; never infer missing geometry from another tier.
4. Prefetch the selected representation and one hysteresis-neighbor tier, retain
   the previous valid tier until replacement upload completes, and fail closed to
   the direct compatibility path on an unresolved residency miss.
5. Add fence-safe staged upload and retirement so no in-flight indirect command
   references an evicted page segment.
6. Report requested, resident, pending-upload, evicted, reused, and peak vegetation
   bytes; residency hits, misses, substitutions, and upload latency; and the exact
   representation set resident for each camera state.
7. Repeat near/transition/far movement and camera-reversal tests under constrained
   budgets, requiring unchanged steady-state captures, bounded transition latency,
   no invalid command references, and a material resident-byte reduction.
8. Extend the fixture matrix to alpha-masked foliage, blended vines, wind animation,
   multiple vegetation materials, and broader plant architectures before changing
   either runtime LOD or indirect submission defaults.

## Acceptance Gates

- Structural output contains no invalid or degenerate triangles.
- Protected boundaries and botanical junction surfaces are not collapsed.
- Organ reduction removes complete organ groups identified by
  `objectPartIndex()`.
- Organ area preservation stays within its configured linear scale limit.
- Meshlets respect both vertex and triangle limits.
- LOD generation and selection are input-order independent and deterministic.
- Runtime selection uses projected error rather than fixed distance bands.
- Renderer integration retains a compatibility fallback and reports evidence;
  performance improvement is not claimed until measured.
