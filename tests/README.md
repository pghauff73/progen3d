# Progen3D P0, P1, and P2 verification

The supplied source snapshot contains implementation files but no matching project headers or complete build configuration. The test suite therefore combines source gates, contract builds of the actual changed implementation units, runtime harnesses, and exact-body extraction tests.

## Vegetation grammar subsystem

Run the complete nonvisual vegetation matrix with:

```bash
./tests/run_vegetation_checks.sh
```

Run the research-enriched Building Vegetation Object Model gate with:

```bash
./tests/run_building_vegetation_triangle_distribution_checks.sh
```

This gate compiles the UML-readable biological profile composition, verifies
the deterministic triangle and three-view fidelity contracts, regenerates and
checks all 50 vegetation building objects, and requires every object to carry
multiscale topology, phenology, root, canopy optical, biomechanics,
environmental-response, semantic-annotation, functional-trait, hydraulic,
size-allometry, substrate-requirement, and evidence-provenance records. Unknown
species and measurement values must remain null rather than being inferred from category.
It also exercises calibrated, uncalibrated, and contradictory biological
profiles through independent calibration-readiness gates. The generated
architectural-archetype suite is placement-ready but remains fail-closed for
taxonomic, topology, root, optical, phenology, biomechanics, environmental,
semantic, trait, hydraulic, allometric, substrate, and measured-fidelity
readiness across fourteen independent domains.
The same gate compiles an independent calibration-evidence harness that verifies
canonical JSON serialization, SHA-256 payload identity, immutable parsing and
binding, order independence, domain-atomic deferral, readiness transitions, and
rejection of duplicate, stale, unsupported, non-finite, mismatched, or
contradictory evidence.
It also compiles a measured-source adapter harness over frozen QSM, root graph,
canopy optical, phenology, and biomechanical fixtures. That harness verifies raw
source hashing, local coordinate and SI unit normalization, graph connectivity,
chronology, accepted/deferred/rejected record reporting, exact adapter
determinism, provenance and uncertainty propagation, Slice C round trips, and
sequential readiness transitions for the five calibrated domains.
The same gate compiles a native-source decoder harness for the pinned TreeQSM
`save_model_text` 1.1.0 cylinder export, metric three-dimensional RSML v1, and
declared canopy, phenology, and biomechanical tables. It verifies exact schema
registration, affine world-to-plant round trips, unchanged raw hashes,
canonical source and transform provenance, explicit derived/lossy/deferred
observations, fail-closed format rejection, and acceptance by every completed
measured-source adapter.
It also compiles `vegetation_point_cloud_ingestion_harness.cpp`, which verifies
the D3A/D3B1 capability catalog, exact source hashes, bounded PLY 1.0 ASCII and
little- and big-endian scalar point ingestion, uncompressed LAS 1.4 point
ingestion, LAS scale/offset and bounds authority, every PLY scalar family,
explicit LAS 1.5/LAZ/E57 rejection, woody/foliage-aware reconstruction
admission, and provenance-bound deterministic job intent.
The same harness now validates D3B2A geometry-only canopy occupancy: seeded
training/holdout splits, organ-label coverage, holdout neighborhood recall,
bounded occupied cells, projected occupancy areas, explicitly named gap
proxies, deterministic repeated output, unchanged source hashes, and rejection
of low-quality or unsupported reconstruction jobs. No test promotes occupancy
proxies into calibrated canopy optical measurements.
The supporting primary-source synthesis is in
`docs/BUILDING_VEGETATION_OBJECT_MODEL_LITERATURE_REVIEW.md`.
The staged implementation sequence and current next slice are recorded in
`docs/BUILDING_VEGETATION_OBJECT_MODEL_IMPLEMENTATION_PLAN.md`.

```bash
./tests/run_vegetation_growth_checks.sh
```

The growth gate validates immutable time-sampled `BranchGraph` snapshots,
independent length/radius/organ growth curves, developmental-state transitions,
hierarchical position reconstruction, radius conservation, complete topology
hashing, deterministic resolution evidence, and fail-closed invalid inputs.

