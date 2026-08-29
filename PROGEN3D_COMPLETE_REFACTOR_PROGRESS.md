# ProGen3D Complete Refactor Progress

Date: Wednesday, August 26, 2026

## Objective

Refactor ProGen3D into a purpose-driven, UML-readable C++ object model while preserving executable grammar behavior, deterministic scene generation, editor workflows, visual output, and accepted compatibility contracts.

This objective is not complete. This record distinguishes validated progress from the repository-wide work that remains.

## Governed OURD Role

OURD is used as a read-only architecture adviser. The current authority manifest allows repository inspection but no commands or file mutation:

- task: `read-only`;
- semantic ceiling: `C1`;
- maximum automatic risk: `L0`;
- post-implementation, pre-record-refresh source snapshot: `c514a93adfcd528292d3fe493e0c997397e44d9518c684ff02b33e7960f5a37c`;
- authority hash: `22cd7397255c6156b821eb7907b77c7385cc236a38c25dc667badc9fead906ba`.

OURD proposals remain advisory. Current source inspection, deterministic tests, visual gates, and human-reviewed Codex edits remain authoritative.

The first lifecycle request used its only model step to establish governance and therefore returned no final answer. A second two-step request remained within the active 8,192-token Ollama context and returned a bounded `EditorStartupSequence` proposal. Human review retained its startup-sequence boundary but rejected duplicate stage records: the implementation composes the existing `ApplicationStartupProgress` authority instead.

The completed-scene publication review used a separately bounded context:

- pre-review source snapshot: `3d210c9cfc160a164f6fdd2ed9f47e49f833f289b6c868e0abbd149197da3f78`;
- context artifact: `build/tests/ourd_scene_publication_context.md`;
- context SHA-256: `dcb992fecf7590dce48553e0f2827fa5522cf052cee322d338e5fecb53764107`;
- review artifact: `build/tests/ourd_scene_publication_review.txt`;
- review SHA-256: `fc65610ebd889673b9d0b04431196f08e20f0f7f6b50adc0bb7d09f6e5504a04`;
- provider: `openai_responses` through `http://127.0.0.1:11434/v1`;
- model: `qwen3.8-27b-fast`, 27.3B parameters, `Q3_K_S` quantization;
- model digest: `07cb98f8840ce491fc28c04a5ecc13c4dec5fd23d9a4732878bc3c02acb5b005`;
- request limits: 6,000 context tokens, 700 output tokens, no model reasoning.

The bounded response was truncated at its configured output limit, but it identified the upload-before-swap seam and the preview snapshot as the canonical grammar and design-nonce owner. Human review corrected its nonexistent `GeneratedSceneContext` suggestion and refined the proposal into an explicit prepared-publication value object, a preparation service interface, and a completed-publication service. OURD did not author, mutate, approve, or certify the implementation.

The preview-resource ownership review used another bounded, separately hashed context:

- pre-review source snapshot: `a79cc2bb22232438ddc57adf871db357d1ab33667bdeb4742df6f4c96bbac152`;
- context artifact: `build/tests/ourd_preview_resource_context.md`;
- context SHA-256: `7f1832b2e2dd6651fc3b663569a38a445e7c39ba8e559f103d3b16015a3b9bae`;
- review artifact: `build/tests/ourd_preview_resource_review.txt`;
- review SHA-256: `c0cc8375dce61534f38af00c9e817c2cc6f438e5ab2c2a7e2b40e1799381596c`;
- provider: `openai_responses` through `http://127.0.0.1:11434/v1`;
- model: `qwen3.8-27b-fast`, 27.3B parameters, `Q3_K_S` quantization;
- model digest: `07cb98f8840ce491fc28c04a5ecc13c4dec5fd23d9a4732878bc3c02acb5b005`;
- request limits: 6,000 context tokens, 700 output tokens, no model reasoning, and zero retries.

OURD correctly identified the material library as the canonical texture/cache and active-projection owner and recommended backend-independent geometry partitioning. Its truncated answer was internally inconsistent about whether resource access should inherit publication preparation. Human review resolved that ambiguity by keeping the abstract interfaces as sibling concepts while allowing `OpenGlScenePreviewResourceService` to implement both roles independently. OURD remained read-only and did not author, mutate, approve, or certify the implementation.

The material-language and texture-resource review used a third bounded context:

- review workspace source snapshot: `50d80cb63adb875b4c15fae2002d6a1265eff84008409a2b81d8c5b721ceca92`;
- context artifact: `build/tests/ourd_material_resource_context.md`;
- context SHA-256: `c34a7929020862c0e80ed9775e1e485e001e1ca3f6860650c618fd5187424ca1`;
- read-only authority artifact: `build/tests/ourd_material_resource_authority.json`;
- authority SHA-256: `8f93f8aa8264a05100e96f88b69bd82dfe95e8413e38c1daddbd7ba6d9ca9c27`;
- provider preflight artifact: `build/tests/ourd_material_resource_preflight.txt`;
- preflight SHA-256: `8453eabfe15e2027fb3f4f56f3dec90c90f5b08bdb190fbf98f1ed3d6df93c3e`;
- review artifact: `build/tests/ourd_material_resource_review.txt`;
- review SHA-256: `1b8f7e77db80416b61650305f6aad708aab60be00db2f36b67e612dfdfa914b8`;
- provider: `openai_responses` through `http://127.0.0.1:11434/v1`;
- model: `qwen3.8-27b-fast`, 27.3B parameters, `Q3_K_S` quantization;
- model digest: `07cb98f8840ce491fc28c04a5ecc13c4dec5fd23d9a4732878bc3c02acb5b005`;
- request limits: 6,000 context tokens, 900 output tokens, no model reasoning, and zero retries.

The persistent OURD workspace could not complete this review within its two-step limit, and a three-step attempt exceeded the bounded context. A fresh review workspace first emitted a malformed governance call; an explicit human-authored read-only authority then produced the retained truncated review. Human review accepted the five-role decomposition but rejected additional public image/noise/swatch APIs, kept deterministic noise private to procedural synthesis, corrected backend-name assumptions to the executable `usertextureN` contract, and rejected the statement that the abstract factory contains no OpenGL type because `PreviewMaterial` still carries `GLuint` fields. OURD remained advisory and did not author, mutate, approve, or certify the implementation.

The application-ownership review used a fourth bounded, separately hashed context:

- review workspace source snapshot: `094b7d0861de37193eaaddafa897e7438f44c281628713659de4fe01ac3b86ee`;
- context artifact: `build/tests/ourd_application_ownership_context.md`;
- context SHA-256: `5d6014a63bc3761ab3e33f8710bf92834c0ecaea4616b9bd503a8f91f1f70d0d`;
- read-only authority artifact: `build/tests/ourd_application_ownership_authority.json`;
- authority SHA-256: `aca399105abeebf85c55113531c3beaf7e70da38d60c984542af16d98a2abcb0`;
- provider preflight artifact: `build/tests/ourd_application_ownership_preflight.txt`;
- preflight SHA-256: `d91e4dca2818ee873caec150c9b6188f16c99fb67cdb1f4bba188e9700ede168`;
- review artifact: `build/tests/ourd_application_ownership_review.txt`;
- review SHA-256: `51c8204477225b8fcf7c20e9c168d11cec5e3fc51ec2a80ef9055aaa212739b9`;
- provider: `openai_responses` through `http://127.0.0.1:11434/v1`;
- model: `qwen3.8-27b-fast`, 27.3B parameters, `Q3_K_S` quantization;
- model digest: `07cb98f8840ce491fc28c04a5ecc13c4dec5fd23d9a4732878bc3c02acb5b005`;
- request limits: 6,000 context tokens, 1,000 output tokens, no model reasoning, and zero retries.

The retained review proposed separate interface-state and presentation-composition owners, runtime-context associations, and a temporary typed boundary for legacy free logging hooks. Human review implemented those concepts with references rather than nullable duplicate state, kept the legacy log binding narrow and application-lifetime-scoped, and removed obsolete debug and autocomplete registries. A live refresh on Wednesday, August 26, 2026 verified the same provider and model as ready, but two bounded attempts failed closed without an admissible final review, including one malformed Ollama tool-call payload. The retained review therefore remained advisory evidence; OURD did not author, mutate, approve, or certify the implementation.

The application-lifecycle review used a fifth bounded, separately hashed context:

- repository source snapshot before review: `da1a0ae84efdb133d2896e5bec68782eedefd597b2fda100133781a1c9d3f0c7`;
- isolated review-workspace source snapshot: `f767f83f5640e23dcff73c7b05b33e444013614d4bfc5d061e88e9b8385e3a1d`;
- context artifact: `build/tests/ourd_lifecycle_context.md`;
- context SHA-256: `79e1ae1171e71a6ac4aaf1499fb9506d165c3aea2f41090830e041d337b8602f`;
- read-only authority artifact: `build/tests/ourd_lifecycle_authority.json`;
- authority SHA-256: `8577c86b22b1d133a297f136cb2b06fbe1070a28e075a376c1f3f6ac3374bca5`;
- provider preflight artifact: `build/tests/ourd_lifecycle_preflight.txt`;
- preflight SHA-256: `d31fade45adb4b66f3bb16d07474f2e2badadbcc000961178450bf63e022a6a4`;
- review artifact: `build/tests/ourd_lifecycle_review.txt`;
- review SHA-256: `ed4430baaf4ee3dff31da42cd9d97fa241fb9741bbb29fc9d71aeb25f6e3d66a`;
- provider: `openai_responses` through `http://127.0.0.1:11434/v1`;
- model: `qwen3.8-27b-fast`, 27.3B parameters, `Q3_K_S` quantization;
- model digest: `07cb98f8840ce491fc28c04a5ecc13c4dec5fd23d9a4732878bc3c02acb5b005`;
- request limits: 6,500 context tokens, 1,200 output tokens, no model reasoning, and zero retries.

The first isolated review attempt failed closed because copying the authority manifest into the reviewed directory correctly changed the exact source snapshot. After human correction moved the authority outside that snapshot, the retained review proposed separate frame-cycle, interactive-loop, smoke-workflow, exit-authorization, and shutdown roles. Human review preserved those responsibility boundaries while rejecting unnecessary generic clock/window abstractions and expressing ProGen3D-specific environment effects through explicit application and service interfaces. OURD remained read-only and did not author, mutate, approve, or certify the implementation.

## Validated Ownership Slices

### Application runtime ownership

`Progen3dEditorApplication` now composes the editor's platform and workspace lifetime:

- `EditorWorkspaceSession` owns application workspace state;
- `EditorRuntimeEnvironment` owns GLFW, ImGui, and renderer startup/shutdown;
- `NativeLocalFileDialogService` is application-owned;
- `EditorApplicationRuntimeContext` represents non-owning associations required during the running application;
- `EditorApplicationRuntimeContext::previewSession()` exposes the workspace's exact `ScenePreviewSession` association rather than a file-scope compatibility facade;
- application shutdown destroys scene generation, grammar state, UI resources, authentication support, file-dialog state, and platform resources in an explicit order.

Removed file-scope bindings and compatibility accessors:

- workspace-session pointer;
- local-file-dialog pointer;
- application-window pointer;
- running application-runtime pointer;
- current workspace, preview-session, scene-snapshot, and authentication-panel accessors.

### Application interface and presentation composition

`Progen3dEditorApplication` now composes two explicit aggregates immediately beside the runtime environment:

- `EditorInterfaceState` owns `ApplicationLog`, `EditorInterfaceFontSet`, and `GrammarEditorInteractionState` as one application-lifetime interface-state model;
- `EditorPresentationComposition` owns the editor panels, presentation controls, presentation controllers, inspection services, and overlay-evidence service previously constructed at file scope;
- `EditorApplicationRuntimeContext` associates workflows with both aggregates through non-owning references and delegates grammar-editor interaction access to `EditorInterfaceState`;
- panels, controllers, services, fonts, and editor interaction state are obtained through the application composition or its runtime associations rather than hidden translation-unit globals;
- `RunningApplicationLogBinding` temporarily associates the application-owned log with the legacy cross-translation-unit `errorout` and `debugout` functions for exactly the duration of `Progen3dEditorApplication::run()`.

Removed file-scope owners and registries:

- application log, interface font set, and grammar editor interaction state;
- editor workspace, authentication, cloud, AI, grammar, preview, material, diagnostics, lighting, electrical, scene-inspection, and console presentation objects;
- preview motion, timeline, and selection controllers;
- spatial inspection and overlay-evidence services;
- the unused `debugstate` registry and obsolete autocomplete helper chain.

### Scene generation runtime ownership

`EditorSceneGenerationRuntime` now owns:

- `GrammarCompilationService`;
- `SceneRegenerationCoordinator`.

The application initializes and shuts down this aggregate. Generation request, status, result, and cancellation paths no longer depend on file-scope owning smart pointers.

`EditorSceneGenerationRuntime` now presents the generation lifecycle through purpose-owned operations:

- request and schedule generation;
- start pending or due work;
- take the completed result;
- cancel scheduled work and join an idle worker;
- query request identity, pending state, scheduled state, and design nonces.

The underlying `SceneRegenerationCoordinator` is no longer exposed to the monolithic editor source through a public compatibility accessor.

Removed file-scope owners and generation facades:

- grammar-compilation service `std::unique_ptr`;
- scene-regeneration coordinator `std::unique_ptr`;
- current scene-generation runtime and regeneration-coordinator accessors;
- file-scope generation-in-progress, queued, scheduled, and scheduled-start-time accessors.

### Application startup ownership

`EditorApplicationStartup` now owns the five-stage startup sequence and composes `ApplicationStartupProgress`:

- interface preparation;
- part-class catalog loading;
- interface artwork loading;
- initial document loading with the existing fallback behavior;
- initial scene generation and completion observation.

`EditorApplicationStartupOperations` is the explicit abstract startup environment. `Progen3dEditorStartupOperations` is its ProGen3D-specific implementation and now lives in its own application source pair. It associates the sequence with the runtime context, runtime environment, launch options, scene-generation runtime, scene-generation publication service, application window, interface resources, startup document service, and header artwork.

`EditorApplicationStartupOutcome` preserves requested-document load failure and window-closure evidence without promoting either to hidden global state. Startup progress is monotonic, and source gates reject any return of inline `ApplicationStartupStage` orchestration to `src/imgui_main.cpp`.

`EditorStartupProgressWindow` now owns startup-loader presentation. `Progen3dEditorApplication` composes this presentation object, and `Progen3dEditorStartupOperations` holds only a non-owning association to it. The old `draw_startup_loader_window` and `render_startup_loader_frame` procedures were removed from `src/imgui_main.cpp`; both normal startup and temporal smoke waiting now publish progress through the same application-owned presentation object.

Startup resource and document responsibilities now have explicit owners:

- `EditorInterfaceImage` models OpenGL texture identity and pixel dimensions, including deterministic ownership release;
- `ApplicationHeaderPanel` composes its header-logo image instead of reading a file-scope image record;
- `OpenGlEditorInterfaceResourceService` owns professional-font preparation and UI-image upload/release behavior;
- `EditorStartupDocumentService` composes grammar and lighting persistence services and preserves new-document, requested-document, fallback, identity, and status behavior;
- `EditorSceneGenerationPublicationService` defines the remaining scene request/publication association used by startup;
- `Progen3dEditorApplication` composes the OpenGL interface-resource service and startup document service.

Source gates reject any return of `Progen3dEditorStartupOperations`, `AppFonts`, `UiImage`, font loading, UI-image loading, or UI-image destruction implementations to `src/imgui_main.cpp`.

### Document publication ownership

`EditorDocumentPublicationService` now owns the atomic publication of editor document text and document identity:

- supplies the canonical default new-document grammar source;
- replaces the editor buffer and associated document identity as one operation;
- clears stale selection, diagnostics, generation, and save-state projections before regeneration;
- preserves the existing lighting-document reset behavior;
- updates the editor status message through the explicit `GrammarEditorInteractionState` association.

Startup loading, local-file opening, cloud-document opening, and new-document creation now use this service. The legacy `build_default_new_document_text_internal`, `set_editor_document_internal`, `default_new_document_text`, and `set_editor_document` functions were removed from `src/imgui_main.cpp`, and `EditorStartupDocumentService` no longer reaches document publication through `CloudAuthWorkflow.h`.

### Completed scene publication ownership

Completed generation results now cross an explicit preparation and publication boundary:

- `PreparedScenePreviewPublication` is the immutable value object for source associations, lighting, primitive count, and material count prepared for preview publication;
- `ScenePreviewPublicationPreparationService` is the abstract environment interface for candidate OpenGL preparation and derived preview data;
- `CompletedSceneGenerationPublicationService` owns stale-result rejection, generation-failure publication, preparation-failure publication, and successful state projection;
- `ScenePreviewSession` is the canonical owner of the last valid scene snapshot, published grammar document, generation context, and design nonce;
- `Progen3dSceneGenerationPublicationService` remains a thin application adapter that takes completed runtime results, delegates state publication, reports the typed outcome, starts pending generation, and restores editor focus.

The service preserves the last valid scene for stale, generation-failed, and preparation-failed results. Successful publication uploads the candidate before replacing the accepted scene, clears the previous grammar runtime snapshots, records generation identity, publishes source associations and lighting, preserves electrical switch state, evaluates lighting, updates statistics and timeline state, and clears selection and non-temporal diagnostics. The former `process_completed_regeneration` procedure and the file-scope `grammar` and `active_grammar_design_nonce` globals were removed.

### Scene preview resource ownership

Preview material and geometry resources now cross explicit model and service boundaries:

- `ScenePreviewMaterialLibrary` owns the named material cache and the ordered active-material projection;
- `PreparedScenePreviewMaterialSynchronization` represents a candidate material synchronization with explicit prepare, commit, and rollback semantics;
- `PreviewMaterialResourceFactory` defines an API-operation-independent creation and deterministic release boundary, while the existing `PreviewMaterial` value still carries OpenGL texture identifiers;
- `ScenePreviewGeometrySource` defines the read-only primitive geometry required for batching;
- `ScenePreviewBatchBuilder` partitions static opaque, static transparent, and dynamic movable-cube geometry independently of the rendering backend;
- `ScenePreviewGeometryBatches` owns the resulting vertex batches and dynamic draw descriptions;
- `ScenePreviewResourceService` defines active-material access, complete scene refresh, dynamic refresh, and explicit material-resource release;
- `OpenGlScenePreviewResourceService` implements resource access and candidate publication preparation as sibling roles while composing the material library and batch builder;
- `Progen3dEditorApplication` composes the material resource factory and OpenGL resource service, while `EditorApplicationRuntimeContext` exposes only a non-owning service association.

Material synchronization preserves the last committed projection if candidate creation or upload fails. Candidate resources are rolled back, inactive resources are released only after commit, and the service attempts to restore the currently published scene resources after a failed candidate upload. Full scene refresh, dynamic simulation refresh, material resolution changes, UI material display, renderer material access, and shutdown now use the explicit service. The former file-scope `active_materials` and `material_cache` owners, preview upload procedures, synchronization procedure, destruction procedure, and nested OpenGL preparation adapter were removed from `src/imgui_main.cpp`.

### Material language and texture resource ownership

Material interpretation and concrete texture creation now have purpose-specific owners:

- `MaterialColorSwatch`, `ProceduralMaterialSpecification`, `MaterialLexemeKind`, `MaterialAutocompleteSuggestion`, and `MaterialAutocompleteAnalysis` model the semantic values exchanged by the material language;
- `MaterialLanguageService` is the canonical owner of the material lexicon, authored-name interpretation, autocomplete category order, prefix filtering, replacements, and user-facing category labels;
- `OpenGlPreviewTextureService` owns RGBA image decoding, OpenGL texture upload, generated alpha/glow/normal/roughness/metallic/ambient-occlusion maps, individual texture deletion, and complete material texture release;
- `ProceduralPreviewMaterialFactory` associates the language and texture services and owns deterministic base-color, height, and emissive synthesis;
- `BackendPreviewMaterialFactory` associates the texture service, procedural fallback factory, and `ApplicationLog`, and owns executable `usertextureN` recognition, backend acquisition, decode, upload, average-swatch derivation, fallback creation, and diagnostics;
- `Progen3dPreviewMaterialResourceFactory` implements the existing abstract factory and composes the concrete procedural and backend factories;
- `Progen3dEditorApplication` composes `MaterialLanguageService` and the abstract material factory, while `EditorApplicationRuntimeContext` exposes the language service through a non-owning association used by material autocomplete.

The texture-library thumbnail path reuses the stateless `OpenGlPreviewTextureService` rather than restoring legacy decode or upload procedures. All prior material lexicon, parser, procedural synthesis, backend image decode, texture upload, auxiliary-map generation, and material texture deletion implementations were removed from `src/imgui_main.cpp`. The focused harness also exposed and fixed a zero-length autocomplete-token loop caused by a fixed-size lexicon array with an unused entry; the canonical lexicon is now an exact-size collection and partial prefixes terminate deterministically. `ApplicationLog` is now owned by `EditorInterfaceState` and associated with the extracted backend factory through application composition.

### Application lifecycle ownership

Application execution now delegates to purpose-specific lifecycle objects composed by `Progen3dEditorApplication`:

- `EditorExitAuthorizationState` is the canonical model for explicit exit permission and replaces the file-scope authorization flag;
- `EditorFrameCycle` owns one complete GLFW, ImGui, presentation, scene-publication, scheduled-generation, buffer-swap, and pending-dialog frame;
- `EditorInteractiveFrameLoop` owns frame timing, window-close interception, exit authorization, and interactive repetition;
- `EditorSmokeTestWorkflow` owns deterministic smoke, temporal, FOV, render-frame, object-visibility, camera-framing, building-knowledge, and visual-capture acceptance behavior;
- `EditorApplicationShutdown` owns the ordered release of cloud requests, generation work, published grammar, interface and preview resources, authentication support, runtime associations, file-dialog state, and the platform environment;
- `EditorApplicationShutdownOperations`, `EditorApplicationExitRequestService`, and `LocalDocumentDialogWorkflowService` expose narrow environment associations without callback bags or generic helper types;
- `GlfwApplicationWindow` now exposes the exact polling, close-state, time, framebuffer, and swap operations required by lifecycle services;
- `GrammarEditorInteractionState::requestFocusRestore()` owns its focus and selection restoration transition rather than repeating that model mutation procedurally.

`Progen3dEditorApplication::run()` now performs composition and dispatch only. The inline frame lambda, interactive loop, approximately 250-line smoke workflow, ordered shutdown sequence, global exit flag, smoke-only camera helpers, and legacy focus-restore procedure were removed from `src/imgui_main.cpp`. The lifecycle source manifest is retained at `build/tests/editor_lifecycle_source_manifest.sha256`; its SHA-256 is `7de1a699dd059709589356913993821a47442be875ca516e05901f98d956e6b1`.

## Current Evidence

Focused checks passed:

```text
./tests/run_editor_application_composition_checks.sh
./tests/run_editor_application_lifecycle_checks.sh
./tests/run_editor_application_runtime_context_checks.sh
./tests/run_editor_application_startup_checks.sh
./tests/run_editor_document_publication_service_checks.sh
./tests/run_material_resource_architecture_checks.sh
./tests/run_editor_interface_resources_checks.sh
./tests/run_editor_scene_generation_runtime_checks.sh
./tests/run_completed_scene_generation_publication_checks.sh
./tests/run_editor_workspace_model_checks.sh
./tests/run_application_launch_options_checks.sh
./tests/run_document_persistence_checks.sh
./tests/run_lighting_scene_persistence_checks.sh
./tests/run_local_file_dialog_workflow_checks.sh
./tests/run_preview_session_checks.sh
./tests/run_scene_preview_resource_checks.sh
./tests/run_scene_regeneration_coordinator_checks.sh
./tests/run_grammar_autocomplete_presentation_checks.sh
./tests/run_application_log_checks.sh
python3 tests/test_p2_time.py
```

Broad checks passed:

```text
make -j2 progen3d-editor-gui
./tests/run_gui_smoke_check.sh
./tests/run_temporal_gui_smoke_check.sh
./tests/run_editor_architecture_checks.sh
```

The GUI smoke gate exercised the extracted startup sequence, application-owned startup progress window, interface-state aggregate, presentation-composition aggregate, explicit frame cycle, smoke workflow, ordered shutdown, extracted font/image loading, extracted startup document service, document publication service, completed-scene publication service, scene preview resource service, material language service, and concrete material resource factory. It loaded `examples/curved_primitives_showcase.p3d`, validated and regenerated the initial scene, rendered the preview, and reported the FOV, render-frame, and GUI smoke markers. The temporal GUI smoke gate also generated its temporal grammar, refreshed dynamic preview resources, and completed a deterministic time sample through the same publication boundary. The complete editor architecture suite passed on Wednesday, August 26, 2026 in 561.74 seconds. Existing signedness and unused-symbol compiler warnings remain unrelated cleanup work.

Retained validation logs:

- `build/tests/progen3d_editor_build_after_lifecycle.log`, SHA-256 `700b3343112712c2730c8ba46e33de5fb4f123ae80d3e6f6fa3744dc9cd3882b`;
- `build/tests/gui_smoke_after_lifecycle.log`, SHA-256 `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff`;
- `build/tests/temporal_gui_smoke_after_lifecycle.log`, SHA-256 `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144`;
- `build/tests/editor_architecture_checks_20260826.log`, SHA-256 `32be52e41fac55ebb870dc672a04e43bdacf52964b9e5ab392dbbcff2e701778`.

The source ownership scan also confirms that no legacy runtime, preview, scene-snapshot, authentication-panel, or scene-generation compatibility facade remains in `src/` or `include/`; no inline startup stage state remains in `src/imgui_main.cpp`; neither removed startup-loader procedure has returned; startup resource/document implementations remain outside the monolith; file-scope `ApplicationLog`, `EditorInterfaceFontSet`, `GrammarEditorInteractionState`, presentation objects, presentation controllers, spatial inspection services, `debugstate`, the obsolete autocomplete registry, and the global exit-authorization flag remain absent; and none of `process_completed_regeneration`, `build_default_new_document_text_internal`, `set_editor_document_internal`, the file-scope `grammar`, `active_grammar_design_nonce`, file-scope `active_materials`, file-scope `material_cache`, `sync_active_materials`, `destroy_materials`, `upload_context_preview_buffers`, `upload_dynamic_preview_buffers`, the nested `OpenGlScenePreviewPublicationPreparationService`, `MaterialAutocompletePrefixState`, `material_lexemes`, `interpret_material_name`, `generate_procedural_material`, `upload_rgba_texture`, `decode_texture_image_bytes`, `destroy_preview_material_textures`, `auto_generate_missing_texture_maps`, the concrete `Progen3dPreviewMaterialResourceFactory`, the inline frame lambda, smoke acceptance markers, the interactive `glfwWindowShouldClose` loop, `scene_generation_runtime_.shutdown()`, or `runtime_context_.reset()` has returned to `src/imgui_main.cpp`.

Current source hashes:

