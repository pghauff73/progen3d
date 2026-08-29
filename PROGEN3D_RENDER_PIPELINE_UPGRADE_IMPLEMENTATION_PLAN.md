# ProGen3D Render Pipeline Upgrade Implementation Plan

## Document Control

- **Project:** ProGen3D preview renderer
- **Plan date:** August 27, 2026
- **Target executable:** `progen3d-editor-gui`
- **Current implementation loop:** Loop 5 — performance and scalability
- **Compatibility boundary:** Preserve the existing `PreviewRenderer` interface, scene-resource ownership, preview capture behavior, PBR material model, and published-scene authority.

## 1. Objective

Upgrade the OpenGL 4.6 preview renderer through explicit, UML-readable render-pass and resource ownership while preserving current editor behavior.

The completed pipeline should provide:

1. configurable anti-aliasing that matches the selected quality level;
2. an HDR scene target;
3. deterministic HDR-to-LDR tone mapping and gamma conversion;
4. implemented FXAA and bloom rather than presentation-only controls;
5. explicit availability status for effects that are not implemented;
6. multi-shadow rendering that follows the accepted shadow assignment model;
7. improved environment lighting and transparent-surface rendering;
8. measured performance evidence before adopting Forward+ or indirect drawing; and
9. deterministic validation through architecture gates, native builds, bounded GUI smokes, and representative visual evidence.

## 2. Current Pipeline

The current renderer composes:

- `OpenGlPreviewShaderProgramService`;
- `OpenGlPreviewGeometryResourceService`;
- `OpenGlPreviewRenderTargetService`;
- `OpenGlPreviewFrameRenderingService`;
- `OpenGlPreviewShadowMapPass`;
- `OpenGlPreviewScenePass`; and
- `OpenGlPreviewResolvePass`.

The current code already provides GLSL 4.60 PBR shading, material map sets, environment reflection, a GL46 light SSBO, one primary shadow map, sorted premultiplied-alpha transparency, overlays, outlines, and RGB capture.

The current gaps are:

- the scene and resolved color targets use `GL_RGBA8` rather than HDR storage;
- the target is always allocated with four samples even when the UI requests another mode;
- FXAA, bloom, exposure, and gamma controls cross the workflow boundary but do not produce their named effects;
- SSAO and lens flare controls imply functionality that has no render pass;
- the accepted multi-light shadow assignment model is projected into one rendered shadow map; and
- render statistics report total CPU submission time but not per-pass GPU time.

## 3. Target Object Model

`OpenGlPreviewRenderer` remains the concrete `PreviewRenderer` and composition root for renderer-owned resources.

### Render-state classes

- `OpenGlPreviewShaderProgramState` owns OpenGL program identities.
- `OpenGlPreviewGeometryResourceState` owns uploaded geometry identities.
- `OpenGlPreviewRenderTargetState` owns scene, HDR resolve, display, depth, and shadow targets.
- `OpenGlPreviewRenderRuntime` composes the three states and the light buffer.

### Service classes

- `OpenGlPreviewShaderProgramService` owns scene-program compilation.
- `OpenGlPreviewPostProcessingShaderProgramService` owns post-processing-program compilation.
- `OpenGlPreviewRenderTargetService` owns target creation, recreation, readback, and release.
- `OpenGlPreviewFrameRenderingService` orchestrates one frame without owning OpenGL resources.

### Render-pass classes

- `OpenGlPreviewShadowMapPass` renders accepted primary shadow evidence.
- `OpenGlPreviewScenePass` renders HDR scene geometry.
- `OpenGlPreviewResolvePass` resolves multisample HDR color.
- `OpenGlPreviewPostProcessingPass` performs bloom, tone mapping, gamma conversion, and optional FXAA into the display target.

Later loops will split additional real passes only when their inputs and outputs exist. Placeholder pass classes are prohibited.

## 4. Step-by-Step Implementation

## Loop 1 — HDR Presentation and Quality-Control Truthfulness

1. Add this implementation plan and record the current renderer file hashes.
2. Make `PreviewRenderSettings` resolve the requested multisample count and FXAA state deterministically.
3. Extend `OpenGlPreviewRenderTargetState` with an HDR resolve texture, display framebuffer, and allocated sample count.
4. Change scene color storage to `GL_RGBA16F`.
5. Allocate 1x, 2x, 4x, or 8x targets according to the selected anti-aliasing mode and supported OpenGL limit.
6. Move multisample resolve responsibility into `OpenGlPreviewResolvePass`.
7. Add `OpenGlPreviewPostProcessingShaderSourceCatalog`.
8. Add `OpenGlPreviewPostProcessingShaderProgramService`.
9. Add `OpenGlPreviewPostProcessingPass` with implemented bloom, exposure, tone mapping, gamma conversion, and FXAA.
10. Keep `previewTextureId()` and RGB capture bound to the final LDR display texture.
11. Mark SSAO and lens flare as planned but unavailable instead of exposing active no-op controls.
12. Extend renderer architecture and runtime-state gates.
13. Build the native GUI and run focused preview, renderer, resource, lighting, and capture checks.

### Loop 1 exit gate

- Every enabled quality control has an implemented renderer effect.
- The allocated MSAA sample count follows the selected mode.
- Scene rendering occurs in HDR storage.
- The final ImGui texture and captured RGB pixels are tone-mapped LDR output.
- Existing PBR, transparent-surface, lighting, overlay, outline, and capture contracts remain intact.

## Loop 2 — Shadow Projection Completion

1. Introduce `OpenGlPreviewShadowTargetCollection`.
2. Render accepted directional and spot assignments into a texture array or deterministic atlas.
3. Add cascades for the primary directional light.
4. Add bounded PCF filtering and bias controls.
5. Bind each `GpuLightRecord` to its rendered shadow allocation.
6. Add deterministic shadow-allocation and visual-evidence checks.

## Loop 3 — Environment Lighting and Glass