```bash
./tests/run_vegetation_placement_checks.sh
./tests/run_vegetation_geometry_assembly_checks.sh
./tests/run_vegetation_grammar_scene_checks.sh
./tests/run_vegetation_fruit_shell_checks.sh
./tests/run_vegetation_lsystem_checks.sh
```

The topology gate also verifies that `BranchGraph` segments decompose into explicit
`PlantInternode` and `PlantStem` objects. The placement gate covers golden-angle
`FlowerHead` packing and Raceme, Spike, Panicle, Umbel, Corymb, and Head
inflorescence topology in addition to phyllotaxis, whorls, compound leaves, and
general organ arrays.

These gates validate phyllotaxis and whorls together with compound leaves and
generalized OrganArray placement: petiole-to-rachis placement,
alternate/opposite leaflet arrays, deterministic transform composition, shared
Bud/Leaf/Petal/Flower/Fruit/Thorn sources, scale falloff and bounded jitter,
separate instanced petiole/rachis/leaflet parts, bounded LOD assembly, grammar
overrides, catalog completion, and fail-closed invalid specifications.

The FruitShell gate proves that the canonical fruit source is a deterministic,
closed, positive-volume Revolve mesh with `BotanicalFruitOuter` tags rather than
a sphere proxy. The L-system gate proves bounded symbolic rewriting, turtle-stack
validation, deterministic `BranchGraph` lowering, radius conservation, tropism,
and the canonical `F -> F[+F]F[-F]F` grammar-to-scene path.

Supervisor-rendered vegetation evidence is generated with:

```bash
./tests/run_vegetation_visual_checks.sh
./tests/run_vegetation_plant_visual_checks.sh
./tests/run_vegetation_lsystem_visual_checks.sh
./tests/run_vegetation_environment_visual_checks.sh
```

Run all four visual gates together with:

```bash
./tests/run_vegetation_visual_matrix.sh
```

The environment visual gate renders the vine, scatter-region, and
space-colonization showcases twice each under supervisor control. It requires
byte-identical captures plus non-collapsed colored silhouettes and writes one
combined evidence record to
`tests/evidence/vegetation_environment_visual_2026-08-22.json`.

Each gate captures twice under a transient user-systemd unit when available,
requires byte-identical frames and GUI smoke/render/FOV/capture markers, and
rejects missing or collapsed botanical silhouettes.

```bash
./tests/run_vegetation_space_colonization_checks.sh
```

The crown-filling gate covers sphere, ellipsoid, cone, inverse-cone, cylinder,
dome, lobed and custom sampled crown volumes; deterministic attraction points;
bounded space colonization; obstacle deflection; crown containment; reconciled
branch radii; active growth tips; and immutable resolution evidence.

```bash
./tests/run_vegetation_vine_growth_checks.sh
```

The vine gate validates deterministic AABB surface attachment, contact and
offset modes, wall-seeking ivy, trellis tendrils, collision avoidance versus
permitted intersection, radius reconciliation, immutable graph evidence,
complexity ceilings, and fail-closed unsupported target geometry.

```bash
./tests/run_vegetation_scatter_region_checks.sh
```

The scatter gate validates deterministic surface density, minimum spacing,
scale variation, surface-normal and upright orientation rules, typed collision
layers and masks, placement ceilings, immutable evidence, and fail-closed
invalid or unsupported surfaces.

## MVGv1 vehicle acceptance

```bash
./tests/run_mvg_vehicle_checks.sh
```

This builds the canonical `MVG_Test_EV_001` modern EV fastback and verifies
package validation, derived datums, deterministic curves, mirrored shell
lofting, wheel assemblies, suspension corners, steering and travel bounds, LOD
reduction, stable hierarchy and mesh hashes, and PLY export.

## Complete P2 suite

```bash
./tests/run_p2_checks.sh
```

This runs every P0 and P1 gate, then:

1. `test_p2_time.py` checks the per-grammar time API, immutable built-in `t`, temporal function vocabulary, bounded rotation syntax, stable design seeding, one-clock preview behavior, quiet playback diagnostics, editor integration, and mathematical invariants.
2. `harness/p2_grammar_harness.cpp` compiles the actual patched `Grammar.cpp` and executes temporal functions, negative time, semantic failures, stable random values across time samples, time-dependent conditionals, explicit angle bounds, and dynamic bounds.
3. `examples/P2_time_showcase.p3d` is parsed and expanded by the runtime harness so the packaged demonstration cannot silently drift away from the implemented syntax.
4. Shell and Python test sources receive syntax checks.

AddressSanitizer and UndefinedBehaviorSanitizer are enabled automatically when supported. Clang is the default compiler. `CXX=g++` selects GCC, although sanitizer-instrumented GCC builds may be substantially slower on constrained systems.

## Editor architecture suite

```bash
./tests/run_editor_architecture_checks.sh
```

This runs the workspace/startup model, application-owned interface and presentation composition, explicit frame-cycle, interactive-loop, smoke-workflow, exit-authorization, and ordered-shutdown ownership, local persistence, native local-file dialog and unsaved-document replacement workflow, diagnostics, symbol index, grammar presentation, regeneration coordinator, preview controller and projection/FOV contracts, lighting and electrical control workflows, curved primitive geometry routing, bounded log, AI proposal review, credential-free remote-service contracts, part-catalog loading and fallback, launch option, and mesh export harnesses.

`run_editor_application_lifecycle_checks.sh` verifies that the application composes purpose-specific lifecycle objects, that exit authorization and editor focus restoration remain explicit model state, and that frame, smoke, interactive-loop, and shutdown responsibilities do not regress into `src/editor/presentation/GrammarEditorDocumentView.cpp`.

`run_lighting_controls_workflow_checks.sh` verifies preset, authored-light, copied-draft, removal, gizmo, switch, dimmer, validation, and exact evidence-publication semantics while rejecting direct canonical mutation and domain-service construction in the lighting panels.

`run_editor_diagnostics_workflow_checks.sh` verifies source-line indexing, existing syntax and compiler-diagnostic presentation, fail-closed range navigation, cursor and selection synchronization, and empty-document behavior. Its source gates reject diagnostics assembly and source-selection ownership in `src/editor/presentation/GrammarEditorDocumentView.cpp`, generic source-navigation callbacks in the diagnostics and symbol panels, direct presentation mutation of editor interaction state, duplicate diagnostic analyzers, and concrete workflow leakage through the runtime context.

`run_scene_preview_interaction_workflow_checks.sh` verifies viewport validity, collision-geometry primitive picking, light-over-primitive precedence, pointer selection, camera intents, source navigation, selection sanitation, camera fit, application ownership, and fail-closed outside-release behavior. Its source gates reject deterministic preview-controller ownership in presentation composition and canonical camera, light, or primitive-selection mutation in `ScenePreviewPanel`.

`run_scene_preview_rendering_workflow_checks.sh` verifies exact camera-frame construction, semantic overlay and outline upload, active material and light association, render-setting translation, texture-handle publication, timing statistics, and fail-closed invalid inputs. Its source gates require abstract runtime ownership and the concrete OpenGL workflow, session-owned texture inspection and mapping state, interaction reuse of the rendered camera frame, renderer-owned overlay evidence, removal of legacy panel procedures and free renderer wrappers, and Makefile plus architecture-suite registration.

`run_open_gl_preview_renderer_architecture_checks.sh` validates the private
OpenGL preview object model: application-owned runtime state, separate shader,
geometry, render-target, frame, pass, renderer, and cubemap owners, explicit
renderer-associated framebuffer capture, independent runtime state, absence of
hidden process-static renderer state, bounded source ceilings, Makefile
ownership, and the retained `build_base_vertex_data` and `draw_box`
compatibility boundaries.

`run_preview_shadow_projection_checks.sh` validates one three-layer directional
cascade range, four deterministic spot-shadow layers, explicit point-cube
deferral, texture-array ownership, layer-aware PCF sampling, authored bias
projection, and compatibility projection of the first rendered shadow.