| Artifact | SHA-256 |
|---|---|
| `include/editor/application/Progen3dEditorApplication.h` | `9ce3b27bfd54252d9f40d3adb285485f0f95f4a3cdd3bda01e18d36f130020d0` |
| `include/editor/application/EditorApplicationShutdown.h` | `cdf0a3a31122c58d20746c6b0ece655150a9a05272b49a785c3feef264fb037e` |
| `include/editor/application/EditorApplicationShutdownOperations.h` | `84dac54c9e3ccce4edbc902530fc6b536293ed9f09d2353c92ecc8a3d8df5c3e` |
| `include/editor/application/EditorFrameCycle.h` | `771d06c86445bee67138560639e433b88190c995c487981ad1ccc09b0adc2198` |
| `include/editor/application/EditorInteractiveFrameLoop.h` | `1a10850cf514198fb60d00d887d0be62dcc9ed0ced21c38139f79c2acd1a4e39` |
| `include/editor/application/EditorSmokeTestWorkflow.h` | `b9d8018e195735df21ac5aa79df9e7de1c67241fae45dd7e23a6162749531c1e` |
| `include/editor/application/EditorRuntimeEnvironment.h` | `ebfa7dec86d8a5cbdf043dbad50cc1cf3ff2dc20b934d60a94d604ce2ec1c2df` |
| `include/editor/application/GlfwApplicationWindow.h` | `b41a57bcbbb0e391a6c6bf5abcacd50e5581b9e23750c630325cc4cf6b135dd3` |
| `include/editor/application/EditorApplicationStartup.h` | `710fb85ffacd76eddf68b162e8e1d6e87cf6f861d13485d040ce96b22d5d1e6a` |
| `include/editor/application/Progen3dEditorStartupOperations.h` | `c5a49744251dfa938cc84cc766886c05a1053de107ad222d357c79c8f12e5701` |
| `include/editor/application/EditorApplicationRuntimeContext.h` | `385c381fb8832de7697b597f53e1f8e46d31520bf1debb3d7234a24db779ef15` |
| `include/editor/application/EditorSceneGenerationRuntime.h` | `f7e2c4fa8d7a7f167dc8e7bf0959881d23ccddab72e8381d90e1c21d5917cb8b` |
| `include/editor/model/EditorInterfaceResources.h` | `e1f5efeb71176ec7359fc8444c14a7bc9a390b6be20a28068ae3ff89373eb75c` |
| `include/editor/model/EditorInterfaceState.h` | `a58652a7a9aad549fe0dc25048945bb428ae9903f9a91537a19bfb3c299912b2` |
| `include/editor/model/ScenePreviewSession.h` | `a1c700b65501712a402f9539ab078ce5e93d167a95821f0d3e68d06181f8b41d` |
| `include/editor/model/PreparedScenePreviewPublication.h` | `c316649305b14929bf1ed0f3229cc1c7f3765706794b57abfbddca1a3893e51e` |
| `include/editor/model/ScenePreviewMaterialLibrary.h` | `be008e8c9c6469f14623d21b308249baac21f22d8619094339ebdd8f7bff9d6f` |
| `include/editor/model/ScenePreviewGeometryBatches.h` | `8e3638a1c4985222ab033e5914fabd419673a7e90c71bcf916a5e21506f488f1` |
| `include/editor/model/MaterialLanguage.h` | `96728662f33e3438e07109d82f38f871e42e5ea609947d7d824c5002ef53d04b` |
| `include/editor/presentation/ApplicationHeaderPanel.h` | `bf1aa54d5047c72fbe1fb44c710eb4cc5783938b299e2a1f1de730e47d8653da` |
| `include/editor/presentation/EditorPresentationComposition.h` | `2b056d14e23ab4bd60871ba3fa8a1863727357fb56d61dcd24f740919269699b` |
| `include/editor/presentation/EditorStartupProgressWindow.h` | `6784aa73b8f3524f32511aeed65458f8f3a49cdd832bff92286b11900cb2f542` |
| `include/editor/model/EditorExitAuthorizationState.h` | `5d601d27d082c108625d3741bb41b427c83770abb318a7bc127c60e0f818e84b` |
| `include/editor/model/GrammarEditorInteractionState.h` | `be9ed0a4263a0b5472b3d42a60194762c7cf73c91cc483d481bf0aa1898cd3ce` |
| `include/editor/service/EditorApplicationExitRequestService.h` | `c55092bfda668baa30bbea1ce63d67a9dcbbaaab4c73a51614f0f0917e8019bb` |
| `include/editor/service/EditorSceneGenerationPublicationService.h` | `7440ada3fdf65b88caf4007a876f316a957b618fc45731f1bac927ed5ff251cb` |
| `include/editor/service/LocalDocumentDialogWorkflowService.h` | `6def2e3d779dd592180746c4cab6132ddf63ad027e9c3b11c910fdc1a13ceafd` |
| `include/editor/service/EditorStartupDocumentService.h` | `e8744f2a0bb6bffa653d017f6542f8c638c94aeeb23563e65d9d1cf2462d89fd` |
| `include/editor/service/OpenGlEditorInterfaceResourceService.h` | `72c8b8db609c4394dc92edb5f74cf13704e9eb8fc0497fa2f9a74d3c9552a795` |
| `include/editor/service/EditorDocumentPublicationService.h` | `d243b06c5742887b400df734814cbfae71ac3ef585b2161fcd0f655e20f71399` |
| `include/editor/service/ScenePreviewPublicationPreparationService.h` | `a618165ea06350d38df3e5a74728de5de171d09a8d328569aad084fce3695de8` |
| `include/editor/service/CompletedSceneGenerationPublicationService.h` | `0e2e9be5e36f48b2aef50f9accdfa9f6288c740bafa4bb93b368e439bc469eb8` |
| `include/editor/service/PreviewMaterialResourceFactory.h` | `ff09a0d90933ab52193d5f2ee4f9e7f4090fc96bdcaa47e3c8540ec65f8e71bf` |
| `include/editor/service/MaterialLanguageService.h` | `c1cad820c780f85548468174a15d217c294f95e06449b920f9221c8b2e51cef1` |
| `include/editor/service/Progen3dPreviewMaterialResourceFactory.h` | `04d2b56b819f6bb3b2838870e5ec7c0068451e568869b931e25d61e857d7e7af` |
| `include/editor/service/ScenePreviewGeometrySource.h` | `8c207c2edcf5b619da11d3bf637fb2329261d82504ea847b553e7ef548505035` |
| `include/editor/service/ScenePreviewBatchBuilder.h` | `9362acdfdc7d7da2e342fc2bc1ba9eafef8c7ca0b7c7e17bb80372653f096046` |
| `include/editor/service/ScenePreviewResourceService.h` | `70fd8bc13654ac069fe3a30b5ccb58ce85399d4d65910dca670b6c912cdd2890` |
| `include/editor/service/OpenGlScenePreviewResourceService.h` | `81502b4e7ae334d4779381bd1a43fe7ed7de88a54391cb009a26e0e0bd555d93` |
| `src/editor/application/EditorApplicationStartup.cpp` | `a33df2980ad4a4faaeb2133d3b0fbe48fe9970a0d22a39201f0ea5e2224f9523` |
| `src/editor/application/Progen3dEditorStartupOperations.cpp` | `14a91a24f5ee0920ef13e7c8e60db93baceb0bdb3877df01db7ea6fb6a04a021` |
| `src/editor/application/EditorApplicationRuntimeContext.cpp` | `fc75f04539f78e6ed750179fec63e84197de3b5a1b33212272bb2e3ddbb4c723` |
| `src/editor/application/EditorApplicationShutdown.cpp` | `392b0d612b58aaf8a3a461e521a0c7acc4783a86cf623fe946b8508d1289775f` |
| `src/editor/application/EditorFrameCycle.cpp` | `5b800c041423f3893829ead6df9a25db703bf6bd64f8f9a73fe82b84531628cb` |
| `src/editor/application/EditorInteractiveFrameLoop.cpp` | `2685d555c93dea0935548f8f7e43231a31cb856cb25a075dde3a4e87821e2cba` |
| `src/editor/application/EditorRuntimeEnvironment.cpp` | `69163e6e802adcc7739329c1bb2e53400690b3d06482fadcb7bbc17bda536a97` |
| `src/editor/application/EditorSmokeTestWorkflow.cpp` | `7983ccb512c535724700e997de901f67979aadf86e664a5ad09f50e39d278f19` |
| `src/editor/application/GlfwApplicationWindow.cpp` | `e5e4f94f7b9d65b8795fae476f050a233544d3039c6ba27b9f1c519b00cc79f9` |
| `src/editor/application/EditorSceneGenerationRuntime.cpp` | `38ba958d8943992acd7a7e35e0c29c6f77bcc5a1232da1b19cc753badb8ff50e` |
| `src/editor/model/EditorInterfaceResources.cpp` | `cad00f35eee7ac7572b5f99c3130c4d26ed6ef18fe3876e117546b569deb25d4` |
| `src/editor/model/EditorInterfaceState.cpp` | `0c2545a059739b29d17752700c391df498dcf45b92c7a0db6dd62faaa1d33e71` |
| `src/editor/model/ScenePreviewMaterialLibrary.cpp` | `d1d482b95ebb0317044effe40c0ab666778193f028c0f652929787b003c70eef` |
| `src/editor/presentation/EditorStartupProgressWindow.cpp` | `e9d6f44482105c7b491df7535e45fd7a67fbdd83f06cd066f4000f456b36b0f4` |
| `src/editor/presentation/EditorPresentationComposition.cpp` | `5087a66939da137853244eb155d04ecabb97189fe65588375f918e825bff6a17` |
| `src/editor/service/EditorStartupDocumentService.cpp` | `b856be950ea204db5e76b9d1e2670da28f091bd196487544e2e84b7c4de66407` |
| `src/editor/service/OpenGlEditorInterfaceResourceService.cpp` | `434ea8df01539becc751ceae532d76713544c0329e3d1b652565215fdb9f7579` |
| `src/editor/service/EditorDocumentPublicationService.cpp` | `4c322fe8ef62c7fa9b458dc3250ce8ee41fabc47ce407b2f7ea1df30836f7ed6` |
| `src/editor/service/CompletedSceneGenerationPublicationService.cpp` | `0d75af1b50ee98863cbdf66ba512ccdacdc4d39ec6a2183b4f78ff61b7bcf154` |
| `src/editor/service/ScenePreviewBatchBuilder.cpp` | `72d4d83843a9a9caa969bab8084a7657dcab8b7f1abae04b39d6b8d07874c7a8` |
| `src/editor/service/OpenGlScenePreviewResourceService.cpp` | `6f48740af293eba1e49b0d02bda516ae34f09118d6a1463c8c40f935b8058485` |
| `src/editor/service/MaterialLanguageService.cpp` | `2cfbed138e84e0e610207d6c04527147dc02fd4bbb5416519055f0482045c5da` |
| `src/editor/service/Progen3dPreviewMaterialResourceFactory.cpp` | `602055b6958a734b8cc1ace062d0fbb897ddb15ca9e0b93aa0f0de2f635faf9c` |
| `src/editor/model/EditorExitAuthorizationState.cpp` | `0b6193cfba257417a318d47292f57553c1c187ed01c5d5d0a586d0cc8a8d5391` |
| `src/editor/model/GrammarEditorInteractionState.cpp` | `2dbf73b416d88d442a22aee7ccbf95df2b2f78d77f782587e8866fa4918e15b3` |
| `src/imgui_main.cpp` | `097874435455dddb896edd50b1e7e228b7fd46cb65f7ff479f0238093e23f12f` |
| `tests/editor_application_startup_harness.cpp` | `bdf5b87c63a49bde1d4395dbd58764bd03812dd3d495b9051e1a28bddcd21371` |
| `tests/run_editor_application_startup_checks.sh` | `2181d44957d980f43aa00b59b35d44dd496be10c50d09375008f4dea0f9f218a` |
| `tests/editor_interface_resources_harness.cpp` | `690b6529058b7a48d086c58b2ef9a6302021a6980030da7b50ab938b86b9b9c6` |
| `tests/run_editor_interface_resources_checks.sh` | `f3014e05ff6d4a0b1caeadc00b8a82e1ef0cb78ebf8bacef54165661bfcf634d` |
| `tests/editor_application_composition_harness.cpp` | `d9f4b8360d8acff3ccae12b0326dab6a2b8e5d2be8f2fbbecc3874ef67dd6288` |
| `tests/run_editor_application_composition_checks.sh` | `ed901e72c308dce7dca007a24e82e6d3a09f44ef159da595b07c272b7655ba22` |
| `tests/editor_application_lifecycle_harness.cpp` | `1ebe11ef9ba85acaf35c75df69fcc61f1b16ac5007fbc9a49fd10e6f52d7a8a4` |
| `tests/run_editor_application_lifecycle_checks.sh` | `85ed133a09bdd4fcff519c14262d6ed9dea66cff282186b72f7629855778bee5` |
| `tests/editor_application_runtime_context_harness.cpp` | `26314468c9b59c071453bf458ab823b3506760c332b2f223a20810307f3aa6a7` |
| `tests/run_editor_application_runtime_context_checks.sh` | `d2a4542839dc8e913d6deab0e685e016353b65ae99a5055f26e49b8b3ebc5937` |
| `tests/editor_scene_generation_runtime_harness.cpp` | `a8e44861b88dcd5c08bc78423729a96152d721b01fd795f1ce071c2cc052c53e` |
| `tests/completed_scene_generation_publication_service_harness.cpp` | `f7921d4ad70d6499f4983057a97bfb0fa64ef4f1ffdebf7828f7728fc8ea1ac0` |
| `tests/run_completed_scene_generation_publication_checks.sh` | `85be94e197a20813d8aeed5fbdd12360cf593d3ee582978444d23acfd291bdc7` |
| `tests/editor_document_publication_service_harness.cpp` | `bbca07cc937bf9f2ab540ecbdaea9062a6870201af354365bad1a29e4fd54d5b` |
| `tests/run_editor_document_publication_service_checks.sh` | `38071b3e6d8bc7bcd55a096c3ae8fce268553c047e459bd2caadaf7d4ed02088` |
| `tests/run_editor_scene_generation_runtime_checks.sh` | `86a96ed1e401b81001fb17ec9c0ac99b5b962f5f04b584ecba0e8dd3a3703e22` |
| `tests/run_editor_workspace_model_checks.sh` | `afa215fe6e15da1ef84a74dcca812a643cc084fe9a5a79e3ccaaffce4703740e` |
| `tests/harness/NullScenePreviewResourceService.h` | `6bf74d6781abf1d7912297ec79fbdefba307f519a52b6c5c539eb9ae5ccc19d5` |
| `tests/scene_preview_material_library_harness.cpp` | `7f1c5936800d93e40f9baa51fbb5be630e3b1f83c0bfdaeb1a346de420c5d5a1` |
| `tests/scene_preview_batch_builder_harness.cpp` | `cd42ce62f9b1ebe8411b6bd5a7c5fafd32bdcbfd0d171ac781b24a0d67f18fb6` |
| `tests/run_scene_preview_resource_checks.sh` | `06e42cda3b59f94c95f67dac56274d23cdaa6fbeb86369426a3b6de786ddd8ed` |
| `tests/material_language_service_harness.cpp` | `dc4b5c2cf5fc5bf1704657c8b05aa2c015196a02cd3873bc73e172c835e01fe1` |
| `tests/run_material_resource_architecture_checks.sh` | `825f4246a6a73b2b2ad8861741627166a8fb74473392f932a7cdaa3f93f27148` |
| `tests/run_editor_architecture_checks.sh` | `3e464d70a6fff60be1c01d6b2088a3bf94b78284868d3da60c2a2e5f66353233` |
| `tests/README.md` | `228811ab063c8b6c94eaf6bad257f89a6930aad69186658174ce83c034f8c932` |
| `tests/test_p2_time.py` | `8c2f9185e5719f32062f1c6399ff1119b90f7c91722eee2bbabd4dde8330cf86` |
| `Makefile` | `c75a58a6df1f7601632cd95865c18098e25f1c96550fbf66d49b5b9cb5f4854d` |

## Known Incomplete Architecture

The interim runtime-context pointer and its preview/generation compatibility facade are removed. Document publication, completed-result state projection, preview material ownership, preview geometry batching, material-language interpretation, concrete material texture resources, interface state, presentation-object lifetime, frame execution, interactive repetition, smoke acceptance, exit authorization, and shutdown ordering now have explicit model, service, or application-composition owners. The remaining editor problem is broader responsibility concentration rather than those ownership bridges.

`src/imgui_main.cpp` no longer owns startup stage sequencing, startup progress rendering, the concrete startup operations adapter, font loading, UI-image loading, UI-image destruction, startup document persistence, editor-document state replacement, completed-generation state projection, canonical preview material state, preview batch construction, the OpenGL preview publication service, material lexicon/parsing/autocomplete semantics, procedural material synthesis, backend texture decode/upload, generated material maps, material texture deletion, frame execution, the interactive loop, smoke-test acceptance, shutdown ordering, exit-authorization state, focus-restoration mutation, `GrammarEditorInteractionState`, `EditorInterfaceFontSet`, `ApplicationLog`, or presentation-object instances. It still contains the request-generation bridge, seeded design state, local/cloud document workflows, authentication workflow logic, AI review workflow logic, lighting and electrical presentation procedures, preview orchestration, diagnostics, texture-library presentation procedures, and numerous GUI render paths. The narrow `RunningApplicationLogBinding` remains as a temporary association for the legacy external free logging API and should be replaced when those cross-translation-unit hooks gain a typed context interface.

The largest remaining translation units are:

| Translation unit | Lines |
|---|---:|
| `src/imgui_main.cpp` | 9,978 |
| `src/Grammar.cpp` | 6,301 |
| `src/Context.cpp` | 3,678 |
| `src/imgui_render.cpp` | 3,379 |
| Total | 23,336 |

Passing architecture tests prove the completed ownership slices. They do not prove complete repository refactoring.

## Next Refactor Slices

1. Move local and cloud document workflows out of the monolithic editor translation unit into explicit workflow and gateway services.
2. Extract authentication and AI review workflows while preserving their authority, feedback, and asynchronous state boundaries.
3. Extract lighting, electrical, texture-library, diagnostics, and preview-orchestration presentation responsibilities.
4. Replace the temporary running-log binding with a typed logging context for legacy cross-translation-unit diagnostics.
5. Split grammar parsing, validation, expression evaluation, expansion, and runtime publication responsibilities currently concentrated in `src/Grammar.cpp`.
6. Split scene state, geometry construction, collision, simulation, spatial binding, and publication responsibilities currently concentrated in `src/Context.cpp`.
7. Split render-resource ownership, draw passes, preview overlays, capture, and camera projection responsibilities currently concentrated in `src/imgui_render.cpp`.
8. Audit all legacy duplicate source paths, generated artifacts, build outputs, packaging inputs, repository hygiene, and all native/grammar/visual/GUI/packaging evidence before any completion claim.

## Completion Rule

The complete-refactor goal remains active until every major responsibility has a canonical owner, the four monolithic translation units have been decomposed into purpose-specific classes and services, interim globals are removed or justified by an explicit external API boundary, compatibility and deterministic evidence are preserved, and repository-wide validation proves the resulting architecture.

## 2026-08-26 Document Workflow Ownership Slice

This append-only record supersedes the earlier statement that local and cloud document workflows remain in `src/imgui_main.cpp`. The complete-refactor goal remains active; this record certifies only the document-workflow slice.

### Governed OURD review

The implementation was preceded by a read-only OURD review over an isolated context workspace. The model output was advisory; the checked source, explicit human corrections, native compiler, deterministic harnesses, architecture suite, and GUI smoke gates remained authoritative.

| Evidence | Value |
|---|---|
| Repository source snapshot value | `1110c99e5b0daeb2493c7694cb52681bf4268cb9826fbcad1fa88e347389a3e7` |
| Isolated review-workspace snapshot value | `33b78c735a55325950ba6f09e7eed8645c232f87de132d0b9845a870929e3f8d` |
| Authority manifest SHA-256 | `90e2effaa7cfa0627738780c56c444b1eaebdaafebd644e568efd2cb967788e0` |
| First review SHA-256 | `cca0e008edf9725c55e8270c420f063d61ab67c2ec58e7feb8662e4b86bb1213` |
| Follow-up review SHA-256 | `3b701e829ccca3ab23c74bf8c959f0de2c6076e95151f43588442c77c22b483a` |

Human review retained the existing `GrammarSourceDocument`, `EditorDocumentPublicationService`, `EditorSceneGenerationPublicationService`, `GlfwApplicationWindow`, and lifecycle contracts. It rejected generic request/window policy abstractions and kept document mutation behind the existing publication boundary.

### Object model and ownership

The document workflow now reads as an explicit class model:

- `LocalDocumentWorkflow`, `CloudDocumentWorkflow`, and `EditorDocumentWorkflow` define behavior contracts used by higher-level collaborators;
- `LocalDocumentWorkflowService`, `CloudDocumentWorkflowService`, and `EditorDocumentWorkflowService` implement those contracts and are composed by `Progen3dEditorApplication`;
- `BackendCloudGrammarRepositoryFactory` implements `CloudGrammarRepositoryFactory`, isolating concrete backend repository construction;
- `CloudDocumentRequestMailbox` owns asynchronous result synchronization and rejects callbacks whose operation epoch was cancelled or superseded;
- `CloudDocumentDialogState` owns only presentation state; request handles, epochs, and synchronization no longer live in `EditorWorkspaceSession`;
- `CloudDocumentDialog` and `UnsavedChangesDialog` own their ImGui presentation instead of leaving modal procedures in `src/imgui_main.cpp`;
- `EditorApplicationRuntimeContext` exposes workflow contracts rather than concrete service classes, preserving testable UML-readable associations;
- `EditorApplicationShutdown` cancels cloud operations before destroying runtime and scene-publication collaborators;
- cloud list/open operations now pass through `DocumentPersistenceService`, preserving one canonical persistence boundary.

`CloudAuthWorkflow` no longer owns cloud list, open, save, publish, depublish, new-document, or document-identity operations. It retains authentication, AI-session, and texture-session responsibilities pending the next slice.

### Behavioral acceptance

`tests/run_editor_document_workflow_checks.sh` adds two focused native harnesses and ownership gates:

- `cloud_document_request_mailbox_harness.cpp` proves one-shot result consumption, superseded-epoch rejection, cancellation invalidation, and failed-operation diagnostic retention;
- `editor_document_workflow_service_harness.cpp` proves unidentified-document Save As routing, local and cloud save routing, dirty replacement confirmation, clean cloud opening, exact selected-path routing, Save As cancellation, save-before-replacement continuation, canonical new-document publication, scene regeneration, and clean exit authorization;
- source gates reject legacy free document procedures, cloud request synchronization inside `EditorWorkspaceSession`, document operations inside `CloudAuthWorkflow`, concrete workflow leakage through the runtime context, and missing native build registration.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Document-slice source manifest | 45 source and gate paths | `912880a8a5e345f67156feba2da34ee9c61f34c103213385800fe07a10e27116` |
| `tests/run_editor_architecture_checks.sh` log | Passed all editor architecture checks | `b6ecea9cc4c7be71033a437fdde120337fdedad747838cec895afcc871195a61` |
| Final native build log | `progen3d-editor-gui` compiled and linked | `d5cddda36cfc3a4e1b9a0368970043e10f4f868468813875a2c6c81498bb479b` |
| Final GUI binary | Exact executable used by both smoke gates | `8f97aef301447842b5440b65256e45ccef5533d4b40c6c89b9e0a3a1d7f539a0` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |

The binary hash was rechecked after both smoke runs and remained unchanged.

### Updated concentration boundary

`src/imgui_main.cpp` decreased from the prior recorded `9,978` lines to `9,475` lines. `src/CloudAuthWorkflow.cpp` is now `201` lines. The document slice removed local/cloud workflow procedures and both modal implementations, but the complete refactor is not finished.

The next ordered slice is authentication and AI review workflow extraction, followed by lighting, electrical, texture-library, diagnostics, and preview-orchestration presentation extraction. Grammar, context, and renderer decomposition plus repository-wide packaging and hygiene validation remain mandatory before a complete claim.

## 2026-08-26 Authentication and AI Workflow Ownership Slice

This append-only record supersedes the earlier statement that authentication and AI review workflow logic remain in `src/imgui_main.cpp` and that `CloudAuthWorkflow` retains those responsibilities. The complete-refactor goal remains active; this record certifies only the authentication and AI workflow slice.

### Governed OURD review

The implementation was preceded by a read-only OURD review over a bounded 134-line context and an exact isolated source snapshot. The authority manifest permitted reading only `context.md`, allowed no repository writes, and required human review of the proposal.

| Review evidence | Value |
|---|---|
| Bounded context SHA-256 | `b26f2d65a653b677450055a41a4b42598c982e202d099f9eca10ccdc13978847` |
| Repository source-snapshot value | `18d52abf3139c5c281cb966f8e25ad82014607ab6ec94f5aa88499ab51204ad9` |
| Repository snapshot artifact SHA-256 | `0d22fec5c43e14943bce099e7ea03a8901aec5fcba12c544903a0c7b27a3e292` |
| Isolated review-workspace snapshot value | `c3a5f023e9d2cd8a696ad4612cc82e3bd0c7075c8339a0312dd6177302506aff` |
| Isolated snapshot artifact SHA-256 | `13d732066bd9bd6a0ad207d5f529c4ab023a7d50d446b3f036a2ec4e9a83f112` |
| Authority manifest SHA-256 | `4cd790d59862af508687dcee8cbaf6c13465fc05a076a26d32b2b4073460b024` |
| Human-review record SHA-256 | `45df24c29d4f8d2c543389cdb465c11e5c406ecf63c33b46c31f5d585fe5420b` |

The preferred `qwen3.8-27b-fast` provider was verified at digest `07cb98f8840ce491fc28c04a5ecc13c4dec5fd23d9a4732878bc3c02acb5b005`, but its bounded attempts failed closed: the 7,000-token attempt rejected an estimated 7,442-token request, and the later attempts returned malformed governance/write tool arguments. Their log hashes are `d1f8720ea17de905b44d88d29c1c8277ed6e6d3831236aa2d753b13261d7ba3c`, `295bd465b4afd11930690ad1f0e360307ddadf8c45cce852308fbcec77971a9d`, and `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`. No malformed model action was promoted.

The governed fallback review used the verified `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`; its successful review/preflight log SHA-256 is `e1bcc189ff8822b3e222ca4561b784847fca4e600d43dd11db28f87c1d9ea888`.

Human review accepted explicit workflow services, model-owned state transitions, and deterministic behavior gates. It rejected the proposed vague `FirebaseAuthenticationServiceManager`, rejected panel construction of services, and retained document publication as an application-composed authority rather than an AI-panel responsibility.

### Object model and authority

The authentication and AI workflow now read as explicit class relationships:

- `AuthenticationWorkflowService` coordinates authentication through the `AuthenticationSessionRepository`, `BackendSessionWorkflow`, and `TextureLibraryWorkflow` contracts;
- `FirebaseAuthenticationSessionRepository` adapts `FirebaseAuthenticationService` without exposing that concrete provider to the presentation layer;
- `BackendSessionWorkflowService` owns backend profile synchronization and the capability-state consequences of synchronization failure;
- `AiConversationWorkflowService` coordinates thread refresh, thread loading, bounded request construction, and proposal staging through `AiGrammarProposalServiceFactory`;
- `BackendAiGrammarProposalServiceFactory` isolates concrete backend proposal-service construction;
- `AiGrammarProposalPublicationService` is the sole collaborator that converts an explicit human accept or reject decision into document publication or review-state change;
- `AuthenticatedUserSession` owns credential-buffer and authenticated-session transitions;
- `AiAssistantSession` owns conversation, thread-list, request-history, feedback, result, and staged-review transitions;
- `GrammarEditorInteractionState` owns exact selected-source extraction and source synchronization;
- `EditorDocumentPublicationService` publishes accepted edited source while preserving local/cloud identity, marking the document dirty, synchronizing editor interaction, restoring focus, and routing scene generation through the canonical publication boundary;
- `Progen3dEditorApplication` composes all concrete services, while `EditorApplicationRuntimeContext` exposes only the `AuthenticationWorkflow`, `AiConversationWorkflow`, and `AiGrammarProposalPublication` contracts.

The authority boundary is fail-closed. AI conversation operations can stage an `AiGrammarProposalReview` but cannot mutate the document. Acceptance requires an exact-source match before the review decision is applied. Rejection never mutates the document or schedules scene generation. Backend synchronization failure preserves the valid Firebase identity while clearing backend-dependent user, AI, texture-library, and material-resource capability state and requesting regeneration.

### Presentation reduction

`src/imgui_main.cpp` no longer constructs authentication or proposal services, assembles AI requests, applies AI results, loads AI threads, mutates AI session state directly, or implements proposal acceptance/rejection as free procedures. Authentication declarations were removed from `CloudAuthWorkflow`; `src/CloudAuthWorkflow.cpp` is now an intentionally empty one-line compatibility translation unit for the remaining shared presentation/log/texture declarations.

The extracted source gates reject presentation-layer service construction, direct backend persistence calls from panels, direct review mutation from presentation code, legacy AI orchestration free functions, concrete workflow leakage through the runtime context, AI-conversation document mutation, and missing native build registration.

### Behavioral acceptance

`tests/run_authentication_ai_workflow_checks.sh` adds three focused native harnesses:

- `authentication_backend_workflow_harness.cpp` proves backend success, backend failure with retained Firebase identity, saved-session refresh, signout cleanup, authenticated feature refresh, token persistence, capability clearing, material release, and regeneration requests;
- `ai_conversation_workflow_harness.cpp` proves token refresh persistence, thread refresh/load behavior, bounded diagnostics and history capture, exact selection capture, proposal staging, failure-credit updates, and the absence of document mutation or scene generation during conversation operations;
- `ai_grammar_proposal_publication_harness.cpp` proves fail-closed source mismatch handling, full and selected acceptance through canonical document publication, local/cloud identity preservation, dirty-state publication, focus and scene-generation requests, and mutation-free rejection.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Authentication/AI source manifest | 49 source and gate paths | `13895a458a0d4d676f236290353cac7585cc2a8cc3e58b8476570c47b13de3e6` |
| Dedicated authentication/AI checks log | All behavior harnesses and source gates passed | `0583beec937d4b9e4cf70dae8714cea3ba477ea5fe842ffbc85b18c951de6dbf` |
| Affected architecture retry log | Runtime-context, publication, and document-workflow gates passed | `f2047e8003ace2cc30bed79fc5f290f2d407a1f5f4fef664c482df5f53422098` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `083ec939edb1ca3e05381e44432f3cb9da586d09a1688e2dee4813f066e23673` |
| Final native build log | `progen3d-editor-gui` compiled and linked | `5523bdfa88a39c1d40ac9b7f3604847f11f938df8f30ba1a9204cbedffd9dcec` |
| Final GUI binary | Exact executable used by both smoke gates | `19bf730a105c213d4e83d274083dac4e3e8f8f401517b2ad7d392e9c928b60d1` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-refactor OURD snapshot value | Current repository source snapshot | `b579796913542c88de0678c20beae3ca44023e610e078073b5b4b8d1165db938` |
| Post-refactor snapshot capture | Exact snapshot-output artifact | `c3a4bdbd9fbd96829fca7d63d503270c3c09e7004ac37f3bff003b149562d8f1` |

The binary hash was rechecked after both smoke runs and remained unchanged.

### Updated concentration boundary

`src/imgui_main.cpp` decreased from the prior recorded `9,475` lines to `9,029` lines. `src/CloudAuthWorkflow.cpp` decreased from `201` lines to `1` line. The authentication and AI slice removes workflow authority and state-transition ownership from presentation code, but the complete refactor is not finished.

The next ordered slice is texture-library and material-session presentation extraction, because `EditorTextureLibraryWorkflowService` still adapts the legacy `clear_texture_library_state` and `refresh_texture_library` entry points and substantial texture-library orchestration remains in `src/imgui_main.cpp`. Lighting, electrical, diagnostics, preview orchestration, typed logging context, grammar decomposition, context decomposition, renderer decomposition, packaging, hygiene, and repository-wide validation remain mandatory before a complete claim.

## 2026-08-26 Texture Library Workflow Ownership Slice

This append-only record supersedes the earlier statement that `EditorTextureLibraryWorkflowService` still adapts legacy presentation entry points and that substantial texture-library orchestration remains in `src/imgui_main.cpp`. The complete-refactor goal remains active; this record certifies only the authenticated texture-library workflow slice.

### Governed OURD review

The implementation was preceded by a read-only OURD review over a bounded 155-line context and an exact isolated context workspace. The authority manifest allowed reading only `context.md`, allowed no commands or source writes, and required a separate human architecture decision.

| Review evidence | Value |
|---|---|
| Bounded context SHA-256 | `c44a7efc887cd97ab510cea43e2663cc33d084ee1d1e9d5c0fc2eae7dc380635` |
| Repository source-snapshot value | `7716cccbe1a739824220d1c54a4f6d738aa1b946f7fd943d74ee8e343bdcec60` |
| Repository snapshot artifact SHA-256 | `4e9b1ec2ae79f38df78e375d25c2515cd94534411e67178b431679334d58858c` |
| Isolated review-workspace snapshot value | `3980758e70b02ba06b4afaa04bfe0ff3bed9e65b75260a3bfd79b5f368a8335e` |
| Isolated snapshot artifact SHA-256 | `554069c8fb2664a92185ccdef268f44c38cdfaaec915a5748e8df7ebbab6588e` |
| Authority manifest SHA-256 | `35a6e1fb6457e2e15f8cd5bbdbf37a25a92b31355d6cf12b0f30a43957f6fbbe` |
| Governed review log SHA-256 | `fdaca827dbfb6c6b57834aef5d9d20c9b9b20483757ebe363a1b9de6c2a743a3` |
| Human-review record SHA-256 | `180dba2de3dfa8e887dda0997f8a38f78bd69371688b4bce5d175dddb2ca01f6` |

The governed review used the verified local `qwen2.5:14b` model digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, with a 9,000-token context budget, 2,200-token output limit, zero transport retries, and no write capability.

Human review accepted the explicit session model, repository abstraction, preview-resource abstraction, workflow service, thin presentation, and fake-based native tests. It rejected repository operations on the session model, rejected OpenGL identifiers on backend texture entries, rejected a duplicate `SceneMaterialService`, retained the existing `TextureLibraryRepository` name, and required reuse of the existing scene-resource and regeneration authorities.

### Object model and ownership

The texture library now reads as an explicit class model:

- `TextureLibrarySession` replaces the passive `TextureLibraryState` struct and owns canonical twenty-slot normalization, selected-slot preservation, selected-entry lookup, editable-input synchronization, alpha clamping, active-slot counting, feedback publication, and clearing;
- `TextureLibraryPreviewImage` is a read-only presentation descriptor for one published preview identity, size, and texture identifier;
- `TextureLibraryPreviewResourceService` defines preview identity matching, publication, inspection, and deterministic release;
- `OpenGlTextureLibraryPreviewResourceService` implements that resource contract through `OpenGlPreviewTextureService`, owns the published GPU resource, replaces it only after successful decode/upload, and releases it during explicit clearing or destruction;
- `TextureLibraryRepositoryFactory` defines authenticated repository construction;
- `BackendTextureLibraryRepositoryFactory` creates `BackendTextureLibraryRepository` instances associated with the current Firebase configuration and mutable session;
- `TextureLibraryWorkflow` now exposes explicit refresh, selection, preview reload, metadata save, deletion, upload, generation, clearing, and preview-inspection intents;
- `EditorTextureLibraryWorkflowService` coordinates repository operations, preview publication, purpose-specific feedback, loaded-credit publication, material invalidation, and scene-regeneration requests;
- `AuthenticatedUserSession::applyBackendCredits()` preserves the authenticated session as the canonical credit owner and ignores unloaded credit summaries;
- `EditorApplicationRuntimeContext` exposes the `TextureLibraryWorkflow` contract rather than the concrete service;
- `Progen3dEditorApplication` composes the backend repository factory, OpenGL preview-resource service, and texture workflow service;
- `EditorApplicationShutdown` clears the texture workflow before authentication and OpenGL runtime shutdown, ensuring deterministic preview-resource release while the graphics context is still valid.

`BackendPreviewMaterialFactory` now queries arbitrary user-texture slots through `TextureLibrarySession::entryForSlot()`, removing its dependency on the deleted passive state type while retaining the existing backend material creation boundary.

### Presentation reduction

`draw_texture_library_controls()` now renders `TextureLibrarySession`, edits its explicit input buffers, and forwards user intent through `TextureLibraryWorkflow`. It no longer constructs `BackendTextureLibraryRepository`, calls backend APIs, normalizes entries, loads or releases GPU previews, publishes credits, invalidates scene materials, requests scene generation, or mutates selected-slot state directly.

The legacy free procedures for entry lookup, selection lookup, preview destruction, state clearing, feedback publication, entry normalization, preview loading, input synchronization, entry application, material invalidation, refresh, and active-slot counting were removed from `src/imgui_main.cpp`. The texture declarations were removed from `CloudAuthWorkflow`; `src/CloudAuthWorkflow.cpp` remains a one-line compatibility translation unit only for the still-external logging and text-trimming functions.

### Behavioral acceptance

`tests/run_texture_library_workflow_checks.sh` adds two native harnesses and source gates:

- `texture_library_session_harness.cpp` proves canonical normalization, placeholder creation, non-canonical rejection, active counting, selection, input synchronization, alpha clamping, feedback preservation/clearing, invalid-selection stability, and full reset semantics;
- `texture_library_workflow_harness.cpp` proves unauthenticated clearing, exact preview release, authenticated refresh, preview identity reuse, active-slot selection, mutation argument forwarding, mutation failure preservation, mutation success invalidation, scene regeneration through existing authorities, upload routing, generated-credit publication, unloaded-credit rejection, deletion to inactive placeholder, list-failure preservation, and explicit clear behavior;
- source gates reject GPU ownership in `TextureLibrarySession`, legacy free texture procedures, `CloudAuthWorkflow` texture bridges, backend repository construction in the texture panel, direct panel credit mutation, direct panel scene authority, concrete workflow leakage through the runtime context, missing shutdown cleanup, and missing native build registration.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Texture workflow source manifest | 38 source and gate paths | `9e21645c2da6b1b172185dd6598696ff45631bd5a2c7e11ea5bd68396a9a9ac3` |
| Dedicated texture workflow checks log | Model harness, workflow harness, and source gates passed | `5124d85c19076d7a5878227531c39846a30d1e91c1728be0b598c69f671ef61e` |
| Affected architecture checks log | Runtime, authentication, document, composition, lifecycle, and material gates passed | `004873edc3bf47df55f8f13306303e34b0860c394095c9ad4d97bc05d5bfd8e5` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `3efc458a582c9a7c63f3b04023c8e7b395c3fee46ae1acb6c08d4f1a010c4b61` |
| Final native build log | `progen3d-editor-gui` compiled and linked | `9eadc73a9b2fcb4660be3d69b8ba25120646b46fb6aca0a4ac9c739c7c41ff97` |
| Final GUI binary | Exact executable used by both smoke gates | `ce23e13d39fa1753e2b7b5a6cb0c621213bdbc99e276477e4f6c26489fbc7408` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-refactor OURD snapshot value | Current repository source snapshot | `91c3452dcd648f788bcb887c3149a48f1265829a4f14897896534805b4086c82` |
| Post-refactor snapshot capture | Exact snapshot-output artifact | `46b3cdf47b512b23dbb16bca7f829bda5455cc8e45b49cd783ee7a3b3b5a118f` |

The binary hash was rechecked after both smoke runs and remained unchanged. Existing grammar and mesh signedness/unused warnings remain non-fatal and were not promoted as evidence of this slice.

### Updated concentration boundary

`src/imgui_main.cpp` decreased from the prior recorded `9,029` lines to `8,735` lines. `src/CloudAuthWorkflow.cpp` remains `1` line. The texture-library slice removes backend, state-transition, GPU-preview, credit, material-invalidation, and regeneration ownership from presentation code, but the complete refactor is not finished.

The next ordered slice is lighting and electrical presentation workflow extraction, followed by diagnostics, preview orchestration, and the typed logging context. Grammar parsing/evaluation decomposition, scene-context decomposition, renderer decomposition, packaging, repository hygiene, and complete native/grammar/visual/GUI validation remain mandatory before a complete claim.

## 2026-08-26 Lighting and Electrical Controls Workflow Ownership Slice

This append-only record completes the bounded lighting and electrical controls workflow slice. It does not complete the repository-wide refactor goal.

### Governed architecture review

A fresh pre-review OURD repository snapshot produced source value `4a4b42110c351c778f6ef6eb9ce46ac136e0a0f7eb8e791b58d4715906c172eb`; the exact captured output artifact is `build/tests/ourd_lighting_controls_snapshot.txt`, SHA-256 `6f28bbe34c705a4cb361cf2564d24bfca12bf40cecfc1ca1879e3edf274f8833`.

The bounded review brief is `build/tests/ourd_lighting_controls_context.md`, 182 lines, SHA-256 `4d32e2fdbcf585023092fab79000a559f8a1fbb635c3cf14abcb3d540d46853e`. It records the raw mutable-panel boundary, existing preset, validation, evaluator, persistence, evidence, and completed-publication authorities, duplicate-authority risks, required mutation semantics, and deterministic acceptance gates.

The brief was copied into the isolated read-only workspace `/tmp/progen3d-ourd-lighting-controls-review-20260826`. Its isolated source snapshot value was `354e7a1fdfbc370370e99c6a41f53c92db0a360a5258aa98730108ae6e1510e1`; the snapshot-output artifact is `build/tests/ourd_lighting_controls_isolated_snapshot.txt`, SHA-256 `d15dcf4d07f3c412fee0331e27b27463669c3a742fe191d0bbd924bcd1b98dfc`.