1. Add irradiance, prefiltered specular, and BRDF integration resources.
2. Separate environment generation from environment-lighting convolution.
3. Add opaque-scene color and depth inputs for physical glass refraction.
4. Preserve sorted premultiplied transparency as the compatibility path.
5. Evaluate weighted blended order-independent transparency only against measured intersecting-transparent workloads.

## Loop 4 — Screen-Space Effects

1. Add a normal/depth prepass or explicit MRT scene outputs.
2. Implement `OpenGlPreviewAmbientOcclusionPass`.
3. Add temporal or spatial denoising appropriate to the preview workload.
4. Enable the SSAO UI only after deterministic pass and restoration checks exist.
5. Implement lens artifacts only from measured bright-light screen projections; otherwise remove the deferred control.

## Loop 5 — Performance and Scalability

1. Add GPU timestamp queries per pass.
2. Record draw calls, submitted triangles, visible lights, target memory, and upload volume.
3. Move repeated material parameters into structured GPU storage.
4. Add instancing for repeated geometry.
5. Add frustum culling.
6. Adopt Forward+ only when measured visible-light workloads justify it.
7. Adopt indirect drawing only when draw-call evidence justifies the added execution path.

## 5. Validation Strategy

Each loop must run the narrowest relevant gates first, then the broader architecture and GUI gates:

```bash
./tests/run_open_gl_preview_renderer_architecture_checks.sh
./tests/run_scene_preview_rendering_workflow_checks.sh
./tests/run_scene_preview_resource_checks.sh
./tests/run_preview_session_checks.sh
make -j2 progen3d-editor-gui
./tests/run_gui_smoke_check.sh
./tests/run_temporal_gui_smoke_check.sh
```

Visual comparisons must use tolerant metrics and stable same-machine captures rather than cross-driver exact pixel hashes. Source manifests, shader contracts, framebuffer formats, ownership, and pass order remain deterministic gates.

## 6. Completion Standard

The render-pipeline upgrade is complete only when all five loops are implemented and their requirement-to-evidence records are current. A successful Loop 1 does not establish multi-shadow, physical refraction, SSAO, Forward+, or release completion.

## 7. Loop 1 Implementation Report

### Status

Loop 1 was implemented and validated on Thursday, August 27, 2026.

### Implemented object model

- `OpenGlPreviewRenderTargetState` now owns multisample HDR scene color, single-sample HDR resolve color, LDR post-processing color, and final display color as distinct resources.
- `OpenGlPreviewRenderTargetService` recreates those resources when viewport dimensions or the requested multisample count changes.
- `OpenGlPreviewPostProcessingShaderSourceCatalog` owns the full-screen, bloom/tone-mapping, and FXAA shader sources.
- `OpenGlPreviewPostProcessingShaderProgramService` owns compilation, linking, diagnostics, and release for the post-processing programs.
- `OpenGlPreviewPostProcessingPass` performs bloom, ACES tone mapping, gamma conversion, and optional FXAA.
- `OpenGlPreviewResolvePass` now owns the multisample HDR resolve operation rather than leaving it hidden in the scene pass.
- `PreviewRenderSettings` deterministically maps Off, FXAA, MSAA 2x, MSAA 4x, and MSAA 8x to the required scene sample count.

### Corrected behavior

- Scene color is rendered into `GL_RGBA16F` storage.
- The requested MSAA level is clamped to the OpenGL implementation's supported sample count.
- FXAA uses a single-sample HDR scene and a real LDR edge-resolution pass.
- Bloom samples bright HDR neighborhoods using the configured threshold and intensity.
- Exposure and gamma are consumed by the post-processing shader.
- The legacy PBR shader no longer performs its own Reinhard mapping or gamma conversion; lighting remains linear until the post-processing stage.
- Background, grid, semantic overlay, and outline colors are converted to linear values before HDR composition.
- SSAO and lens flare remain disabled and are explicitly labeled as later-loop capabilities.
- `previewTextureId()` and framebuffer capture continue to expose the final LDR display texture.

### Requirement-to-evidence audit

| Loop 1 requirement | Current evidence | Result |
|---|---|---|
| Configurable sample mapping | `preview_render_quality_harness.cpp` checks Off, FXAA, 2x, 4x, and 8x | Passed |
| Supported-sample clamping | `OpenGlPreviewRenderTargetService::supportedMultisampleCount()` and source gate | Passed |
| HDR scene storage | Renderer source gate requires `GL_RGBA16F` | Passed |
| Implemented bloom | Post-processing shader and source gate require `sample_bloom` | Passed |
| Implemented tone mapping and gamma | Post-processing shader and source gate require `aces_filmic_tone_map` and gamma uniform | Passed |
| Implemented FXAA | FXAA shader, activation mapping, and source gate | Passed |
| Separate resolve and presentation ownership | Renderer architecture gate requires resolve followed by post-processing | Passed |
| Final LDR capture | Visual capture produced a valid 501x462 raw PPM after post-processing | Passed |
| Full native linkage | `make -j2 progen3d-editor-gui` | Passed |
| Standard OpenGL runtime | Visual capture reported FOV, rendered-frame, visual-capture, and GUI smoke markers | Passed |
| Temporal compatibility | `run_temporal_gui_smoke_check.sh` reported all temporal and GUI markers | Passed |
| Preview model compatibility | `run_preview_session_checks.sh` | Passed |
| Preview workflow compatibility | `run_scene_preview_rendering_workflow_checks.sh` | Passed |
| Preview resource compatibility | `run_scene_preview_resource_checks.sh` | Passed |
| Material resource compatibility | `run_material_resource_architecture_checks.sh` | Passed |
| Lighting compatibility | `run_lighting_model_checks.sh` and `run_lighting_controls_workflow_checks.sh` | Passed |
| Diagnostic ownership compatibility | `run_application_logging_context_checks.sh` | Passed |

### Evidence identities