`run_render_pipeline_upgrade_checks.sh` validates HDR scene storage, exact
quality-selection to multisample-count mapping, implemented bloom and ACES tone
mapping, FXAA activation, post-processing pass order, multi-shadow projection,
environment convolution ownership, opaque color/depth capture, scene-aware glass
refraction, multisample opaque view-normal ownership, deterministic view-space
SSAO, bilateral denoising, opaque-only AO composition before transparency, and
the removal of the former actionable lens-flare no-op.

`run_preview_environment_lighting_visual_checks.sh --refresh` renders the
advanced hatchback twice with procedural HDR environment lighting and once with
the environment disabled. It requires byte-identical environment captures, a
different control capture, successful runtime convolution readiness, no OpenGL
errors, and nontrivial 501x462 image evidence. Running it without `--refresh`
verifies the current implementation, scene, and retained PNG against
`tests/evidence/preview_environment_lighting_visual_2026-08-27.json`.

`run_preview_ambient_occlusion_visual_checks.sh --refresh` renders the modern
luxury residence twice with SSAO enabled and once with SSAO disabled. It requires
byte-identical enabled captures, a measured nonzero control delta, successful
runtime AO-resource readiness, no OpenGL errors, and nontrivial 501x462 image
evidence. Running it without `--refresh` verifies the current implementation,
scene, and retained PNG against
`tests/evidence/preview_ambient_occlusion_visual_2026-08-27.json`.

`run_application_logging_context_checks.sh` verifies typed severity and source ownership, bounded thread-safe retention, deterministic display text, publication to the correct terminal stream, empty-message rejection, injected runtime-diagnostic publication, terminal fallback, exact diagnostic-to-application source mapping, explicit application/runtime/renderer composition, concurrent publication, and typed console rendering. Its source gates reject logging ownership in `EditorInterfaceState`, direct `ApplicationLog` storage dependencies in workflow services, string-prefix severity inference, the removed legacy bridge and `LegacyRuntime` source, mutable global publisher or renderer state, free `debugout`/`errorout` production paths, and unregistered diagnostic sources or architecture checks.

`run_grammar_decomposition_architecture_checks.sh` verifies exact source token spans, logical-rule assembly, lexical-frame shadowing and restoration, semantic catalogs and call relationships, immutable expansion limits, mutable expansion budgets, deferred-material references, deterministic grammar random sampling, checked arithmetic precedence, captured and resampled values, temporal functions, function-vocabulary consistency, and fail-closed expression errors. Its source gates require the purpose-specific grammar models, contexts, relationships, parsing, semantic-analysis, recursive-expansion, diagnostic, snapshot-publication, sampling, semantic-inspection, and evaluation services; reject duplicate legacy owners in `src/Grammar.cpp`; require every direct grammar harness to link the extracted core; and enforce the bounded monolith reduction.

`run_spatial_construction_architecture_checks.sh` verifies scene spatial scope and construction state, authored parent-local transform derivation, explicit-parent mismatch diagnostics, containment ownership, declaration forwarding, admitted primitive binding, persistent blocking diagnostics, atomic published model references, explicit `SceneGenerationContext` composition and delegation, the deliberately retained finalization-to-positioning compatibility association, canonical standalone link registration, removal of the nested spatial construction state from `Context`, and bounded scene-context concentration.

`run_mesh_export_architecture_checks.sh` verifies the explicit texture projection basis and render vertex value models, exact eight-component vertex layout, dominant-axis and degenerate-normal UV projection, legacy texture-scale policy, cube-template and triangle-mesh assembly, authored and generated UV behavior, inverse-transpose normals, material filtering and pre-counting, removed-instance exclusion, deterministic preview/export parity, and empty export behavior. Its source gates require purpose-specific models and services in the native and standalone link manifests, preserve the `SceneGenerationContext` compatibility facade and existing writer ownership, reject the former anonymous UV and mesh-append procedures from `Context`, reject context back-references and file writing in the extracted services, migrate curved-geometry gates to the canonical owner, and enforce the reduced scene-context concentration boundary.