The read-only authority manifest is `build/tests/ourd_lighting_controls_authority.json`, SHA-256 `63a3541cc455956da55def366c86c2a75e0884934539dcc333cb5e7e6af72036`. OURD used local `qwen2.5:14b`, model digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, 9,000 context tokens, 2,200 output tokens, zero transport retries, semantic ceiling `C1`, risk ceiling `L0`, no command capabilities, and no write authority. The retained review log is `build/tests/ourd_lighting_controls_review.log`, SHA-256 `87f2fccc344d7e12606be62b6d41861e4e196935e916c5e568d0a8930462c432`.

Human review is retained at `build/tests/ourd_lighting_controls_human_review.md`, SHA-256 `85df702065063ce4aca7c0f5348f0c815c6620268e2397d6abf09f7c1e83bf1d`. It accepted separate lighting and electrical workflows, copied-light draft editing, workflow-accessed validation, and explicit application composition. It rejected duplicate generic evidence and validation services, concrete workflow leakage, presentation-owned identity or provenance replacement, and a broad lighting-controls manager. Human approval remained bounded to architecture; OURD did not author, mutate, approve, or certify the implementation.

### Canonical evidence publication

`LightingEvidencePublisher` now expresses the publication role for canonical `LightingSceneState::evidence`. `LightingEvidencePublicationService` is the one concrete publication authority and composes the existing `LightingEvidenceHashService`; it calculates evidence from the current light collection, fixture and circuit counts, and active preset name.

`Progen3dEditorApplication` owns one shared `LightingEvidencePublicationService`. The same association is supplied to the lighting workflow, electrical workflow, completed-scene publication service, local document workflow, and startup document service. Successful completed-scene publication evaluates preserved electrical state and then publishes lighting evidence once. Successful local and startup lighting-sidecar loads also synchronize evidence through the shared authority. Stale, failed, and preview-preparation-failed scene publications leave evidence unchanged.

### Lighting workflow object model

`EditorAuthoredLightFactory` now owns the default construction rules for editor point, spot, directional, and rectangular-area lights, including purpose-specific identity bases, position, provenance, photometric unit, intensity, and directional emission. `PreviewLightCollection` remains the final unique-identity authority.

`LightingSceneWorkflow` is the abstract intent contract exposed by `EditorApplicationRuntimeContext`. `EditorLightingSceneWorkflowService` implements preset discovery and application, stable light selection, editor-light creation, copied-draft replacement, provenance-aware removal, light-gizmo visibility, lighting validation forwarding, and explicit evidence synchronization. It composes the existing `LightingPresetCatalog`, `EditorAuthoredLightFactory`, and `LightingValidationService`, and associates with the shared evidence publisher.

Preset application occurs on a candidate collection and fails closed for unknown names. Selected-light replacement restores canonical `LightId` and `LightProvenance`, validates the candidate collection, and commits only an accepted draft. Grammar-provenance lights cannot be removed through editor intent. Accepted evidence-changing transitions publish exactly once; rejected and presentation-only transitions do not publish.

### Electrical workflow object model

`ElectricalControlWorkflow` is the abstract intent contract exposed by `EditorApplicationRuntimeContext`. `EditorElectricalControlWorkflowService` implements controls-mode activation, switch on/off intent, bounded dimmer intent, and electrical validation forwarding. It composes the existing `LightingStateEvaluator` and `ElectricalControlValidationService`, and associates with the same evidence publisher.

Switch intent is located by `ElectricalObjectId`. Non-finite dimmer input fails closed, finite dimmer input is clamped to `[0, 1]`, and no-op assignments do not evaluate or publish. Each accepted switch mutation evaluates the electrical graph once and publishes the resulting lighting evidence once. Controls-mode activation is explicit but does not republish because it is not an evidence input.

### Presentation boundary

`LightingPanel::draw()` and `ElectricalControlsPanel::draw()` now receive `EditorApplicationRuntimeContext&`. Both render const canonical state and forward typed intent through abstract workflow contracts.

The lighting panel edits a copied `SceneLight` draft only. It no longer constructs `LightingPresetCatalog`, `LightingValidationService`, or `LightingEvidenceHashService`; creates domain lights; applies presets; changes canonical selection, preset, or gizmo state; mutates canonical light properties; removes lights; or assigns evidence directly.

The electrical panel no longer mutates controls mode or `LightSwitch::state()`, constructs `LightingStateEvaluator` or `ElectricalControlValidationService`, or performs electrical-to-light evaluation during rendering.

### Behavioral and source acceptance

`tests/lighting_controls_workflow_harness.cpp` and `tests/run_lighting_controls_workflow_checks.sh` prove:

- catalog preset discovery, accepted preset publication, and unknown-preset fail-closed behavior;
- all four editor-authored light types, unique identities, selection, editor provenance, and custom-state publication;
- copied-draft replacement with stable canonical identity and provenance;
- invalid-draft, unknown-selection, and grammar-light-removal rejection without state or evidence publication;
- accepted editor-light removal, selection clearing, and exact publication;
- gizmo and controls-mode no-op semantics without evidence publication;
- switch lookup, on/off evaluation, dimmer clamping, non-finite rejection, authored-exposure preservation, control-exposure calculation, and exact publication counts;
- lighting and electrical validation forwarding;
- abstract runtime-context associations and concrete application composition;
- no mutable `LightingSceneState*` panel signatures, presentation-owned domain service construction, direct canonical field or switch-state writes, direct evidence assignment, concrete workflow leakage, missing publication/load synchronization, or missing Makefile registration.

The complete editor architecture suite now includes the inherited lighting model, lighting grammar-to-scene, lighting persistence, and new lighting/electrical workflow checks. `tests/README.md` documents the new acceptance boundary.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Lighting controls source path list | 40 source, integration, documentation, harness, and gate paths | `9749dfdcda002b071d3b113ab0d79b179de012bae07b6a94a999bb302168487a` |
| Lighting controls source manifest | Exact SHA-256 manifest for the 40 paths | `c3c7f134ec8471b64dc57833a62b7f7da27709205827f6570d4b90547ae61fc9` |
| Dedicated lighting/electrical workflow log | Behavior harness and ownership gates passed | `31b2435b01ba00cc74af1ad6181c0303b161223e39f252ff7d09d51f265e5ea8` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `66a0914b47cc8719e7ee1281c634a715c6c580dfffc8e708524982aa5186676e` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked | `177e320c143eb2ca330b656b71527d735062e4f023b709cb7db68f678426f7ab` |
| Final GUI binary | Exact executable used by both successful smoke gates | `abd9239942e3c32f08e3e87c9a0a1f747e480ddbfd69d9b0811a80a55fae0c03` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `5296738636035401cb488fd30aa41c9dcb0fdb56f6f3478b97f8da1b14335f93` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `ee522375a4c0f24211ec9a36357414a3bb04ef09d1a594dddaba42bcdfadb12e` |

The binary hash was rechecked after both successful smoke runs and remained unchanged. The temporal smoke gate was rerun alone after an initial parallel invocation exited before launching; process inspection found no leftover ProGen3D or Xvfb processes, and the isolated rerun produced every required acceptance marker. Existing grammar, mesh, generated-catalog, and stb compiler warnings remain non-fatal and were not promoted as evidence of this slice.

### Updated concentration boundary

`src/imgui_main.cpp` is now `8,745` lines, compared with the prior recorded `8,735` lines. The ten-line increase is explicit application composition and runtime-context wiring; the lighting and electrical orchestration was already located in presentation translation units rather than inline in the monolith. `LightingPanel.cpp` is `296` lines and `ElectricalControlsPanel.cpp` is `78` lines, but their responsibilities are now restricted to rendering, local draft editing, and intent forwarding. `src/CloudAuthWorkflow.cpp` remains `1` line.

The next ordered slice is diagnostics and preview orchestration, followed by the typed logging context. Grammar parsing/evaluation decomposition, scene-context decomposition, renderer decomposition, packaging, repository hygiene, and complete native/grammar/visual/GUI validation remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Diagnostics Presentation and Source Navigation Workflow Ownership Slice

### Governed OURD review

The next diagnostics boundary was reviewed through the governed local OURD agent before implementation.

- The pre-review repository source snapshot value was `20a55c7e6ac729579165a9f0b66c040b7143c4e774e9a7c2bc658d09735f6a69`; the exact snapshot-output artifact SHA-256 is `94909607ec93f03581b8a7348f1b675836293f9706e8669e537070137c2c1c88`.
- The bounded 205-line review context SHA-256 is `1aab949b7cd79dbc4d011b9455d66eae190d00c5dfbd7bfdcfa5397a6e0f80d1`.
- The isolated read-only workspace snapshot value was `989a19cf6cd7a281660ca4960dd08ebe7958ab48b24400a9950c53f34f58c627`; the exact isolated snapshot-output artifact SHA-256 is `06e587fd9f890bf9acda4590fb81177f0fde03e72a7d1f588a9f2b695c3231da`.
- The exact-hash read-only authority manifest SHA-256 is `1aaa6b725a41f80313d25a459b891247eb321231e20505f7ea1a43d470b565a4`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD review log SHA-256 is `de5b9a5683d431e41198d3be3236526b98b68bdf642ae2ca6ee1b499f2c88901`.
- The human review and correction record SHA-256 is `87288f1a3619df5b5a4c6f74cafe2886bc077bd3bc88fde50fc26da9705f9c0e`.

The human review accepted the bounded diagnostic-preparation and typed source-navigation extraction. It preserved `DocumentDiagnosticCollection`, `GrammarDiagnosticPresentation`, `EditorWorkspaceSession::document.source_text`, `EditorInterfaceState`, and `GrammarEditorInteractionState` as existing authorities. It rejected a vague `EditorInteractionState` rename, broad runtime-context getters, whole-document fallback navigation, retained generic source-navigation callbacks, a duplicate diagnostic analyzer, a broad editor navigation manager, autocomplete-authority expansion, and grammar structural-validation expansion.

### UML-readable object model

- `GrammarSourceLineIndex` is the source-position model for one owned source-text view. It retains trailing empty lines, guarantees one empty line for empty source, validates line identity, clamps columns only within a valid line, and calculates document offsets.
- `GrammarDiagnosticWorkflow` is the abstract read-oriented application contract for the current document's editor diagnostics.
- `GrammarEditorDiagnosticWorkflowService` implements that contract by composing the existing `GrammarDiagnosticPresentation` and associating it with the current `EditorWorkspaceSession`; it contains no replacement syntax scanner or compiler-diagnostic merger.
- `GrammarSourceNavigationWorkflow` is the abstract intent-oriented application contract for selecting one `EditorRange` or moving the editor cursor to one source position.
- `GrammarEditorSourceNavigationWorkflowService` implements deterministic cursor, selection, synchronization, focus, and scroll requests over `GrammarSourceLineIndex` and the interface-owned `GrammarEditorInteractionState`.
- `Progen3dEditorApplication` owns both concrete services.
- `EditorApplicationRuntimeContext` associates with and exposes only the abstract workflow contracts.

### Fail-closed navigation semantics

- A range with `found == false`, a negative or out-of-bounds line, or a reversed range after valid-line column clamping is rejected without changing any cursor, selection, focus, synchronization, or scroll field.
- Columns on a valid line are clamped to `[0, line_length]`.
- A collapsed valid range is accepted as cursor placement with equal selection endpoints.
- Empty source text exposes line zero, column zero as a valid cursor target.
- Accepted navigation updates current and requested cursor and selection offsets, requests cursor and selection synchronization, requests focus and scroll, and records the target line.
- Navigation does not change source text, document dirty state, compiler diagnostics, preview state, or scene-generation state.

### Presentation and integration boundary

`DiagnosticsPanel::draw(EditorApplicationRuntimeContext&)` now retains only local error/warning filter state, rendering, and diagnostic-selection intent forwarding. It obtains immutable diagnostic presentation values through `GrammarDiagnosticWorkflow` and delegates selected ranges through `GrammarSourceNavigationWorkflow`.

The smart editor obtains underline diagnostics through the same workflow. `GrammarSymbolInspectorPanel` delegates previous/next occurrence selection through the typed navigation contract rather than a generic `std::function<void(const EditorRange&)>`. Preview-instance source navigation retains the existing scene-source association authority and delegates only the final selection or cursor transition to the navigation workflow.

The monolith no longer owns `breakup_into_lines`, `build_editor_diagnostics`, `select_editor_range`, or `jump_to_editor_location`. Autocomplete source replacement and its offset helper remain outside this bounded slice because they represent a distinct document-editing authority.

### Behavioral and source acceptance

`tests/editor_diagnostics_workflow_harness.cpp` and `tests/run_editor_diagnostics_workflow_checks.sh` prove:

- stable source-line ownership, trailing and empty lines, newline-aware offsets, valid-line column clamping, and invalid-line rejection;
- current-document syntax presentation and canonical compiler-diagnostic merging through the inherited authority without source mutation;
- accepted clamped selection, accepted collapsed selection, empty-document cursor movement, synchronization, focus, and scroll semantics;
- missing-range, invalid-line, reversed-range, and invalid-cursor rejection with exact preservation of navigation state;
- abstract runtime-context associations and concrete application composition;
- no diagnostics or navigation free-function ownership in `src/imgui_main.cpp`, no generic source-navigation callbacks in diagnostics presentation, no direct presentation mutation of interaction state, no duplicate analyzer logic, no concrete workflow leakage, and complete Makefile registration.

The complete editor architecture suite now runs the new workflow gate alongside the inherited `DocumentDiagnosticCollection` and `GrammarDiagnosticPresentation` checks. `tests/README.md` documents the new acceptance boundary.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Diagnostics source path list | 29 source, integration, documentation, harness, and gate paths | `1a95fe73505013c3ea973a864493b644d6d8cadbc3f050bf549715cde26e86f9` |
| Diagnostics source manifest | Exact SHA-256 manifest for the 29 paths | `bfc027588ebb499436b14ce6e993cb2ceedd45022940cbea69e1c6a016b4af17` |
| Dedicated diagnostics workflow log | Behavior harness and ownership gates passed | `9d1549353e0d36737871bda6c8db2bd5cc23d01ec547d78753d17a8287e6587a` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `e56ecb8454aeaab663c11f0cd3b0fe62b8beed28a53926a5212ea590dc163ff9` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked | `2b70bf9587b4d9ae37bb6fba48b7d4e9c1f9ab8cdd7ab92a5439a5448adf6d47` |
| Final GUI binary | Exact executable used by both successful smoke gates | `e5af96e64f6fd09b1ea1a1c481be2021c19fda5dfdf30709eafcf34e5ca026a1` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `c02fc7ce31daf2a01156f09b0078d64f5f64171378e465f5bbd3b8d91de49a5e` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `178d9499ca6869d376766a4a7df328ef3d582a340a5e84880122c4a83bc1da4c` |

The binary hash was rechecked after the dedicated gate and both GUI smoke runs and remained unchanged. Existing grammar, mesh, generated-catalog, backend, and monolith compiler warnings remain non-fatal and were not promoted as evidence of this slice.

### Updated concentration boundary

`src/imgui_main.cpp` is now `8,659` lines, down `86` lines from the prior recorded `8,745`. `DiagnosticsPanel.cpp` is `51` lines, `GrammarSymbolInspectorPanel.cpp` is `113` lines, `GrammarEditorDiagnosticWorkflowService.cpp` is `15` lines, `GrammarEditorSourceNavigationWorkflowService.cpp` is `68` lines, and `GrammarSourceLineIndex.cpp` is `55` lines. The removed line count represents diagnostics assembly, source splitting, and source-selection orchestration that no longer belongs to frame rendering.

The next ordered slice is the remaining preview interaction and render orchestration boundary, followed by the typed logging context. Grammar parsing/evaluation decomposition, scene-context decomposition, renderer decomposition, packaging, repository hygiene, and complete native/grammar/visual/GUI validation remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Scene Preview Interaction Workflow Ownership Slice

This append-only record certifies only the scene-preview interaction slice. The complete repository-refactor goal remains active.

### Governed OURD review

The preview interaction boundary was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `d299491d3754cc20d3617f418bae5f484e1b4a979d0b88955768180649c7d185`; the exact snapshot-output artifact SHA-256 is `64dfdc5dc77ebe33e178b56574c68f167ebfdffbd0064c08a3324b031bdc4324`.
- The bounded review context SHA-256 is `cf13c19809a561e59c44a634c248e29ccd47335e8e3f75d033fa7d32ff93bc67`.
- The isolated read-only workspace snapshot value was `6c62c6d254d42d96919140eee0342c36d5f0c74fb0be9d33c1a6989432a4f6ef`; the exact isolated snapshot-output artifact SHA-256 is `2c8ac38e8767701e16b3c9347036a93ea8169bce974ce9539ff2c83fa6fdc3db`.
- The exact-hash read-only authority manifest SHA-256 is `86c40a4df66c9bcbc2a3c50f3e0973f4b166fa3fa33fe8549a51183a841d78f8`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD review log SHA-256 is `1fb63e531164f78af18b59b5dfbddf3c4f0d09ee566c7619eb1522cd8faa4521`.
- The human review and correction record SHA-256 is `2a1f0d17fe1830ea1875420b5497ae6ccf541d9ee677574a629b83b2d6109775`.

The human review accepted explicit preview viewport, pointer, interaction, picking-result, and primitive-index models; a collision-geometry picking service; and one interaction workflow owning camera, light-over-primitive precedence, pointer selection, source navigation, view-cube orientation, camera fitting, and selection sanitation. It rejected a generic preview manager, event bus, callback bags, a broad runtime-context association, renderer or timeline expansion, raw magic picker integers, and retained deterministic controller ownership in presentation composition.

### UML-readable object model

- `ScenePreviewViewport` is the validated preview-dimension value model.
- `ScenePreviewPointerPosition`, `ScenePreviewPointerInput`, and `ScenePreviewInteractionInput` model presentation-observed interaction facts and requested orientation without owning editor behavior.
- `ScenePrimitivePickResult` is the purpose-specific result model for either no hit or one primitive-instance identity.
- `ScenePrimitiveInstanceIndex` is the relationship object between a `SceneGenerationContext` and its active, non-removed primitive instances.
- `ScenePrimitivePickingService` is the collision-geometry query service. It owns ray construction and bounds/triangle intersection without ImGui, GLFW, OpenGL, editor-state mutation, or source navigation.
- `ScenePreviewInteractionWorkflow` is the abstract application contract for selection sanitation, scene fitting, and one preview interaction transition.
- `EditorScenePreviewInteractionWorkflowService` implements that contract by composing `PreviewCameraMotionController`, `SceneSelectionController`, `ScenePrimitivePickingService`, and `LightGizmoPickingService`, while associating the existing `LightingSceneWorkflow` and `GrammarSourceNavigationWorkflow` authorities.
- `Progen3dEditorApplication` owns the concrete workflow. `EditorApplicationRuntimeContext` exposes only the abstract workflow association.

### Deterministic interaction semantics

- Light-gizmo picking has precedence over primitive picking. A selected light is delegated to `LightingSceneWorkflow`; primitive hover and pointer arming are cleared.
- Primitive hover, pointer arming, click-sized completion, selection, and cancellation are delegated through `SceneSelectionController`.
- Same-line non-empty source associations become typed `EditorRange` selection requests. Multi-line or collapsed source associations move the cursor to the source start. Missing, invalid, removed, or unassociated primitive identities perform no source-navigation mutation.
- View-cube orientation, keyboard navigation, orbit, pan, zoom, camera fit, and fit-pending transitions mutate `ScenePreviewSession::camera` only through the workflow.
- Invalid viewports, non-finite pointer or motion values, invalid matrices, invalid preview scale, missing generation context, invalid collision geometry, removed primitive instances, and outside releases fail closed.

### Presentation and integration boundary

`ScenePreviewPanel` now renders the preview, observes ImGui input facts, constructs `ScenePreviewInteractionInput`, and delegates sanitation, fit, and interaction transitions through `ScenePreviewInteractionWorkflow`. It no longer owns primitive ray intersection, primitive picking, camera fitting, selection sanitation, selected-primitive source navigation, view-cube camera snapping, direct camera motion, direct light selection, or direct primitive-selection transitions.

`EditorPresentationComposition` no longer owns or exposes `PreviewCameraMotionController` or `SceneSelectionController`. The concrete application service owns those deterministic collaborators. The runtime-context contract remains abstract, and all runtime harnesses use an inert implementation rather than depending on the concrete workflow.

### Behavioral and source acceptance

`tests/scene_preview_interaction_workflow_harness.cpp` and `tests/run_scene_preview_interaction_workflow_checks.sh` prove:

- valid viewport dimensions and fail-closed one-pixel viewports;
- real cube collision-geometry picking, invalid collision-geometry rejection, removed-instance rejection, and no magic picker sentinel at the service boundary;
- primitive hover, pointer arming, click-sized selection, same-line range navigation, and multi-line cursor navigation;
- light-over-primitive precedence and delegation to the lighting workflow;
- view-cube orientation, keyboard camera navigation, selection sanitation, scene fitting, and outside-release cancellation;
- abstract runtime-context ownership, concrete application composition, no deterministic preview-controller ownership in presentation composition, no canonical interaction mutation in `ScenePreviewPanel`, no legacy preview interaction procedures in `src/imgui_main.cpp`, and complete Makefile and architecture-suite registration.

The inherited preview-session, diagnostics-navigation, runtime-context, application-composition, scene-generation, transform, collision, curved-geometry, authentication, texture, lighting, service-contract, repository, export, GUI, and temporal GUI gates also passed against this slice.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Preview-interaction source path list | 38 source, integration, documentation, harness, and gate paths | `28d93ba53f3173e3e37807c0588c24e12734efbee41b9aba3ed787030bebaa98` |
| Preview-interaction source manifest | Exact SHA-256 manifest for the 38 paths | `7c3c5ca6324345c7d99caa59f9ac793d84622322f58f0f6b6f2bf9bda2346720` |
| Dedicated preview interaction workflow log | Behavior harness and ownership gates passed | `2f5718d2885b055a74e729600d244ebc08ef0fc0e0d290ca5c1f29a35f745901` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `fc1b6b7d227a933276cbe35610447c42f2b14786a56b1901217e580ef26e887c` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked | `0bb399c8e6c8de37fd4a67f0dd599672d77228e00a064cec47738e666e29b8cf` |
| Final GUI binary | Exact executable used by both successful smoke gates | `21cb171336742f3a6916a0d414fcb756ec4b704da9841e05c6472cd91395b612` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `bdb67b56608febcc7d5c53c11ef6702fcc7f2cb1b1e933584c916b02102fe104` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `acbd7b90038facb031b0c991057692ec0d1880f51183eb3db338fde62e2f476b` |

The final binary hash was rechecked after the dedicated workflow gate, full architecture suite, and both GUI smoke runs and remained unchanged. Existing grammar, mesh, generated-catalog, backend, and monolith compiler warnings remain non-fatal and were not promoted as evidence of this slice.

### Updated concentration boundary

`src/imgui_main.cpp` is now `8,323` lines, down `336` lines from the prior recorded `8,659`. `EditorScenePreviewInteractionWorkflowService.cpp` is `246` lines, `ScenePrimitivePickingService.cpp` is `245` lines, `ScenePrimitiveInstanceIndex.cpp` is `28` lines, and `ScenePreviewViewport.cpp` is `30` lines. The removed monolith lines represent preview picking, camera interaction, selection, fitting, view-cube orientation, and source-navigation behavior that no longer belongs to presentation rendering.

The next ordered slice is preview renderer and overlay orchestration, followed by the typed logging context. Grammar parsing/evaluation decomposition, scene-context decomposition, renderer decomposition, packaging, repository hygiene, and complete native/grammar/visual/GUI validation remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Scene Preview Rendering Workflow Ownership Slice

### Governed OURD review

The preview renderer and overlay boundary was reviewed through the governed local OURD agent before implementation.

- The pre-review repository source snapshot value was `64de9d29ae60f3165abd5e94f4a16da53e772539bd59221ea698e353c312ff8f`; the exact snapshot-output artifact SHA-256 is `7f777a5a2cf756e3b08c708a72ba5557cb004b4a092fee0c1dbe96a3af7ac765`.
- The bounded 197-line review context SHA-256 is `b1b95120e579c183511af469757bebe95a0217a80c3a2fb13bbfd564fcf2085f`.
- The isolated read-only workspace snapshot value was `df7193453025cc42f7f4a0ce8069953beb0f78a85521a089971297553171f1a6`; the exact isolated snapshot-output artifact SHA-256 is `6fbf9639a94449f9d98a091d6f8df955be3f6fe019b325983d120d2716265d6f`.
- The exact-hash read-only authority manifest SHA-256 is `0c6d87a5605887a7a8ad8f3fe6a16b828f9b6fb1dc1aa649791b4df643dd4810`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no successful command capability, and read-only evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`.
- The first advisory attempt failed closed because it requested a nonexistent semantic command. The second attempt produced an inadequate generic Thai-language review and was not accepted as design authority. Neither attempt mutated source or promoted a claim.
- The final bounded advisory review log SHA-256 is `35a8c288a8c104921417132b3cabd334084fd154d9b02ff7477f80762518a508`.
- The human review and correction record SHA-256 is `ba90e2c63f5b119fc518c75e8e4b901362348a77665c0ff262f986ce4afc9976`.

The human review accepted extraction of camera-frame construction, overlay geometry, overlay assembly, renderer-resource coordination, and preview-rendering orchestration behind an abstract workflow. It preserved `ScenePreviewSession`, renderer resources, `ScenePreviewPanel`, `SpatialOverlayEvidenceService`, the lighting workflow, and OpenGL renderer internals as existing authorities with narrower responsibilities. It rejected presentation-owned renderer orchestration, duplicate interaction cameras, file-global preview configuration, concrete runtime-context leakage, and a speculative rewrite of private renderer mechanics in the same slice.

### Object model

The scene-preview rendering boundary now reads as a purpose-driven object model:

- `ScenePreviewCameraFrame` is the immutable camera, projection, light-camera, and viewport model for one rendered preview frame.
- `ScenePreviewOverlayFrame` owns the assembled line vertices, outline batches, and overlay legend semantics for one frame.
- `ScenePreviewTextureHandle` identifies the renderer-owned preview texture without exposing renderer internals to presentation code.
- `ScenePreviewRenderedFrame` composes the exact camera frame, overlay frame, and texture handle returned by the workflow.
- `PreviewSurfacePresentationConfiguration` owns preview mapping mode and texture-inspection state inside `ScenePreviewSession`; file-global preview state no longer exists.
- `SceneConnectionOverlaySemantics` gives connection overlay meaning explicit names rather than duplicating anonymous presentation enums.
- `ScenePreviewOverlayColorPalette` provides the canonical relationship between semantic overlay categories and their displayed colors.

`ScenePreviewSession` remains the canonical per-workspace preview state owner and now composes `PreviewSurfacePresentationConfiguration`. Renderer texture ownership remains inside renderer resources, while the session owns only presentation configuration and interaction state.

### Workflow boundary

`ScenePreviewRenderingWorkflow` is the abstract runtime contract. `OpenGlScenePreviewRenderingWorkflowService` is its concrete application service and composes:

- `ScenePreviewCameraFrameFactory`, which derives one canonical camera frame for rendering and interaction reuse;
- `ScenePreviewOverlayAssemblyService`, which coordinates spatial evidence, connections, axial profiles, collisions, lights, and outlines;
- `ConnectionOverlayGeometryService`, which constructs explicit connection relationship geometry;
- `AxialProfileOverlayGeometryService`, which constructs axial-profile evidence geometry;
- `SpatialOverlayEvidenceService`, which remains the source of authored spatial evidence rather than being duplicated in presentation;
- the renderer resource service, which uploads assembled evidence and renders the preview texture.

`EditorApplicationRuntimeContext` now associates the abstract rendering workflow. `Progen3dEditorApplication` composes the concrete OpenGL workflow after renderer-resource construction, and all seven runtime harnesses use an inert workflow implementation. `EditorPresentationComposition` no longer owns or exposes `SpatialOverlayEvidenceService`.

### Presentation boundary

`ScenePreviewPanel` now requests a `ScenePreviewRenderedFrame`, displays its `ScenePreviewTextureHandle`, draws the ImGui view cube and legend, and forwards input facts to the interaction workflow. It no longer constructs the preview camera frame, connection overlays, axial overlays, spatial overlays, collision overlays, light gizmos, outline batches, texture uploads, or renderer calls.

The interaction workflow reuses the exact `ScenePreviewCameraFrame` returned by rendering, so picking and camera interaction cannot silently derive a second frame with different view, projection, viewport, or light-camera values. Legend chips use `ScenePreviewOverlayColorPalette`, and the preview popup reads and updates `ScenePreviewSession::surface_presentation` rather than hidden file globals.

The legacy public free renderer wrappers `upload_overlay_lines`, `upload_outline_batches`, the long `render_scene_to_preview`, and `preview_texture_id` were removed from `include/imgui_render.h` and `src/imgui_render.cpp`. Private renderer mechanics remain encapsulated inside `imgui_render_internal`; this slice changes orchestration ownership without claiming completion of private renderer decomposition.

### Behavioral and source acceptance

`tests/scene_preview_rendering_workflow_harness.cpp` and `tests/run_scene_preview_rendering_workflow_checks.sh` prove:

- canonical camera-frame construction, valid and fail-closed viewport behavior, and renderer/interaction reuse of the exact frame;
- semantic connection colors, authored spatial evidence, axial-profile evidence, collisions, light gizmos, outlines, and deterministic overlay assembly;
- preview texture-handle propagation without presentation access to renderer internals;
- default and updated preview mapping and texture-inspection state owned by `ScenePreviewSession`;
- abstract runtime-context ownership, concrete application composition, inert harness implementations, and no concrete OpenGL leakage into runtime contracts;
- removal of file-global preview configuration, legacy public renderer wrappers, duplicate connection enums, and procedural renderer/overlay orchestration from `src/imgui_main.cpp`;
- complete Makefile registration, editor-architecture-suite registration, acceptance documentation, exact-camera reuse, and renderer-workflow ownership of spatial evidence.

The inherited preview-session, preview-interaction, diagnostics-navigation, runtime-context, application-composition, scene-generation, transform, collision, curved-geometry, authentication, texture, lighting, service-contract, repository, export, GUI, and temporal GUI gates also passed against this slice.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Preview-rendering source path list | 46 source, integration, documentation, harness, and gate paths | `c33108609b4a4c8da0c9e8c0ad26860d702cc493d4e1c1542902f1bd5c2a16d7` |
| Preview-rendering source manifest | Exact SHA-256 manifest for the 46 paths | `fb22819e60e46ef1367349589d9c91d38881629a7cdb6ac1805861e30b3b1606` |
| Dedicated preview rendering workflow log | Behavior harness and ownership gates passed | `045c795357ae58bb7fb43d0bc7fb70bc27a53cdb166168d2493fced68abd17eb` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `659ea0a3c191ddf896ac469351f065870f861b0548fd66d6e0f9d3552e9b7a06` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked | `6f161c908084170f9d56907b37160a0866293a26efbec7f45dd6bedb85d28a5e` |
| Final GUI binary | 268,408,688-byte executable used by both successful smoke gates | `8fb270a2852946d82ef1b5ccf89dba184b656835f338a64599adf697b9f1d6fe` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `d5fb7b83cc8d2c2b8b510a0517ed2bdcd04af85d16989933d9d6d2737a0feae1` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `4ff8242360adbfd473c769213025e1401380505ed462cf66c58c63d2881c9e3b` |

The final binary hash was rechecked after both successful smoke runs and remained unchanged. The first temporal smoke invocation was launched in parallel and exited before application launch; its log contained only the up-to-date build message. Process inspection found no leftover ProGen3D or Xvfb processes, and the isolated rerun produced the temporal, FOV, render-frame, and GUI smoke acceptance markers. The complete architecture suite was also rerun from the beginning after updating one obsolete spatial-ownership source gate, and the final log ends with `All ProGen3D editor architecture checks passed.` Existing grammar, mesh, generated-catalog, backend, and monolith compiler warnings remain non-fatal and were not promoted as evidence of this slice.

### Updated concentration boundary

`src/imgui_main.cpp` is now `6,916` lines, down `1,407` lines from the prior recorded `8,323`. `ConnectionOverlayGeometryService.cpp` is `938` lines, `ScenePreviewOverlayAssemblyService.cpp` is `247` lines, `OpenGlScenePreviewRenderingWorkflowService.cpp` is `211` lines, `AxialProfileOverlayGeometryService.cpp` is `133` lines, `ScenePreviewOverlayColorPalette.cpp` is `127` lines, and `ScenePreviewCameraFrameFactory.cpp` is `107` lines. The dedicated renderer harness is `269` lines and its source gate is `126` lines.

The removed monolith lines represent camera-frame construction, connection and axial-profile geometry, spatial/collision/light/outline assembly, renderer upload coordination, preview rendering, texture mapping configuration, and debug inspection state that no longer belong to presentation rendering.

The next ordered slice is the typed logging context. Grammar parsing/evaluation decomposition, scene-context decomposition, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Typed Application Logging Context Ownership Slice

### Governed OURD review

The application logging ownership boundary was reviewed through the governed local OURD agent before implementation.

- The pre-review repository source snapshot value was `66770221fd056a017ad792be03ff21cafecb15ba5018c4a250f3531b799a6f69`; the exact snapshot-output artifact SHA-256 is `b9d70c97167e00da3d1c4249990d7f27ffb1ac78fd6e93e52732222e8e2e9fe6`.
- The bounded 211-line review context SHA-256 is `89d3ac5ad58b80362470ae0b2e363e5b1adfb5b1d357c720d354de3041484bc8`.
- The isolated read-only workspace snapshot value was `ff7bdc61922ceb9e31a457f71c0a8a596d25682edf6d11c032654f2de637be4f`; the exact isolated snapshot-output artifact SHA-256 is `4284602d7ae258d80563c066da6e2bea6c1af9e888f7b67e68af5e1bfb7fd06a`.
- The exact-hash read-only authority manifest SHA-256 is `5041f1fb779f58ea028ff24ede7de4b090aa65186a041671164fefb0a02a6625`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`.
- The exact OURD advisory review log SHA-256 is `77c8ac26efe340ff2c91dee48548e095e9c78ed60f63921dade6cad6530fafe8`.
- The human review and correction record SHA-256 is `d578157ddd3ba5df9015312f1f7f2ba10b72e2b9114ae14dc09bfd87250492ce`.

OURD correctly identified string-encoded severity, interface-state ownership, direct storage dependencies, console prefix inference, and hidden running-log state. The human review rejected its vague `LogEntry`, `LogPublisher`, `ApplicationLogPublisher`, and `DebugOutputBridge` names; rejected `EditorApplicationRuntimeContext` as the composition owner; and rejected a speculative migration of every grammar, scene-context, renderer, and smoke caller in the same slice. The corrected design requires purpose-specific `ApplicationLog*` classes, application-owned context composition, an abstract publisher dependency, and one explicit temporary legacy bridge.