- Native GUI binary: 290,224,792 bytes; SHA-256 `2748fc7aaa466bd34458b3493dcbe71cffadb598ace2719d8791ea5552969b4d`.
- Tone-mapped visual capture: 694,401 bytes; SHA-256 `783d11b72531a1950deec64f7d94cb8839a7e6adc337f7dfcc2430f1e3f6527f`.
- Capture dimensions: 501x462.
- Source formatting, shell syntax, target freshness, and renderer ownership ceilings passed.

### Next implementation loop

Loop 2 will complete projection of the existing shadow-assignment object model into rendered resources:

1. inspect `ShadowAssignmentPlan`, `PreviewLightingFrame`, and `GpuLightRecord` as the authoritative allocation contracts;
2. introduce `OpenGlPreviewShadowTargetCollection` with deterministic texture-array ownership;
3. render each accepted directional or spot allocation into its assigned layer;
4. preserve the existing primary-shadow compatibility projection while migrating shader sampling;
5. add bounded PCF sampling and purpose-specific bias settings;
6. add model, source-contract, shader, and runtime harnesses;
7. run lighting, renderer, full GUI, temporal, and representative modern-residence visual checks; and
8. record a new requirement-to-evidence audit without claiming Loops 3-5 complete.

## 8. Loop 2 Implementation Report

### Status

Loop 2 was implemented and validated on Thursday, August 27, 2026.

### Implemented object model

- `PreviewShadowMapFrame` represents one rendered shadow projection, including its light identity, texture-array layer, cascade relationship, projection matrix, authored resolution request, softness, and bias values.
- `ShadowAssignmentRecord` preserves its authoritative type-local storage allocation and now separately records the rendered texture layer range owned by the current backend.
- `PreviewLightingFrame` aggregates ordered `PreviewShadowMapFrame` values, reports required layer capacity and resolution, and retains the first rendered projection through the existing primary-shadow compatibility fields.
- `OpenGlPreviewShadowTargetCollection` owns the framebuffer and depth texture array independently of the HDR preview render targets.
- `OpenGlPreviewShadowTargetService` owns allocation, layer attachment, framebuffer validation, supported-resolution bounding, and release of the shadow texture array.
- `OpenGlPreviewShadowMapPass` is associated with the shadow-target service and renders every accepted projection into its assigned layer.
- `OpenGlPreviewRenderer` composes the preview-target and shadow-target services as separate renderer lifecycle owners.

### Corrected behavior

- The accepted directional-light budget now projects one directional light into three ordered cascade layers.
- The accepted spot-light budget now projects four spot lights into deterministic layers 3 through 6.
- Allocated point-light cube shadows remain explicit but are not falsely projected into the 2D texture-array backend.
- `GpuLightRecord::metadata.y` identifies the first rendered shadow layer and `metadata.w` identifies the light's layer count.
- The scene shader selects directional cascades by camera distance and directly samples each accepted spot layer.
- Shadow sampling uses a bounded 3x3 PCF kernel with authored softness, constant bias, slope bias, and normal bias projected per layer.
- Scene and dynamic-cube shadow casters are rendered for every accepted layer.
- The former single `shadow_fbo`, `shadow_depth_texture`, `sampler2D shadow_map`, primary-only shader branch, and vertex-to-fragment primary shadow position have been removed from current source.
- Texture-array and cubemap bindings are restored after the scene pass.
- The August 22 residence lighting evidence remains unchanged; the upgraded backend writes a new August 27 v2 evidence record.

### Requirement-to-evidence audit

| Loop 2 requirement | Current evidence | Result |
|---|---|---|
| Dedicated multi-shadow resource owner | Architecture gate requires `OpenGlPreviewShadowTargetCollection` and `OpenGlPreviewShadowTargetService` | Passed |
| Deterministic directional cascade range | `preview_shadow_projection_harness.cpp` requires layers 0-2 with monotonically increasing coverage | Passed |
| Deterministic spot-shadow range | Harness requires accepted spots on layers 3-6 | Passed |
| Explicit unsupported point projection | Harness requires cube-array allocation with no rendered 2D layer or GPU shadow flag | Passed |
| Layer-aware GPU relationship | Harness requires `metadata.y` first-layer and `metadata.w` layer-count values | Passed |
| Texture-array allocation and layer attachment | Source gate requires `GL_TEXTURE_2D_ARRAY` and `glFramebufferTextureLayer` | Passed |
| Every accepted projection rendered | Source gate requires iteration over `PreviewShadowMapFrame` and `bindLayerForWriting()` | Passed |
| Layer-aware shader sampling | Source gate requires `sampler2DArray`, projection arrays, and `select_shadow_layer` | Passed |
| Bounded PCF and authored bias projection | Shader source and source gate require 3x3 sampling, softness, constant, slope, and normal bias arrays | Passed |
| Primary compatibility projection | Harness requires the first rendered frame to populate the primary compatibility fields | Passed |
| Residence multi-shadow evidence | August 27 v2 evidence records three rendered spot assignments on layers 3-5 | Passed |
| Full native linkage and freshness | `make -j2 progen3d-editor-gui` followed by `make -q progen3d-editor-gui` | Passed |
| Standard OpenGL runtime | `run_gui_smoke_check.sh` reported FOV, rendered-frame, and GUI smoke markers | Passed |
| Temporal compatibility | `run_temporal_gui_smoke_check.sh` reported temporal, FOV, rendered-frame, and GUI smoke markers | Passed |
| Representative residence capture | Direct visual test loaded 105 rules and produced a valid 501x462 capture | Passed |
| Preview model and workflow compatibility | Preview session, rendering workflow, and resource gates | Passed |
| Lighting and material compatibility | Lighting model, lighting controls, and material resource gates | Passed |
| Diagnostic ownership compatibility | Application logging context gate | Passed |
| Source and script hygiene | `git diff --check`, shell syntax, architecture ceilings, and source registration | Passed |

### Evidence identities