`run_scene_transform_scope_stack_checks.sh` verifies structural root-scope ownership, complete inherited-scope cloning, position-only fresh-scope creation, dual-transform and dual-axis preservation or reset, exact parent-object restoration, nested last-in-first-out behavior, empty-restoration stability, and repeated RAII destruction. Its source gates require explicit `SceneGenerationContext` composition and delegation, preserve the public compatibility methods and exact empty-pop diagnostic, register the model in native and standalone link manifests, reject the former raw pointer stack and manual deletion from `Context`, reject logging and spatial-declaration coupling, forbid ownership inheritance, and enforce the reduced scene-context concentration boundary.

`run_lighting_scene_declaration_architecture_checks.sh` verifies exact light, fixture, switch, and circuit diagnostics, stable declaration order, duplicate and empty-ID rejection, atomic light-plus-fixture admission, direct model compatibility wrappers, direct service behavior, and independent definition copy semantics. Its source gates require purpose-specific service ownership, by-value scene-context composition and delegation, a controlled service-to-model association, native and standalone link registration, removal of validation and transaction algorithms from the state model, removal of optional lighting state from `Context`, absence of renderer, persistence, evidence, evaluation, editor, or context dependencies, and the reduced scene-context concentration boundary.

`run_vehicle_joint_declaration_architecture_checks.sh` verifies ordered valid joint admission, every kinematic invalidity class, exact invalid and duplicate diagnostics, validation-before-duplicate ordering, graph non-mutation on rejection, direct graph duplicate-only compatibility, and independent graph copy semantics. Its source gates require purpose-specific declaration-service and validator composition, by-value context graph and service composition, context delegation, native and standalone registration, removal of optional graph state and validation orchestration from `Context`, preservation of graph duplicate ownership, absence of geometry, wheel-envelope, collision, rendering, evidence, editor, or context dependencies, and the reduced scene-context concentration boundary.

`run_scene_primitive_admission_architecture_checks.sh` verifies the scene-state inheritance model, canonical physical limits, pending next-primitive state and scoped reset relationship, exact material canonicalization and stable slots, authored simulation bounds, exact source spans, and explicit admission request/result values. Its source gates require the extracted scene models and admission service, reject duplicate pending-state, material, authored-bound, mass-resolution, and primitive-selection owners in `Context`, require every direct `Context.cpp` harness to link the canonical extracted sources, and enforce bounded reductions for `src/Context.cpp` and `include/Context.h`.

The architecture suite also runs `run_collision_positioning_checks.sh`, which verifies penetration correction for Cube, Cylinder, and Sphere instances, including mass-weighted movement, immovable anchors, and full containment.

`run_collision_simulation_architecture_checks.sh` verifies the canonical simulation state, normalized collision-axis and pair values, explicit collision-participation and lifecycle policies, shared primitive transform and collision-cache service, deterministic spatial-hash pairing, and explicit particle effects. Its source gates require broad-phase, detection, response, effects, and step orchestration services; reject the legacy collision, gravity, particle, transform-cache, and pair owners from `Context`; reject direct scene-context back-references from the extracted services; require every source in the native and standalone link manifests; and enforce the reduced `Context.cpp` concentration boundary.

The transform evidence layer adds exact matrix and grammar-to-scene verification:

```bash
./tests/run_transform_scope_checks.sh
./tests/run_transform_grammar_scene_checks.sh
```

These gates cover transform order, signed translation, all rotation axes, nested scope restoration, secondary and dual transforms, arithmetic and temporal expressions, deterministic random declarations, `R*`, `&name`, rule-header rerolls, and fail-closed diagnostics.

`run_example_collision_positioning_checks.sh` enforces the corpus convention that every intended-valid example contains runtime `I()` geometry and that `!I()` is reserved for one explicit floor, wall, foundation, slab, plaza, or anchor. Dynamic contact positioning still follows each resolved shape's collision policy.

## Application smoke suites

```bash
./tests/run_gui_smoke_check.sh
./tests/run_temporal_gui_smoke_check.sh
```