### Typed object model

The logging boundary now reads as a purpose-driven class model:

- `ApplicationLogSeverity` explicitly distinguishes `Debug`, `Information`, `Warning`, and `Error`.
- `ApplicationLogSource` explicitly distinguishes application runtime, document workflow, cloud document workflow, authentication workflow, AI conversation workflow, preview material resources, and legacy runtime compatibility.
- `ApplicationLogEntry` is the typed value model for one normalized severity/source/message record and owns deterministic `displayText()` formatting. `[DEBUG] ` remains a display convention rather than stored semantic data.
- `ApplicationLog` remains the bounded thread-safe storage model, now retaining `ApplicationLogEntry` values through `record()` rather than untyped strings through `append()`.
- `ApplicationLogPublisher` is the abstract publication-service category and provides explicit debug, information, warning, and error operations.
- `ApplicationLogPublicationService` is the concrete publisher. It records one typed entry and mirrors its normalized message exactly once to the standard or error terminal stream.
- `ApplicationLoggingContext` composes one canonical `ApplicationLog` and one associated `ApplicationLogPublicationService`. It exposes the publisher, typed snapshots, entry count, and explicit clear semantics without exposing mutable storage.
- `LegacyApplicationLoggingBridge` is the named temporary relationship between legacy free output functions and the currently scoped publisher.
- `ScopedLegacyApplicationLoggingBinding` is the RAII relationship that binds one publisher for a lexical runtime scope and restores the previous publisher for deterministic nesting.

### Application and service ownership

`Progen3dEditorApplication` now composes `ApplicationLoggingContext logging_context_` separately from `EditorInterfaceState`. `EditorInterfaceState` retains only interface font resources and grammar-editor interaction state and is explicitly non-copyable and non-movable.

The following concrete services now associate with `ApplicationLogPublisher&` rather than `ApplicationLog&` storage:

- `LocalDocumentWorkflowService`;
- `CloudDocumentWorkflowService`;
- `EditorDocumentWorkflowService`;
- `AuthenticationWorkflowService`;
- `AiConversationWorkflowService`;
- `BackendPreviewMaterialFactory` inside `Progen3dPreviewMaterialResourceFactory`.

Every migrated publication now assigns explicit severity and source. Repeated empty-message checks, `[DEBUG]` string construction, direct storage mutation, and duplicate `std::clog` or `std::cerr` writes were removed from those services.

`EditorApplicationRuntimeContext` associates with the purpose-specific `ApplicationLoggingContext` and exposes it for console presentation and explicit application operations. It does not become the logging composition owner or a generic service locator.

### Presentation and compatibility boundaries

`ConsolePanel` now snapshots typed entries from `ApplicationLoggingContext`, renders `ApplicationLogEntry::displayText()`, styles debug entries through `ApplicationLogSeverity`, and clears through the context. It no longer inspects strings for a `[DEBUG]` prefix or receives mutable log storage.

`src/imgui_main.cpp` no longer owns `running_application_log`, `RunningApplicationLogBinding`, `append_console_message`, `clear_console_messages`, or the global `debugout` and `errorout` definitions. `Progen3dEditorApplication::run()` establishes `ScopedLegacyApplicationLoggingBinding` before runtime initialization, and generation requests clear the canonical context explicitly.

The global compatibility functions are declared by `LegacyApplicationLoggingBridge.h` and defined only in `LegacyApplicationLoggingBridge.cpp`. `Grammar.cpp`, `Context.cpp`, `imgui_render.cpp`, `EditorSmokeTestWorkflow.cpp`, and startup services now include that authoritative compatibility interface rather than declaring the functions independently. Bound calls publish typed `LegacyRuntime` entries; unbound calls fail safely to terminal output. Direct dependency injection into those legacy subsystems remains later grammar, scene-context, smoke-workflow, and private-renderer work.

### Behavioral and source acceptance

`tests/application_logging_context_harness.cpp` and `tests/run_application_logging_context_checks.sh` prove:

- typed severity, source, normalized message, and deterministic debug display text;
- empty-message rejection and one-trailing-newline normalization;
- bounded retention, deterministic batch trimming, snapshot, count, and clear semantics;
- abstract publisher ownership and separate non-copyable application logging context composition;
- information/debug routing to the standard stream and warning/error routing to the error stream without duplicate output;
- nested legacy scoped binding, previous-publisher restoration, typed `LegacyRuntime` records, and unbound terminal fallback;
- concurrent publication from four worker threads while another thread snapshots the canonical log, retaining all 300 bounded entries without error-stream leakage;
- migration of all six concrete service owners to `ApplicationLogPublisher` and removal of direct log-storage mutation or string-encoded severity;
- separate application ownership, purpose-specific runtime association, typed console rendering, removal of hidden logging state from `src/imgui_main.cpp`, and single-source compatibility definitions;
- complete Makefile, architecture-suite, and acceptance-documentation registration.

The inherited application-composition, runtime-context, document, cloud, authentication, AI, texture, diagnostics, preview-session, preview-interaction, preview-rendering, scene-generation, transform, collision, curved-geometry, lighting, service-contract, repository, export, GUI, and temporal GUI gates also passed against this slice.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Typed-logging source path list | 60 source, integration, documentation, harness, and gate paths | `b865d877023c5ca0446d7ff02170b1991f77fafb562c4cef2af3e06734e33c2f` |
| Typed-logging source manifest | Exact SHA-256 manifest for the 60 paths | `32a7f806bd9794cbec0cb4b53cca5eabd529a6cf4c872ba20e12e1097898e871` |
| Dedicated application logging context log | Behavior harness and ownership source gates passed | `8ac5856344a4d8e27aabfbc2acfc3af16f11d8153c105299122c13633123044a` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `6f86fb9539e9c73959df6ac90ac85434b1f1c226c547be3f1942f6db02696f91` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked | `41172024fff5e3572ceca1ee4c2e00cd4dd011e9d1d3ce850aec75688b9eed7d` |
| Final GUI binary | 268,668,536-byte executable used by both successful smoke gates | `68f5c74f985537aca6e15991f715a5ccaa661ef70918e0b9dcf91e0210292148` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `93760b74bc61b42aa7ff8e5ab46fea4d2f502f69049b0eee9bba51aac62eebb1` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `c6c3b5892b3311e7def26a07fb234237555b9762ce4f4ac590f4f69574c9446a` |

The first full architecture attempt reached the dedicated logging harness and then failed one exact README phrase check after the behavior test passed. The source gate was corrected to require the documented command name, the dedicated gate passed, and the complete architecture suite was rerun from the beginning. After concurrent publication/snapshot coverage was added, the complete architecture suite was rerun again from the beginning and ended with `All ProGen3D editor architecture checks passed.`

The forced build rebuilt every application translation unit before the harness-only concurrency extension. No application source changed afterward, the final binary hash remained unchanged, and both GUI smoke artifacts retain the exact successful binary behavior. Standard and temporal smoke gates were run sequentially and left no ProGen3D or Xvfb process behind. Existing grammar, mesh, generated-catalog, backend, and monolith compiler warnings remain non-fatal and were not promoted as evidence of this slice. A strict whole-file whitespace scan identified pre-existing whitespace in `src/Grammar.cpp`; diff-scoped checks and all newly created logging files passed integrity checks without modifying unrelated legacy formatting.

### Updated concentration boundary

`src/imgui_main.cpp` is now `6,868` lines, down `48` lines from the prior recorded `6,916`. `ApplicationLogEntry.cpp` is `43` lines, `ApplicationLog.cpp` is `43` lines, `ApplicationLogPublisher.cpp` is `27` lines, `ApplicationLogPublicationService.cpp` is `36` lines, `ApplicationLoggingContext.cpp` is `41` lines, and `LegacyApplicationLoggingBridge.cpp` is `81` lines. The dedicated logging harness is `182` lines and its source ownership gate is `95` lines.

The removed monolith lines represent hidden log binding, console append and clear procedures, and free logging definitions that no longer belong to presentation or application-main orchestration. The new compatibility bridge remains explicit temporary debt until grammar, scene-context, smoke-workflow, and private-renderer dependencies receive direct purpose-specific publication associations.

The next ordered slice is grammar parsing and evaluation decomposition. Scene-context decomposition, direct legacy logging dependency removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Grammar Parsing and Expression Evaluation Decomposition Slice

### Governed OURD review

The grammar parsing and checked-expression ownership boundary was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `47506665ed40c0e34c779198a81f906a986eb009f52917fc2865eccc6c169420`; the exact snapshot-output artifact SHA-256 is `409c3a4666b29888efd4a5ce8d476239b294eb0a979b2718931b312678069a01`.
- The bounded 214-line review context SHA-256 is `9d262c62ab81b5eba6365afb9ecd3f5ac4f831c7a9487a2918a05c9146c54cd9`.
- The isolated read-only workspace snapshot value was `aaee26de6b07c7cb213ab9009f476ba4d48c72bac47e6820b7d7453fccf45b9d`; the exact isolated snapshot-output artifact SHA-256 is `37e8b1a7cc460287f96d4916211b5ea66a4b907f0cbea8b886a2d16f7771e319`.
- The exact-hash read-only authority manifest SHA-256 is `bf74cc65172c66a4b4ec6ab69b8d302c27ccc52e2a8340e6b8701e91f19e508b`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`.
- The exact OURD advisory review log SHA-256 is `ff193a831b5affa26fb72f40bc1599f3f59eaaaed4912a50a7f4dc3ab5526873`.
- The human review and correction record SHA-256 is `6c6e460bf86c9545637a2890a6b015473c298d8efe7fef6a408236c809aea55f`.

OURD correctly identified source assembly, tokenization, diagnostic publication, expression evaluation, lexical lookup, and random sampling as separable responsibilities. The human review rejected vague `*Model` wrappers, a broad `GrammarDocumentService`, and a speculative variable-lookup relationship. The corrected design uses domain-native token, logical-rule, lexical-environment, evaluation-context, scoped-frame, parsing, inspection, evaluation, diagnostic, and sampling classes while preserving the public `GrammarDocument` compatibility boundary.

### UML-readable grammar object model

- `GrammarSourceToken` is the source-aware token value model. It owns token text plus exact line and column span evidence.
- `GrammarLogicalRule` is the assembled logical-rule value model. It retains normalized rule text and the physical source-line interval from which that rule was assembled.
- `GrammarRuntimeVariable` is the runtime variable value model. It explicitly distinguishes the current value, authored expression, and whether the value must be resampled.
- `GrammarExpressionIdentifierUse` is the semantic inspection result for one identifier and whether it is used as a function call.
- `GrammarExpressionEvaluationResult` is the fail-closed checked-evaluation result. It carries either a finite value or one explicit error message.
- `GrammarLexicalEnvironment` is the lexical context model. It composes ordered variable frames and owns visible lookup, visible assignment, frame creation, and frame removal semantics.
- `GrammarExpressionEvaluationContext` associates one lexical environment, evaluation time, immutable-time policy, and random sampling service for one checked expression evaluation.
- `ScopedGrammarLexicalFrame` is the RAII relationship object between one lexical environment and one temporary lexical frame.
- `GrammarSourceTokenizationService` converts physical source lines into exact-span source tokens without owning grammar-document mutation.
- `GrammarLogicalRuleAssemblyService` assembles continued physical lines into logical rules while preserving their source interval.
- `GrammarSourceDiagnosticService` creates source-aware diagnostics for tokens and logical rules.
- `GrammarDiagnosticPublicationService` publishes diagnostics through the existing grammar-document diagnostic authority.
- `GrammarRuleParsingService` owns top-level grammar rule recognition and delegates tokenization, logical-rule assembly, and diagnostic construction to their purpose-specific services.
- `GrammarExpressionSemanticInspectionService` owns identifier discovery and the supported-function vocabulary used by semantic validation.
- `GrammarExpressionEvaluationService` owns checked arithmetic, precedence, function evaluation, lexical lookup, time access, and fail-closed numeric behavior.
- `GrammarRandomValueSamplingService` owns deterministic random-range sampling through an explicitly associated random engine.

These are composition and association relationships rather than artificial inheritance. The classes are separable in a UML diagram: `GrammarRuleParsingService` associates the source services and a `GrammarDocument`; `GrammarExpressionEvaluationService` consumes a `GrammarExpressionEvaluationContext`; the context associates the lexical environment and sampling service; and `ScopedGrammarLexicalFrame` controls one temporary frame lifetime.

### Parsing and evaluation boundaries

`GrammarDocument::rebuildFromLines()` now delegates top-level rule recognition to `GrammarRuleParsingService`. Source tokenization, continuation-line assembly, source-range preservation, diagnostic construction, and diagnostic publication no longer live as anonymous procedures in `src/Grammar.cpp`.

Checked expression evaluation now builds a `GrammarExpressionEvaluationContext` and delegates to `GrammarExpressionEvaluationService`. Arithmetic precedence, right-associative powers, scalar functions, temporal functions, lexical and captured values, resampling, domain validation, finite-result validation, and immutable-time enforcement have one purpose-specific owner. Semantic validation uses `GrammarExpressionSemanticInspectionService` and the same supported-function vocabulary as evaluation rather than maintaining a duplicate list.

The internal runtime state now associates with `GrammarLexicalEnvironment`. The former `RuntimeVariableValue`, `ScopedLexicalFrame`, anonymous checked-expression parser, logical-rule structure, token structure, diagnostic wrappers, and direct random-range procedures were removed from `src/Grammar.cpp`. Public compatibility is retained through `using SourceTokenSpan = GrammarSourceToken`, existing `GrammarDocument` entry points, existing diagnostic publication, and unchanged grammar language semantics.

### Build and harness integration

The Makefile registers all nine extracted grammar implementation files. `tests/grammar_core_link_sources.sh` defines the canonical standalone grammar source set, while `tests/grammar_extension_link_sources.sh` extends it for grammar packages that require shape, lighting, spatial, or vehicle declarations. The P0 harness and every direct grammar compilation script now consume those source lists instead of maintaining incomplete copies.

`tests/grammar_decomposition_architecture_harness.cpp` and `tests/run_grammar_decomposition_architecture_checks.sh` prove:

- exact line and column token spans plus logical-rule source intervals;
- lexical shadowing, outer-frame lookup, visible mutation, nested frame restoration, and RAII frame removal;
- arithmetic precedence and right-associative exponentiation;
- lexical, captured, resampled, temporal, and scalar-function evaluation;
- fail-closed division, missing-variable, domain, period, immutable-time, unknown-function, and floating-underflow behavior;
- one shared supported-function vocabulary and semantic identifier inspection;
- deterministic random sampling from equal random-engine seeds;
- extracted service delegation, complete build and harness registration, absence of duplicate legacy owners, and a `src/Grammar.cpp` concentration ceiling below 5,600 lines.

The inherited P0, P1, P2, editor architecture, scene-generation, transform, collision, curved-geometry, axial-profile, spatial, logging, authentication, AI, texture, lighting, repository, export, GUI, temporal GUI, courtyard OMv2, SMB OMv2, and MVPv2.5 vehicle collision gates all passed against the extracted source set.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Grammar-decomposition source path list | 60 source, integration, documentation, harness, and gate paths | `4441cde736714c0480d1b937cc0810394939e9a2da5f3e571023636d5a115e56` |
| Grammar-decomposition source manifest | Exact SHA-256 manifest for the 60 paths | `f5ad20d36779de37517424de03a840409f9ce6dd081ad476a8d2953ef7d15140` |
| Dedicated grammar decomposition log | Behavior harness and source ownership gates passed | `3800e77aa13f0dc7de54bb1f7bc1d2bba9753ce4b73f7032d70be2e8f77588c5` |
| P0-P2 grammar acceptance log | P0, P1, and P2 syntax and runtime harnesses passed | `786fe3021579faff94062a1df03b0afe732c4c2f04a1d733dad99556350ae03e` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `2c45a14721da8c74a0f7fc4dae2a7c0c2139d20699fb56a26193cf275a04c591` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked | `0592708d2a7ee06421313d5837eb433b82d1bfc28e460bd39998b250d68f9d98` |
| Final GUI binary | 270,452,120-byte executable used by both successful smoke gates | `d53abef3abdd6624b204ae982b7411cd0346564ac777d95d3ef2303450473ce8` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Courtyard OMv2 standalone log | Deterministic generation and runtime grammar checks passed | `62372a061d3196e97c51b339c66d7fdc68604ecba67fe930d62125de2c0f3494` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, and topology checks passed | `94d634590ee81b8b60f87ac49195651d52d8a5caf5e61670bba4dd7fa41c8f5c` |
| MVPv2.5 vehicle collision grammar log | Reference-image, view, placement, and collision gates passed | `813b2246a8c8fae6fec1078b9c5b5f09f5cae81f0fe5e45a8ea5d69a18a665af` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `c2614d145e2a584e6296a0bbdcaf835b3dffd389f3d285ee22808e93ee1519e6` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `03525b10f556e81dd168dc7c295c0885a75486019220136ecf113e1b682d03be` |

The complete architecture suite ran from the beginning after the final extracted-source integration changes and ended with `All ProGen3D editor architecture checks passed.` The forced build then rebuilt every application translation unit and linked the final GUI binary. Standard and temporal smoke gates ran sequentially against that binary. Each smoke script left an Xvfb instance that was explicitly terminated before the next runtime gate, and no ProGen3D or Xvfb process remained after validation.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, generated-catalog variable-tracking notices, and existing `Mesh.cpp` warnings remain unrelated cleanup work. Newly introduced grammar sources compile without new warning categories, and the dead wrapper procedures exposed by the extraction were removed rather than suppressed.

### Updated concentration boundary

`src/Grammar.cpp` is now `5,375` lines, down `924` lines from the prior `6,299`. The extracted grammar implementation sources contain `1,312` lines in total: `GrammarLexicalEnvironment.cpp` is `87` lines, `GrammarDiagnosticPublicationService.cpp` is `59`, `GrammarExpressionEvaluationService.cpp` is `479`, `GrammarExpressionSemanticInspectionService.cpp` is `64`, `GrammarLogicalRuleAssemblyService.cpp` is `64`, `GrammarRandomValueSamplingService.cpp` is `32`, `GrammarRuleParsingService.cpp` is `182`, `GrammarSourceDiagnosticService.cpp` is `106`, and `GrammarSourceTokenizationService.cpp` is `239`. The dedicated behavior harness is `194` lines and its source ownership gate is `97` lines.

The removed monolith lines represent source tokenization, logical-rule assembly, source-aware diagnostic construction and publication, top-level rule parsing, lexical-frame ownership, checked expression parsing and evaluation, semantic identifier inspection, supported-function authority, and random-range sampling that no longer belong to one grammar implementation file.

The next ordered slice is grammar semantic-table and recursive-expansion decomposition. Scene-context decomposition, direct legacy logging dependency removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Grammar Semantic Catalog and Recursive Expansion Decomposition Slice

### Governed OURD review

The grammar semantic-analysis, lexical-runtime, expansion-budget, recursive-rule, and runtime-snapshot ownership boundary was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `140161a0dc7788962244441a4376057aaa2ca15adbcadd55d385be42e337f809`; the exact snapshot-output artifact SHA-256 is `ea016b6f395bdeadfd4b0a56018ae5fd10e1a1407b7a9e2eb2af15e7061fcac5`.
- The bounded 259-line review context SHA-256 is `6dbf1b10f379bfdffdbf941429e909ffbf1b1d8b0216f80cc0837b8ad1ad74d6`.
- The isolated read-only workspace snapshot value was `db5fa6c80ab07f1eba5a121a08ee920dda88ff9b7a96e93387c980ed48659e33`; the exact isolated snapshot-output artifact SHA-256 is `d504fdb306642b3a25282adf5f7b625e4d5f75e6eb79a9e5bc39dacc8f9f8898`.
- The exact-hash read-only authority manifest SHA-256 is `c2340a0a35445f2b12e7d0d815d7f9dabf62c726c80308a05ceed349027f60ba`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`.
- The exact OURD advisory review log SHA-256 is `9892ad9d9ca59e85a97336875e18818ffdb03008f9bb1878d5cec92d24013cdd`.
- The 89-line human review and correction record SHA-256 is `b316dd2fa6f7a090065a8e6389723094ce03a0d8e7eb90bdf7944bc9445dab8a`.

OURD correctly identified semantic catalogs, call relationships, expansion limits, mutable expansion counts, lexical frames, scoped invocation, diagnostic publication, and runtime snapshots as separable responsibilities. The human review rejected a call-edge object that also owned lexical bindings, duplicated runtime-snapshot storage, setter-heavy session objects, mutable limits mixed with counts, and generic runtime wrappers. The corrected design preserves one semantic catalog, one lexical environment, one expansion budget, one random engine, one snapshot authority, and one expanded action stream for each active expansion context.

### UML-readable semantic and expansion object model

- `GrammarRuleSemanticBindings` is the per-rule semantic value model. It owns the distinct parameter, declared-variable, and reroll-binding name sets and reports duplicate insertions explicitly.
- `GrammarBuiltInVariableCatalog` is the canonical built-in variable catalog. It owns the reserved `t` vocabulary instead of duplicating a static set in `src/Grammar.cpp`.
- `GrammarSemanticCatalog` aggregates rule-name ownership, per-rule bindings, call relationships, potential lexical visibility, known variables, and the built-in catalog.
- `GrammarRuleCallRelationship` is the explicit association between one caller rule and one callee rule. It does not own either rule and does not absorb lexical-binding responsibilities.
- `GrammarDeferredMaterialReference` is the encoded deferred-material value model. It centralizes encoding, recognition, and decoding instead of exposing prefix procedures in the grammar monolith.
- `GrammarExpansionLimits` is the immutable expansion-policy value model. Recursion, invocation, work, action, primitive, and repeat ceilings have one canonical owner shared by grammar expansion and scene execution.
- `GrammarExpansionBudget` is the mutable per-expansion accounting context. It owns counts, call-stack evidence, abort state, admission, work reservation, action reservation, and bounded call-path rendering.
- `GrammarExpansionContext` composes one grammar document association, semantic catalog, lexical environment, expansion budget, expression evaluator, random sampler, runtime-snapshot publisher, diagnostic publishers, and random engine for one expansion session.
- `ScopedGrammarExpansionContextBinding` is the thread-local RAII relationship that associates compatibility calls such as `MathF`, `update_token`, and public `Recurse` with the current explicit expansion context.
- `ScopedGrammarRuleInvocation` is the RAII relationship between one rule invocation and one expansion budget. Construction admits the rule; destruction removes its call-stack frame.
- `GrammarExpansionDiagnosticService` owns fail-closed expansion abort publication and appends the bounded rule-call path exactly once.
- `GrammarRuntimeVariableSnapshotPublicationService` owns synchronized per-document runtime snapshots. Snapshot storage no longer exists in `src/Grammar.cpp`.
- `GrammarSemanticAnalysisService` owns rule registration, semantic bindings, call-graph construction, potential lexical visibility, expression-symbol validation, deferred-material resolution, probability validation, random-range validation, call arity, duplicate binding diagnostics, and entry-rule validation.
- `GrammarRuleExpansionService` owns conditional evaluation, random declarations, rule lookup, parameter binding, reroll sampling, scoped lexical frames, sectioned repetition, alternate selection, action capture, recursive expansion, and expansion-budget enforcement.

`GrammarDocument::rebuildFromLines` now orders source validation, rule parsing, semantic analysis, explicit expansion-context construction, immutable time binding, snapshot publication, and recursive expansion without constructing hidden runtime or budget structs. `GrammarDocument::Recurse` remains source-compatible but delegates to the active `GrammarRuleExpansionService`; when called outside a rebuild it creates a bounded fallback context rather than recreating the deleted legacy runtime.

`SceneGenerationContext::addPrimitive`, `GrammarActionToken::performAction`, rule-header repeat parsing, recursive expansion, and geometry preflight now obtain their ceilings from `GrammarExpansionLimits`. The last duplicated `kMaxGeneratedPrimitiveCount` owner was removed from `src/Context.cpp`.

### Acceptance and ownership gates

The grammar decomposition harness now covers:

- built-in variable ownership and rule registration;
- purpose-specific parameter, declared-variable, and reroll bindings;
- deduplicated caller-to-callee relationships;
- potential lexical visibility and known-variable publication;
- deferred-material encoding and decoding;
- immutable expansion limits and mutable recursion, work, action, primitive, path, and abort accounting;
- the inherited tokenization, logical-rule assembly, lexical-frame, expression-evaluation, semantic-inspection, temporal-function, and deterministic random-sampling behavior.

The source gate requires the semantic catalog, expansion context, scoped relationships, semantic-analysis service, expansion service, expansion diagnostics, snapshot publication, Makefile registration, and shared grammar harness linkage. It rejects the deleted `RuleSemanticSymbols`, `GrammarSemanticTables`, `GrammarRuntimeState`, `ExpansionBudgetState`, `ScopedExpansionBudgetSession`, `ScopedGrammarRuntimeSession`, `ScopedRuleExpansion`, semantic-table builder, token-expansion procedure, and sectioned-expansion procedure owners. The bounded `src/Grammar.cpp` ceiling is now 4,200 lines.

P0, P1, and P2 source guards were migrated from monolith-string checks to the canonical semantic, expansion, snapshot, context-binding, budget, and built-in-variable owners. During that migration, the P1 gate exposed missing duplicate parameter and duplicate reroll diagnostics in the extracted semantic service; `GrammarRuleSemanticBindings` now returns insertion status and `GrammarSemanticAnalysisService` restores those blocking diagnostics. The compiled P1 harness then passed its duplicate-binding cases.

The inherited editor architecture, scene-generation, transform, collision, curved-geometry, axial-profile, spatial, logging, authentication, AI, texture, lighting, repository, export, GUI, temporal GUI, courtyard OMv2, SMB OMv2, and MVPv2.5 vehicle collision gates all passed against the extracted source set.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Recursive-expansion source path list | 87 source, integration, documentation, harness, and gate paths | `382d83070d70cb672d927040d1a39589a9bd741c772dfade0f139846436498eb` |
| Recursive-expansion source manifest | Exact SHA-256 manifest for the 87 paths | `3b8354d7cf5325e2f4d31296b109155208e6c8a9673327ef0f8b00a3bb80bb4e` |
| Dedicated grammar decomposition log | Semantic catalog, expansion budget, behavior, and source ownership gates passed | `27ee78c09505b2c12512d70e6c71515476acc43f6b1e8f3f49af8bc12e956e5b` |
| P0-P2 grammar acceptance log | P0, P1, and P2 syntax, semantic, temporal, and runtime harnesses passed | `aadfdbe211f457712292f7c5b45805250bcdcacac2fa4f140a5eefb7706c636f` |
| Full editor architecture log | All ProGen3D editor architecture checks passed | `1061145f498803a49bb11ed833d30da5d340b8a93c028b55bcd13a41b56aa59d` |
| Forced full native build log | Every application source rebuilt; `progen3d-editor-gui` linked in 369.02 seconds | `722a8a9b7eef2fe6d24a723a1d6df390bcdbe31e4206540de7f119b6c0c4370d` |
| Final GUI binary | 272,834,608-byte executable used by both successful smoke gates | `754b1ec31214ed5928ff0d8a8b3c0758a810698697b39a1b8cf621da1248fc8b` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Courtyard OMv2 standalone log | Deterministic generation and runtime grammar checks passed | `9ba7ab50f79f2f1ebd6e037540b47cc68b541e3cccadd89fb08fc7c88b809558` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, and topology checks passed | `8205d9b3a007fcc8afd983efca47f729fd8c9a31ed587e49a2c8196ea0727540` |
| MVPv2.5 vehicle collision grammar log | Reference-image, view, placement, and collision gates passed | `6524ac3a767787439ee18134b513d82ac684b7b4f736cda86d6c7c0daea54bf3` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `ce83e7d39ad3dca37f12a5e40f5b473370b6762248fba64dc56f836eb99cf391` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `a87a4afab920169c559c2f798379f00c1fd19aab40d1f4f1d5edf76f6f53f855` |

The complete architecture suite ran from the beginning after the final semantic and expansion integration and ended with `All ProGen3D editor architecture checks passed.` The forced build rebuilt every application translation unit and linked the final GUI binary. Standard and temporal smoke gates then ran sequentially against that unchanged binary. An initial cleanup probe matched its own later binary argument and exited before changing application state; exact executable and `Xvfb` process-name checks then removed the smoke-owned Xvfb processes and confirmed that no ProGen3D or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, generated-catalog notices, and existing `Mesh.cpp` warnings remain unrelated cleanup work. The new semantic and expansion sources introduce no new warning category. The P0, P1, and P2 source gates now assert the extracted owners rather than preserving obsolete monolith spellings.

### Updated concentration boundary

`src/Grammar.cpp` is now `4,094` lines, down `1,281` lines from the prior `5,375`. The eleven new semantic and expansion implementation sources contain `1,683` lines in total: `GrammarBuiltInVariableCatalog.cpp` is `16` lines, `GrammarSemanticCatalog.cpp` is `102`, `GrammarDeferredMaterialReference.cpp` is `23`, `GrammarExpansionBudget.cpp` is `99`, `GrammarExpansionContext.cpp` is `39`, `ScopedGrammarExpansionContextBinding.cpp` is `35`, `ScopedGrammarRuleInvocation.cpp` is `30`, `GrammarExpansionDiagnosticService.cpp` is `31`, `GrammarRuntimeVariableSnapshotPublicationService.cpp` is `54`, `GrammarSemanticAnalysisService.cpp` is `646`, and `GrammarRuleExpansionService.cpp` is `608`. The dedicated behavior harness is now `261` lines and its ownership gate is `130` lines.

The removed monolith lines represent rule semantic binding, rule-call graph construction, potential lexical visibility, built-in symbol authority, deferred material resolution, expansion limits and mutable accounting, thread-local runtime ownership, scoped rule invocation, condition evaluation, random declaration execution, sectioned expansion, recursive parameter and reroll binding, runtime snapshot storage, and recursive action-stream construction that no longer belong to one grammar implementation file.

The next ordered slice is scene-generation context decomposition. Direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Scene Primitive Admission and Canonical Scene State Slice

### Governed OURD review