- Native GUI binary: 290,373,264 bytes; SHA-256 `1a9511ae1b61ec904ad7a7bf074365426bd112c919346bc2afff8359d47ae608`.
- Modern residence visual capture: 694,401 bytes; SHA-256 `0d1d870eff3ea81b3078a32dabe2c97fe3404131015af7815fb1b64c5cdce948`.
- Capture dimensions: 501x462; runtime pixel hash `3480326198373706539`.
- August 27 lighting evidence: 17,616 bytes; SHA-256 `3a74845e2e9a3605eb75914757530318bf5b29d83364d0e211b8f8f821f82243`.
- Preserved August 22 lighting evidence SHA-256: `76c56b91b3a5cf6ec3336536110fff3ba28588dd918b8c5eaa97df9bcb5e92f3`.
- Existing unrelated compiler warnings remain in `include/grammar.h`, `include/Solution.h`, and `src/Mesh.cpp`.

### Remaining limits

- Point-light cube-map shadow rendering remains a later extension; Loop 2 fails closed instead of substituting an incorrect 2D projection.
- The directional cascades use deterministic preview-oriented coverage rather than stabilized texel snapping or blend bands.
- Shadow-map resolution is shared across the texture array and bounded to 2048 for preview memory control.
- Loops 3-5 remain unimplemented and no release or full render-pipeline completion claim is made.

### Next implementation loop

Loop 3 will upgrade environment lighting and glass through the following steps:

1. audit current cubemap generation, PBR environment sampling, transparency, and material transmission contracts;
2. introduce explicit irradiance, prefiltered-specular, and BRDF-integration resource models;
3. separate environment-source generation from environment-lighting convolution services;
4. bind diffuse irradiance and roughness-dependent specular image-based lighting in the opaque PBR path;
5. introduce opaque scene color and depth inputs for physical glass refraction while preserving sorted premultiplied transparency as the compatibility path;
6. add deterministic shader, resource-lifecycle, material, and capture gates;
7. run the renderer, material, preview, GUI, temporal, and representative glass-scene validation matrix; and
8. publish a Loop 3 requirement-to-evidence audit without claiming screen-space or scalability loops complete.

## 9. Loop 3 Implementation Report

### Status

Loop 3 was implemented and validated on Thursday, August 27, 2026.

### Implemented object model

- `OpenGlPreviewEnvironmentLightingState` owns the derived irradiance cubemap, prefiltered-specular cubemap, BRDF integration texture, convolution framebuffer, and convolution depth renderbuffer while associating them with one non-owned source cubemap revision.
- `OpenGlPreviewEnvironmentLightingShaderSourceCatalog` owns bounded diffuse convolution, GGX prefilter, and split-sum BRDF integration shader sources.
- `OpenGlPreviewEnvironmentLightingShaderProgramService` owns compilation, linking, diagnostics, and release for the three environment programs.
- `OpenGlPreviewEnvironmentLightingService` allocates and renders the derived environment resources only when the explicit source texture revision changes, preserves surrounding OpenGL state, and never assumes ownership of the source cubemap.
- `OpenGlPreviewRenderTargetState` now owns a single-sample opaque HDR color texture and depth-stencil texture in addition to the multisample scene target and final resolve targets.
- `OpenGlPreviewOpaqueSceneCapturePass` copies opaque color and depth before transparent rendering.
- `OpenGlPreviewScenePass` composes the opaque capture between opaque and sorted premultiplied transparent drawing.
- `PreviewCubemapGenerator` now owns source-cubemap release; presentation code no longer calls `glDeleteTextures` directly.
- `PreviewRenderSettings` and `RenderConfiguration` carry an explicit cubemap revision so reused OpenGL texture names cannot falsely preserve stale convolution products.

### Implemented rendering behavior

- Procedural enhanced sky cubemaps retain linear HDR radiance; tone mapping remains the final presentation pass responsibility.
- Diffuse image-based lighting samples a dedicated irradiance cubemap.
- Roughness-dependent specular image-based lighting samples a five-level GGX-prefiltered cubemap.
- The environment BRDF term samples a generated two-channel integration texture, with the former analytic approximation retained only as a fail-safe fallback.
- Transmission samples the opaque HDR scene through an IOR-, thickness-, normal-, and roughness-dependent screen offset.
- Opaque depth validates that a sampled surface is behind the glass fragment before scene color is accepted; invalid samples fall back to the environment.
- Material attenuation color and thickness remain authoritative for transmitted absorption.
- Sorted back-to-front premultiplied-alpha transparency remains the compatibility path.
- Weighted blended order-independent transparency was not adopted because no measured intersecting-transparent workload currently justifies replacing the compatibility path.
- The smoke-only `--environment-lighting-smoke` option generates a bounded source cubemap and verifies renderer-observable convolution readiness after a real frame.

### Requirement-to-evidence audit

| Loop 3 requirement | Current evidence | Result |
|---|---|---|
| Explicit environment resource owner | Architecture gate requires `OpenGlPreviewEnvironmentLightingState` and renderer-composed environment services | Passed |
| Diffuse irradiance convolution | Source gate requires `renderIrradiance` and the bounded irradiance shader | Passed |
| GGX prefiltered specular convolution | Source gate requires `importance_sample_ggx`, five mip levels, and `renderPrefilteredSpecular` | Passed |
| BRDF integration texture | Source gate requires the BRDF integration shader, program, texture, and render phase | Passed |
| Revision-safe source association | Runtime harness requires default/reset revision state; smoke path observes readiness for the exact texture and revision | Passed |
| Linear HDR source radiance | Source gate requires the retained-HDR generation contract and final-pass tone-mapping ownership | Passed |
| Service-owned source release | Source gate requires `releaseCubemap`; presentation and shutdown use the service | Passed |
| Opaque color and depth inputs | Target and pass gates require the HDR color texture, depth-stencil texture, and color-plus-depth blit | Passed |
| Scene-behind refraction validation | Shader gate requires `scene_surface_is_behind_glass` before opaque scene color is accepted | Passed |
| IOR, thickness, attenuation, and roughness projection | Current scene shader projects each material property into the transmission result | Passed |
| Compatibility transparency retained | Current pass retains depth-write disable, back-to-front sorting, and `GL_ONE, GL_ONE_MINUS_SRC_ALPHA` blending | Passed |
| No unjustified OIT replacement | Source contains no weighted blended accumulation/revealage path; the accepted sorted path remains authoritative | Passed |
| Deterministic live glass-scene evidence | Two environment-enabled hatchback captures are byte-identical and differ from the disabled control | Passed |
| Runtime shader and resource execution | `PROGEN3D_GUI_ENVIRONMENT_LIGHTING_PASSED` follows actual generation, convolution, and rendering under Xvfb | Passed |
| Native linkage and freshness | `make -j2 progen3d-editor-gui` succeeded after the final source changes | Passed |
| Standard and temporal GUI compatibility | Ordinary and temporal GUI smoke gates reported their required success markers | Passed |
| Representative residence compatibility | Current 105-rule residence capture completed at 501x462 without OpenGL errors | Passed |
| Preview, material, lighting, logging, and editor ownership compatibility | Focused repository-native workflow, lifecycle, material, lighting, logging, and composition gates | Passed |
| Source and script hygiene | `git diff --check`, shell syntax, architecture ceilings, source registration, and retained-evidence verification | Passed |