The first opens `examples/curved_primitives_showcase.p3d` and verifies canonical Cylinder/Sphere forms plus procedural aliases while rendering narrow, standard, and wide FOV values without changing document or generation state. The second starts with `examples/curved_primitives_temporal.p3d`, publishes an expression-driven curved-domain sample without changing its design nonce, then performs the same projection checks on that sampled scene.

## Transform visual and supervisor evidence

```bash
./tests/run_transform_visual_checks.sh
./tests/run_transform_visual_matrix.sh
./tests/run_transform_supervisor_matrix.sh
```

The orientation gate captures the asymmetric axis sentinel twice from the resolved preview framebuffer, requires byte-identical PPM output, and checks that the positive-Y marker is above the origin while the positive-X and positive-Z projections occupy their expected sides. The visual matrix captures every valid transform and expression evidence fixture, repeats deterministic static and temporal cases byte-for-byte, verifies that every preview contains nontrivial image evidence, and creates a labeled montage when ImageMagick is available. The intentionally session-random fixture receives a single visual smoke capture while its seeded determinism remains covered by the grammar-to-scene harness. Both matrix evidence and the montage are retained in the unique temporary directory reported by the command. The supervisor matrix runs numerical scope, grammar-to-scene, collision settlement, the 80-example positioning convention, visual orientation, the complete visual matrix, temporal preview, and invalid-axis fail-closed cases in collected transient user units when a user systemd bus is available.

## Curved primitive suites

```bash
./tests/run_curved_shape_model_checks.sh
./tests/run_curved_geometry_pipeline_checks.sh
./tests/run_curved_grammar_scene_checks.sh
```

These cover syntax/evaluation, alias equivalence, deterministic Cylinder and Sphere mesh generation, topology and volume evidence, transformed preview/export/picking parity, density rejection for open geometry, and production grammar-to-scene generation.

## AxialProfilev1 suites

```bash
./tests/run_axial_profile_geometry_checks.sh
./tests/run_axial_profile_grammar_scene_checks.sh
./tests/run_axial_profile_editor_checks.sh
./tests/run_axial_profile_gui_smoke_check.sh
./tests/run_axial_profile_temporal_gui_smoke_check.sh
./tests/run_axial_profile_evidence_checks.sh
```

These cover polygon validation, deterministic caps and contained steps, analytic volumes, topology hashes, caching, nested grammar expressions, scene preview/export parity, structured editor completion, inspector evidence, section overlays, ordinary and temporal GUI frames, benchmark recording, and the external Component 24 provenance gate.

## Authoritative release gate

```bash
./tests/run_release_checks.sh
```

This performs a clean warning-enabled application build, the inherited P0-P2 suite, every editor architecture harness, GUI and temporal smoke modes, deterministic transform orientation and matrix capture, the supervisor matrix, and whitespace validation.

## Complete P1 suite

```bash
./tests/run_p1_checks.sh
```

This runs every P0 gate and then:

1. `test_p1_semantics.py` checks parser side-effect removal, instance-owned runtime state, semantic preflight, checked expressions, explicit material intent, snapshot isolation, and matrix-authoritative pose handling.
2. `harness/p1_grammar_harness.cpp` compiles the actual patched `Grammar.cpp` against narrow contracts and executes lexical binding, random declaration, expression, range, arity, material, isolation, and failure-atomicity cases.
3. `harness/p1_scope_harness.cpp` compiles the actual patched `Scope.cpp` and tests cumulative transform/metadata coherence.
4. `generate_p1_context_transform_harness.py` extracts the exact patched dynamic transform functions from `Context.cpp`, then compiles and executes them.

## P0-only suite

```bash
./tests/run_p0_checks.sh
```

The stubs under `tests/harness` are test-only interface contracts. They are not replacements for the missing project headers.
# SMB-OMv1 Small Modern Building

Run the five-deep positioned building source and runtime acceptance gate:

```bash
tests/run_smb_omv1_checks.sh
```

The gate checks all 124 SMB object rules, complete L1-L5 template chains,
fail-closed configuration validation, semantic zero-geometry leaves, finite
bounds, and collision positioning for loose contents.

## SMB-OMv2 spatial model kernel

Run the immutable object-model and fail-closed graph construction gate:

```bash
tests/run_spatial_object_model_checks.sh
```

The gate covers identity, nested containment, parent/world frame composition,
interface-frame normalization, compatibility, collision masks, deterministic
constraint ordering, state transitions, evidence hashing, primitive bindings,
and typed failures for duplicates, missing parents, cycles, invalid frames,
undefined endpoints, and invalid bindings.

## SMB-OMv2 Spatial Query Evidence

`run_spatial_query_checks.sh` validates SMB-OMv2 P0 spatial queries: transformed
explicit AABBs, compound multi-primitive object bounds, candidate transforms
without scene mutation, deterministic distance and containment evidence, and
world projection of point, plane, axis, and object-boundary-face interfaces.

## SMB-OMv2 Spatial Positioning Evidence

`run_spatial_positioning_checks.sh` validates deterministic P0 assembly placement:
CubeY, Cylinder, and Sphere drops; sequential cabinet floor and wall constraints;
requested clearance; dependency and source-order determinism; immutable evidence
hashes; primitive binding commits; starting-overlap, maximum-travel, forbidden
obstacle, and excluded-layer failures; and transaction rollback without mutation.

## ModernLuxuryTownhousev1 camera and spatial acceptance

`run_modern_luxury_townhouse_v1_checks.sh` discovers the live procedural-shape
catalog, proves the grammar exercises all 45 available geometry primitives,
validates the 23-object spatial hierarchy, resolves four fail-closed support
constraints, and renders two byte-identical isometric captures. The GUI also
supports `--preview-object <spatial-object-id>` for isolated object framing and
the `.camera` walkthrough format for deterministic single-frame or sequence
captures.

`../tools/render_taxonomy_object_views.sh` renders every comparison-enabled
townhouse taxonomy object from front, back, left, right, top, and bottom views.
It isolates direct object geometry, hides the editor grid, assembles 21 six-view
camera cards, pairs them with the 21 ImageGen reference cards, and records 126
advisory normalized-RMSE comparisons. `../tools/render_camera_walkthrough.sh`
renders the deterministic `.camera` path as PNG frames and an H.264 fly-through,
with frame hashes and a machine-readable validation manifest.

`run_preview_object_camera_view_checks.sh` validates direct-object and hierarchy
isolation, all six orthographic view names, deterministic primitive selection,
and fail-closed behavior for container-only and semantic-only objects.

`run_preview_object_camera_transition_checks.sh` validates object-centered
camera destinations, smoother-step target motion, shortest-path angle changes,
stable projection during movement, projection switching at arrival, exact
center-height side views, and fail-closed invalid bounds or timing.

`run_camera_navigation_scripting_checks.sh` validates typed object-waypoint
camera scripts, deterministic parser/writer round trips, exact generation from
grammar-derived spatial-object geometry bindings, smooth walkthrough and
flythrough transitions, holds, completion, and fail-closed missing-object
diagnostics.

## Full SMB-OMv2 building acceptance

```bash
./tests/run_smb_omv2_checks.sh
./tests/run_smb_omv2_visual_checks.sh
./tests/run_smb_omv2_supervisor_matrix.sh
```

## SMB-OMv2.1 all-building-object knowledge model

`run_smb_omv21_all_objects_checks.sh` regenerates the JSON, CSV, and typed C++
catalog twice, proves byte identity, validates the 124-profile catalog, runs the
native knowledge-model kernel, reuses the accepted SMB-OMv2 spatial/runtime gate,
and writes the requirement-to-evidence audit to
`tests/evidence/smb_omv21_all_objects_acceptance_2026-08-24.json`.

```bash
./tests/run_smb_omv21_all_objects_checks.sh
```

`run_smb_omv21_visual_checks.sh` enables only the opt-in building knowledge
overlays, selects the kitchen sink profile, captures the preview twice, proves
byte identity, retains the reviewed first capture, and records the native model
and profile hashes alongside the pixel evidence.

```bash
./tests/run_smb_omv21_visual_checks.sh
./tests/run_smb_omv21_supervisor_matrix.sh
```