The material-identity, pending-physical-state, primitive-admission, and authored-simulation-boundary seam inside `SceneGenerationContext` was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `50615cd1ea647762e468995bf9ef1cf2767a465d5f9bf41bc62af9196ccc96d0`; the exact snapshot-output artifact SHA-256 is `5b5dad78e7ccace979e81d2f82ca1a1e5d5d4d051b1a3b9511c437c6725be9ab`.
- The bounded 254-line review context SHA-256 is `ca256e3618fe76eae706d0d0ee268f7c3f3b78d1f57d05bbbfc611ab841669da`.
- The isolated read-only workspace snapshot value was `f416ea5b257e93cb0f838ea5fe2eca2cda3536f1723e35a4788676b6beacbb25`; the exact isolated snapshot-output artifact SHA-256 is `fa9c95496921767991e2ab16581f6cd0c5e311fe77f42ef5d6bdb41cac88628d`.
- The exact-hash read-only authority manifest SHA-256 is `cf87a1cea4ab062a49d1d4e9abbaae28c44ed32448001eb87c83d7284fbc10dc`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`.
- The exact OURD advisory review log SHA-256 is `c7c731abf3fa6fed26962ab72378ea07e80f66e8f05a38d663e9f0999e5f799f`.
- The 179-line human review and correction record SHA-256 is `a0d50262daa7198142d076fe3ac703e5910881be45caac335b5785fe7f90ac5b`.

OURD correctly identified next-primitive physical state, material identity, authored bounds, an explicit admission request/result, and a primitive-admission service as separable responsibilities. The human review corrected four important ownership errors: the admission service does not own or consult `GrammarExpansionLimits`; resolver, catalog, and pending state are explicit per-call associations rather than service-owned copies; the stored min/max/valid triple represented authored simulation bounds rather than the public live `getSceneBounds()` query; and physical constants require one canonical policy owner. The corrected design also introduced exact source-span ownership and a scoped pending-state reset relationship.

### UML-readable scene admission object model

- `PrimitiveDefinition` remains the primitive-category model and now lives in its purpose-specific scene model source instead of `Context`.
- `SceneObjectState` remains the polymorphic simulation-state base model.
- `SceneRenderableState : SceneObjectState` remains the renderable-state specialization.
- `ScenePrimitiveInstance : SceneRenderableState` remains the primitive-instance specialization and retains transforms, resolved geometry, source evidence, material identity, physical state, and collision-cache state.
- `CollisionParticle` is now an independent collision-effect value model rather than an unrelated declaration in `include/Context.h`.
- `ScenePrimitivePhysicalLimits` is the immutable canonical owner of default mass `1.0`, minimum mass `0.001`, and maximum rotational speed `720.0` degrees per second. Admission state, density-derived mass, inertia, impulse resolution, and simulation clamping reuse it.
- `PendingPrimitivePhysicalState` is the cohesive next-admission state model. It owns velocity, clamped rotational velocity, clamped mass, optional density, and reset defaults.
- `ScopedPendingPrimitivePhysicalStateReset` is the RAII relationship between one `addPrimitive` invocation and the pending state. Its destructor restores defaults on every budget, null-scope, geometry, mass, or successful exit.
- `SceneMaterialCatalog` is the canonical ordered material-identity catalog. It owns exact alphanumeric/lowercase canonicalization, numbered user-texture preservation, default plaster selection, texture-family recognition, fallback suffixing, stable slot reuse, and clearing.
- `AuthoredSceneBounds` is the optional authored admission-boundary model consumed by simulation. It is deliberately distinct from `SceneGenerationContext::getSceneBounds()`, which continues to recompute live bounds from non-removed instances.
- `ScenePrimitiveSourceSpan` owns all four start/end line and column coordinates.
- `ScenePrimitiveAdmissionRequest` is the immutable association between primitive type, nullable compatibility definition, valid scope, material input, render attributes, immovability, source span, and optional shape specification.
- `ScenePrimitiveAdmissionResult` is the explicit success-or-diagnostic value. It owns zero or one complete intrinsic `ScenePrimitiveInstance`.
- `ScenePrimitiveAdmissionService` is stateless. It associates the request, existing `PrimitiveGeometryResolver`, mutable material catalog, and immutable pending physical state to resolve geometry, initialize authoritative transform metadata, resolve transformed density mass, resolve material identity, and return one complete intrinsic instance.

`SceneGenerationContext::addPrimitive` now performs compatibility orchestration only: scoped pending-state reset, emitted-primitive budget enforcement, null-scope rejection, compatibility primitive-definition association, service invocation, diagnostic publication, successful append, spatial-object binding, previous-render evidence initialization, and authored-bound inclusion. The public method signatures, aliases, raw primitive-definition compatibility fields, and public `primitive_instances` collection remain unchanged.

Material slots are still created only after geometry and mass validation, so rejected primitives cannot publish material names. Density-derived cube volume preserves the existing dual-transform and secondary-transform semantics; resolved triangle geometry preserves watertight volume evidence multiplied by the absolute primary-transform determinant. The exact open/non-watertight and finite-nonzero-volume diagnostics remain unchanged.

### Acceptance and ownership gates

`tests/run_scene_primitive_admission_architecture_checks.sh` now verifies:

- the real `SceneObjectState` to `SceneRenderableState` to `ScenePrimitiveInstance` inheritance chain;
- exact physical defaults, clamps, optional density, and scoped reset behavior;
- exact material canonicalization, stable repeated slots, ordered names, and clearing;
- invalid, first, combined, and cleared authored bounds;
- exact four-coordinate source spans and explicit admission request/result values;
- required scene model, relationship, and service ownership in `Context`;
- complete native Makefile and standalone `Context.cpp` harness registration;
- rejection of the deleted pending-state fields, material vector, authored-bound triple, material canonicalizer, mass resolver, cube-volume resolver, free primitive selector, and `SceneGenerationContext` material-slot owner;
- bounded ceilings of 3,500 lines for `src/Context.cpp` and 180 lines for `include/Context.h`.

All 28 standalone scripts that compile `src/Context.cpp` now source `tests/scene_context_link_sources.sh`, which is the canonical extracted scene-source list. The complete editor architecture suite runs the new scene-admission gate before the inherited workflow, rendering, transform, collision, curved, axial-profile, spatial, logging, authentication, texture, lighting, repository, and export gates.

The collision-positioning harness now additionally proves successful intrinsic admission, exact source spans, material-slot reuse, clamped rotational velocity, scoped state reset after success, no partial instance or material after unknown geometry, reset after null-scope rejection, exact density-derived cube mass, and preservation of the public live scene-bounds query.

P0 and P1 source gates initially exposed their former monolith ownership assumptions. The final gates require `ScopedPendingPrimitivePhysicalStateReset` instead of a manual `clearPendingPrimitiveState()` call and inspect authoritative transform metadata inside `ScenePrimitiveAdmissionService` instead of the compatibility orchestrator. P0, P1, and P2 then passed completely.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Scene-admission source path list | 63 source, integration, documentation, harness, and gate paths | `8395732582aeeabf7b771deafc4bf87cdd2dfa073567bc3122bd300b8ed60b2f` |
| Scene-admission source manifest | Exact SHA-256 manifest for the 63 paths before this append-only entry | `4a053ecd29bc00e796e20278b6dae3c5180b5a17dc9d6dc240b392ec9a1efdd2` |
| Dedicated scene-admission architecture log | State, catalog, bounds, request/result, linkage, ownership, and concentration gates passed | `55d7e76d8a168744313c0f652a29ed85f3eeabad1b6aa9c8e723a9baa95ec267` |
| Collision-positioning integration log | Admission invariants plus inherited collision positioning passed | `2e4df2a30668569541e0be7b67ea3a324bae3f51a4485e7498a20886fff6295e` |
| Curved geometry pipeline log | Density mass, curved geometry, and source gates passed | `e51aba179099d0247a2d3d573a62287dc51ee530f6e1bb6742438422ffbe40e5` |
| P0-P2 grammar acceptance log | P0, P1, and P2 source, syntax, semantic, temporal, and runtime harnesses passed | `b1249fbffea96fde4a7c39942a8a8400daa84b347a0fc2ea8703b90d36cd81e2` |
| Full editor architecture log | All ProGen3D editor architecture checks passed from the beginning | `11429e1ed9caadbaad59830fcd6fba6bc16912a3a3031a4612aaef12cb4aa513` |
| Full editor architecture timing | `elapsed=812.16 user=756.22 system=53.13 maxrss_kb=999336 exit=0` | `88cc921eea20afebd9b1a510d98095217d1bab2df55caf0ed2c1b41c3b16aae1` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `5ed3958255cc9da4227a3429b0647c6314c0a5ad6b8865f3e817503054e42a4f` |
| Forced full native build timing | `elapsed=583.14 user=561.58 system=19.00 maxrss_kb=2566184 exit=0` | `b4872b33256879cdc76c0c760f90fdf26f329101c85c77bb61d478b4fba1fd31` |
| Final GUI binary | 273,664,128-byte executable used by both successful smoke gates | `f30c27f4b949b74ebfd99bd1cc381ab8c80a87d170ebc9014c30a1abce1178ec` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Courtyard OMv2 standalone log | Deterministic grammar generation and runtime scene checks passed | `9f23d99a67d5332fbbb03b9064b469569f46f8f420cd74487e254dfe50a75698` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, collision-query, and topology checks passed | `1630fba491081dc12e1e2b1245204f0a00f849a961f44c864cfec0e1ad93787a` |
| MVPv2.5 vehicle collision grammar log | Reference-image, ProGen3D-view, placement, joint, and collision gates passed | `0c906453ffbe8fe869b0301061bfd64de1a17a0142b91de26d4cc59fb335e5b4` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `95302660558dc7edafbfa2bde70a339f7052457b6233ae1017e5b6d0a89eb17e` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `2e05638a8a47758864c70597adad27684c32d505e3d40ea40207529f0b3f24f1` |

The binary hash remained unchanged after both GUI smoke runs. Exact `/proc` executable-path inspection and exact `Xvfb` process-name inspection confirmed that no ProGen3D application or Xvfb process remained. Scoped `git diff --check`, trailing-whitespace inspection, Python bytecode compilation, and shell syntax validation all passed after the final source-gate migrations.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, existing `Mesh.cpp` warnings, generated-catalog notices, backend unused-function warnings, and remaining large-file warnings are unrelated cleanup work. The extracted scene admission sources introduce no new warning category.

### Updated concentration boundary

`src/Context.cpp` is now `3,421` lines, down `257` lines from the prior `3,678`. `include/Context.h` is now `156` lines, down `83` lines from the prior `239`.

The nine new scene admission implementation sources contain `790` lines in total: `PrimitiveDefinition.cpp` is `64` lines, `ScenePrimitiveInstance.cpp` is `29`, `PendingPrimitivePhysicalState.cpp` is `77`, `SceneMaterialCatalog.cpp` is `72`, `AuthoredSceneBounds.cpp` is `33`, `ScenePrimitiveAdmissionRequest.cpp` is `73`, `ScenePrimitiveAdmissionResult.cpp` is `40`, `ScopedPendingPrimitivePhysicalStateReset.cpp` is `14`, and `ScenePrimitiveAdmissionService.cpp` is `388`.

The removed concentration represents scene-state model declarations and reset behavior, primitive-definition rendering, pending physical state, physical limits, scoped reset lifetime, material canonicalization and slot ownership, authored admission bounds, exact source spans, transform-derived instance construction, density-derived mass resolution, material assignment, and intrinsic admission result construction that no longer belong to `SceneGenerationContext`.

The next ordered scene-context slice is collision simulation decomposition: broad-phase spatial hashing, collision-pair relationships, collision response, gravity/inertia policy, sleep/wake transitions, and simulation-step orchestration must be separated without duplicating the existing `ConvexCollisionDetector` or changing authored simulation-boundary semantics. Spatial construction state, mesh/export assembly, scope-stack ownership, direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim.

## 2026-08-26 Collision Simulation and Primitive Geometry Slice

### Governed OURD review

The collision-detection, collision-response, particle-effect, primitive-transform, and simulation-step seam inside `SceneGenerationContext` was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `997e771ffd607740e374a2665b46495820b5b0d98a0cc68ce8f9813d887ea8c0`; the exact snapshot-output artifact SHA-256 is `98abe0b8d4202dde61ebb54330e96c6f8d38d2ca3ccd4101687ae53b94d4c2da`.
- The bounded 368-line review context SHA-256 is `e82ea8fca56373969564de20afaf93b75ecda89c048ac6885f3ac9369aadb597`.
- The isolated read-only workspace snapshot value was `277f3668e87d33c6900d9bca2c14c368a74f98b80eeb1c08587614bab5a519d9`; the exact isolated snapshot-output artifact SHA-256 is `8c1fa3b4ea30f11dd1bfa32710665ade0b5446a426800c11511d97243d1845c9`.
- The exact-hash read-only authority manifest SHA-256 is `46886dcca119537138ac9f786a8d6c2866ea872b301741e6fcec691cd24d8c2f`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its timing record SHA-256 is `a5ab7e6f526e2e780f045887662945273c66f0827f52ea8528243a8e24e82559`.
- The exact OURD advisory review log SHA-256 is `8bdd8f8e1c8d99d80e7ba2de3186de35debf987e6c7102a64b2146fdc21b6109`; its timing was `elapsed=41.44 user=0.07 system=0.01 maxrss_kb=33976 exit=0` and timing-record SHA-256 is `1c8a161f9f3f29ba4210cfad409261f3c7208303a99a75cc1a2980e1e10b1035`.
- The 213-line human review and correction record SHA-256 is `0273cfb10a78e731ca108c5b1fe938fa36d1034788cc9ea7aecc64bfd4b234e7`.

OURD correctly identified transform evaluation, broad-phase candidate generation, narrow-phase detection, collision response, particle emission, lifecycle policy, and step orchestration as separable responsibilities. Its proposed abstractions were too generic to preserve the actual compatibility and ownership boundaries. The human review replaced them with domain-native scene models, policies, relationships, and services; retained public `SceneGenerationContext::time` as the sole compatibility time authority; kept the existing `ConvexCollisionDetector` as the convex narrow-phase owner; and rejected context back-references, duplicate collision contact models, generic physics managers, and a speculative inheritance hierarchy.

### UML-readable collision simulation object model

- `CollisionAxisCollection` is the canonical collision-axis value collection. It owns finite non-zero normalization, near-parallel de-duplication, ordered access, and explicit clearing.
- `ScenePrimitiveTransformState` is the evaluated primitive-transform value model. It owns the primary render transform, secondary render transform, collision transform, render position, collision position, and secondary-transform-use evidence for one evaluation.
- `SceneSimulationState` is the cohesive mutable simulation-state model. It owns gravity acceleration and the collision-particle collection; it does not own the compatibility clock.
- `SceneCollisionPairRelationship` is the normalized relationship between two scene primitive indices. It guarantees stable lower-index/upper-index ordering and equality semantics independently of discovery order.
- `ScenePrimitiveCollisionParticipationPolicy` owns collision participation predicates, including removed-state exclusion, simulation-active or immovable participation, and static-static rejection.
- `ScenePrimitiveSimulationLifecyclePolicy` owns gravity eligibility, inverse-mass and inverse-inertia policy, rotational-speed clamping, wake/sleep transitions, global simulation-boundary stopping, and authored-scene-boundary cube removal.
- `ScenePrimitiveGeometryService` owns primitive transform evaluation, previous-render-state capture and initialization, world translation and rotation application, collision-cache invalidation, collision-geometry construction, and collision-geometry cache publication.
- `SceneCollisionBroadPhaseService` owns deterministic spatial-hash cell sizing, occupied-cell indexing, candidate-pair normalization, duplicate rejection, and static-static pruning.
- `SceneCollisionDetectionService` owns the existing scene primitive overlap algorithms and returns the established `CollisionContact` model. Convex-mesh detection continues to delegate to `ConvexCollisionDetector` rather than duplicating GJK/EPA ownership.
- `SceneCollisionParticleService` owns bounded deterministic collision-particle emission and lifetime integration through an explicitly associated effects random engine and `SceneSimulationState`.
- `SceneCollisionResponseService` owns impulse resolution, friction, positional correction, wake decisions, and non-resting-contact particle emission. It consumes `CollisionContact` and has no `SceneGenerationContext` back-reference.
- `SceneSimulationService` owns reset and step orchestration. It preserves the established update sequence, the exact three collision-solver iterations, authored boundary semantics, particle stepping, and compatibility-time advancement through an explicit `float *` association.

These classes form explicit composition and association relationships. `SceneGenerationContext` composes `SceneSimulationState`, `ScenePrimitiveGeometryService`, and `SceneSimulationService`; the orchestration service composes or associates the lifecycle, broad-phase, detection, response, and particle responsibilities; and collision pairs and transform states remain value models rather than hidden anonymous structures.

### Context compatibility boundary

`SceneGenerationContext::setGravity`, `getGravity`, `getCollisionParticles`, `emitCollisionParticles`, `resetSimulation`, and `stepSimulation` now delegate to the extracted state and services. Public `time`, public `primitive_instances`, existing method signatures, existing collision diagnostics, and the effects random stream remain compatible.

Rendering and collision queries now share `ScenePrimitiveGeometryService` for transform evaluation and collision geometry. Dual-axis `CubeX`, `CubeY`, and `CubeZ` transform behavior, resolved-geometry routing, previous-render evidence, collision-cache dirtiness, and world-space transform mutations retain their prior semantics. The former anonymous `PrimitiveTransformCache`, transform procedures, spatial-hash cell and pair structures, broad-phase construction, overlap algorithms, response procedure, lifecycle procedures, gravity/inertia procedures, particle procedures, and simulation-step procedural block were removed from `src/Context.cpp`.

The effects random stream remains separate from grammar randomness. `SceneGenerationContext` passes `effects_rng` explicitly to the simulation and particle services; `SceneCollisionParticleService` owns the distribution draws and has no dependency on `grammar_rng`.

### Acceptance and ownership gates

`tests/run_collision_simulation_architecture_checks.sh` now verifies:

- normalized unique collision-axis ownership and normalized collision-pair relationships;
- simulation-state gravity and particle ownership;
- collision participation and lifecycle policy behavior;
- transform evaluation, collision cache invalidation, previous-render initialization, and world translation behavior;
- deterministic spatial-hash candidate construction and static-static pruning;
- deterministic collision-particle emission and lifetime integration;
- syntax of the extracted narrow-phase, response, and orchestration services;
- required `SceneGenerationContext` composition and delegation;
- removal of every legacy collision, transform-cache, lifecycle, gravity, particle, and simulation-step owner from `Context`;
- rejection of direct `SceneGenerationContext *` or generic `Context *` service back-references;
- Makefile and standalone-harness registration for every extracted implementation source;
- bounded `src/Context.cpp` and `include/Context.h` concentration ceilings.

The scene-admission architecture gate now scans only shell harnesses that compile `src/Context.cpp` as an actual compiler argument, so source-inspection gates are not falsely required to link the scene context. P0 random-stream guards now inspect the extracted particle service while requiring the explicit `effects_rng` handoff. P1 exact transform extraction now targets `ScenePrimitiveGeometryService`, and the curved-geometry source gate preserves resolved-geometry routing while accepting the explicit geometry-service association.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Collision-simulation source path list | 36 source, integration, documentation, harness, and gate paths | `25a6c034ba86a1f87897b92013d4335231c28b943762b90d9df5a07db01f1638` |
| Collision-simulation source manifest | Exact SHA-256 manifest for the 36 paths before this append-only entry | `6253d400dc00c5ba5a37afa3a6ce8c82378c1639a1b1c15a0b8231b4b98a6394` |
| Focused syntax log | All extracted collision simulation sources and `Context.cpp` passed syntax compilation | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Scene-admission ownership rerun | Admission architecture and source ownership gates passed after the exact harness scan correction | `55d7e76d8a168744313c0f652a29ed85f3eeabad1b6aa9c8e723a9baa95ec267` |
| Dedicated collision architecture log | State, relationship, policy, geometry, broad-phase, particle, linkage, ownership, and concentration gates passed | `426c21633abdee93eff3db9298846542e0018d8531f5ff467464551b4eb768b4` |
| Collision positioning integration log | Existing positioning, response, authored-boundary, and simulation behavior passed | `44e784062765ff0892e6a886cd20ee9c90be23aa6654aaf46deeecefcedcb80a` |
| Curved geometry pipeline log | Density mass, curved geometry, resolved-geometry routing, and source gates passed | `88624791beb72628ff443ff3f8b97d4eef9651cff42fec9dc9c80cc88a1eb102` |
| P0-P2 grammar acceptance log | P0, P1, and P2 source, syntax, semantic, temporal, and runtime harnesses passed | `70f64e3c0ff48af1774325da8cdaaa816dc7848f52421bf40c670a19d9c70902` |
| Full editor architecture log | All ProGen3D editor architecture checks passed from the beginning | `582e665a8824b0a49b5e7ea51f3eb5225fee632d57007e2db0ec58b041a4f2e8` |
| Full editor architecture timing | `elapsed=829.29 user=770.85 system=55.00 maxrss_kb=1002820 exit=0` | `21af61e8f14288ea731bb84e5a972c614117b426fe998b223d103c8c39eff777` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `0669e23897aa5a7911c3c7ef9783f936b76dc5d9546bbdd20929dfbe2c298d81` |
| Forced full native build timing | `elapsed=358.44 user=582.25 system=18.96 maxrss_kb=2634176 exit=0` | `22cd3f38956c9202a7b6f6483cfb66379576eb4bb0aa64817564e54f45875d86` |
| Final GUI binary | 274,408,536-byte executable used by both successful smoke gates | `3980847d74b5648e0c56fa9ac704bec4e861a06440954501fc89388f43ec4a13` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Courtyard OMv2 standalone log | Deterministic generation and two governed runtime resolution records passed | `9ba7ab50f79f2f1ebd6e037540b47cc68b541e3cccadd89fb08fc7c88b809558` |
| SMB OMv1 standalone log | Source contract and runtime scene checks passed | `121c9762d09fe1cb679518f80069ce0fa7f9a3cd6a002548f3b94da72294f943` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, collision-query, residual, and topology checks passed | `8205d9b3a007fcc8afd983efca47f729fd8c9a31ed587e49a2c8196ea0727540` |
| MVPv2.5 vehicle collision grammar log | Reference-image, ProGen3D-view, placement, joint, and collision gates passed | `6524ac3a767787439ee18134b513d82ac684b7b4f736cda86d6c7c0daea54bf3` |
| Final source hygiene log | Shell syntax, Python compilation, trailing whitespace, scoped diff, and process cleanup passed | `98e5aad96fe9b4f94ab9d1582657bb8df49fec654e77af42f85308993cb4e96c` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `8ccc7f91911becad78989c6bddff8223fbecaab38de80e052da85753a88b27d4` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `f5afb0c02e1b7756b65651294fb57443f7c466e8c12ac7914beaaf06cff7d9a7` |

The complete architecture suite ran from the beginning after the final ownership-gate migrations and ended with `All ProGen3D editor architecture checks passed.` The forced build rebuilt every application translation unit and linked the final GUI binary. Standard and temporal headless smoke gates then ran against that unchanged binary. Exact `/proc` executable-path inspection and exact `Xvfb` process-name inspection confirmed that no ProGen3D application or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, existing `Mesh.cpp` warnings, generated-catalog notices, backend unused-function warnings, and the vendored stb warning remain unrelated cleanup work. The extracted collision simulation sources introduce no new warning category.

### Updated concentration boundary

`src/Context.cpp` is now `1,103` lines, down `2,318` lines from the prior `3,421`. `include/Context.h` is now `158` lines; the two-line increase records explicit composition of the simulation state and services while the implementation concentration moved out of the context.

The eleven new collision simulation implementation sources contain `3,047` lines in total: `CollisionAxisCollection.cpp` is `37` lines, `SceneSimulationState.cpp` is `41`, `SceneCollisionPairRelationship.cpp` is `28`, `ScenePrimitiveCollisionParticipationPolicy.cpp` is `52`, `ScenePrimitiveSimulationLifecyclePolicy.cpp` is `248`, `ScenePrimitiveGeometryService.cpp` is `698`, `SceneCollisionBroadPhaseService.cpp` is `189`, `SceneCollisionDetectionService.cpp` is `1,144`, `SceneCollisionParticleService.cpp` is `197`, `SceneCollisionResponseService.cpp` is `152`, and `SceneSimulationService.cpp` is `261`. Their twelve purpose-specific headers contain `325` lines in total.

The removed concentration represents transform-state evaluation, collision-geometry cache construction, spatial hashing, pair normalization, overlap detection, impulse response, friction, positional correction, gravity and inertia policy, wake/sleep transitions, authored and global simulation-boundary handling, particle emission and integration, and step orchestration that no longer belong to `SceneGenerationContext`.

This append-only record certifies only the collision simulation and primitive geometry slice. Spatial construction state, mesh/export assembly, scope-stack ownership, direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim. The next ordered scene-context slice is spatial construction state and object-binding orchestration.

## 2026-08-26 Spatial Construction State and Object Binding Slice

### Governed OURD review

The spatial object declaration, primitive binding, authored-transform, persistent diagnostic, model-finalization, and derived-building-model seam inside `SceneGenerationContext` was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `07fd0913f9c44fff2c8eaa32655a437dc8f8ab762683c3a372f275878e761f8b`; the exact snapshot-output artifact SHA-256 is `723322f1180221efd9997976417aa45a8815967c98fc81c3e4cca2154fb2ee98`.
- The bounded 400-line, 1,244-word, 16,738-byte review context SHA-256 is `785bde14e906c5b107115a9038e45278a31c53e820c1acbd58d9418407eafac1`.
- The isolated read-only workspace snapshot value was `7dfab83eaaf00ecbe981f153628a9f930af349da06ae46c36075108e3a2068ac`; the exact isolated snapshot-output artifact SHA-256 is `c73b2502e6eff34edadee4c6acfeaaa882272b878da881fac470d62041f8bc4d`.
- The exact-hash read-only authority manifest SHA-256 is `ad755df949b31eed53f8894c56bf8a0d2a52c4df9a4506f36265ad52ee1c9dd7`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.09 user=0.07 system=0.00 maxrss_kb=33588 exit=0` timing-record SHA-256 is `73f75a7bf6cd7a5b530ba5c96d8233d0701c8974a44537a341c83d76c076f522`.
- The first review attempt failed closed because the estimated 11,617-token request exceeded the authorized 9,000-token context budget. The preserved failure log SHA-256 is `4dd286835457acccc856dcf00a5935d6761da00d94a6eaeb108a6205a112effc`; its `elapsed=12.82 user=0.08 system=0.00 maxrss_kb=34416 exit=1` timing-record SHA-256 is `baf0436fb1eecee465d3f068f51737b7b604bd3252e1cc15cf8612583d552968`. The review context was narrowed rather than increasing the authority budget.
- The final OURD advisory review log SHA-256 is `1dbe0be36d799e132cc8ff614bca0c86421468c0e6e987735a65dd8e406979d1`; its timing was `elapsed=23.57 user=0.06 system=0.01 maxrss_kb=34020 exit=0` and timing-record SHA-256 is `863a41a084c122b8e0e50850d6eb9f6d877f7db3a1d174fe585d1cc88b263c6f`.
- The 222-line human review and correction record SHA-256 is `b32c5ca4247747907116dd3725a82454f5ea36c40d6c94e36cd4491b32222005`.

The final OURD advisory was not accepted as an implementation plan. It invented an `addSpatialObject()` API that does not exist, proposed an assertion against an invented `declarations_started` member, ignored the required declaration/binding/finalization separation, and recommended debug mutations despite the read-only review task. The human review retained the useful responsibility boundary, rejected the hallucinated details, and replaced them with domain-native scene construction models and services grounded in the inspected source.

### UML-readable spatial construction object model

- `SceneSpatialObjectScope` is the value model for one active spatial object declaration. It associates one `SpatialObjectId` with its authored world transform.
- `SceneSpatialConstructionState` is the cohesive mutable construction context composed by `SceneGenerationContext`. It owns the established `SpatialBuildingModelConstructionContext`, active object-scope stack, declared authored world transforms, declaration-started state, persistent blocking diagnostic, resolved spatial model, and optional derived small-modern-building model.
- `SceneSpatialObjectDeclarationService` owns begin-object, interface, connection, constraint, and end-object declaration workflows. It validates parent-scope agreement, derives authored local transforms from the declared parent world transform, records containment, maintains the active scope relationship, and preserves exact blocking diagnostics.
- `SceneSpatialPrimitiveBindingService` owns the association between one admitted scene primitive and the active spatial object. It computes `inverse(authored object world transform) * primitive primary transform`, publishes the established `SpatialObjectGeometryBinding`, and records the exact non-invertible-transform or binding diagnostic in the construction state.
- `SceneSpatialModelFinalizationService` owns construction finalization, active-scope rejection, persistent-diagnostic rejection, existing assembly-resolution invocation, optional `SmallModernBuildingModel` derivation, and atomic publication of both resolved models only after the complete workflow succeeds.
- The existing `SpatialBuildingModelConstructionContext` remains the canonical domain construction owner. The existing `SpatialAssemblyResolutionService::resolve(..., SceneGenerationContext *)` association remains the deliberate compatibility boundary until the later spatial-positioning refactor; declaration and primitive-binding services contain no context back-reference.

`SceneGenerationContext` now composes all four spatial owners by value. The deleted nested `SpatialSceneConstructionState`, heap-owned `spatial_scene_construction_state_`, unused `next_instance_index`, active-scope vector, authored-transform map, declaration flag, blocking diagnostic, and resolved-model fields no longer concentrate spatial construction ownership in the scene context.

Public spatial methods remain source-compatible and delegate to the purpose-specific services. Beginning a nested object uses the active scope transform as the authored parent world transform. Primitive admission binds only after the scene primitive has been admitted and therefore preserves the exact primitive instance index. Finalization returns success when no spatial declarations were authored, rejects unclosed scopes, finalizes against the exact scene primitive count, invokes the established positioning resolver, derives the optional building model, and publishes neither model when any required stage fails.

### Acceptance and ownership gates

The dedicated spatial construction harness covers:

- construction-state scope, authored-transform, declaration, diagnostic, and resolved-model ownership;
- declaration-before-scope rejection, parent-container mismatch diagnostics, nested object containment, authored child-local transform derivation, balanced scope closure, empty-scope closure rejection, and null-constraint rejection;
- admitted primitive index preservation, object-local transform derivation, construction finalization, and persistent rejection of non-invertible authored transforms.

The source gate requires all four by-value owners in `SceneGenerationContext`, all public delegation sites, Makefile and shared scene-link registration, and a finalization-only `SceneGenerationContext *` compatibility association. It rejects the deleted `SpatialSceneConstructionState`, `spatial_scene_construction_state_`, `active_object_scopes`, `declared_world_transforms`, `declarations_started`, `blocking_diagnostic`, and `next_instance_index` owners from `Context`. It also enforces a sub-1,000-line `src/Context.cpp` ceiling and a sub-180-line `include/Context.h` ceiling.

Inherited object-model, grammar-scene, spatial-positioning, spatial-query, scene-admission, collision-positioning, P0-P2, complete editor-architecture, native build, GUI, temporal GUI, courtyard OMv2, SMB OMv1, SMB OMv2, and MVPv2.5 reference-collision gates all passed against the extracted source. The first temporal GUI attempt was run concurrently with the standard smoke and exited during Xvfb startup; a clean isolated rerun against the unchanged binary passed all temporal, FOV, render-frame, and GUI markers.

### Validation evidence

The exact 18-path source list is `build/tests/spatial_construction_source_paths.txt`; its SHA-256 is `41e6faaa75334ed7df8d938277b11ddca77bafc9951cfed363c07ad9108dc16c`. The corresponding current-source manifest SHA-256 is `d913159721773ddd035f2c98c975a0cd4353d92a9bb28f2e35d76dd771b89ef7`. The complete validation manifest SHA-256 is `0fdf253b7673d1d7cce278a87f17d76eaaa72ad10e2cc192e9fba76cb76a6589`.

| Evidence | Result | SHA-256 |
|---|---|---|
| Focused spatial syntax log | All new spatial units and `Context.cpp` passed `-fsyntax-only`; the empty log records no warnings or errors | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Dedicated spatial architecture log | State, nested declaration, primitive binding, non-invertible-transform, and source ownership checks passed | `a20f2a7521da43fd3448fe65e4f0c66e943348908912f7d3b342cfbaf03ed55e` |
| Dedicated architecture timing | `elapsed=7.61 user=7.02 system=0.57 maxrss_kb=260748 exit=0` | `8e744655f45c19e8e035b4c5d0965439f7c4334cf1ca12fc12538d59f5d89672` |
| Spatial object-model log | Established spatial model checks passed | `c1d2236dc37fbc17377db93a799a513fd85f5244e6d7b168bdcf46394683b8c0` |
| Spatial grammar-scene log | Three objects, two bindings, cabinet position, and deterministic evidence passed | `e2e476a3d8de840b804cbdddc9e976021b8b2b4fdcd2ac54a92fbfa1722e98f8` |
| Spatial positioning log | Constraint resolution and model publication passed | `cfed31afb71fa289502d40f3e034d9764846a13ca49ccf6cac28706d1ede3174` |
| Spatial query log | Resolved spatial query behavior passed | `5b30ea0addeb03d7422346a7752ab8c3a5455095ad08efb27a4998e36a62b91c` |
| Scene admission architecture log | Scene primitive admission ownership remained valid | `55d7e76d8a168744313c0f652a29ed85f3eeabad1b6aa9c8e723a9baa95ec267` |
| Collision positioning log | Collision-aware spatial positioning remained valid | `44e784062765ff0892e6a886cd20ee9c90be23aa6654aaf46deeecefcedcb80a` |
| P0-P2 log | Complete P0, P1, and P2 gates passed | `27211a19c7657089f49345e9e7ca710fce9077b459c5ec10d6de913a64c92bba` |
| Full editor architecture log | Ended with `All ProGen3D editor architecture checks passed.` | `b69cf1143d64a31b676469e4d35a3cc9f5cfe8c103de0c34a7038c59e57db374` |
| Full editor architecture timing | `elapsed=850.63 user=790.70 system=56.81 maxrss_kb=1002340 exit=0` | `b72fca7557c50424a867b169ae956367535c291a1851630a550f70c4a489121b` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `da3fd450c8ea1030bf74bd598b716712f3fc931d4d5cb51b73ec1ba883971f8c` |
| Forced full native build timing | `elapsed=368.00 user=590.39 system=24.45 maxrss_kb=2497576 exit=0` | `37bb76b1ee9ac58c08950d839d78b34ca89ed8681fee98d9821bb2298a5412a0` |
| Final GUI binary | 277,509,976-byte executable used by both successful smoke gates | `5cb9bf0f73f66c7e67298d7dd19962132e5969f931d33e37b1d9b1b83433b7fc` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Courtyard OMv2 standalone log | Deterministic generation, 666 objects, 291 interfaces, 274 bindings, and two governed resolution records passed | `9ba7ab50f79f2f1ebd6e037540b47cc68b541e3cccadd89fb08fc7c88b809558` |
| SMB OMv1 standalone log | Source contract and runtime scene checks passed | `121c9762d09fe1cb679518f80069ce0fa7f9a3cd6a002548f3b94da72294f943` |
| SMB OMv2 standalone log | Deterministic source, 124 objects, 178 interfaces, 28 connections, three constraints, collision-query, residual, and topology checks passed | `8205d9b3a007fcc8afd983efca47f729fd8c9a31ed587e49a2c8196ea0727540` |
| MVPv2.5 reference collision log | Reference image, ProGen3D views, 124 primitives, 37 objects, 14 joints, placement, and collision gates passed | `6524ac3a767787439ee18134b513d82ac684b7b4f736cda86d6c7c0daea54bf3` |
| Final source hygiene log | Shell syntax, trailing whitespace, purpose-revealing names, context boundary, scoped diff, and exact process cleanup passed | `5b2e9fd7b200e106f3dcff4e297bf40f5c46a11f3ae7a4169358fd706fc711d6` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `5d5976dfcb7483b89245cf27f2af1d4de4a788d48b10c86bc4b088737848aaf0` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `e0ee37711d0e395b55b99843b7632cf45f331fc50d61e9799456a80cdae91656` |

The complete architecture suite ran from the beginning after the final ownership-gate integration and ended successfully. The forced build rebuilt every application translation unit and linked the final GUI binary. Standard and isolated temporal headless smoke gates then ran against that unchanged binary. Exact `/proc` executable-path inspection and exact `Xvfb` process-name inspection confirmed that no ProGen3D application or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, existing `Mesh.cpp` warnings, and generated-catalog variable-tracking notices remain unrelated cleanup work. The extracted spatial construction sources introduce no new warning category.

### Updated concentration boundary

`src/Context.cpp` is now `898` lines, down `205` lines from the prior `1,103`. `include/Context.h` is now `163` lines; the five-line increase records explicit by-value composition of the spatial construction state and services while the implementation concentration moved out of the context.

The five new spatial construction implementation sources contain `402` lines in total: `SceneSpatialObjectScope.cpp` is `21` lines, `SceneSpatialConstructionState.cpp` is `96`, `SceneSpatialObjectDeclarationService.cpp` is `157`, `SceneSpatialPrimitiveBindingService.cpp` is `50`, and `SceneSpatialModelFinalizationService.cpp` is `78`. Their five purpose-specific headers contain `138` lines in total.

The removed concentration represents active spatial scope ownership, declared authored-transform storage, containment and parent-transform derivation, interface/connection/constraint declaration routing, primitive-to-object binding, persistent diagnostic ownership, construction finalization, assembly resolution orchestration, optional building-model derivation, and resolved-model publication that no longer belong to `SceneGenerationContext`.

This append-only record certifies only the spatial construction state and object binding slice. Mesh/export assembly, texture-coordinate generation, scope-stack ownership, lighting and vehicle declaration extraction, direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim. The next ordered scene-context slice is mesh/export assembly and texture-coordinate generation.

## 2026-08-26 Mesh/Export Assembly and Texture Coordinate Generation Slice

### Governed OURD review