### Evidence identities

- Native GUI binary: 290,666,880 bytes; SHA-256 `6d30b9ea596d4bf80b11d5c1825c12544c570d050baac14ec302376cde533c57`.
- Environment-enabled hatchback capture: 694,401 bytes; SHA-256 `e6525627858e51673a71a7ed12bfddc6a5d01a934f6ef732b0c743d1f761c802`.
- Disabled environment control capture SHA-256: `ea3852f576a5980d25b804e5176be2b2ae22ff56282d28e941375f80a7b3f9f9`.
- Environment capture dimensions: 501x462; runtime pixel hash `7193995482140735809`.
- Retained environment PNG: 111,297 bytes; SHA-256 `a8956373d3366ecf1c7846ebf3d8ac91fc1641ff1acd4de826a008310b0db7db`.
- Environment evidence record: 1,286 bytes; SHA-256 `6f98f4a913c515165ce238d1f94642ce73fd68f5c1387cac0ae0349d238ce292`.
- Environment implementation fingerprint: `6e98619f5e3f7688aaca073203415028b4676f924284b4690d499a65e41f6ed0`.
- Current residence capture: 694,401 bytes; SHA-256 `350fac9e10a37fe3a874ea18d76f3f80c14f3fc92466817480a9069a4354e346`; runtime pixel hash `3903638578799107957`.
- August 27 residence lighting evidence remains 17,616 bytes with SHA-256 `3a74845e2e9a3605eb75914757530318bf5b29d83364d0e211b8f8f821f82243`.
- The accepted August 22 residence visual evidence was verified without rewriting its frozen captures.
- Existing unrelated compiler warnings remain in `include/grammar.h`, `include/Solution.h`, and `src/Mesh.cpp`.

### Remaining limits

- Environment preprocessing is synchronous when a new source revision is generated; no asynchronous upload or convolution queue exists yet.
- The current source generator is procedural; equirectangular HDR import and authored environment assets remain future work.
- Refraction uses one opaque color/depth snapshot and cannot reconstruct multiple transparent layers or objects hidden only behind other transparent surfaces.
- Transparent surfaces remain order-dependent for intersections; Loop 3 intentionally retains the established sorted compatibility path.
- Screen-space normals, SSAO, AO denoising, and bright-light lens artifacts remain unimplemented and their UI controls remain unavailable.
- Per-pass GPU timing, instancing, culling, Forward+, and indirect drawing remain Loop 5 work.
- Point-light cube-map shadow rendering remains deferred from Loop 2.
- No release or full five-loop render-pipeline completion claim is made.

### Next implementation loop

Loop 4 will implement truthful screen-space effects through the following steps:

1. audit current scene outputs, opaque snapshot ordering, viewport resize behavior, and the disabled SSAO and lens-flare controls;
2. introduce an explicit screen-space geometry resource model for resolved opaque normals and the existing opaque depth texture;
3. extend opaque scene rendering with a normal MRT while preserving HDR color, shadow, material, and transparent compatibility;
4. add `OpenGlPreviewAmbientOcclusionShaderSourceCatalog`, program service, target resources, and `OpenGlPreviewAmbientOcclusionPass`;
5. implement bounded hemisphere sampling plus edge-aware spatial denoising using the resolved depth and normal inputs;
6. composite ambient occlusion onto opaque scene color before transparent surfaces and overlays;
7. enable the SSAO control only after deterministic allocation, restoration, resize, disable-path, and visual gates pass;
8. measure bright-light screen projections and either implement a bounded lens-artifact pass or remove the deferred control rather than retaining a no-op; and
9. run the renderer, material, lighting, GUI, temporal, residence, and new SSAO control-capture matrix before publishing the Loop 4 audit.

## 10. Loop 4 Implementation Report

### Status

Loop 4 was implemented and validated on Thursday, August 27, 2026.

### Implemented object model

- `OpenGlPreviewAmbientOcclusionTargetState` owns two full-resolution single-channel textures and their framebuffers for occlusion generation and bilateral denoising.
- `OpenGlPreviewAmbientOcclusionShaderSourceCatalog` owns the deterministic view-space sampling, bilateral blur, and multiplicative composite shader sources.
- `OpenGlPreviewAmbientOcclusionShaderProgramService` owns compilation, linking, diagnostics, and release for the three ambient-occlusion programs.
- `OpenGlPreviewAmbientOcclusionTargetService` owns AO target allocation, resize replacement, completeness validation, and release.
- `OpenGlPreviewAmbientOcclusionPass` composes occlusion sampling, horizontal blur, vertical blur, and opaque HDR composition while restoring surrounding OpenGL state.
- `OpenGlPreviewRenderTargetState` now owns a multisample opaque view-normal attachment and a resolved opaque view-normal texture alongside the existing color and depth resources.
- `OpenGlPreviewScenePass` exposes explicit opaque and transparent phases; `OpenGlPreviewFrameRenderingService` composes ambient occlusion between them.
- `OpenGlPreviewRenderer` remains the composition root for AO program and target services and exposes read-only runtime readiness evidence.