The OMv2.1 supervisor matrix runs launch-option parsing, the native building
knowledge kernel, deterministic all-object generation and acceptance, SMB-OMv1
compatibility, the accepted SMB-OMv2 visual baseline, and the opt-in knowledge
overlay capture in collected transient user units when available. It retains
per-case log hashes and authoritative acceptance/visual artifact hashes in
`tests/evidence/smb_omv21_supervisor_matrix_2026-08-24.json`.

The completed requirement-to-evidence audit is documented in
`SMB_OMV21_ALL_BUILDING_OBJECTS_VERIFICATION.md`. Its machine-readable release
record, exact artifact hashes, counts, invalid-fixture coverage, and explicit
pending-evidence limits are retained in
`tests/evidence/smb_omv21_all_objects_completion_2026-08-24.json`.

The source/runtime gate regenerates the spatially aware building from the
unchanged 124-object SMB-OMv1 fixture, rejects centered `Cube` geometry, verifies
178 typed interfaces, 28 interface-level connections, 3 deterministic P0
constraints, 143 primitive ownership bindings, and all priority building
validity predicates. It records collision-query work, residuals, clearances,
resolution hashes, aggregate topology, and source artifact hashes in
`tests/evidence/smb_omv2_acceptance_2026-08-21.json`.

The visual gate opens the full building with `--spatial-overlays`, captures the
resolved preview twice, and requires byte-identical images plus the GUI render,
FOV, smoke, and capture markers. The supervisor matrix runs the object model,
queries, positioning and invalid fixtures, grammar integration, editor evidence,
SMB-OMv1 compatibility, full SMB-OMv2 acceptance, and visual capture under
collected transient user units when available.

The architectural contract and explicit P0 limits are documented in
`docs/SMB_OMv2_SPATIALLY_AWARE_BUILDING.md`.

## GPU preview material records

`run_gpu_preview_material_record_checks.sh` validates the nine-`vec4` std430
material record layout, deterministic property normalization, texture-dependent
height and emission behavior, and authored projection-axis preservation. The
render pipeline gate additionally requires binding-point 3 SSBO ownership and
rejects the former repeated scalar/vector material uniforms.

`run_preview_view_frustum_checks.sh` validates six normalized clip planes,
identity and perspective visibility decisions, fail-open missing bounds,
deterministic interleaved-vertex spheres, and conservative dynamic-cube bounds.
The render pipeline applies those relationships to opaque, transparent,
outline, and dynamic preview submissions while leaving shadow casters unculled
by the camera frustum.

`run_gpu_dynamic_cube_instance_record_checks.sh` validates the four-`mat4`,
seven-`vec4` std430 instance layout and its deterministic mapping from dynamic
cube simulation state. The render pipeline gate requires binding-point 4 SSBO
ownership, stable material grouping, instanced scene and shadow draws, and the
absence of the former per-cube transform uniforms.

`run_preview_advanced_render_path_assessment_checks.sh` validates the explicit
Forward+ and indirect-drawing candidate boundaries. A candidate remains
separate from a justified replacement: the service keeps both justification
flags false until comparative runtime evidence exists.

`run_vegetation_renderer_lod_pipeline_checks.sh` validates deterministic
protected-edge structural decimation, complete-organ reduction with bounded
area preservation, semantic/spatial meshlet construction, LOD bundle evidence,
projected-error selection, semantic bias, and hysteresis.

`run_vegetation_renderer_scene_integration_checks.sh` validates resolved
representation ownership, transformed indexed uploads, meshlet bounds and error
scaling, per-object batching, compatibility fallback counts, near/far and
hysteresis transitions, fail-closed representation ordering, frame evidence,
indexed OpenGL submission ownership, and selection-before-shadow ordering.
Vegetation LOD remains opt-in until the hardware A/B benchmark is refreshed.

`run_preview_render_performance_checks.sh` verifies retained GPU timestamp,
submission, memory, culling, and dynamic-instance evidence. Refresh mode runs
the residence workload and a 64-instance deformable-cube fixture, then records
measured medians and the current source, scene, and binary hashes without
treating timing values as byte-stable fixtures.