The texture-coordinate generation, render-vertex layout, material-buffer assembly, instance-buffer assembly, and export-mesh assembly seam inside `SceneGenerationContext` was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `057ba691392854a8f8e9b9ffe25332fbf04ab3a2ab2c1d0ae1ce21c5cdcec740`; the exact snapshot-output artifact SHA-256 is `a0aac3028cd2a07595fbb57042e3d5d7df2ee7b1137782a8aa435884746efa3e`.
- The bounded 244-line, 1,096-word, 9,126-byte review context SHA-256 is `7241ed5289c0f120e42f52dd23cd96482c233dca7dd04ee29dc5526a40a06f0a`.
- The isolated read-only workspace snapshot value was `25d8830530e2a1f05c3d9046f09da0282b15fb1ba3d49073b818b9fb24a517d0`; the exact isolated snapshot-output artifact SHA-256 is `38013eb931a37574b7ae9ff0f9e53ad9af642f889925eec1f28067c1568f68fd`.
- The exact-hash read-only authority manifest SHA-256 is `cab5a7426cd0897ef245c578ffabdd636d07c2b4829fbedbed67562e0841d68b`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.09 user=0.06 system=0.00 maxrss_kb=33672 exit=0` timing-record SHA-256 is `aabf78d9bfef395891e53228a74890f0d0a5fa6a34f15cd8e271be8eb5b1a103`.
- The first review attempt stopped after announcing a source read and did not produce a substantive recommendation. The preserved incomplete log SHA-256 is `c5df09bce69532e0ba9d8585d05e54e38f2758c96c1eebb7edfa955ba39b8a40`; its `elapsed=13.37 user=0.06 system=0.00 maxrss_kb=33680 exit=0` timing-record SHA-256 is `a0f621aa889fdeaeeb2aac44b42662921112e4b0be5d9785d48199d7b10a7e81`.
- The final context-embedded OURD advisory log SHA-256 is `960a39ccb316bca57bc789a012adcfa948578f0f19e1906fe82f1bbfcbfb1099`; its timing was `elapsed=24.51 user=0.07 system=0.00 maxrss_kb=34000 exit=0` and timing-record SHA-256 is `41532fbfc53475862603510f94cee912673e6547012f817efb53a948d0d4a5db`.
- The 169-line human review and correction record SHA-256 is `18b02abd0e48d46d117600c5404d123b50785fd31ec9254755321e2a26ac1aae`.

The final OURD advisory correctly identified texture projection as a separable responsibility, recognized the `calc` compatibility boundary, and recommended a shared vertex-assembly path for rendering and export. It was not accepted as an implementation plan because its generic names did not satisfy the repository object-model standard, its proposed `MeshExporter` collapsed assembly and output responsibilities, and its PLY ownership proposal conflicted with the established `MeshExportService`, `MeshExportWriter`, and `PLYWriter` boundaries. The human review retained the useful seams and replaced the rejected details with purpose-specific scene models and services grounded in the inspected code.

### UML-readable mesh/export object model

- `SceneTextureProjectionBasis` is the value model for one face-aligned texture projection. It owns the horizontal and vertical world-space projection axes selected from the six dominant-normal directions.
- `SceneRenderVertex` is the value model for one renderable vertex. It owns world position, world normal, and texture coordinate and publishes the established eight-float buffer layout in one explicit operation.
- `SceneMaterialVertexBufferCollection` is the aggregate value model for material-indexed render buffers. It owns the deterministic collection of contiguous per-material float buffers returned by material-buffer assembly.
- `SceneTextureCoordinateGenerationService` owns dominant-axis basis selection, projected-axis normalization, legacy texture-scale interpretation, generated face-aligned UV coordinates, and authored triangle-mesh UV scaling.
- `ScenePrimitiveVertexAssemblyService` is associated by reference with the existing `ScenePrimitiveGeometryService` and composes `SceneTextureCoordinateGenerationService`. It owns cube dual-transform assembly, general primitive transform and inverse-transpose normal handling, stored-or-derived triangle-mesh normals, stored-or-generated triangle-mesh UVs, material filtering, exact vertex pre-counting, deterministic reservation, and scene-order append.
- `SceneExportMeshAssemblyService` is associated by reference with `ScenePrimitiveVertexAssemblyService`. It reuses the canonical vertex assembly, filters removed primitives, groups complete triangle triples, copies positions into the established `Mesh`, and preserves deterministic scene order without acquiring file-format or path ownership.
- `SceneGenerationContext` composes the primitive-vertex and export-mesh services. Its public `calc`, `buildMaterialBuffers`, `buildInstanceBuffer`, and `buildExportMesh` compatibility methods now delegate to those services.
- The existing `MeshExportService`, `MeshExportWriter`, and `PLYWriter` remain the canonical export orchestration, path, and format owners. `SceneGenerationContext::PLY` retains the exact compatibility route `PLYWriter::writeMesh(filename, buildExportMesh(vertex_data))`.

No new inheritance relationship was introduced. The design is composition and association among scene value models, geometry services, assembly services, the existing scene context, and existing export writers.

### Preserved compatibility semantics

The extraction preserves the six dominant-axis projection bases and the positive-Y fallback for degenerate normals. Projected axis lengths retain the `0.000001` minimum. Texture scales at or below `0.1255` still select the legacy `0.90` tile scale; larger authored values still use `max(scale, 0) * 2`. Authored mesh UVs retain their established scaling path, while absent UVs retain generated face-aligned projection.

Cube instances still use the separate primary and secondary transforms. Non-cube instances still use the primary transform and inverse-transpose normal transform. Triangle meshes still prefer stored normals and UVs when present, derive normals or generate UVs when absent, preserve material filtering, skip removed instances, and append in scene order. Export assembly still returns an empty mesh for null source data, reuses the same transformed vertex stream, discards incomplete trailing groups, emits positions only, and keeps one deterministic vertex record per assembled render vertex.

### Acceptance and ownership gates

The dedicated mesh/export architecture harness verifies all six projection directions, the degenerate-normal fallback, both legacy texture-scale branches, non-uniform transform handling, the exact eight-float render-vertex layout, authored and generated UV paths, inverse-transpose normals, cube dual-transform behavior, material filtering, exact pre-counting, removed-instance handling, deterministic export construction, and source ownership. Source gates reject the removed anonymous projection, primitive append, mesh append, and vertex-count helpers from `Context.cpp`; they also require the new services, require `Context` delegation, and prevent the assembly service from acquiring writer dependencies.

The curved-geometry, preview-resource, preview-rendering, axial-profile, extrude-profile, vegetation, P0-P2 grammar, full editor architecture, GUI, temporal GUI, courtyard OMv2, MVPv1 red hatchback, SMB OMv1, SMB OMv2, and MVPv2.5 reference-collision gates all passed against the extracted source set. The vegetation gate exported 2,616 triangles. The MVPv1 red-hatchback gate exercised 124 scene primitives and its export assertions.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Mesh/export source path list | 21 model, service, integration, documentation, harness, and gate paths | `a8797e0bb5cdd7d5b27c7f86cc487df971920bbcfc747e5cd3364b9ceb006d06` |
| Mesh/export source manifest | Exact SHA-256 manifest for the 21 paths | `97abb3b55c43a1cd62a07e64ff343e0b0d770135930244f76d7f01c5f2642a35` |
| Focused source syntax log | New and changed C++ sources compiled with syntax-only checks and no diagnostics | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Dedicated mesh/export architecture log | Behavior, determinism, compatibility, and source ownership gates passed | `f951d69d7f2b9c9322afd119e4db63c69a6f7a2949e3252849108e712c1b2fc3` |
| Dedicated mesh/export architecture timing | `elapsed=3.39 user=3.08 system=0.31 maxrss_kb=252720 exit=0` | `fc866d01387dde7fe0ef0bd643d93d8d43ac75f832648906b7326ab061910391` |
| Curved geometry pipeline log | Curved primitive geometry and extracted mesh/export integration passed | `88624791beb72628ff443ff3f8b97d4eef9651cff42fec9dc9c80cc88a1eb102` |
| Curved geometry pipeline timing | `elapsed=66.44 user=61.93 system=4.28 maxrss_kb=951096 exit=0` | `2d8dc3ead878b77af5d4b9595e0825ca53bad893f734d1324974e8b34b8d22b8` |
| Mesh export service log | Existing export orchestration and writer boundary passed | `a7b80079fed29f75ad9e36fe219c24972f85b176ecff38be0d2c60da6d4d1af4` |
| Mesh export service timing | `elapsed=1.69 user=1.54 system=0.13 maxrss_kb=252248 exit=0` | `ccd59f28136df98cfabbe44b15e02142064c86c76a262a5200c36be20707bc7e` |
| Preview resource ownership log | Existing preview resource boundary passed | `7eefd952ec26b38e1f9e3acaad34b33f9c85bc2afd8db08b31af99a77b52ad3a` |
| Preview resource ownership timing | `elapsed=2.76 user=2.49 system=0.25 maxrss_kb=266828 exit=0` | `11f92300669228d2edc9e142f4d01f976ec5cac6afd46039af70f1f3665c493d` |
| Preview rendering ownership log | Existing preview rendering workflow passed | `f38b8bcb460313cad06367e654e5fe455af1d9e905fa6342539fdd8c3432782a` |
| Preview rendering ownership timing | `elapsed=71.24 user=66.38 system=4.60 maxrss_kb=951184 exit=0` | `0d60cc44c5384c15393e057d25235346b9f6295634b85d128686f0f05586ea67` |
| Axial profile scene log | Axial-profile scene generation and export path passed | `9849e24c5c384dd67976f6ba038515a1107549173f5367f2619f493be0abc62c` |
| Axial profile scene timing | `elapsed=88.48 user=82.66 system=5.52 maxrss_kb=1000484 exit=0` | `908aa4e15512e3e624f168298f5003ddf72b32a612415c2447a76283d0b2e8a4` |
| Extrude profile scene log | Extrude-profile scene generation and export path passed | `aac4d11885a5b2eb3d2184acc07c4d559e3daa05a16146c54dd8ac101dd18d43` |
| Extrude profile scene timing | `elapsed=88.11 user=82.30 system=5.47 maxrss_kb=1002184 exit=0` | `5f72f049d09371ef3b72c3db2731d4ca1e47ff3c0ef88ce18c9bf321c6058bac` |
| Vegetation scene log | Wider vegetation generation passed and exported 2,616 triangles | `307f31e918f306d9113c82131f0b6d65292e43e5f1b3f5badc0f4f901989cfdd` |
| Vegetation scene timing | `elapsed=103.97 user=98.27 system=5.35 maxrss_kb=1002152 exit=0` | `d402db5fbf5d635289b057c1946baeccfd7933ad869204663c839ca7a1d60436` |
| P0-P2 grammar acceptance log | P0, P1, and P2 syntax, semantic, temporal, and runtime harnesses passed | `a2ce9a6a7f1f90c116e2abcbf0fac4507fc638598bc55ffd104cf1735cb072be` |
| P0-P2 grammar acceptance timing | `elapsed=224.11 user=207.41 system=15.82 maxrss_kb=519972 exit=0` | `b4a8a23ddfb2011a463cac2cd3905659641d085e5b718333af2b5bb489140994` |
| Full editor architecture log | Ended with `All ProGen3D editor architecture checks passed.` | `70f71f189cc183e4e9ef982b526925d7e20fda442393f3d734c72ebf8e4f5f5f` |
| Full editor architecture timing | `elapsed=861.57 user=800.95 system=57.69 maxrss_kb=1002312 exit=0` | `a2dd693dfa13152c3eab3b2755bd4692eae2cfb2984f9f5bc8da3eb59ad23f0c` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `f3ff0ff31ccef1aa16c4bfe30e5d161a54d937fdda92df9fa63d9fd6bb4eff07` |
| Forced full native build timing | `elapsed=358.70 user=584.97 system=18.78 maxrss_kb=2643400 exit=0` | `7dce0ab889cddb5be946ccd617f813d27fe7463d3be0e955d7a58765dd0a2b72` |
| Final GUI binary | 277,852,456-byte executable used by both successful smoke gates | `d8f23f3e42d12c109d3dbfbbb0d3ed68f342059d2cc575d95c0dbbb8273e5dbd` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Standard GUI smoke timing | `elapsed=6.41 user=5.03 system=0.10 maxrss_kb=216100 exit=0` | `088520a75f3015109f1de5a198c8e769de56b802fad084f091b27cf561889b45` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Temporal GUI smoke timing | `elapsed=3.66 user=1.53 system=0.12 maxrss_kb=203952 exit=0` | `da960208576e25958db2922ef30fcfd9cbc9de1543399c5b73e09e94426bf460` |
| Courtyard OMv2 standalone log | Deterministic generation, governed resolution, and runtime grammar checks passed | `9ba7ab50f79f2f1ebd6e037540b47cc68b541e3cccadd89fb08fc7c88b809558` |
| Courtyard OMv2 timing | `elapsed=91.20 user=84.90 system=5.95 maxrss_kb=1001884 exit=0` | `e97c50dddb4ed2b6c75d15073f830e81778a030783791324e3a01cd2343748ab` |
| MVPv1 red-hatchback log | 124-primitive generation and export-heavy compatibility checks passed | `7028a8032047ae0cf345663d28941e26630be79d2b6a8de2b5bbf0579f91f8a4` |
| MVPv1 red-hatchback timing | `elapsed=89.96 user=84.05 system=5.52 maxrss_kb=1001728 exit=0` | `87dc24c7256c16ab86e625dda9d5053e00c376ab43389e1f3fe2b235eb4c2d30` |
| SMB OMv1 standalone log | Source contract and runtime scene checks passed | `121c9762d09fe1cb679518f80069ce0fa7f9a3cd6a002548f3b94da72294f943` |
| SMB OMv1 timing | `elapsed=88.79 user=82.99 system=5.50 maxrss_kb=1001420 exit=0` | `cbd3c4f0c5c609ffb9f57d452aa36cb03c90180a1503932427b868fef103b93b` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, and topology checks passed | `8205d9b3a007fcc8afd983efca47f729fd8c9a31ed587e49a2c8196ea0727540` |
| SMB OMv2 timing | `elapsed=92.05 user=86.21 system=5.52 maxrss_kb=1001376 exit=0` | `a67d071a41e402c0e493a71ab1d8d272415bd513ed7b55ab1023813ae385472d` |
| MVPv2.5 reference-collision log | Reference image, ProGen3D views, placement, and collision gates passed | `6524ac3a767787439ee18134b513d82ac684b7b4f736cda86d6c7c0daea54bf3` |
| MVPv2.5 reference-collision timing | `elapsed=84.87 user=79.31 system=5.27 maxrss_kb=1001872 exit=0` | `4fcd79c690353d18b49eeeb483252c5d8241898304d38b5b31f8266fe8777917` |
| Final source hygiene log | Shell syntax, trailing whitespace, names, context independence, writer boundary, scoped diff, unchanged binary, and exact process cleanup passed | `211c7e157018b8168270213c1e8d44667623a783d1f97ae9bcbab6b77c1e2e82` |
| Validation evidence manifest | Exact SHA-256 manifest for the governed review and validation artifacts | `282ad2de9e35f7c7c3e46c1824c58b037ca08ab97069b990aa6a2c7a8ec636d9` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `4528075a846020a63857bf1d49bc34a834edacf102bdf41c2ca669b4d380d721` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `f70eb50d1985c8a39332222bae07665f2a0d998a7b46f5f574baf21c2d050b8a` |

The complete architecture suite ran from the beginning after the final ownership-gate integration and ended successfully. The forced build rebuilt every application translation unit and linked the final GUI binary. Standard and isolated temporal headless smoke gates then ran sequentially against that unchanged binary. Exact executable-path and process-name checks confirmed that no ProGen3D application or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, existing `Mesh.cpp` signedness and unused-function warnings, and generated-catalog variable-tracking notices remain unrelated cleanup work. The extracted mesh/export sources introduce no new warning category.

### Updated concentration boundary

`src/Context.cpp` is now `607` lines, down `291` lines from the prior `898`. `include/Context.h` is now `167` lines; the four-line increase records explicit by-value composition of the primitive-vertex and export-mesh assembly services while the implementation concentration moved out of the context.

The six new mesh/export implementation sources contain `490` lines in total: `SceneTextureProjectionBasis.cpp` is `21` lines, `SceneRenderVertex.cpp` is `43`, `SceneMaterialVertexBufferCollection.cpp` is `33`, `SceneTextureCoordinateGenerationService.cpp` is `96`, `ScenePrimitiveVertexAssemblyService.cpp` is `229`, and `SceneExportMeshAssemblyService.cpp` is `68`. Their six purpose-specific headers contain `165` lines in total.

The removed concentration represents projection-basis selection, legacy texture scaling, generated and authored UV handling, render-vertex serialization, cube and general primitive vertex assembly, triangle-mesh normal and UV selection, material buffer counting and grouping, instance-buffer assembly, and deterministic export-mesh construction that no longer belong to `SceneGenerationContext`.

This append-only record certifies only the mesh/export assembly and texture-coordinate generation slice. Scope-stack ownership, lighting and vehicle declaration extraction, direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim. The next ordered scene-context slice is scope-stack ownership.

## 2026-08-26 Scene Transform Scope Stack Ownership Slice

### Governed OURD review

The mutable grammar transform-scope lifetime and nesting seam inside `SceneGenerationContext` was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `eae871c25f2ad4b309299b726b24b1e23983439ad27b1b95bd35486f083f9939`; the exact snapshot-output artifact SHA-256 is `144977b56925892e9d0bc345e573684d74105bf986adc0eed1a0aa8355a9ae83`.
- The bounded 251-line, 1,045-word, 8,752-byte review context SHA-256 is `ae256e16b647e7b995a079daa9fe2e3fbcb51d3c14d2bd44a2508b04c69bf378`.
- The isolated read-only workspace snapshot value was `a0d01f487ba72c168b1b737e829e469974af37749755dd6894f8d6d4bb0a8b08`; the exact isolated snapshot-output artifact SHA-256 is `0dc0e95716bf8d81b6a86a94bffb59e8ed2056686098eac7943adfdf6ab9c49a`.
- The exact-hash read-only authority manifest SHA-256 is `cadb3ee49ac0ae767c8250d67ade37529e18d1474e1b37d16f38fc9226780b52`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.08 user=0.06 system=0.01 maxrss_kb=34424 exit=0` timing-record SHA-256 is `6ae6c1b21f65ee7dda8cc6ddf297f258f1e0ba1e27791bbb4524a6a91fb37060`.
- The final OURD advisory review log SHA-256 is `6f5b014cd6749c52c04631951d603d3daaf3129769e1656d15ad86e67244f0d2`; its timing was `elapsed=30.12 user=0.06 system=0.03 maxrss_kb=33868 exit=0` and timing-record SHA-256 is `0428d6a7763ce48d985d673662de5858e108dcf5a8c00da97da5a8dea82dcd90`.
- The 172-line human review and correction record SHA-256 is `51495670697bb07dd851d46b2a1576e1c3e6dc389aceeba748e6d38d680dc6cc`.

The OURD advisory correctly identified the raw-pointer stack as one cohesive RAII ownership seam, retained the four public compatibility methods, and required focused behavior and source gates. It was not accepted as an implementation plan because its proposed `TransformScope` subclass treated ownership as an is-a relationship, duplicated the established concrete transform model, used a role-ambiguous stack name, and did not represent empty restoration without coupling the new owner to logging. The human review retained the useful ownership boundary and replaced the rejected subclass with one purpose-specific scene aggregate.

### UML-readable transform-scope object model

- `SceneTransformScopeStack` is the cohesive stateful scene model for mutable transform-scope lifetime and nesting. It uniquely owns one current `SpatialTransformScope` and zero or more suspended `SpatialTransformScope` instances.
- Its constructor creates the root identity scope, so current-scope existence is a structural invariant rather than conditional raw-pointer state.
- `enterInheritedScope()` creates a distinct child through the established complete `SpatialTransformScope` clone semantics, then suspends the exact prior current object.
- `enterPositionPreservingScope()` creates a default child, copies only the current world position into both transforms through `setPosition`, and suspends the exact prior current object.
- `restorePreviousScope()` returns status without logging. Empty restoration leaves current object identity and state unchanged; successful restoration destroys the temporary child and restores the exact most recently suspended parent object.
- `currentScope()` exposes a non-owning reference, and `suspendedScopeCount()` exposes bounded inspection for deterministic tests.
- The existing `SpatialTransformScope` remains the canonical concrete transform-frame model. It still owns primary and secondary matrices, synchronized position, size, rotation and basis metadata, dual-axis scale and translation state, and all transform mutations.
- `SceneGenerationContext` composes one `SceneTransformScopeStack`. Its public `pushScope`, `newScope`, `popScope`, and `getCurrentScope` methods now delegate while retaining their source-compatible names and return types.
- The context facade retains the exact empty-pop diagnostic and the defensive active-scope diagnostic used by spatial-object construction. The new model has no application logging, grammar token dispatch, spatial declaration state, primitive admission, or rendering dependency.

No inheritance relationship was added. `SceneGenerationContext` composes the scope stack, the scope stack aggregates transform scopes through unique ownership, and grammar, primitive-admission, and spatial-declaration callers remain non-owning users of the compatibility facade.

### Preserved compatibility semantics

A root identity scope still exists immediately after context construction. `pushScope` still preserves both transforms, all dual-axis scale and translation state, synchronized position and size, and rotation metadata in a distinct child object. `newScope` still preserves translation in both transforms while resetting rotation, primary and secondary scale differences, and dual-axis state to defaults. Nested restoration remains last-in, first-out, and successful restoration returns the exact suspended parent address and state.

An empty `popScope` still returns the existing current scope unchanged and publishes `Grammar execution error: attempted to pop an empty scope stack.` through the existing facade. `getCurrentScope` remains a non-owning `Scope *` compatibility method whose result remains valid until a later scope transition or context destruction. The separately extracted `SceneSpatialConstructionState` object-scope stack remains a distinct authored spatial-declaration concept.

### Acceptance and ownership gates

The dedicated scope-stack harness verifies structural root ownership, inherited full-state cloning, position-only child creation, primary and secondary transform behavior, dual-axis preservation and reset, exact parent-object restoration, nested last-in-first-out behavior, empty-restoration stability, and repeated RAII construction and destruction. Source gates require explicit context composition and delegation, native and standalone source registration, retained diagnostics, absence of raw `std::stack<Scope *>` ownership, absence of manual scope allocation and deletion in `Context`, no logging or spatial-declaration coupling, no ownership inheritance, purpose-revealing naming, and the reduced scene-context concentration boundary.

The first P0 acceptance rerun exposed one obsolete monolith-string guard that searched `SceneGenerationContext::popScope` for `scopes.empty()` before `scopes.top()`. That gate and its exact-body generated harness were migrated to verify the canonical `SceneTransformScopeStack::restorePreviousScope` empty guard plus the context diagnostic facade. The complete P0, P1, and P2 suite then passed.

The inherited primitive-admission, spatial-construction, mesh/export, curved-geometry, transform grammar, full editor architecture, native build, GUI, temporal GUI, axial-profile, extrude-profile, vegetation, courtyard OMv2, SMB OMv1, SMB OMv2, MVPv1 red hatchback, and MVPv2.5 reference-collision gates all passed against the extracted source set.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Scope-stack source path list | 12 model, integration, documentation, harness, and gate paths | `47a1fd22bef669d1c2f14165b8ef559017d835eebbde9b5e4a80b3b970731e13` |
| Scope-stack source manifest | Exact SHA-256 manifest for the 12 paths | `9934d6fab702adb64bdd9ee4e587851d98bdfa8ce76d997aa88c028fb2a08e80` |
| Focused source syntax log | New model, context integration, and harness compiled with no diagnostics | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Dedicated scope-stack architecture log | Root, inherited, position-only, restoration, RAII, and source ownership gates passed | `e3c6aa6f51478f20fce2bc8b7ad804c0059d3a1775274238559448fd3f2d5269` |
| Dedicated scope-stack architecture timing | `elapsed=0.89 user=0.74 system=0.11 maxrss_kb=194288 exit=0` | `dbef529dd0b4ad01283fbb796622ea6b9eb5019ccf8771e7b5a9bc02fbb6e125` |
| Transform scope evidence log | Exact transform-frame evidence checks passed | `f3601e46d36e923a9b5b5c60f7ca0f101f022fd2d0c0ca1b82e02f0029786b90` |
| Transform scope evidence timing | `elapsed=0.90 user=0.81 system=0.08 maxrss_kb=199588 exit=0` | `0e88b6674ad2d726d2ea299b048399908299673d3645aedf55045d1bad7b8f2e` |
| Transform grammar-to-scene log | Nested scope and transform grammar evidence passed | `c7e2d449b5c33cd6b59d0ddd6625d5952f8de37442c8ec58fa269c26a75b109b` |
| Transform grammar-to-scene timing | `elapsed=84.67 user=79.02 system=5.35 maxrss_kb=1003660 exit=0` | `8c918f083e8539d55be99c8ed2df9df8f4c6100ffbf65fe9ce61473a8db766ac` |
| Primitive-admission architecture log | Admission behavior and source ownership passed | `55d7e76d8a168744313c0f652a29ed85f3eeabad1b6aa9c8e723a9baa95ec267` |
| Spatial-construction architecture log | Spatial declaration, binding, finalization, and ownership passed | `a20f2a7521da43fd3448fe65e4f0c66e943348908912f7d3b342cfbaf03ed55e` |
| Mesh/export architecture log | Mesh assembly, projection, export, and ownership passed | `f951d69d7f2b9c9322afd119e4db63c69a6f7a2949e3252849108e712c1b2fc3` |
| Curved geometry pipeline log | Curved primitive generation and source gates passed | `88624791beb72628ff443ff3f8b97d4eef9651cff42fec9dc9c80cc88a1eb102` |
| P0-P2 grammar acceptance log | P0, P1, and P2 guards, exact-body harnesses, semantics, and runtime checks passed | `20c38cabadb1fbef542a5557880b5182486ac1096b1bcc21fb545eeb146fd543` |
| P0-P2 grammar acceptance timing | `elapsed=99.69 user=91.47 system=7.82 maxrss_kb=518776 exit=0` | `85954fdbc1619a0d2fa99eba27d38e9bef970b9f5a1eb241cf0dc59f2d0e0909` |
| Full editor architecture log | Ended with `All ProGen3D editor architecture checks passed.` | `4a12edd20e78ef86307978b8c4f64c70c72909955ecf7774f66f9ff149195cb6` |
| Full editor architecture timing | `elapsed=862.03 user=801.00 system=57.61 maxrss_kb=1003024 exit=0` | `de4db82fe0b2b7e4770499260e4d80ad36d188e5a2f81f7974e3d4a8c80a9687` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `ba44d3981383eaf6229d6caa93cb2c61b2f3f9bf878ca1a33102c61b73e5ec74` |
| Forced full native build timing | `elapsed=284.44 user=1061.70 system=62.88 maxrss_kb=2642032 exit=0` | `e9e8dcfad987d7f9c1003f3d9016e46c5ceed486c89da11bfe3ec65d54024d1c` |
| Final GUI binary | 277,904,616-byte executable used by both successful smoke gates | `15b8007ba6616ed59406c1d9b68d6812ff4547bdf734130533da65330b5f619a` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Standard GUI smoke timing | `elapsed=6.42 user=5.06 system=0.13 maxrss_kb=218480 exit=0` | `cf4882c86e0cc88fb60cfe55af072276d8defa3de3ac22946e3cace77dc42000` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Temporal GUI smoke timing | `elapsed=3.66 user=1.50 system=0.12 maxrss_kb=204272 exit=0` | `d1e32fee92acc48b5ec88ae2b06136b4058191a5f1842ffacff8c31d3a9b7b13` |
| Axial profile scene log | Valid and fail-closed axial-profile grammar-to-scene gates passed | `9849e24c5c384dd67976f6ba038515a1107549173f5367f2619f493be0abc62c` |
| Extrude profile scene log | Valid and fail-closed extrude-profile grammar-to-scene gates passed | `aac4d11885a5b2eb3d2184acc07c4d559e3daa05a16146c54dd8ac101dd18d43` |
| Vegetation scene log | Plant, vine, scatter, colonization, and L-system grammar-to-scene gates passed | `307f31e918f306d9113c82131f0b6d65292e43e5f1b3f5badc0f4f901989cfdd` |
| Courtyard OMv2 standalone log | Deterministic 666-object nested grammar and governed resolution passed | `9ba7ab50f79f2f1ebd6e037540b47cc68b541e3cccadd89fb08fc7c88b809558` |
| SMB OMv1 standalone log | Source contract and runtime scene checks passed | `121c9762d09fe1cb679518f80069ce0fa7f9a3cd6a002548f3b94da72294f943` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, collision-query, and topology checks passed | `8205d9b3a007fcc8afd983efca47f729fd8c9a31ed587e49a2c8196ea0727540` |
| MVPv1 red-hatchback log | 124-primitive generation, redesign, and collision-positioning gates passed | `7028a8032047ae0cf345663d28941e26630be79d2b6a8de2b5bbf0579f91f8a4` |
| MVPv2.5 reference-collision log | Reference image, views, 124 primitives, 37 objects, 14 joints, placement, and collision gates passed | `6524ac3a767787439ee18134b513d82ac684b7b4f736cda86d6c7c0daea54bf3` |
| Final source hygiene log | Shell and Python syntax, trailing whitespace, names, ownership, manifests, unchanged binary, and exact process cleanup passed | `f80fc26a89463e5dc772347bf603361e229ba394c4a58f2e1786ad26f6871444` |
| Validation evidence manifest | Exact SHA-256 manifest for the governed review and 58 validation artifacts | `129a6f0f0d9759def59a0a85434fe5718ba6311b6324993d4e9ce13b775766bc` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `989097b0f114698e582c945024d1bc3510dbfa49617f61c7f82797623c90cf83` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `0f7092bbc534858f254415601e9a6cf021418e6f1cc4b979b716d74a198d07f6` |

The complete architecture suite ran from the beginning after the P0 source-gate migration and ended successfully. The forced build rebuilt every application translation unit, including `SceneTransformScopeStack.cpp`, and linked the final GUI binary. Standard and isolated temporal headless smoke gates then ran sequentially against that unchanged binary. Exact executable-path and process-name checks confirmed that no ProGen3D application or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, existing `Mesh.cpp` warnings, unused legacy functions in `imgui_main.cpp`, and generated-catalog variable-tracking notices remain unrelated cleanup work. The extracted transform-scope source introduces no new warning category.

### Updated concentration boundary

`src/Context.cpp` is now `572` lines, down `35` lines from the prior `607`. `include/Context.h` is now `166` lines, down one line from the prior `167`; two public raw owning fields and the `<stack>` dependency were removed, while one explicit by-value `SceneTransformScopeStack` composition was added.

The new `SceneTransformScopeStack.cpp` implementation is `51` lines and its purpose-specific header is `24` lines. The dedicated behavior harness is `246` lines and its source-ownership gate is `98` lines.

The removed concentration represents root-scope allocation, current-scope lifetime, suspended-scope lifetime, full inherited child creation, position-only child creation, last-in-first-out restoration, manual deletion, null repair, and stack cleanup that no longer belong to `SceneGenerationContext`.

This append-only record certifies only the scene transform scope stack ownership slice. Lighting and vehicle declaration extraction, direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim. The next ordered scene-context slice is lighting declaration ownership.

## 2026-08-26 Lighting Scene Declaration Ownership Slice

### Governed OURD review