### Implemented rendering behavior

- Opaque scene fragments write HDR color and packed view-space normals through an explicit multisample MRT.
- `OpenGlPreviewOpaqueSceneCapturePass` resolves opaque color, view normals, and depth before any transparent surface is submitted.
- SSAO reconstructs view-space positions from the resolved depth texture and evaluates a deterministic 16-sample hemisphere oriented by the resolved view normal.
- A separable bilateral blur rejects samples across depth and normal discontinuities before the AO texture is composed into opaque HDR color.
- AO composition occurs after opaque capture and before sorted premultiplied transparency, so glass and other transparent compatibility behavior remains intact.
- The SSAO checkbox is active and truthfully controls a real render pass.
- The unavailable lens-flare checkbox was removed; stale persisted enablement is forced off and the panel states that no bright-light artifact pass is owned.
- AO allocation and shader execution are observable through the `--ambient-occlusion-smoke` launch mode and `PROGEN3D_GUI_AMBIENT_OCCLUSION_PASSED` runtime marker.

### Requirement-to-evidence audit

| Loop 4 requirement | Current evidence | Result |
|---|---|---|
| Explicit screen-space geometry inputs | Multisample `preview_normal_ms`, resolved `preview_opaque_normal`, and resolved opaque depth are renderer-owned and architecture-gated | Passed |
| Real ambient-occlusion pass | Purpose-specific AO state, shader catalog, program service, target service, and pass are registered in the native build | Passed |
| Bounded deterministic sampling | Shader source gate requires the fixed 16-sample hemisphere; two enabled captures are byte-identical | Passed |
| Edge-aware denoising | Horizontal and vertical bilateral passes weight both depth and view-normal agreement | Passed |
| Correct frame ordering | Source gate requires opaque phase, AO pass, then transparent phase before resolve and post-processing | Passed |
| Transparent compatibility | Transparent draws retain back-to-front sorting, depth-write disable, and premultiplied-alpha blending after AO composition | Passed |
| Truthful SSAO control | `RenderSettingsPanel` exposes an active SSAO checkbox only after the live pass and gates exist | Passed |
| Truthful lens-artifact status | The former disabled no-op checkbox is absent and stale persisted state is cleared | Passed |
| Runtime allocation and shader execution | Isolated Xvfb smoke reports `PROGEN3D_GUI_AMBIENT_OCCLUSION_PASSED` with no OpenGL, shader, program, or framebuffer diagnostics | Passed |
| Deterministic visual effect | Two enabled residence captures are byte-identical and differ from the disabled control in 82,578 color channels | Passed |
| Native linkage and freshness | `make -j2 progen3d-editor-gui` succeeded after the final source changes | Passed |
| Preview, material, lighting, logging, and editor compatibility | Repository-native workflow, resource, session, material, lighting, logging, runtime-context, and composition gates passed | Passed |
| Standard and temporal GUI compatibility | Ordinary and temporal GUI smoke gates reported all required markers | Passed |
| Residence and environment compatibility | Residence lighting and frozen visual evidence passed; current environment evidence was refreshed after shared-source drift without changing its PPM capture | Passed |
| Source and evidence hygiene | Architecture ceilings, launch-option checks, current fingerprints, retained evidence verification, and explicit control comparison passed | Passed |

### Evidence identities

- Native GUI binary: 290,934,480 bytes; SHA-256 `6ee1374ad4b5bcd0859c2cc4772d2161eb89aa73b17c42a03506384a294d5f12`.
- SSAO-enabled residence capture: 694,401 bytes; SHA-256 `518d5e6ac44f5e945a7112e101fcffd97e34cd0f387d60cbc71cb7e7e70a0eb4`.
- Disabled SSAO control capture SHA-256: `ad99be209b822c4f270918c755ecd1907d156a074d9ee4123107bd4ebdfee5ee`.
- SSAO capture dimensions: 501x462; runtime pixel hash `16260234159206950962`.
- Measured SSAO delta: 82,578 changed channels, mean absolute channel delta `5.980942876152457`, maximum channel delta `87`.
- Retained SSAO PNG: 133,621 bytes; SHA-256 `269b1b32c923ea378d64ae767da878edc435f83179cc6303a2fcd4bc01db9615`.
- SSAO evidence record: 1,441 bytes; SHA-256 `b95e703d333f7c431ddf44a51b1b90c5758b28d83c935a386f1d08b3130437aa`.
- SSAO implementation fingerprint: `29f0d3d3117f90ef46cd62f4bf4b648d48adc76e54d1a2a9acf486746b9c1441`.
- Current environment-enabled hatchback capture remains SHA-256 `e6525627858e51673a71a7ed12bfddc6a5d01a934f6ef732b0c743d1f761c802`; its current binary/source evidence record SHA-256 is `ea19292f3d85ee6bee39cb11f44e5c9ae09ccd1cf4fc1d07a506a31600aa83b6`.
- Current retained environment PNG: 111,297 bytes; SHA-256 `e5d5ddab5f92a957f9d82ae6adccd42f19df6c8ab3fa4d61c94becbeb54fbe13`.
- Existing unrelated compiler warnings remain in `include/grammar.h`, `include/Solution.h`, and `src/Mesh.cpp`.

### Remaining limits

- SSAO affects only the resolved opaque layer; transparent-only geometry neither occludes nor receives the screen-space term.
- The AO pass is full-resolution and spatially denoised; it has no temporal accumulation, half-resolution mode, or adaptive sample count.
- The 16-sample kernel is preview-oriented rather than a physically calibrated ground-truth occlusion integrator.
- Environment preprocessing remains synchronous when a source revision changes.
- Refraction still uses one opaque color/depth snapshot and transparent intersections remain order-dependent.
- GPU pass timing, detailed submission statistics, structured material storage, measured instancing, frustum culling, Forward+, and indirect-draw decisions remain Loop 5 work.
- Point-light cube-map shadow rendering remains deferred from Loop 2.
- No release or full five-loop render-pipeline completion claim is made.