The lighting declaration validation, diagnostic, atomic light-plus-fixture admission, state lifetime, and compatibility-wrapper seam was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `ead0f1082d45b0c4faa2c949c2d37e44cec57c13f13680130424e2fd3340cefc`; the exact snapshot-output artifact SHA-256 is `8214914720893c2f4077f589dd488cadc0184c1ca64043a2fc41ca685cabf7dd`.
- The bounded 237-line, 879-word, 8,783-byte review context SHA-256 is `0dbce4bee043f2eb0ef0da93d02562ab166cbd4cd262692f3bc63d0693bb638b`.
- The isolated read-only workspace snapshot value was `92baa909b94ec6276a7fe997f6010fe0f6e4e98bd95bf979e5714a1e3e80df0c`; the exact isolated snapshot-output artifact SHA-256 is `05de6291322356c65780147aeaf574fb6d9e243d1e8823484d6521fbb9ba432a`.
- The exact-hash read-only authority manifest SHA-256 is `fcbab4bb857d89e450ae2a0e0b3c948419594282071a0636b8eadefccfd3509e`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.08 user=0.05 system=0.01 maxrss_kb=34284 exit=0` timing-record SHA-256 is `fd60ca6f56fda30599659479e1bc9fb52bd575f02a8a7f6cd9cc75d5641d28b0`.
- The final OURD advisory review log SHA-256 is `598f67b7cdc19b66ea8921bbce7f03bfa22c98dfa234a13bf9301f048089ba8f`; its timing was `elapsed=32.10 user=0.07 system=0.00 maxrss_kb=33764 exit=0` and timing-record SHA-256 is `84fbbcf025d54f453cc7679bd4d6cc8f15edb5cc5353bf3665a619b54659f41a`.
- The 165-line human review and correction record SHA-256 is `9f90d04ea08dafd30fd02a9c0af1ab6410375aa51e61c1d3e41e6f7538b5f548`.

The OURD advisory correctly identified declaration validation and atomic mutation as a service responsibility distinct from stored lighting state, retained `SceneGenerationContext` as a compatibility facade, and preserved diagnostics, order, uniqueness, atomicity, and copyability. It was not accepted verbatim because it described `LightingSceneDefinition` as immutable, used the role-ambiguous name `LightingDeclarationService`, and suggested delegating validation back to the state model. Human review retained the seam and corrected it to an explicit scene declaration service with controlled state access and both compatibility surfaces preserved.

### UML-readable lighting declaration object model

- `LightingSceneDefinition` remains the mutable copyable scene-lighting state model. It composes the ordered `SceneLight` collection and canonical `ElectricalControlGraph` and publishes const state access.
- Its established `addLight`, `addFixture`, `addFixtureWithEmitter`, `addSwitch`, and `addCircuit` signatures remain source-compatible thin wrappers. They delegate to `LightingSceneDeclarationService` and no longer own validation, diagnostic, uniqueness, or candidate-transaction algorithms.
- `LightingSceneDeclarationService` is the stateless service for lighting scene declaration admission. It owns empty and duplicate ID validation, exact diagnostics, ordered commits, electrical graph admission routing, and atomic combined light-plus-fixture declaration.
- The service has an explicit privileged association with `LightingSceneDefinition`; the model declares it as a friend so declaration mutation remains controlled without exposing general mutable lighting collections.
- `declareFixtureWithEmitter` copies the complete definition, applies both service declaration paths to the candidate, and replaces the original only after both succeed. Failure in either half changes neither collection.
- `SceneGenerationContext` now composes `LightingSceneDefinition` and `LightingSceneDeclarationService` by value. Its five public lighting declaration methods delegate to the service with the composed definition, and `lightingSceneDefinition()` returns the definition by const reference.
- The former always-allocated `std::unique_ptr<LightingSceneDefinition>` and all lighting-state null checks were removed. Lighting state is now structurally available throughout the context lifetime.
- `ElectricalControlGraph` remains the fixture, switch, circuit, identity, lookup, and ordering owner. Preview evaluation, validation reports, persistence, evidence, rendering, and editor workflows remain separate downstream owners.

No inheritance relationship was added. The context composes state and service, the lighting definition composes its scene and electrical state, and the declaration service is associated with the definition for controlled mutation.

### Preserved compatibility semantics

Empty light IDs still produce `Light requires a non-empty stable ID.` Duplicate light IDs still produce `Light ID '<id>' is declared more than once.` Fixture, switch, and circuit empty or duplicate IDs retain their exact established diagnostics. Light, fixture, switch, and circuit collections retain declaration order. Rejected declarations leave state unchanged.

Direct `LightingSceneDefinition` mutation calls and direct `LightingSceneDeclarationService` calls produce the same state and diagnostics. `PreparedScenePreviewPublication` continues to store and copy definitions by value. A copied definition remains independently mutable. Successful combined declarations commit both a light and fixture; duplicate-light and duplicate-fixture failures commit neither candidate record.

### Acceptance and ownership gates

The dedicated declaration harness verifies exact diagnostics, empty and duplicate rejection, stable order, successful and failed atomic transactions, direct model compatibility wrappers, direct service behavior, and independent value-copy semantics. Source gates require the purpose-specific service, friend association, by-value context composition and delegation, native and standalone link registration, removal of declaration algorithms from `LightingSceneDefinition.cpp`, removal of optional lighting state from `Context`, absence of rendering, persistence, evidence, evaluation, editor, or context dependencies in the service, purpose-revealing naming, and the bounded scene-context concentration.

The inherited lighting model, lighting grammar-to-scene, persistence, electrical control workflow, completed scene publication, P0-P2, full editor architecture, native build, GUI, temporal GUI, modern luxury residence lighting evidence, courtyard OMv2, SMB OMv2, and MVPv1 red hatchback gates all passed against the extracted source set. The residence evidence retained 27 lights, 27 fixtures, and stable state hash `7986025927890646612`; the vehicle grammar retained four lights.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Lighting declaration source path list | 13 model, service, integration, documentation, harness, and gate paths | `225323ca15a660644636f8b6dfbb6926d4b6d296083f8dea19a7461b2001ecae` |
| Lighting declaration source manifest | Exact SHA-256 manifest for the 13 paths | `ab4f3d7a713b220919e2e82d91f97a42f1880c726594917d9d7a900f85cc7578` |
| Focused source syntax log | Model wrappers, declaration service, context integration, and harness compiled with no diagnostics | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Dedicated lighting declaration log | Diagnostics, ordering, atomicity, compatibility, copying, and ownership gates passed | `e5ffa37c93ef2fb3ec4b16c474105f0e269cb40ceda7e94af377e1bc9d732068` |
| Dedicated lighting declaration timing | `elapsed=1.42 user=1.25 system=0.18 maxrss_kb=196968 exit=0` | `9855d68031fae10c033ee966d726a458d93e798c687d80f5520899c8c7325048` |
| Lighting model log | Photometry, GPU layout, controls, evidence, and shadow allocation passed | `a9d66502ca748993bc539698871d06380a9ce36a03b5c964af7e516b8a803e8f` |
| Lighting grammar-to-scene log | Lighting declaration grammar and runtime scene checks passed | `96584c3e97e782106216d9148fd08b520a5a653343e4e1ad0f4ea2eb0e09390a` |
| Lighting grammar-to-scene timing | `elapsed=85.56 user=79.76 system=5.49 maxrss_kb=1003628 exit=0` | `dfae051678cf11ef407823db71a066e149f3be0d768c701900a17cdb55af861b` |
| Lighting persistence log | Scene lighting persistence checks passed | `2dcda4956574b78bdd85635eea7ab9911b5585234a57cb68c9dae13aa8472f1f` |
| Lighting controls workflow log | Lighting and electrical behavior and ownership checks passed | `31b2435b01ba00cc74af1ad6181c0303b161223e39f252ff7d09d51f265e5ea8` |
| Completed publication log | Prepared lighting definition copy and publication source gates passed | `3b32ef8f631181e3a6c51b295a414daa0442e705ba1a51e6ff219228b34167f6` |
| P0-P2 grammar acceptance log | P0, P1, and P2 guards, semantics, and runtime harnesses passed | `20c38cabadb1fbef542a5557880b5182486ac1096b1bcc21fb545eeb146fd543` |
| P0-P2 grammar acceptance timing | `elapsed=100.80 user=92.77 system=7.61 maxrss_kb=519416 exit=0` | `9e7bfd446607676cab8adc4e857e4b5b8468a46b4484fb7c2026d3306e7761b0` |
| Full editor architecture log | Ended with `All ProGen3D editor architecture checks passed.` | `e11f3b6329194dfd1c7face291946ad03e67a193febd4df2db3783a22c2c82ae` |
| Full editor architecture timing | `elapsed=873.18 user=811.79 system=58.49 maxrss_kb=1003456 exit=0` | `8ad04f8108c4589f29466fa8d3ce76a6fcd09b0b7d8f58b9a9a4b6e624363a8c` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `f3fc408c70789b00e1fd3f70f923ec7f806d46219d1154e6a3739b9c0ee1611d` |
| Forced full native build timing | `elapsed=289.02 user=1084.39 system=72.48 maxrss_kb=2620924 exit=0` | `78260e358e4813fedd0289d8623b873ec054c38f3a3db85c9182ea2454d66762` |
| Final GUI binary | 278,686,392-byte executable used by both successful smoke gates | `376ec71e9c0a5d6bd0c020cac41dbd17e6e4005a2aebb81dae6ea94277c3267d` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Modern luxury residence lighting evidence log | 27 lights, 27 fixtures, deterministic evidence, and exact state checks passed | `7b87dd532687846e352822290911e7c13c7ef42e89dd2f2f3cdef895db32cf3b` |
| Courtyard OMv2 standalone log | Deterministic nested generation and governed resolution passed | `9ba7ab50f79f2f1ebd6e037540b47cc68b541e3cccadd89fb08fc7c88b809558` |
| SMB OMv2 standalone log | Deterministic source, runtime, evidence, collision-query, and topology checks passed | `8205d9b3a007fcc8afd983efca47f729fd8c9a31ed587e49a2c8196ea0727540` |
| MVPv1 red-hatchback log | 124 primitives, 15 objects, four lights, six joints, redesign, and collision checks passed | `7028a8032047ae0cf345663d28941e26630be79d2b6a8de2b5bbf0579f91f8a4` |
| Final source hygiene log | Shell syntax, whitespace, names, ownership, dependencies, manifests, unchanged binary, and exact process cleanup passed | `f29ff7108b3445a21b63fe4864ec2c46c4b4f11cc2fc787e34faa009290082f3` |
| Validation evidence manifest | Exact SHA-256 manifest for the governed review and 48 validation artifacts | `4c3044f757cd0a4ac7ffcc29693abf01ac8fe3973d021fe400583c9cf11db7c6` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `3de00c8cab34461c09f9f1ec04d7202a8d11b25f1a2a6c06bd35c997816d6b3d` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `b46600282532ac56aaecb347c1145af45d790a28d49f0d0f82e90575bdb28953` |

The complete architecture suite ran from the beginning after final service registration and ended successfully. The forced build rebuilt every application translation unit, including `LightingSceneDeclarationService.cpp`, and linked the final GUI binary. Standard and isolated temporal headless smoke gates then ran sequentially against that unchanged binary. Exact executable-path and process-name checks confirmed that no ProGen3D application or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, the unused `maxPrimeIndex` declaration in `include/Solution.h`, existing `Mesh.cpp` warnings, unused legacy functions in `imgui_main.cpp`, and generated-catalog variable-tracking notices remain unrelated cleanup work. The extracted lighting declaration sources introduce no new warning category.

### Updated concentration boundary

`src/Context.cpp` remains `572` lines. Its lighting section no longer owns optional state checks or direct model mutation; the unchanged line count preserves readable multi-line delegation. `include/Context.h` is now `168` lines, up two lines from `166`, because the implicit pointer relationship was replaced by explicit by-value composition of both lighting state and declaration service.

`LightingSceneDefinition.cpp` is now `40` lines and contains only compatibility wrappers, down from the prior `56`-line mixed state-and-workflow implementation. `LightingSceneDeclarationService.cpp` is `70` lines and its purpose-specific header is `26` lines. The lighting definition header is `30` lines and records the controlled friend association. The dedicated behavior harness is `179` lines and its source-ownership gate is `110` lines.

The removed concentration represents light identity validation, duplicate diagnostics, electrical declaration diagnostic mapping, atomic candidate construction, two-part commit orchestration, and scene-context optional lighting lifetime that no longer belong to either the state model or `SceneGenerationContext`.

This append-only record certifies only the lighting scene declaration ownership slice. Vehicle declaration extraction, direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim. The next ordered scene-context slice is vehicle joint declaration ownership.

## 2026-08-26 Vehicle Joint Declaration Ownership Slice

### Governed OURD review

The vehicle-joint validation, diagnostic, ordered admission, graph lifetime, and scene-context compatibility seam was reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `ba415592d0578ad140b9c07cb449f596b37b81112401121f884c9b32f949065c`; the exact snapshot-output artifact SHA-256 is `8f96d71376859270602a52b0a72003be9085a823f58de4c5912a25b787f5c0e2`.
- The bounded 191-line, 740-word, 6,889-byte review context SHA-256 is `441992faa27a4c0aaa3ec391f6360d439980763ad23064ae454ec0c1fa2b0f4a`.
- The isolated read-only workspace snapshot value was `8b0a45734c90ee3a3647164ab3b7b8c5461f0cde0cf4c974f29039e57b9e53d5`; the exact isolated snapshot-output artifact SHA-256 is `83fd7625c452127dd920734fdc07babf9a01d60714b1003c0704e824c6a0962b`.
- The exact-hash read-only authority manifest SHA-256 is `9ec90a3a07059c75cf1199c8f9151183696f3841f5a9aafb43d8285dff6d9c8e`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.09 user=0.06 system=0.01 maxrss_kb=33748 exit=0` timing-record SHA-256 is `36ca82ff40451e9722494e234455e442a8570d509ff7a826f16b8ccf54dae476`.
- The final OURD advisory review log SHA-256 is `cb9e41d08587532e5e37dd288f6acf53164c3027179c42088b3f18ae02e7ab58`; its timing was `elapsed=35.77 user=0.07 system=0.00 maxrss_kb=34044 exit=0` and timing-record SHA-256 is `fc34238290b09c9c23a23fc2a74a0d9adfc6168a8aaba4479cf67b13e2a1b52d`.
- The 137-line human review and correction record SHA-256 is `63280c6049aa60ef42d59ce81802c23afcc5afd7b1ba1d2f5284856ed78046c8`.

The OURD advisory correctly identified validated vehicle-joint admission as one service responsibility distinct from stored graph state, retained `SceneGenerationContext` as a compatibility facade, and required focused behavior and source-ownership gates. It was not accepted verbatim because it retained a nullable graph pointer and proposed exposing graph state through the declaration service. Human review retained the useful service boundary while making graph availability structural, keeping state access on the context facade, and preserving the graph's independent duplicate-only contract.

### UML-readable vehicle declaration object model

- `VehicleJointGraph` remains the canonical mutable, copyable, ordered vehicle-joint state model. It owns joint identity uniqueness and the exact duplicate diagnostic, but does not acquire kinematic validation responsibility.
- `VehicleKinematicValidationService` remains the canonical domain-validation service for joint identifiers, endpoint identities, axes, finite limits, ordered ranges, and bounded current state.
- `VehicleJointDeclarationService` is the purpose-specific admission service. It composes one `VehicleKinematicValidationService`, validates a candidate first, publishes the first established validation issue when invalid, and delegates successful ordered admission to a supplied `VehicleJointGraph`.
- The declaration service has a non-owning association with the graph supplied to `declareJoint`; it does not retain, expose, or manufacture graph state.
- `SceneGenerationContext` now composes both `VehicleJointGraph` and `VehicleJointDeclarationService` by value. `addVehicleJoint` delegates to the service and `vehicleJointGraph` returns the composed graph by const reference.
- The former always-allocated `std::unique_ptr<VehicleJointGraph>` and unreachable `Vehicle joint graph is unavailable.` branch were removed. Vehicle-joint state is now a structural context invariant.
- `McsMv22ClosureKinematicEvaluationService` and other direct graph users remain associated with the graph's duplicate-only admission surface; they are not forced through context-specific validation.

No inheritance relationship was added. The context composes graph state and declaration service, the declaration service composes the validator, and the service is associated with the graph only for the duration of one explicit admission call.

### Preserved compatibility semantics

Valid joints still enter the graph in declaration order. Every established validation condition still fails before mutation, and the diagnostic remains the first message from `VehicleKinematicValidationService`: `Vehicle joint '<id>' has an invalid axis, endpoint, limit, or current state.` Duplicate valid joints still fail through `VehicleJointGraph` with `Vehicle joint '<id>' is declared more than once.`

Validation still precedes duplicate admission, so an invalid candidate whose identifier already exists reports the validation diagnostic rather than the duplicate diagnostic. Direct `VehicleJointGraph::addJoint` remains intentionally duplicate-only and therefore still accepts an otherwise invalid unique joint; this preserves its existing low-level state-model contract and closure-evaluation caller behavior. Graph copies remain independent ordered value objects.

### Acceptance and ownership gates

The dedicated vehicle-joint declaration harness verifies valid ordered admission, exact duplicate diagnostics, empty identity, missing source and target endpoints, non-finite and zero required axes, non-finite limit and current values, reversed limits, below-range and above-range state, validation-before-duplicate ordering, direct graph duplicate-only compatibility, and graph-copy independence.

Source gates require explicit by-value graph and service composition in `SceneGenerationContext`, context delegation without validation or nullable graph ownership, validator composition in `VehicleJointDeclarationService`, retained duplicate ownership in `VehicleJointGraph`, no graph getter on the declaration service, no editor, renderer, wheel, collision, or evidence dependency, native and standalone source registration, purpose-revealing class names, and the reduced context concentration boundary.

The inherited model/grammar vehicle gate, MCSMv2.2 suspension gate, MVPv2.5 bonnet gate, MVPv1 red-hatchback gate, MVPv2.5 reference-collision gate, complete P0-P2 suite, full editor architecture suite, forced native build, standard GUI smoke, and temporal GUI smoke all passed against the extracted source.

### Validation evidence

| Validation artifact | Result | SHA-256 |
|---|---|---|
| Vehicle declaration source path list | 10 production, integration, documentation, harness, and gate paths | `8ea202ed6f6afcd297a6a37cf1ca6179811468094b91ae717fae03b321234483` |
| Vehicle declaration source manifest | Exact SHA-256 manifest for the 10 paths | `78d1a838f954e6025c9c568278a330d48c31e1278aa49182193da2596d81bcfb` |
| Focused syntax log | Empty successful compiler-diagnostic capture | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Vehicle declaration architecture log | Behavior and source-ownership checks passed | `a318217c0797efd1736e72c4fdb8e848e631f96188e632479188b6a7aeb255f5` |
| Vehicle declaration architecture timing | `elapsed=1.19 user=1.02 system=0.14 maxrss_kb=188056 exit=0` | `d0e5079e7bb1f476c1891f4b9474f1b26a31d04f6e88072d30a4fed86a9a0134` |
| Model/grammar vehicle log | Nine assemblies, 16 joints, 54,378 vertices, 18,126 triangles, and deterministic hash `6642185694028990626` passed | `a9ee7fa61ea902f5da738ea5da9cb78e02aafd356dfd9bbab74064ddf203d267` |
| MCSMv2.2 suspension log | Suspension kinematic checks passed | `8c23db7b6e71cb13aca5da81fa7e844880cd9b5902b4fcf9b12ca9a004696f90` |
| MVPv2.5 bonnet log | 29 primitives, 29 parameterized primitives, 16 objects, six connections, and three joints passed | `caf4b02383d06ae4f23bd9d3b53646b81ad16eaf157d1d28fbd014622cdebb43` |
| MVPv1 red-hatchback log | 124 primitives, 15 objects, four lights, and six joints passed | `7028a8032047ae0cf345663d28941e26630be79d2b6a8de2b5bbf0579f91f8a4` |
| MVPv2.5 reference-collision log | 124 primitives, 37 objects, 14 joints, and reference-collision checks passed | `6524ac3a767787439ee18134b513d82ac684b7b4f736cda86d6c7c0daea54bf3` |
| Complete P0-P2 log | Ended with `All Progen3D P2 checks passed.` | `20c38cabadb1fbef542a5557880b5182486ac1096b1bcc21fb545eeb146fd543` |
| Complete P0-P2 timing | `elapsed=101.14 user=93.18 system=7.45 maxrss_kb=519788 exit=0` | `d6c41c671a80ed7fd58cf7b9142a344ed6862f2906885b9842680b6f1b9c81a7` |
| Full editor architecture log | Ended with `All ProGen3D editor architecture checks passed.` | `9c5c5e38fdb3aad28ea698c048919e2f89ec2f39f605ef3545323de78b0affe0` |
| Full editor architecture timing | `elapsed=889.89 user=827.45 system=59.19 maxrss_kb=1003712 exit=0` | `4352129041e6d90d2a5537f9d392bcacc16b8abd3f0c5ed6ad5f753164b77ea3` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `374df1f1ee180d247cf395db190926436f02ad523fa2fa7d6be7c1d1d8b9d65f` |
| Forced full native build timing | `elapsed=294.45 user=1071.45 system=82.45 maxrss_kb=2554984 exit=0` | `05aad9d8f9d9986e01653d9ebe6667e746686f760acaea9152fa6bdc19cdfb7c` |
| Final GUI binary | 279,005,392-byte executable used unchanged by both successful smoke gates | `1b6594d5cc9cf9603f7acf29bbb78bea1c04c13b284b53740593f3861ac882aa` |
| Standard GUI smoke log | Passed FOV, render-frame, and GUI smoke markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and GUI smoke markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Final source hygiene log | Shell syntax, whitespace, names, ownership, dependency boundaries, registrations, line counts, and binary identity passed | `92bcc3e3f9d54e2e0ff9853bfbbca17d00e52f7b0613611a9875259c17558e03` |
| Validation evidence manifest | Exact SHA-256 manifest for the governed review and 40 validation artifacts | `65cf8a4a5eff1c95d79715815600379a755178d79d70320ac8dec59d8f2ad21b` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `22855abcb12299a9ff454f82a38710b37a85ef8d15944a072367640186dd9706` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `ee8f8856c736dfd127e9fd667e4289951c0c99f7e889a39d2607aeeb36f4fe8f` |

The complete architecture suite ran from the beginning after final service registration and ended successfully. The forced build rebuilt every application translation unit, including `VehicleJointDeclarationService.cpp`, and linked the final GUI binary. Standard and temporal headless smoke gates then ran sequentially against that unchanged binary. Exact executable-path and process-name checks confirmed that no ProGen3D application or Xvfb process remained.

Existing signedness warnings in `include/grammar.h`, existing `Mesh.cpp` warnings, the vendored `stb_image` diagnostic, and generated-catalog variable-tracking notices remain unrelated cleanup work. The extracted vehicle declaration sources introduce no new warning category.

### Updated concentration boundary

`src/Context.cpp` is now `558` lines, down from `572`. Its vehicle section no longer owns optional graph lifetime, validation invocation, first-issue selection, or direct graph admission. `include/Context.h` is now `170` lines, up from `168`, because implicit pointer ownership was replaced by explicit by-value composition of both vehicle state and declaration service.

`VehicleJointDeclarationService.cpp` is `21` lines and its purpose-specific header is `17` lines. The canonical `VehicleJointGraph.cpp` remains `22` lines and preserves its duplicate-only responsibility. The dedicated behavior harness is `139` lines and its combined compile/source gate is `68` lines.

The removed concentration represents nullable graph lifetime, an impossible unavailable-state diagnostic, kinematic validation orchestration, first-issue publication, and validated admission routing that no longer belong to `SceneGenerationContext`.

This append-only record certifies only the vehicle joint declaration ownership slice. Direct legacy logging bridge removal, private renderer decomposition, packaging, repository hygiene, and complete repository-level certification remain mandatory before a complete repository-refactor claim. The next ordered slice is direct legacy logging bridge removal.

## 2026-08-26 Direct Legacy Logging Bridge Removal Slice

### Governed OURD review

The temporary process-global application logging bridge, its scoped binding, its production free-function callers, and the explicit publication relationships required to replace them were reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `a5dfa9671c75d9b70dfcf876a93fce5a13dce4ac6d3cdc160d488c4d8c7f3489`; the exact snapshot-output artifact SHA-256 is `a4a0b2483e5443a496866b230465b22d370d45496c4c1379898def9172dee249`. Snapshot capture timing was `elapsed=9.37 user=6.70 system=2.08 maxrss_kb=51592 exit=0`, and the timing-record SHA-256 is `de810e5e515889213bc4543a1713b6fb493f6cdb4a34ca35acaf9d35f99fb095`.
- The bounded 357-line, 1,638-word, 14,853-byte review context SHA-256 is `d5e7ad5294495e9ef9f96bbe11a7345fc3be7e18e71ba9b788197f05641a8e3e`.
- The isolated read-only workspace snapshot value was `e9f8b35f67a19a094db3a57b9c2c281c668532f9db973ebafb9ae66f6abb8d48`; the exact isolated snapshot-output artifact SHA-256 is `a7aa7b67d5f491775b6c572064dc9cc5d7f9f54b7785ee7757bc3f858b48751a`. Its timing was `elapsed=0.06 user=0.05 system=0.00 maxrss_kb=30916 exit=0`, and the timing-record SHA-256 is `b42b38a0d4793edb2a995e744becadf3cb115487ba1317b17631a31f6443db53`.
- The exact-hash read-only authority manifest SHA-256 is `f11da660d2de9baf203700394d65009eb2048023c7b275f640c8b23732eb4183`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.10 user=0.05 system=0.01 maxrss_kb=33872 exit=0` timing-record SHA-256 is `60a06a2e9c5d7815da70654ae07232e7194c9ea6c2d5390e6e2b6df7991c8bb7`.
- The final 145-line, 677-word, 6,372-byte OURD advisory review log SHA-256 is `4a6d43f1b514688840b19f5c4c7d1eac6fee861b8fa56433da21cd57cd526303`; its timing was `elapsed=51.40 user=0.07 system=0.00 maxrss_kb=34180 exit=0` and timing-record SHA-256 is `9c1f77efe9006042bc735d87576ca953b6a4b25e89fdff371d86ce99ff1ae348`.
- The 249-line, 1,369-word, 11,826-byte human review and correction record SHA-256 is `6002fde6f0e86ead9f06b35c39d93a1655cf230468f835e40bb0bb338edbc179`.

The OURD advisory correctly identified the global bridge as hidden process-wide lifetime and required explicit caller-to-owner publication relationships, terminal fallback, background-worker safety, exact messages, exact smoke markers, and source-ownership gates. It was not accepted as an implementation design because it misclassified services and contexts as models, moved parser and scene facts into orchestration services, proposed one broad renderer owner for unrelated responsibilities, and did not define exact source mapping, standalone fallback ownership, construction order, or GLFW treatment. Human review retained the useful boundary and supplied the authoritative object model below.

### UML-readable runtime diagnostic object model

- `RuntimeDiagnosticSeverity` is the generic severity model with the bridge-compatible `Debug` and `Error` values.
- `RuntimeDiagnosticSource` is the generic source model with `ApplicationRuntime`, `GrammarProcessing`, `GrammarExecution`, `SceneGeneration`, and `PreviewRendering`.
- `RuntimeDiagnosticPublisher` is the abstract runtime diagnostic publication-service category. `TerminalRuntimeDiagnosticPublicationService` and `ApplicationLogRuntimeDiagnosticPublicationService` are concrete is-a relationships because both publish the same severity, source, and message contract.
- `TerminalRuntimeDiagnosticPublicationService` owns standalone terminal behavior. It suppresses empty messages, removes one trailing newline, writes debug diagnostics to `std::clog`, writes errors to `std::cerr`, and emits each retained message exactly once.
- `RuntimeDiagnosticPublicationContext` composes one terminal publication service and retains a structurally non-null `std::reference_wrapper<RuntimeDiagnosticPublisher>` association. Default construction selects the composed terminal service; injected construction selects the supplied publisher. It contains no global, static, atomic, or thread-local publisher state.
- `ApplicationLogRuntimeDiagnosticPublicationService` associates with the canonical `ApplicationLogPublisher`. It maps `ApplicationRuntime` to `ApplicationLogSource::ApplicationRuntime`, both grammar sources to `GrammarRuntime`, scene generation to `SceneGeneration`, and preview rendering to `PreviewRendering` while preserving debug/error severity.
- `ApplicationLogSource::LegacyRuntime` was removed. `GrammarRuntime`, `SceneGeneration`, and `PreviewRendering` now name the actual application-log responsibilities.

### Explicit grammar and scene publication relationships

- `GrammarDocument` now composes `RuntimeDiagnosticPublicationContext`. Existing default, path, and stream constructors remain source-compatible and retain terminal fallback; injected overloads associate the document with an external `RuntimeDiagnosticPublisher`.
- `GrammarRuntimeDiagnosticPublicationService` is the purpose-specific grammar publication service. It routes processing debug/error facts through a `GrammarDocument`, routes execution errors through either a `GrammarDocument` or `SceneGenerationContext`, and creates one local terminal service only when no owner is available.
- `GrammarDiagnosticPublicationService` retains structured diagnostic collection and rendered-message publication. `GrammarRuleExpansionService` retains exception-path expansion ownership. Neither service acquires global publisher state.
- `SceneGenerationContext` composes its own `RuntimeDiagnosticPublicationContext`. `GrammarDocument::addContext()` injects the document publisher into the generated scene context, so parse, semantic, expansion, scene-VM, primitive-admission, and scope-restoration facts preserve one explicit publication chain.
- `GrammarCompilationService` associates with `RuntimeDiagnosticPublisher &`; `EditorSceneGenerationRuntime` receives that association during initialization. The background worker therefore publishes through the application adapter without consulting process state, and ordered shutdown still joins the worker before the application logging owner is destroyed.
- The established terminal fallback remains available to standalone grammar and scene harnesses. The P0 exact-body scope harness and legacy grammar stub were updated to model the new publisher association rather than recreate `errorout`.

### Explicit application, renderer, and GLFW ownership

- `Progen3dEditorApplication` now composes, in lifetime order, `ApplicationLoggingContext`, `ApplicationLogRuntimeDiagnosticPublicationService`, one owned `PreviewRenderer`, one owned `PreviewCubemapGenerator`, the workspace, and `EditorRuntimeEnvironment`.
- `createOpenGlPreviewRenderer(RuntimeDiagnosticPublisher &)` and `createOpenGlPreviewCubemapGenerator(RuntimeDiagnosticPublisher &)` replace the former global renderer and generator accessors. The private concrete renderer and cubemap generator retain non-owning publisher associations.
- `EditorApplicationRuntimeContext` explicitly associates with `RuntimeDiagnosticPublisher &`, `PreviewRenderer &`, and `PreviewCubemapGenerator &`. Startup, smoke, render-settings, resource, rendering, generation, and application paths receive those relationships through construction rather than service-location functions.
- `EditorRuntimeEnvironment` associates with the application-owned renderer and calls its `initialize` and `shutdown` operations directly. Resource and rendering workflow services continue to associate with that same renderer instance.
- `EditorSmokeTestWorkflow`, `Progen3dEditorStartupOperations`, `EditorStartupDocumentService`, and `imgui_main.cpp` publish directly through the typed application or runtime context owners. The exact standard, temporal, FOV, and render-frame marker strings remain unchanged.
- `GlfwApplicationWindow` associates with `RuntimeDiagnosticPublisher &`. The process-global GLFW error callback was removed; the window polls `glfwGetError` at bounded operation boundaries and publishes `ApplicationRuntime` diagnostics through its explicit association.
- `Progen3dEditorStartupOperations::isApplicationWindowClosing()` now delegates to `GlfwApplicationWindow::closeRequested()` rather than bypassing the window abstraction with `glfwWindowShouldClose`. The wrapper polls and publishes any pending GLFW error after the query.

### Removed bridge and compatibility surface

`include/editor/service/LegacyApplicationLoggingBridge.h` and `src/editor/service/LegacyApplicationLoggingBridge.cpp` were deleted. `ScopedLegacyApplicationLoggingBinding`, the process-global atomic publisher pointer, nested binding restoration, `ApplicationLogSource::LegacyRuntime`, and production `debugout` / `errorout` forwarding were removed. No production header or source contains a bridge reference or bridge-dependent free-function call. The separate file-local terminal `debugout` in `src/StlCatalog.cpp` remains explicitly outside this slice and is not connected to application logging.

The public compatibility requirements remain: exact diagnostic text and ordering, structured grammar diagnostic state, no partial expansion publication after abort, source-compatible default grammar and scene construction, background publication safety, one terminal emission per non-empty message, renderer initialization and shutdown ordering, preview texture identity, and all GUI smoke marker strings.

### Acceptance and validation evidence

| Evidence | Result | SHA-256 |
|---|---|---|
| Slice source path inventory | 77 source, test, documentation, and build-registration paths derived from the pre-review snapshot boundary | `a13e30b00f9b5e757ab8f3773515e4dbb4ed23a46cae21e4d4cd3ef5a48323e0` |
| Slice source manifest | Exact SHA-256 manifest for all 77 inventoried paths | `ef1e1a8e2e31fb13cf71f9619685eb3b9f49ec9725c3720afb96abba0ee05fe5` |
| Runtime diagnostic/application adapter log | Terminal fallback, injected context, exact mapping, concurrency, and source ownership passed | `8ac5856344a4d8e27aabfbc2acfc3af16f11d8153c105299122c13633123044a` |
| Runtime diagnostic/application adapter timing | `elapsed=1.30 user=1.12 system=0.21 maxrss_kb=148316 exit=0` | `37734bd73202823a5d82042712710a19c955deea3b6360ae6fae60b047887b00` |
| Canonical application log log | Typed bounded application-log checks passed | `ef67ff896faf99f9664cef00c90c3bc5753e549f03d513c0d8b19c838f989d8b` |
| Canonical application log timing | `elapsed=0.42 user=0.36 system=0.05 maxrss_kb=113444 exit=0` | `8e4026b261febe50fbc1912291cc4b46005be57842b8607efc397eac309162ae` |
| Startup ownership log | Startup behavior and wrapper ownership gates passed | `3ba63ea29d9ed38275b42588b23b34004f67fa2870250c64ac95de07cdd6764e` |
| Preview-session ownership log | Explicit renderer association and no GLFW callback bridge passed | `bd450308857d632b987e18b1439d3010c5694cdbd21fdce9451a3308fb90076f` |
| Transform grammar log | Exact grammar-to-scene diagnostic propagation passed after canonical diagnostic source registration | `c0e774e315e8844b7d6d776b31c5e0742f1632ee73177993c3085f792dfe0d11` |
| Curved geometry pipeline log | Open-density rejection was verified through an injected typed scene diagnostic publisher | `3918cbaa35e6d8315227407acc4cd6cbddfb0403917b6027ea1193d16b5035fb` |
| Complete editor architecture log | Ended with `All ProGen3D editor architecture checks passed.` | `a9cf74975d334a00434ec0fd4cedce23da854dbd32909bb6c23abf7ff2900bc7` |
| Complete editor architecture timing | `elapsed=902.52 user=839.70 system=59.66 maxrss_kb=1000416 exit=0` | `c44568d9b68ef4bf4535a284dc1bde1cb57ebdaee979fba9fe395c66839c6f1d` |
| Complete P0-P2 log | Ended with `All Progen3D P2 checks passed.` | `b75359762b4dc72e411304b20418b855865f72a51814922c538dc6dad04f5361` |
| Complete P0-P2 timing | `elapsed=103.63 user=95.22 system=8.00 maxrss_kb=519708 exit=0` | `a6596345d90beba02b87bd12f05ab95699cfbec3d4908f9caa68e5bf25f80bfe` |
| Forced full native build log | Every application translation unit rebuilt and `progen3d-editor-gui` linked | `b249c9ca57e9bf0ac31a44fb4b327bcc190168334906b71a6794c1a1664f0e79` |
| Forced full native build timing | `elapsed=376.12 user=605.28 system=23.69 maxrss_kb=2606292 exit=0` | `5e5d7993af8d1da955c698195dd9d6faa1766a53c9c663a878af06caf98c595d` |
| Final GUI binary | 280,000,704-byte executable used unchanged by both successful smoke gates | `9770dd3c6f97bdfafd7735f19983cc3c39ca4761607f7afcdfcd8961af3179c5` |
| Standard GUI smoke log | Passed FOV, render-frame, and standard GUI markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Standard GUI smoke timing | `elapsed=6.43 user=5.07 system=0.08 maxrss_kb=219092 exit=0` | `0ac684dd69393416ead3053682411dafea5de911845259696aaf11c9fa8e4316` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and standard GUI markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Temporal GUI smoke timing | `elapsed=3.66 user=1.57 system=0.11 maxrss_kb=204620 exit=0` | `d0497aa0bf5663638a728d745b43735e0a96a30bb426ddbd557e1fc03ed2c4d0` |
| Unchanged-binary identity record | Pre-smoke and post-smoke hashes both equal the final GUI binary hash | `d2b4d404d3d7b8014eeb4e6d16f9238e05f2e2b905bb024b375bd6ca7fe5ad1c` |
| Final source hygiene log | Changed-line whitespace, shell/Python syntax, bridge absence, hidden-state absence, GLFW ownership, registrations, line ceilings, generated-artifact cleanup, binary identity, and process cleanup passed | `5a11412cff0f2be9137deaf87533d6e6e6a7592ac944bec7f3fac5bbda9b204e` |
| Validation evidence manifest | Exact SHA-256 manifest for 42 governed-review and validation artifacts | `482764c689a56b154645f406c0c3f430b6cd80073ffbd0982483ea21e46ce455` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `ee662b6d5e35dcd936728a2c9c6a7a401c9d25b3df2b1d335f8b1ca661542df5` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `446e53383a390f747d778194e22e65a475c015e34b1217b794ab2486be24c760` |

Validation initially exposed four real acceptance gaps: duplicated diagnostic link sources in combined grammar/procedural harnesses, a curved-geometry harness that still captured the removed `errorout` seam, a P0 stub missing the injected scene-context publisher relationship, and an exact-body scope harness generated around the old free function. Each gap was corrected at its ownership boundary, its focused gate passed, the remaining architecture tail passed, and the complete architecture suite was then rerun from the beginning to the final success marker. The final P0-P2 run was also repeated after the stub and generator corrections.

The forced build introduced no new warning category. Existing signedness warnings in `include/grammar.h`, existing `Mesh.cpp` warnings, the vendored `stb_image` diagnostic, and generated-catalog variable-tracking notices remain unrelated cleanup work. Exact process-name and executable-path checks confirmed that no ProGen3D application or Xvfb process remained after validation.

### Updated concentration boundary

`src/Grammar.cpp` is now `4197` lines and remains below the enforced `4200`-line decomposition ceiling. Its runtime diagnostic calls are routed through `GrammarRuntimeDiagnosticPublicationService` or explicit document/context publishers rather than a bridge. `GrammarRuntimeDiagnosticPublicationService.cpp` is `85` lines and its purpose-specific header is `19` lines.

`src/Context.cpp` is now `542` lines, down from the `558`-line vehicle-slice baseline. Constructor, destructor, and diagnostic-accessor lifetime code moved into the 44-line `SceneGenerationContextLifecycle.cpp`; primitive and scope errors now publish through the context's explicit diagnostic association. `include/Context.h` is `174` lines and makes the composed diagnostic context visible.

The generic diagnostic abstraction is bounded: `RuntimeDiagnosticPublisher` is 19 header lines and 17 implementation lines; the terminal publication service is 12 header lines and 20 implementation lines; `RuntimeDiagnosticPublicationContext` is 19 header lines and 17 implementation lines; the application adapter is 20 header lines and 52 implementation lines.

`src/imgui_render.cpp` remains `3406` lines. This slice removed global renderer/generator access and global diagnostic routing, but intentionally retained its deeper private process-static OpenGL resource state. Older specialized test harnesses may still define isolated no-op or capture functions named `debugout` / `errorout`; they do not include the removed bridge, do not participate in production ownership, and remain test-modernization work rather than hidden application state.

This append-only record certifies only the direct legacy logging bridge removal slice. Private renderer decomposition, remaining monolithic application/rendering concentration, packaging, repository-wide generated/untracked-artifact hygiene, and complete exact-snapshot repository certification remain mandatory before a complete repository-refactor claim. The next ordered slice is private preview renderer decomposition.

## 2026-08-27 Private Preview Renderer Decomposition Slice

### Governed OURD review

The process-static OpenGL preview runtime, shader compilation, geometry-resource ownership, framebuffer ownership, procedural cubemap generation, render-pass orchestration, RGB capture seam, and public renderer compatibility boundary were reviewed through the governed local OURD coding agent before implementation.

- The pre-review repository source snapshot value was `efea3545191e2cb46fd8f9d07c8c9402bd06281d428e96cc540c5b2db728a55a`; the exact snapshot-output artifact SHA-256 is `770d91e095f7a03435645fd8395edaf3cb892514694cd35d116103e27bdcd12d`. Snapshot capture timing was `elapsed=8.88 user=6.78 system=2.07 maxrss_kb=52112 exit=0`, and the timing-record SHA-256 is `21567710d999c9b764f7cbef43ce011ad91e1ffc491f47e8b2e330af0f88dce9`.
- The bounded 328-line, 1,686-word, 15,012-byte review context SHA-256 is `ad2738db9d822a89cc33272ca862d2df2f8fead2e2579a5944bcf70b9aa5edcf`.
- The isolated read-only workspace snapshot value was `c9cd7f0d3871b4a6289e85174fb6c41ce27f7109d9aa53c868b7618089420f49`; the exact isolated snapshot-output artifact SHA-256 is `063ded35a1be0d679aefb6e3e9f0da2b5a14b172cb497688025db21b451b37f7`. Its timing was `elapsed=0.06 user=0.05 system=0.01 maxrss_kb=31088 exit=0`, and the timing-record SHA-256 is `f03200e4d430b13e9c7478a39c12431455e1ff9786de9ffbe7f678deb240face`.
- The exact-hash read-only authority manifest SHA-256 is `f818d1cdeccbdf5caf59b1babe4f4e4d981d118bceed86cd16e3face1335dcdb`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.09 user=0.05 system=0.01 maxrss_kb=34112 exit=0` timing-record SHA-256 is `0a7af983e48ff134bcab80707c1da2227456b8159f8649af43bf7a8a89bed5b5`.
- The final 75-line, 553-word, 5,335-byte OURD advisory review log SHA-256 is `9bdce2837703e68ea1ba3f3d5de4cadeabded1fe1f9fd01564bdcd1bddde7b64`; its timing was `elapsed=43.79 user=0.07 system=0.01 maxrss_kb=34420 exit=0` and timing-record SHA-256 is `635f99bad2264125b1d51df0f97589ee06c360aae2c718922c4b07b25447980b`.
- The 378-line, 2,045-word, 17,570-byte human review and correction record SHA-256 is `16777d37be34903416a7c414492ea0209538737ac8fce6e4dc4f20518b9f9842`.