### Next implementation loop

Loop 5 will establish measured scalability before adopting additional execution paths:

1. audit current frame-statistic ownership, pass boundaries, geometry batching, repeated dynamic geometry, and available representative workloads;
2. introduce explicit GPU timestamp-query state and a non-blocking per-pass timing publication service;
3. record draw calls, submitted and culled triangles, visible lights, render-target memory, and geometry upload volume through purpose-specific frame evidence;
4. move repeated scalar and vector material parameters into structured GPU storage while retaining per-material texture ownership;
5. add deterministic view-frustum culling before opaque, transparent, outline, and repeated-geometry submission;
6. add instanced submission for measured repeated dynamic-cube geometry while preserving temporal interpolation and material identity;
7. capture current workload evidence and adopt Forward+ only if visible-light counts justify replacing the existing GL46 light SSBO loop;
8. adopt indirect drawing only if measured post-culling draw-call counts justify a second submission path; and
9. run the full renderer, editor, GUI, temporal, representative-scene, performance-evidence, and requirement-to-evidence audit before any five-loop completion claim.

## 11. Loop 5 Implementation Report

### Status

Loop 5 is implemented and validated. The renderer now owns explicit performance
evidence, structured material and dynamic-instance GPU storage, deterministic
camera-frustum visibility decisions, and measured candidate assessments for
advanced submission paths.

### Implemented object model

- `PreviewRenderPassTimingEvidence` and `PreviewRenderPerformanceEvidence`
  model pass timings, submission counts, memory ownership, upload volume,
  dynamic-instance visibility, and advanced-path decisions.
- `OpenGlPreviewGpuTimingState` composes double-buffered query frames;
  `OpenGlPreviewGpuTimingService` begins, ends, and publishes non-blocking
  `GL_TIME_ELAPSED` results without stalling the frame.
- `OpenGlPreviewFrameSubmissionEvidence` records scene and non-scene draws,
  framebuffer transfers, submitted and culled triangles, line submissions, and
  visible dynamic instances.
- `OpenGlPreviewPerformanceEvidenceService` composes timing, submission,
  render-target, environment, shadow, AO, geometry, material, and lighting
  evidence into one published frame model.
- `GpuPreviewMaterialRecordFactory` creates a nine-`vec4` std430 material
  record; `PreviewMaterialBuffer` owns its binding-point 3 SSBO.
- `PreviewBoundingSphereFactory`, `PreviewViewFrustumFactory`, and
  `PreviewSceneVisibilityService` model the bound, six plane relationships, and
  fail-open visibility decision separately.
- `GpuDynamicCubeInstanceRecordFactory` creates a four-`mat4`, seven-`vec4`
  std430 record; `PreviewDynamicCubeInstanceBuffer` owns its binding-point 4
  SSBO; `OpenGlPreviewDynamicCubeInstanceUploadService` owns synchronization.
- `PreviewAdvancedRenderPathAssessmentService` separates a measured candidate
  from a justified replacement.
- `EditorRenderPerformanceSmokeWorkflow` owns warmup, sampling, median
  publication, and the machine-readable runtime marker.

### Implemented rendering behavior

- Seven render phases are timed with non-blocking, double-buffered GPU queries:
  environment, shadow, opaque, ambient occlusion, transparent, resolve, and
  post-processing.
- Per-frame evidence records total and direct scene draw calls, transfers,
  submitted and culled triangles, visible lights, render-target bytes, geometry
  upload bytes, material upload bytes, lighting upload bytes, and dynamic cube
  totals.
- Repeated scalar and vector material uniforms were replaced by an indexed
  std430 material record while texture objects remain owned by each material
  resource.
- Opaque, transparent, outline, and dynamic preview submissions use
  deterministic camera-frustum culling; camera-invalid bounds fail open and
  shadow casters are not incorrectly culled by the camera frustum.
- Dynamic cubes retain previous/current transforms, dual deformation, cube
  mode, material identity, and transparency semantics in one instance record.
- Visible opaque dynamic cubes are stable-grouped by material and submitted by
  `glDrawArraysInstanced`; transparent cubes retain depth ordering and select
  the required instance record explicitly.
- The 64-cube retained workload produces one direct scene draw while preserving
  all 64 visible temporal instances.
- Forward+ becomes a candidate only above 24 visible lights. Indirect drawing
  becomes a candidate at 128 direct scene draws. Neither candidate is reported
  as justified without a controlled implementation comparison.

### Measured workload evidence

The retained timings were collected under Xvfb using Mesa 26.1.4 llvmpipe,
which reports direct rendering but no hardware acceleration. These values are
diagnostic medians for pipeline ownership and workload comparison; they are not
production-GPU benchmarks.

| Workload | CPU median | GPU median | Direct scene draws | Submitted triangles | Visible lights | Dynamic instances | Decision |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | --- |
| Modern luxury residence | 869.3732 ms | 992.3785 ms | 73 | 146,891 | 30 | 0 | Forward+ candidate; not justified. Indirect drawing is not a candidate. |
| 64 deformable cubes | 30.9384 ms | 30.9354 ms | 1 | 775 | 3 | 64/64 visible | Instancing is effective. Forward+ and indirect drawing are not candidates. |

The residence GPU median is dominated by opaque rendering at 738.9712 ms,
with transparent rendering at 123.5619 ms and resolve at 124.1668 ms under the
software renderer. This identifies the future hardware-benchmark focus without
authorizing an unmeasured replacement path.

### Requirement-to-evidence audit

| Requirement | Current evidence | Result |
| --- | --- | --- |
| Non-blocking per-pass GPU timing | Double-buffered `GL_TIME_ELAPSED` service plus eight post-warmup median samples | Passed |
| Explicit frame submission evidence | Draw, transfer, submitted-triangle, culled-triangle, line, and instance counters are published per frame | Passed |
| Render and upload memory evidence | Render-target, geometry, material, and lighting byte counts are retained in both workloads | Passed |
| Structured material GPU storage | Nine-`vec4` record, binding-point 3 SSBO, factory normalization tests, and removal of repeated material uniforms | Passed |
| Deterministic frustum culling | Six normalized planes, conservative bounds, fail-open invalid bounds, and source-level integration gates | Passed |
| Repeated dynamic geometry instancing | Four-`mat4`/seven-`vec4` record, binding-point 4 SSBO, instanced scene/shadow draws, and 64-to-1 runtime evidence | Passed |
| Temporal and material identity preservation | Instance factory checks and live deformation workloads retain previous/current and material data | Passed |
| Forward+ decision is evidence-constrained | 30-light residence is candidate=true and justified=false; llvmpipe provenance prevents a production-GPU claim | Passed |
| Indirect-drawing decision is evidence-constrained | Residence has 73 direct scene draws and the instance workload has 1, both below 128 | Passed |
| Runtime marker completeness | Both workloads publish all seven timing medians, counters, memory bytes, and decision flags | Passed |
| Native build freshness | Full `make -j2` reports the final native target current | Passed |
| Renderer source ownership | Renderer architecture ceilings and purpose-driven naming gates pass | Passed |
| Editor and subsystem compatibility | Preview, material, lighting, logging, runtime-context, composition, and all remaining editor sub-gates pass in isolated sequence | Passed |
| Standard and temporal GUI compatibility | Ordinary and temporal Xvfb smoke checks report all required markers | Passed |
| Cross-loop visual compatibility | Residence placement, environment lighting, and SSAO evidence verify against the final source and binary | Passed |
| Retained evidence freshness | Source, scene, binary, renderer-provenance, and evidence hashes verify without timing-value equality requirements | Passed |

The broad editor architecture wrapper stopped once during its post-preview
segment without a retained diagnostic. The same remaining sub-gates were then
run command-by-command and all passed; the failure was not reproducible and no
render-owned regression was identified.

### Evidence identities

- Native GUI binary: 292,361,656 bytes; SHA-256
  `f672678f5f94957b96cd9a8df37d5bcb4a01e5cbd5e379a49d68a7a9c856edb8`.
- Loop 5 implementation fingerprint:
  `f1be9babb7873a95ba19fe72029fba584fa9d4b9a22b7df52e071167baf36ebe`.
- Performance evidence record: 3,790 bytes; SHA-256
  `d968604d275af07c2a64d8779a2134e1ed3323e4546ab0d2d09cfe6018c91ee7`.
- Residence source SHA-256:
  `8cde0c9cf9721f0959152bf67430e4e5afbaacce8369ccc533b46268ae67faae`.
- Dynamic-instance fixture SHA-256:
  `0fc4bb011d478caae02f8ce2e830a8553048e54573c8182a82d21b5ee1c94427`.
- Current environment evidence record: 1,286 bytes; SHA-256
  `1ab1cb802cd7ad81a3927bcea24d286d0ba31db83fb500abeae607bc6c82c55c`.
- Current SSAO evidence record: 1,441 bytes; SHA-256
  `4402c1613f5dbf8532b730378a50a66e535b7c531e1eb4940236129eb8ef7e07`.
- Environment-enabled capture remains SHA-256
  `e6525627858e51673a71a7ed12bfddc6a5d01a934f6ef732b0c743d1f761c802`.
- SSAO-enabled capture remains SHA-256
  `518d5e6ac44f5e945a7112e101fcffd97e34cd0f387d60cbc71cb7e7e70a0eb4`.
- Existing unrelated compiler warnings remain in `include/grammar.h`,
  `include/Solution.h`, and `src/Mesh.cpp`.

### Remaining limits

- Hardware-accelerated GPU benchmark evidence has not been collected. Forward+
  remains a measured candidate, not an authorized implementation replacement.
- No indirect or multi-draw path is implemented because the retained workloads
  remain below the explicit direct-scene-draw threshold after batching.
- The retained fit-to-extents workloads produce zero culled triangles; focused
  deterministic frustum tests are authoritative for off-screen rejection.
- GPU query samples are diagnostic and driver-dependent; only schema,
  provenance, required sample count, valid ranges, and decision consistency are
  deterministic evidence requirements.
- Point-light cube-map shadow rendering remains deferred from Loop 2.
- Environment preprocessing remains synchronous when a source revision changes.
- Refraction remains based on one opaque color/depth snapshot, and transparent
  intersections remain order-dependent.
- SSAO remains full-resolution, spatial-only, and limited to the opaque layer.

## 12. Five-Loop Completion Audit

### Completion status

The five-loop render-pipeline upgrade defined by this plan is complete at the
implementation and repository-validation level on August 27, 2026.

| Loop | Planned capability | Completion evidence | Status |
| --- | --- | --- | --- |
| 1 | HDR presentation, real MSAA, bloom, ACES, gamma, FXAA, truthful controls | HDR/post-process source gates, quality harness, native build, GUI smoke | Complete |
| 2 | Directional cascades and spot-light shadow projection | Layered shadow target, projection harness, shader/pass gates, runtime compatibility | Complete |
| 3 | Environment IBL and scene-aware glass | Deterministic visual/control evidence, source ownership, HDR/depth snapshot, sorted compatibility transparency | Complete |
| 4 | View-normal MRT and deterministic SSAO | AO harness/source gates, enabled/control image delta, current visual evidence | Complete |
| 5 | Timing, metrics, material SSBO, culling, instancing, measured advanced-path decisions | Performance harnesses, 64-to-1 workload evidence, source/binary/driver provenance, full compatibility matrix | Complete |

### Completion boundary

This completion claim applies to the five planned implementation loops and
their repository-native evidence. It does not certify a release package, all
hardware vendors, production-GPU performance, point-light cube shadows,
order-independent transparency, temporal SSAO, or asynchronous environment
preprocessing. Those remain explicit future work rather than hidden completion
assumptions.