The OURD advisory correctly identified hidden process lifetime, OpenGL resource ownership, shader compilation, framebuffer management, explicit pass ordering, deterministic cleanup, and capture-state restoration as mandatory decomposition boundaries. It was not accepted as the implementation design because it proposed a broad `PreviewRenderService`, redundant pass-service abstractions, filesystem-owned framebuffer readback, and names that obscured the concrete OpenGL object relationships. Human review retained the useful separation pressure and supplied the authoritative object model below.

### UML-readable preview-rendering object model

- `OpenGlPreviewShaderProgramState`, `OpenGlPreviewGeometryResourceState`, and `OpenGlPreviewRenderTargetState` are explicit OpenGL state models. `OpenGlPreviewRenderRuntime` composes one instance of each state model and exposes deterministic reset behavior. No process-global, static, atomic, or thread-local preview runtime remains.
- `OpenGlPreviewShaderSourceCatalog` is the immutable shader-source catalog. It owns the exact vertex, fragment, shadow, overlay, outline, background, and resolve GLSL source definitions without also compiling or linking them.
- `OpenGlPreviewShaderProgramService` associates with `OpenGlPreviewRenderRuntime`, compiles and links the catalog sources, publishes the existing phase-specific diagnostics, and releases all eight shader programs deterministically.
- `OpenGlPreviewGeometryResourceService` associates with the runtime and owns base, grid, overlay, background, opaque, transparent, dynamic, outline, and light-buffer OpenGL resource creation, upload, and release.
- `OpenGlPreviewRenderTargetService` associates with the runtime and owns shadow-map and multisampled preview target construction, resizing, release, resolved-texture access, and RGB framebuffer readback. Readback preserves pack alignment, read-buffer state, framebuffer binding, and the existing bottom-to-top row inversion.
- `OpenGlProceduralPreviewCubemapGenerator` is the concrete is-a implementation of `PreviewCubemapGenerator`. It owns the existing standard and enhanced procedural cubemap algorithms and their diagnostic phases without sharing renderer lifetime state.
- `OpenGlPreviewShadowMapPass`, `OpenGlPreviewScenePass`, and `OpenGlPreviewResolvePass` are concrete render-pass services. They accept explicit runtime state and perform the established shadow, scene, and multisample-resolve stages in order.
- `OpenGlPreviewFrameRenderingService` composes the three pass services and associates with the runtime and render-target service. It builds camera and light matrices, ensures render targets, uploads light data, invokes each pass, and retains the existing final `render_scene_to_preview` diagnostic boundary.
- `OpenGlPreviewRenderer` is the sole concrete is-a implementation of `PreviewRenderer`. It composes one runtime plus shader, geometry, target, and frame-rendering services, preserving explicit initialization, upload, frame-render, readback, and shutdown relationships.
- `PreviewFramebufferCaptureService` associates with the abstract `PreviewRenderer` rather than a free OpenGL function. `PreviewRenderer::readPreviewRgbPixels` is now the public capture contract, so capture follows the selected renderer instance and cannot address hidden global state.

### Implemented decomposition and preserved behavior

- `src/imgui_render.cpp` is now a 77-line compatibility translation unit containing only `build_base_vertex_data`, the retained no-op `draw_box` compatibility symbol, and the explicit compatibility boundary. The former 3,406-line renderer monolith no longer owns OpenGL lifetime or pass behavior.
- The concrete renderer composition is visible in `OpenGlPreviewRenderer.h`: runtime, shader-program service, geometry-resource service, render-target service, and frame-rendering service are purpose-specific composed objects rather than mutable file-static data.
- Initialization still creates shader programs, base geometry, grid geometry, overlay geometry, background geometry, and target resources in the established order. Shutdown releases targets, geometry, and programs and then resets runtime state.
- The shader catalog retains the exact GLSL bodies. Compilation and link failures retain their existing diagnostic prefixes and publication source.
- Geometry uploads retain the established opaque, transparent, dynamic-cube, overlay, outline, and light-buffer layouts and their phase-specific OpenGL diagnostics.
- Shadow, scene, and resolve behavior retains material uniforms, texture binding, blend transitions, overlay and outline drawing, background rendering, multisample resolve, camera field-of-view behavior, and temporal-frame behavior.
- Procedural cubemap generation retains both standard and enhanced algorithms, texture parameters, face order, mipmap generation, and existing failure diagnostics.
- The capture workflow retains RGB byte layout, framebuffer-state restoration, and vertical row inversion while removing the public free `read_preview_rgb_pixels` seam.
- All new production sources are registered in `Makefile`; the focused architecture gate is registered in the complete editor-architecture suite and documented in `tests/README.md`.

### Validation evidence

| Validation artifact | Result | SHA-256 |
| --- | --- | --- |
| Exact renderer source-path list | 35 purpose-specific source, contract, composition, and acceptance files | `2772da36d12c03bc6d91affce8c4c976cabbd79f8e7c47f6e5ce8cbf66cc2951` |
| Exact renderer source manifest | SHA-256 values for all 35 listed source files | `c81d0515bca1ca9214f3f3ec9849a234aa732abd6607fa19eb239b144a186b6b` |
| Focused OpenGL renderer architecture log | Runtime-state harness and source-ownership gates passed | `eb6d413c87e1f95c175c17baf6a625170c704b12e67042a170bd7b7cab576b21` |
| Focused architecture timing | `elapsed=0.41 user=0.33 system=0.12 maxrss_kb=185612 exit=0` | `787e989870728c4a320d7d638d8d394a8b96e85673b0bb7c9375a2462a8a8ec2` |
| Complete editor architecture log | All 58 ordered editor architecture and workflow gates passed from the beginning to the final marker | `ed9c6f26dc43b724d75d0f60a83262a35ce573e0f7860769ec49c3df337b3f3f` |
| Complete editor architecture timing | `elapsed=913.19 user=849.28 system=60.66 maxrss_kb=1001964 exit=0` | `549cda87c846e2b7ac0ddc1637731c43baa041da5a1ebd620e610aebf617dede` |
| Complete P0-P2 log | P0, P1, and P2 passed sequentially through the final P2 grammar marker | `8a09c801345649cdbf55c2348d7ed4758882c7b5603707d27d6850a956aad455` |
| Complete P0-P2 timing | `elapsed=243.84 user=225.21 system=17.57 maxrss_kb=519328 exit=0` | `e9eace789eff0237c9a1082aa9ab42e32b854cbb8d34cd8f32c38380c1ad1049` |
| Forced native GUI build log | `make -B -j2 progen3d-editor-gui` completed successfully | `755267e019dc3b00d1119ed963b0d632a66863ab14fecc9d09ed238911e448a5` |
| Forced native GUI build timing | `elapsed=375.76 user=606.26 system=22.46 maxrss_kb=2608396 exit=0` | `cf044ae934aa8769386b98760da1aec697d3cf2b6dc4836ff2d4d048b6cd2935` |
| Final GUI binary | 280,439,888-byte executable used unchanged by both successful smoke gates | `5a4b6f4357e33f739fa03c9cb903c4a4d4977d83de23b68aa64f61b8a2e1a9a8` |
| Standard GUI smoke log | Passed FOV, render-frame, and standard GUI markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Standard GUI smoke timing | `elapsed=6.46 user=5.03 system=0.11 maxrss_kb=218284 exit=0` | `00bda4b0b674096788e6fc6afbfd70e05e412615d55786015f0f6f43768aa0e1` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and standard GUI markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Temporal GUI smoke timing | `elapsed=3.67 user=1.55 system=0.12 maxrss_kb=203952 exit=0` | `65d6cc28d4090d61bb75691dd1e23686b66933a2996df83d4d1c96dcbb04506d` |
| Unchanged-binary identity record | Pre-smoke, post-standard, and post-temporal size and SHA-256 values are identical | `526ee1d2cddbc75bbce319589db90f0f893c4aa2ba52872374477267aba4521e` |
| Final source hygiene log | Focused architecture, tracked and untracked whitespace, shell syntax, legacy-name absence, hidden-runtime absence, diagnostic-call absence, concentration ceilings, binary identity, and process cleanup passed | `418c9eff35c9bd120ae8cacc50c56692c9e0e2101a6e2b8d9be021e1ce314ec0` |
| Final source hygiene timing | `elapsed=0.62 user=0.47 system=0.21 maxrss_kb=185728 exit=0` | `f6d2c029cb420930ea0e43f954389fe963d4a34219e7ce12cc47835fa88eee4a` |
| Validation evidence manifest | Exact SHA-256 manifest for 31 governed-review, source, build, smoke, hygiene, binary, and snapshot artifacts | `6600f419ae9546a2d304e0af7337d0d13b82f08d2115cc1f55bc386049845735` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `40eb5025eb4dc2f8ee49b94050b7b00c3db2af757d1f4b6933945aa602da9f84` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `5e269108c22d7193b4af2346e0b7f6ca386d4f2619006fa2b136b99993f7c340` |
| Post-implementation snapshot timing | `elapsed=11.66 user=7.01 system=2.75 maxrss_kb=51364 exit=0` | `89b0b14b065386b1e0af5dcccaad77196959a43804f3cbba993b6cec825e9196` |

The complete architecture run independently re-exercised the preview session, preview interaction, scene-preview rendering workflow, application logging context, renderer architecture, resource, lighting, material, grammar, geometry, persistence, and export acceptance layers. The forced build retained the repository's existing signedness, `Mesh.cpp`, vendored `stb_image`, and generated-catalog diagnostic categories; no renderer-specific build failure or new hidden-lifetime warning boundary was introduced. Exact executable identity and process-name checks confirmed that neither smoke altered the binary and that no ProGen3D or Xvfb process remained after validation.

### Updated concentration boundary

The previous 3,406-line `src/imgui_render.cpp` concentration is now a 77-line compatibility translation unit. Its former responsibilities are bounded across `OpenGlPreviewShaderSourceCatalog.cpp` at 866 lines, `OpenGlPreviewRenderPasses.cpp` at 792 lines, `OpenGlProceduralPreviewCubemapGenerator.cpp` at 506 lines, `OpenGlPreviewGeometryResourceService.cpp` at 350 lines, `OpenGlPreviewShaderProgramService.cpp` at 247 lines, `OpenGlPreviewRenderTargetService.cpp` at 240 lines, `OpenGlPreviewFrameRenderingService.cpp` at 169 lines, and `OpenGlPreviewRenderer.cpp` at 139 lines. The focused gate enforces ceilings of 900, 850, 550, 400, 300, 300, 220, and 180 lines respectively, plus a 100-line compatibility ceiling.

The public renderer contract now exposes instance-owned readback, the application capture service holds an explicit renderer association, and the concrete renderer's composed resource and pass relationships are visible to a human reviewer and directly translatable into a UML class diagram. No mutable renderer runtime or render service remains process-static.

This append-only record certifies only the private preview renderer decomposition slice. Remaining monolithic editor-application concentration, packaging, repository-wide generated/untracked-artifact hygiene, and complete exact-snapshot repository certification remain mandatory before a complete repository-refactor claim. The next ordered slice is decomposition of the remaining editor application and immediate-mode interface concentration.

## 2026-08-27 Editor Application and Immediate-Mode Interface Decomposition Slice

This append-only section records the governed decomposition of the remaining
6,924-line `src/imgui_main.cpp` application and immediate-mode interface
concentration. The former translation unit is absent from the filesystem and
from `Makefile`; application composition, grammar generation, smart-editor,
preview, panel, inspection, style, text-input, and window-resource
responsibilities now have purpose-specific class owners.

### Governed OURD review

- The pre-review repository source snapshot value was `2ba735f4741a061ba3895479ca14034a1984a03faf2b8e5fe67c7eb0648670e4`; the exact snapshot-output artifact SHA-256 is `8f45c54c713c6d1d350675068164ef6fad913923facc2927abe011217ee37056`. Snapshot timing was `elapsed=8.87 user=6.84 system=2.00 maxrss_kb=52080 exit=0`, and the timing-record SHA-256 is `7ebfc6fd5a83a8e1dfb24847b5b1c186b01866c03f569bf2d64739b4944b8ba2`.
- The bounded 384-line, 1,530-word, 16,325-byte review context SHA-256 is `09f5e7c32b11005141d2c54f5a80cd6b62907a03bfea40bb69a7b2af023bf771`.
- The isolated read-only workspace snapshot value was `4be3600d4eb94090923580961f694447c6bcaa3c58bfc119243f405dfe9b4a0d`; the exact isolated snapshot-output artifact SHA-256 is `a6ab93e47351332190efaaed0b2df133a19dde12996b759c58a0baf17d42fbe2`. Its timing was `elapsed=0.06 user=0.05 system=0.01 maxrss_kb=31272 exit=0`, and the timing-record SHA-256 is `7930bd515a286cfafdcb769d80513bf60acc494b04f898d64f1bdd0982ef8ab9`.
- The exact-hash, read-only, no-command `C1` authority manifest SHA-256 is `790a267667b0a421836eab47298a673a9c399a19e373c946951ee1cdd6a77ec4`.
- OURD used local `qwen2.5:14b` digest `7cdf5a0187d5c58cc5d369b255592f7841d1c4696d45a8c8a9489440385b22f6`, a 9,000-token context budget, a 2,200-token output ceiling, zero transport retries, no command capability, and read-only `C1` evidence access.
- The exact OURD preflight log SHA-256 is `8a1e969bfd280b61912a3559884863d7b86df1d3d4f5bfdd0a164e9d4d2eef26`; its `elapsed=0.09 user=0.06 system=0.01 maxrss_kb=34000 exit=0` timing-record SHA-256 is `07fb60938cf337a3c30b9f8cd2f88d1b1b7138ea367ef2ac1f1f853c57d6ff50`.
- The first 119-line advisory attempt stopped after a generic partial draft. It remains preserved with SHA-256 `2240d8c2fb4b7c377ccb1607a5bf27d920a22bbba122b7a213d7888873da6f63`; its `elapsed=56.96 user=0.07 system=0.01 maxrss_kb=33616 exit=0` timing-record SHA-256 is `60a7e8dc7c83a431596ef0bc6c8d7854d40cd63b810e028b94aa0f74aeda68cf`.
- The final 158-line, 829-word advisory SHA-256 is `efea2e4efbebb25c05b25bfa0479ce3ca9b62e7a08340be63546b8bd067e2814`; its timing was `elapsed=53.10 user=0.08 system=0.00 maxrss_kb=34176 exit=0` and timing-record SHA-256 is `be6025c4c8845a34168abcd5c50129e2d55914a175036a81a9f1a5b028fd2b0f`.
- The human-authoritative review and correction record SHA-256 is `e51e8d676c400f2b8162b511fdc8d7f2b4d73b77709ac5d9432b9cd998902041`.

The advisory correctly identified the source-region boundaries and the need to
finish existing class implementations rather than rename them. Human review
rejected duplicate generic `Model`, `Service`, `View`, `Context`, and lifecycle
abstractions; retained the existing workflow, runtime-context, session,
publication, and presentation authorities; required exact visible-behavior
compatibility; and kept the application-wide grammar and effects engines as an
explicit compatibility boundary rather than disguising them as panel state.
OURD remained advisory and read-only. Checked source, deterministic tests,
native compilation, smoke evidence, exact hashes, and this human review remain
authoritative.

### UML-readable application and generation object model

- `Progen3dEditorApplication` remains the composition root. Its constructor,
  destructor, and `run()` now reside in
  `src/editor/application/Progen3dEditorApplication.cpp`; the application
  retains explicit lifetime-ordered composition of workflows, runtime state,
  publication services, presentation composition, resources, frame cycle,
  smoke workflow, interactive loop, startup, and shutdown.
- `Progen3dEditorApplicationShutdownOperations` is-a
  `EditorApplicationShutdownOperations` and owns the concrete published-grammar
  destruction and Firebase shutdown adapter behavior that previously lived in
  a local class.
- `Progen3dSceneGenerationPublicationService` is-a
  `EditorSceneGenerationPublicationService`. It composes
  `CompletedSceneGenerationPublicationService`, associates with preview
  publication preparation, randomization, and structural-validation services,
  and owns generation-runtime initialization, monotonic design nonce state,
  normal and temporal requests, completion publication, pending restart, and
  focus restoration.
- `GrammarGenerationRandomizationService` owns the application session seed,
  source hashing, seed mixing, stream separation, and per-design grammar
  reseeding. The externally linked `grammar_rng` and `effects_rng` remain in its
  purpose-specific source as an explicit core-runtime compatibility boundary.
- `GrammarStructuralValidationService` owns structural tokenization,
  rule-block reconstruction, duplicate and shape checks, exact line and token
  diagnostics, optional workspace publication, and quiet validation behavior.
- `GlfwApplicationWindow` owns native window-icon loading and assignment;
  presentation classes do not own GLFW lifetime.

The only inheritance relationships are the established conceptual service and
shutdown contracts. The application composes concrete owners; generation
publication associates with the services it requires; panel and presentation
objects receive runtime relationships explicitly.

### UML-readable interface and smart-editor object model

- `EditorInterfaceStyle` owns the professional ImGui style definition.
  `EditorInterfaceComponentPresentation` owns reusable surface, divider,
  heading, chip, button-style, splitter, and color presentation.
  `EditorTextInputPresentation` and `GrammarEditorTextInputPresentation` own
  their respective text-input adapters rather than leaving callback procedures
  in an application monolith.
- `EditorDocumentIdentityPresentation` owns document-title and identity facts
  used by `ApplicationHeaderPanel`; the header panel owns header layout,
  backend status, authenticated identity, credits, and artwork presentation.
- `GrammarEditorDocumentAnalysisService` owns line splitting, document
  position conversion, source ranges, delimiter analysis, and identifier
  occurrence facts. `GrammarEditorDocumentEditingService` owns text commit and
  interaction-state synchronization. `GrammarEditorSelectionAnalysisService`
  owns selected identifier, occurrence, and bracket-match models.
- `GrammarEditorAutocompleteService` owns canonical material, part-class,
  structured-shape, and spatial-declaration completion models and source
  replacement. `GrammarEditorAutocompletePopupPresentation` owns the popup
  drawing and delegates every accepted suggestion back to that service.
- `Progen3dGrammarSyntaxPresentationFactory` is the single explicit owner that
  constructs `GrammarSyntaxPresentation` with the ProGen3D identifier and
  tooltip vocabulary. No hidden function-local syntax singleton remains.
- `GrammarEditorSurfaceGeometryService` owns ImGui editor-surface resolution,
  column-to-screen mapping, source range rectangles, exact token rectangles,
  and clipping geometry. Its public header exposes purpose-specific layout
  facts without leaking ImGui headers across the composition boundary.
- `GrammarEditorOverlayPresentation` has explicit non-owning associations with
  surface geometry, syntax, symbol-detail, and autocomplete-popup
  presentation. It owns decoration, diagnostics, selection and bracket
  highlights, caret following, hover tooltips, and overlay coordination, but no
  canonical document or generation state.
- `GrammarSymbolDetailPresentation` owns selected rule and synchronized runtime
  variable details. Runtime snapshot access therefore remains in the symbol
  detail presentation rather than the document coordinator.
- `GrammarEditorDocumentView::draw` is now the class-owned document
  coordination method; the last free `draw_smart_editor_document` wrapper was
  removed. It coordinates document services, text input, overlay delegation,
  status facts, and symbol inspection without duplicating overlay geometry or
  autocomplete procedures.
- `GrammarEditorPanel` owns editor tabs, toolbar actions, diagnostics routing,
  material, lighting, electrical, AI, render-settings, and inspection routing
  through `EditorPresentationComposition`. It does not directly construct or
  bypass semantic workflow services.

`EditorPresentationComposition` visibly composes the grammar document view,
text-input presentation, syntax presentation, surface geometry, autocomplete
popup, overlay presentation, analysis, editing, autocomplete, selection,
symbol detail, every panel, controllers, and inspection services. These are
composition relationships with application lifetime; runtime model and
workflow references remain non-owning associations.

### Panel, preview, and inspection ownership

- Existing `AuthenticationPanel`, `AiGrammarProposalPanel`,
  `AiAssistantPanel`, `MaterialLibraryPanel`, `RenderSettingsPanel`,
  `ScenePreviewPanel`, `PreviewTimelinePanel`, and `EditorWorkspaceWindow`
  implementations now reside in matching source files and delegate semantic
  changes through established workflow and publication contracts.
- `TextureLibraryPanel` is composed by `MaterialLibraryPanel` and owns texture
  search, slots, preview, upload, generation prompt, and action presentation.
  Repository, credit, GPU, and scene authority remain in
  `TextureLibraryWorkflow` and resource services.
- `PreviewOrientationControl` owns view-cube projection, hit testing, drawing,
  and orientation requests without owning camera state.
  `PreviewTimelineController` owns grammar-time progression and fixed-step
  physics simulation timing. `PreviewTimelinePanel` owns timeline, camera,
  lens, overlay, mapping, and debug controls and delegates runtime operations.
- `ScenePreviewPanel` owns preview texture presentation and pointer/keyboard
  input assembly. Its last render-settings hash is explicit panel instance
  state, not function-static storage.
- `RenderSettingsPanel` owns render and environment controls. Procedural
  cubemap result state and message are explicit panel instance state, not
  function-static storage.
- `ScenePrimitiveInspectionService` owns non-ImGui primitive inspection
  construction. `SpatialObjectInspectionService` retains spatial facts and now
  owns bounded selection-entry and current-selection resolution.
- `EditorWorkspaceWindow` owns the root window, columns, splitters, preview and
  console layout. `GrammarEditorPanel` owns the editor-side tab set, keeping
  workspace layout distinct from editor feature routing.

### Preserved compatibility semantics

Startup, smoke, interactive-loop, and shutdown order remain unchanged. Visible
ImGui labels, IDs, shortcuts, popup and modal behavior, layout ratios, style
values, grammar diagnostics, RNG mixing, stream tags, design-nonce behavior,
editor colors and tooltips, autocomplete order and insertion, focus
restoration, zoom, auto-run timing, document persistence, preview camera and
FOV, view cube, pointer and keyboard interaction, overlays, timeline,
simulation, authentication, AI, texture, material, lighting, electrical,
inspection, export, and exact smoke markers remain behind their established
authorities.

The legacy `upload_fulltext`, `set_window_icon`, `validate_grammar`,
`draw_smart_editor`, `draw_editor_workspace_content`, and thin free panel
adapters are absent from production source. No mutable runtime context, panel,
or generation service was introduced at process scope.

### Acceptance and evidence

Existing source-ownership gates were corrected to follow canonical owners
rather than weakened: runtime snapshot checks now inspect
`GrammarSymbolDetailPresentation.cpp`; temporal tooltip checks inspect
`Progen3dGrammarSyntaxPresentationFactory.cpp`; exact RNG body extraction uses
`GrammarGenerationRandomizationService.cpp`; lighting panel routing is checked
in `GrammarEditorPanel.cpp`; runtime-diagnostic initialization is checked in
`Progen3dSceneGenerationPublicationService.cpp`; and preview, document,
authentication, texture, diagnostics, spatial, vegetation, startup, lifecycle,
and publication gates retain negative monolith-regression checks.

The new `tests/run_editor_interface_decomposition_checks.sh` gate verifies the
complete owner map, application and composition relationships, matching panel
implementations, Makefile ownership, former-monolith absence, retired free
functions, function-static panel-state absence, current canonical test owners,
and bounded source concentration.

| Evidence | Result | SHA-256 |
| --- | --- | --- |
| Exact source path list | 115 implementation and acceptance files | `aa0848ceb37bdd528975be0e6d202e8598784637333b4dbbc2f87495cabccfa8` |
| Exact source manifest | SHA-256 entries for all 115 files | `1f9b4976366976a0d387322c653cdfdfd20ae8c87a2096c05795477e3af6cd20` |
| Focused interface-decomposition gate | Passed complete source ownership and concentration contract | `eeee9f99824ec0888441539a2abb77615eb4bbc5cbd3baa7decbfd5cb697eac6` |
| Focused gate timing | `elapsed=0.06 user=0.03 system=0.06 maxrss_kb=75940 exit=0` | `fcb43d0e3da24f7da09b7d387084ad75637b8c0f46296bbecdda597ffc0b32a1` |
| Complete editor architecture log | Passed every application, document, grammar, preview, renderer, geometry, lighting, vehicle, catalog, and export architecture check | `d7d40d758e0baf0d87539f844371c93c7a67de778df9afe069330667c96839e3` |
| Complete editor architecture timing | `elapsed=933.36 user=867.53 system=62.66 maxrss_kb=999756 exit=0` | `2ea5c915b106b3542c64573e6e192ecfbd0d5821e85446beed7ce13f525bd12d` |
| Complete P0-P2 log | Passed P0, P1, and P2 through the final temporal grammar marker | `bb765fbb641624d1183e979bce0108497210db11ac260d8a31754ac345fcd429` |
| Complete P0-P2 timing | `elapsed=104.86 user=96.27 system=8.11 maxrss_kb=519140 exit=0` | `5c9dea73052b5ccc7257c68542476696f16b6ddadfd3a36e970c59fe068361eb` |
| Forced native GUI build log | `make -B -j2 progen3d-editor-gui` completed successfully | `9f9ac3b09f0784f76da89a5fef96484b96dcd06bbbd743fc4218e744122f0b4f` |
| Forced native GUI build timing | `elapsed=387.07 user=628.87 system=30.56 maxrss_kb=2560860 exit=0` | `6302a976f02eb7846a96cf926d131d9eb5bd370a14fb758a625dd9e1e20b72ec` |
| Final GUI binary | 288,519,576-byte executable used unchanged by both smoke gates | `d0554bc5ea3645e5f4d360d2c3a44315dff98842f0457ff4965a4e6f3c5ca719` |
| Standard GUI smoke log | Passed FOV, render-frame, and standard GUI markers | `5b6a7f7a36237fa0d9b04607e3cc3b154cd8e96a495f5fc79389894ea1f956ff` |
| Standard GUI smoke timing | `elapsed=6.47 user=5.02 system=0.13 maxrss_kb=217572 exit=0` | `2b0b5a483c32b91868fd3ff0b0c4acbcd195cfca3161e6d864cfddb71364a5af` |
| Temporal GUI smoke log | Passed temporal, FOV, render-frame, and standard GUI markers | `eec05e77b8cc3e67f52ce881cb29e7199e4893cee2b9f812f2ebeb91f85ba144` |
| Temporal GUI smoke timing | `elapsed=3.68 user=1.59 system=0.08 maxrss_kb=204292 exit=0` | `7e1560ca2e1736d082a41d9eb0f3d55c4fd7f76ed45b84c401f6a22ebc7e9b71` |
| Unchanged-binary identity record | Pre-smoke, post-standard, and post-temporal size and SHA-256 values are identical | `c3dda0e0210a91581c4f2d9ffbb1b629aea000740ff1b4e0fe6c51c780dfba06` |
| Post-smoke process cleanup | No ProGen3D or Xvfb process remained | `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855` |
| Final bounded hygiene log | Source manifest, trailing whitespace, tracked diff whitespace, shell syntax, Python syntax, focused ownership, validation markers, binary identity, process cleanup, former-monolith absence, and up-to-date build passed | `f5daa37a1a0a747741f5987bc58354061e6f17bf67016c3932fec95ab1ffd996` |
| Final bounded hygiene timing | `elapsed=0.27 user=0.14 system=0.11 maxrss_kb=82132 exit=0` | `e8f9cdd4987bea5ef044101b9f3ba8a691160c1b4fa1be323ddf683661a9b316` |
| Validation evidence manifest | Exact SHA-256 manifest for 34 governance, source, validation, build, smoke, binary, hygiene, and snapshot artifacts | `7e5bc36f308ba0304e819afc77744b0cdbe8c3cb4148f21b00ceeb71f9d53c4a` |
| Validation manifest check | Every listed artifact verified `OK` | `d9c87663f3c9bcc755ce5c914f62f8d883a7be627f04a37838656e1e69f890b0` |
| Post-implementation OURD snapshot value | Source snapshot before this append-only ledger entry | `c7e612733c5749a560c76c59cc6502742b8a57b164b9f7db7c9a5317e2319a5c` |
| Post-implementation snapshot capture | Exact snapshot-output artifact | `ba5620ed600fd0bb1d121f20e0512c7336fde10b7c7685760d1eb723c7bc2a63` |
| Post-implementation snapshot timing | `elapsed=11.79 user=6.96 system=2.87 maxrss_kb=51004 exit=0` | `1a92c69247d68ec69c10cf6cc0dcde69f6ef946bfc1e13c073949e1e028a4347` |

### Updated concentration boundary

The former 6,924-line `src/imgui_main.cpp` is removed. The largest immediate
interface owner in this slice is now the 596-line
`GrammarEditorOverlayPresentation.cpp`; `PreviewTimelinePanel.cpp` is 447
lines, `GrammarEditorPanel.cpp` is 400, the autocomplete popup is 305,
`RenderSettingsPanel.cpp` is 291, `Progen3dEditorApplication.cpp` is 255,
`GrammarEditorDocumentView.cpp` is 232, the surface geometry service is 211,
and `ScenePreviewPanel.cpp` is 203. The focused gate enforces ceilings of 700,
500, 450, 350, and 350 lines for the highest-risk overlay, timeline, editor,
document, and application boundaries.

This slice closes the governed editor-application and immediate-mode interface
concentration stage while preserving the established behavior and authority
boundaries. It certifies the 115-file source set and the validation artifacts
listed above. It does not certify unrelated pre-existing dirty or untracked
repository artifacts, packaging outputs, or a clean distributable checkout;
those remain separate repository-hygiene and release-certification concerns.
