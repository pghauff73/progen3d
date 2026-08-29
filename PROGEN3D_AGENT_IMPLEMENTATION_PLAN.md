# ProGen3D Design Agent Implementation Plan

## Document Control

- **Project:** ProGen3D
- **Target executable:** `progen3d-editor-gui`
- **Plan date:** August 27, 2026
- **Status:** Proposed implementation sequence; implementation not started by this document
- **Primary technology:** C++17, Dear ImGui, GLFW, OpenGL, existing backend AI API
- **Primary release goal:** Validated grammar-repair and grammar-authoring agent
- **Authority model:** Model output is advisory; deterministic validation and exact-candidate human approval are authoritative

## 1. Executive Decision

Implement a `Progen3dDesignAgent` inside the editor as a proposal-first application service.

The agent will:

1. capture an immutable snapshot of the current grammar and editor context;
2. send a structured request through a replaceable model-provider boundary;
3. receive an explanation or candidate grammar proposal;
4. compile and validate proposed grammar in an isolated candidate runtime;
5. collect deterministic evidence about the candidate scene;
6. present source differences, diagnostics, evidence, and preview controls;
7. require human approval bound to the exact candidate source fingerprint;
8. reject stale or changed candidates;
9. publish only through an explicit publication authority; and
10. retain a rollback transaction for every accepted proposal.

The provider will never receive direct document, filesystem, shell, OpenGL, or publication authority.

The first release will support these agent modes:

- `ExplainSelection`
- `DiagnoseGrammar`
- `RepairGrammar`
- `DraftGrammar`

Variant exploration, visual-reference reasoning, and multi-document planning are later releases and must use the same validation and approval path.

## 2. Current Repository Baseline

### 2.1 Existing Components to Preserve

The repository already contains the major editor boundaries required for an agent implementation:

- `AiAssistantPanel` presents requests, conversation threads, responses, and modes.
- `AiGrammarProposalPanel` presents line-based proposal differences and explicit acceptance or rejection controls.
- `AiConversationWorkflowService` assembles the current source, selection, diagnostics, thread, and request fields before calling an AI proposal service.
- `AiGrammarProposalService` and `BackendAiGrammarProposalService` isolate the current backend transport.
- `AiGrammarProposalPublicationService` prevents direct presentation-layer document mutation and rejects proposals based on changed source text.
- `EditorDocumentPublicationService` is the existing document-edit publication path.
- `GrammarStructuralValidationService` is the existing structural validation service.
- `GrammarCompilationService`, `SceneRegenerationCoordinator`, and `EditorSceneGenerationRuntime` provide asynchronous scene generation machinery.
- `CompletedSceneGenerationPublicationService` preserves the last valid scene when generation or preview preparation fails.
- `ScenePreviewPublicationPreparationService` and `OpenGlScenePreviewResourceService` prepare candidate scene resources before canonical scene publication.
- `ScenePreviewSession` owns the last valid published `GeneratedSceneSnapshot`.
- `EditorApplicationRuntimeContext` exposes presentation and workflow associations.
- `Progen3dEditorApplication` is the composition root.

### 2.2 Existing Behavior That Is Not Yet a Governed Agent

The current AI workflow is a useful proposal UI, but it does not yet establish a complete design-agent authority model:

- The backend returns a full grammar string rather than a domain-level candidate with a client-owned identity.
- Proposal identity is based on source-text equality rather than an explicit cryptographic source fingerprint.
- A received grammar proposal is staged for review before deterministic candidate compilation has passed.
- Accepted selected changes are composed immediately and are not revalidated as a new exact candidate before publication.
- Acceptance replaces editor source and schedules a new generation rather than publishing the exact scene candidate that was reviewed.
- The reviewed evidence is therefore not necessarily bound to the scene produced after acceptance.
- Candidate diagnostics, generation statistics, source associations, and unresolved constraints are not represented by a dedicated evidence model.
- Candidate preview state is not separated from the published `ScenePreviewSession`.
- There is no durable approval object, append-only transaction record, or user-visible rollback operation.
- Provider DTOs from `BackendApiClient.h` are stored directly in editor domain state.
- Provider requests are synchronous from the workflow call site and can block the interactive interface.

### 2.3 Working-Tree Constraint

The repository currently contains extensive unrelated modified, deleted, and generated files.

Agent implementation work must:

- preserve unrelated working-tree state;
- avoid reset, checkout, or cleanup operations that discard current work;
- record the pre-change status of every planned implementation slice;
- modify only files listed for that slice plus discovered build/test registration files; and
- distinguish implementation completion from release certification.

## 3. Scope

### 3.1 MVP Scope

The MVP will provide a validated single-document grammar agent that can:

- explain the active selection or complete grammar;
- diagnose parser and structural problems;
- propose a complete grammar repair or authored grammar;
- validate the exact proposed source with existing compiler and structural rules;
- generate one deterministic candidate scene using an explicit candidate design nonce;
- show validation diagnostics and measured candidate statistics;
- show line-based source differences;
- allow complete or selected difference groups;
- invalidate evidence whenever selected differences change;
- recompile the exact selected candidate;
- preview the validated candidate without replacing the published scene owner;
- approve the exact validated candidate fingerprint;
- publish the exact validated source and candidate scene through one authority;
- reject stale proposals after source drift;
- retain the previous source for explicit rollback; and
- operate with a scripted provider in automated tests without network access.

### 3.2 Deferred Scope

The MVP will not:

- modify C++ source files;
- execute shell commands;
- edit multiple grammar documents in one transaction;
- publish cloud documents automatically;
- upload assets automatically;
- infer missing reference-image measurements as facts;
- silently repair invalid proposals;
- approve its own proposal;
- rank candidates using model opinion as authoritative evidence;
- replace the existing grammar language or generation engine; or
- provide unrestricted autonomous operation.

## 4. Architectural Invariants

The implementation must preserve these invariants.

### 4.1 Authority Invariants

- Model output is a proposal, never an instruction to mutate state.
- Only `ApprovedAgentCandidatePublicationService` may publish an agent-authored source into the editor document.
- Presentation classes may request actions but may not mutate proposal, approval, document, or published-scene state directly.
- An approval is valid only for one exact candidate source fingerprint.
- Approval is rejected when the current document fingerprint differs from the candidate base-document fingerprint.
- Changing selected difference groups invalidates prior validation, evidence, preview activation, and approval.
- A candidate that has not passed deterministic generation is not approvable.
- An explanation-only or diagnosis-only response cannot acquire publication authority.
- Provider text embedded in grammar comments, imported metadata, conversation history, or external references is treated as untrusted data.

### 4.2 Generation Invariants

- Candidate compilation runs in a runtime separate from normal editor regeneration.
- Candidate and normal editor compilation are serialized through an explicit `GrammarGenerationExecutionGate` while the grammar engine still uses process-global `grammar_rng` and `effects_rng` instances.
- Candidate generation never replaces `ScenePreviewSession::lastValidSceneSnapshot()`.
- Failed candidate generation leaves editor source and the published scene unchanged.
- Candidate evidence records the exact source fingerprint, design nonce, evaluation time, and generation request identity.
- The scene published after approval must be the exact validated candidate snapshot or a deterministically verified equivalent with the same source fingerprint and design nonce.
- Candidate preview activation is reversible and restores published OpenGL resources when closed, rejected, invalidated, or failed.

### 4.3 Compatibility Invariants

- Existing backend AI endpoints and thread history remain usable during the migration.
- Existing `active_helper_chat`, `draft_grammar`, and `repair_grammar` backend mode strings remain accepted by the backend adapter.
- Existing non-agent editing, generation, cloud, material, lighting, export, and temporal behavior remains unchanged.
- Existing AI source gates remain green until their replacements are installed in the same implementation slice.
- The complete GUI build remains authoritative; focused harnesses alone do not establish completion.

## 5. Target Object Model

### 5.1 Model Classes

#### `AgentOperationMode`

Purpose-revealing enum for supported behavior:

- `ExplainSelection`
- `DiagnoseGrammar`
- `RepairGrammar`
- `DraftGrammar`
- later: `ExploreVariants`
- later: `AuditDesign`

Backend mode strings are translated only inside the provider adapter.

#### `GrammarSourceFingerprint`

Immutable value object containing:

- algorithm identifier;
- lowercase hexadecimal digest; and
- exact source byte count.

Equality compares algorithm and digest. The canonical algorithm for approval and transaction identity is SHA-256 over the exact UTF-8 source bytes without newline normalization.

#### `AgentDesignRequest`

Immutable request model containing:

- local request ID;
- operation mode;
- user instruction;
- complete source snapshot;
- base-document fingerprint;
- selected source and selected source range;
- complete deterministic diagnostic collection;
- recent conversation messages;
- document title and storage identity as documentary metadata;
- requested candidate design nonce;
- request creation timestamp; and
- provider conversation identity when available.

The request does not contain references to mutable editor objects.

#### `AgentProviderResponse`

Provider-neutral response model containing:

- provider request ID;
- provider name and model name;
- conversation identity;
- assistant response text;
- proposed grammar source when present;
- summary;
- proposed changes;
- warnings;
- unresolved constraints;
- provider usage evidence; and
- raw response only when diagnostic retention is explicitly enabled.

#### `AgentProposalCandidate`

Canonical proposal model containing:

- candidate ID;
- source used as the proposal base;
- base-document fingerprint;
- proposed source;
- candidate source fingerprint;
- operation mode;
- provider provenance;
- candidate design nonce;
- model rationale and warnings;
- unresolved constraints.

The class owns no editor or provider service references.

#### `AgentProposalDifference`

Relationship model connecting an original source range to proposed replacement lines.

It contains:

- stable difference ID;
- original line range;
- original lines;
- proposed lines; and
- selection state.

#### `AgentProposalReviewSession`

Review model containing:

- the active `AgentProposalCandidate`;
- ordered `AgentProposalDifference` relationships;
- selected-source composition state;
- selected candidate fingerprint;
- optional exact `AgentCandidateValidationReport`;
- optional owned `AgentCandidateScene`;
- current review decision;
- validation invalidation reason; and
- optional exact approval.

Every difference-selection change must:

1. rebuild the selected source;
2. compute a new selected candidate fingerprint;
3. clear previous validation and evidence;
4. clear previous approval; and
5. deactivate candidate preview.

#### `AgentCandidateValidationReport`

Deterministic validation model containing:

- candidate fingerprint;
- base-document fingerprint;
- structural validation outcome;
- compiler/generation outcome;
- complete diagnostics;
- generation duration;
- rule count;
- token count;
- primitive count;
- material count;
- source-association count;
- candidate design nonce;
- evaluation time;
- validation timestamp; and
- explicit unresolved constraints.

Provider warnings are not stored as deterministic diagnostics. They remain separately identified advisory claims.

#### `AgentCandidateScene`

Candidate scene owner containing:

- exact candidate source fingerprint;
- candidate design nonce;
- evaluation time;
- `std::unique_ptr<GeneratedSceneSnapshot>`;
- `PreparedScenePreviewPublication` derived facts after preparation;
- candidate `PreviewStatistics`.

This model is separate from `ScenePreviewSession`. It cannot publish itself and does not own lifecycle state; `Progen3dAgentSession` is the canonical lifecycle owner.

#### `AgentProposalApproval`

Immutable approval value containing:

- approval ID;
- candidate ID;
- base-document fingerprint;
- approved candidate fingerprint;
- candidate design nonce;
- approval timestamp; and
- explicit human-approval marker.

The approval contains no model-authored authorization field.

#### `AgentPublicationTransaction`

Transaction model containing:

- transaction ID;
- previous transaction hash;
- base source and fingerprint;
- approved source and fingerprint;
- approved candidate identity;
- approval identity;
- provider provenance;
- validation report;
- publication status;
- publication timestamp;
- previous published design nonce;
- rollback status; and
- transaction hash.

The durable transaction records source-level rollback provenance. An in-memory rollback state may additionally retain the previous `GeneratedSceneSnapshot` for exact same-session scene restoration. After application restart, rollback restores the previous source and regenerates it; that regenerated scene must be labeled as regenerated rather than claimed to be the original snapshot.

#### `AgentPublicationRollbackState`

Same-session rollback owner containing:

- previous source and fingerprint;
- previous published design nonce;
- `std::unique_ptr<GeneratedSceneSnapshot>` for the prior published scene when available;
- prepared preview facts required to republish that scene; and
- the transaction ID that created the rollback opportunity.

This state is intentionally not serialized as a scene snapshot. It is cleared after rollback, source drift, or a later accepted publication according to explicit lifecycle rules.

#### `Progen3dAgentSession`

Canonical workspace-owned agent state containing:

- conversation threads and messages;
- request input and operation mode;
- active provider/model presentation state;
- current request state;
- active proposal review session;
- latest transaction summary;
- optional same-session `AgentPublicationRollbackState`;
- current status message; and
- cancellation and busy state.

`EditorWorkspaceSession` will compose one `Progen3dAgentSession`.

Backend DTOs from `BackendApiClient.h` must not remain canonical members of this class after the migration is complete.

### 5.2 Service Classes

#### `GrammarSourceFingerprintService`

Computes SHA-256 fingerprints for exact grammar source bytes.

Implementation requirement:

- use the already linked OpenSSL crypto library through the EVP interface;
- never use `std::hash` for approval or persistence identity;
- return lowercase hexadecimal output; and
- provide deterministic test vectors.

#### `AgentRequestContextAssemblyService`

Builds an immutable `AgentDesignRequest` from:

- `EditorWorkspaceSession`;
- `GrammarEditorInteractionState`;
- current diagnostics;
- current conversation history; and
- a candidate nonce supplied by the agent workflow.

This service replaces ad hoc request assembly inside `AiConversationWorkflowService`.

#### `DesignAgentProvider`

Abstract provider category with operations to:

- list conversation threads;
- load one conversation thread; and
- submit one immutable `AgentDesignRequest`.

It returns provider-neutral conversation and response models.

#### `BackendDesignAgentProvider`

Concrete adapter that:

- composes the existing `AiGrammarProposalServiceFactory`;
- translates `AgentOperationMode` to existing backend mode strings;
- maps `AgentDesignRequest` to `BackendAiGenerateRequest`;
- maps backend threads, messages, usage, and results into domain models;
- persists refreshed authentication sessions through the existing repository; and
- never mutates editor source or preview state.

The existing `BackendAiGrammarProposalService` remains the HTTP/Firebase transport adapter.

#### `ScriptedDesignAgentProvider`

Test-only provider used by harnesses and GUI smoke tests.

It supports:

- deterministic responses;
- delayed completion;
- provider failure;
- stale response ordering;
- explanations without grammar;
- valid grammar proposals; and
- invalid grammar proposals.

#### `AgentRequestCoordinator`

Owns provider-request worker lifecycle.

Responsibilities:

- assign monotonic local request IDs;
- run synchronous provider adapters off the UI thread;
- retain only bounded pending work;
- expose completed responses to the main thread;
- mark superseded responses stale;
- provide cooperative cancellation state; and
- join worker threads during shutdown.

This is associated with provider work only. It does not compile grammar.

#### `GrammarGenerationExecutionGate`

Explicit process-level serialization boundary for grammar compilation.

The current grammar runtime reads process-global `grammar_rng` and `effects_rng` objects from `Grammar.cpp` and `Context.cpp`. Two independent generation workers therefore cannot safely seed or consume those streams concurrently.

For the MVP:

- every `GrammarCompilationService::compile()` acquires this gate before seeding, parsing, validating, and evaluating grammar;
- normal editor and agent candidate runtimes retain independent request queues but never execute grammar generation concurrently;
- waiting occurs on worker threads rather than the UI thread; and
- tests prove that two generation runtimes cannot overlap protected execution.

A later grammar-runtime refactor may replace the gate with context-owned random streams. Removing the gate is not permitted until all global random-engine uses have been removed or made explicitly thread-local and deterministic.

#### `AgentCandidateGenerationRuntime`

Owns isolated candidate compilation lifecycle.

It composes:

- one `GrammarCompilationService`; and
- one `SceneRegenerationCoordinator`.

It uses the same `GrammarGenerationRandomizationService`, `GrammarStructuralValidationService`, and `GrammarGenerationExecutionGate` behavior as normal editor generation but owns an independent queue and completed-result channel.

It cannot access `CompletedSceneGenerationPublicationService` directly.

#### `AgentCandidateValidationService`

Converts one selected proposal source into an isolated candidate-generation request.

Responsibilities:

- verify that the candidate fingerprint still matches selected source;
- assign or preserve the candidate design nonce;
- submit generation to `AgentCandidateGenerationRuntime`;
- reject stale generation results;
- convert diagnostics and generation facts into `AgentCandidateValidationReport`;
- create `AgentCandidateScene` only after successful generation; and
- leave canonical editor diagnostics and published scene unchanged.

#### `AgentProposalEvidenceCollectionService`

Builds evidence from the generated candidate and prepared preview facts.

Evidence includes:

- source and base fingerprints;
- provider provenance;
- candidate nonce and evaluation time;
- compiler and structural status;
- complete deterministic diagnostics;
- rule, token, primitive, material, and source-association counts;
- generation duration;
- optional preview capture fingerprint; and
- unresolved constraints separated into provider claims and deterministic findings.

#### `AgentCandidatePreviewActivationService`

Temporarily presents a validated candidate scene.

Responsibilities:

- require a successful validation report for the exact candidate fingerprint;
- call `ScenePreviewPublicationPreparationService` on the OpenGL-owning thread;
- activate candidate geometry and materials without replacing the canonical scene snapshot;
- retain whether the candidate or published scene is currently displayed;
- restore published scene resources when candidate preview ends; and
- restore published resources after every candidate-preparation failure.

The service must not move the candidate snapshot into `ScenePreviewSession`.

#### `AgentProposalApprovalService`

Creates `AgentProposalApproval` only when:

- review is pending;
- exact selected source has a successful validation report;
- candidate source fingerprint matches the report;
- candidate scene identity matches the report;
- current document fingerprint matches the base fingerprint; and
- the request originated from an explicit human UI action.

#### `PreparedScenePreviewPublicationService`

New canonical service extracted from the successful-state projection portion of `CompletedSceneGenerationPublicationService`.

It publishes:

- a prepared `GeneratedSceneSnapshot`;
- source associations;
- lighting definition;
- preview statistics;
- timeline state;
- editor selection reset; and
- document generation identity.

The extraction must preserve existing successful-publication behavior, including grammar runtime-variable cleanup, electrical switch-state preservation, lighting evaluation/evidence publication, statistics, timeline reset, selection reset, and diagnostic clearing.

`CompletedSceneGenerationPublicationService` will continue to own stale-result and failure handling, then delegate successful projection to this service.

#### `ApprovedAgentCandidatePublicationService`

Sole agent publication authority.

It will:

1. verify the pending review and exact approval;
2. recompute current document and candidate fingerprints;
3. reject source drift, candidate drift, missing evidence, or nonce mismatch;
4. prepare the exact candidate scene for OpenGL publication;
5. create a pending `AgentPublicationTransaction` containing the previous source;
6. publish the validated source without scheduling a different stochastic generation;
7. move the exact candidate snapshot into canonical scene publication;
8. record publication success and append the transaction event;
9. clear candidate preview ownership; and
10. expose explicit rollback through the same authority.

The service must fail closed before source mutation if preview preparation fails.

#### `AgentPublicationRollbackService`

Restores the prior source from the most recent eligible transaction.

Rollback will:

- require explicit human action;
- verify the transaction has not already been rolled back;
- verify the current source fingerprint still matches the transaction's published fingerprint unless the user explicitly resolves drift through a separate confirmation path;
- restore previous source through `EditorDocumentPublicationService`;
- schedule normal deterministic editor regeneration;
- append a rollback event; and
- never delete transaction history.

When the previous same-session `GeneratedSceneSnapshot` remains available, rollback prepares and republishes that exact snapshot. When it is unavailable after restart, rollback restores the previous source and previous design nonce, performs a new generation under the current recorded generation environment, and records that the scene was regenerated rather than exactly restored.

#### `AgentTransactionRepository`

Abstract repository for append-only transaction events.

#### `JsonAgentTransactionRepository`

Concrete repository that stores transactions in the existing application-data path infrastructure.

Requirements:

- one append-only JSON Lines ledger per workspace or local document identity;
- no secrets, authentication tokens, or complete raw provider payloads;
- exact source may be stored only when local transaction history is enabled;
- previous-event hash and event hash on every event;
- atomic state-summary replacement through a temporary file and rename; and
- deterministic schema versioning.

#### `Progen3dDesignAgent`

Application service orchestrating the agent state machine.

It aggregates:

- `AgentRequestContextAssemblyService`;
- `DesignAgentProvider` creation;
- `AgentRequestCoordinator`;
- `AgentCandidateValidationService`;
- `AgentProposalEvidenceCollectionService`;
- `AgentProposalApprovalService`;
- `ApprovedAgentCandidatePublicationService`;
- `AgentCandidatePreviewActivationService`;
- `AgentPublicationRollbackService`; and
- application logging.

It does not own editor source, the published scene, authentication data, or OpenGL resources.

### 5.3 Context Classes

#### `Progen3dAgentRuntimeContext`

Narrow context supplied to `Progen3dDesignAgent` operations.

It associates the agent with only:

- `EditorWorkspaceSession`;
- `GrammarEditorInteractionState`;
- `GrammarSourceFingerprintService`;
- `AgentCandidateGenerationRuntime`;
- `ScenePreviewPublicationPreparationService`;
- `ScenePreviewResourceService`;
- `EditorDocumentPublicationService`;
- `PreparedScenePreviewPublicationService`;
- `AgentTransactionRepository`;
- `ApplicationLogPublisher`; and
- the provider factory or provider session required for the current request.

It must not expose arbitrary filesystem, command execution, renderer internals, or cloud publication services.

`EditorApplicationRuntimeContext` will expose one `Progen3dAgentRuntimeContext &progen3dAgentRuntimeContext() const` association and presentation-facing workflow interfaces. The agent core will not depend on the complete editor runtime context.

### 5.4 Presentation Classes

#### Existing Compatibility Presentation

The first implementation slices will retain:

- `AiAssistantPanel`; and
- `AiGrammarProposalPanel`.

Their behavior will be migrated before optional renaming. This avoids mixing an authority redesign with an unnecessary presentation rename.

#### Target Presentation Responsibilities

`AiAssistantPanel` will present:

- operation mode;
- request input;
- provider busy/cancel state;
- conversation history;
- provider response;
- proposal lifecycle status; and
- deterministic validation progress.

`AiGrammarProposalPanel` will present:

- base and candidate short fingerprints;
- source drift status;
- line-based differences;
- difference selection controls;
- deterministic diagnostics;
- candidate statistics;
- provider warnings;
- unresolved constraints;
- `Validate Selected Candidate` when evidence is invalidated;
- `Preview Candidate` and `Return to Published Scene`;
- `Approve Exact Candidate`;
- `Publish Approved Candidate`;
- `Reject Proposal`; and
- `Rollback Last Agent Publication` when eligible.

The panel must never call `proposal_review.acceptFullProposal()` or mutate source directly.

## 6. Agent State Machine

`Progen3dAgentSession` will expose one explicit lifecycle state:

1. `Idle`
2. `AwaitingProvider`
3. `ProviderResponseReceived`
4. `CandidateValidationQueued`
5. `CandidateValidationRunning`
6. `CandidateInvalid`
7. `ReadyForReview`
8. `CandidatePreviewActive`
9. `Approved`
10. `Publishing`
11. `Published`
12. `Rejected`
13. `Stale`
14. `Failed`
15. `Cancelled`

Allowed transitions must be implemented as named methods on `Progen3dAgentSession` or a dedicated lifecycle model. Presentation code must not assign lifecycle state directly.

Required fail-closed transitions:

- document edit during `ReadyForReview`, `CandidatePreviewActive`, or `Approved` -> `Stale`;
- selected difference change -> `CandidateValidationQueued` or `ProviderResponseReceived` with all evidence cleared;
- provider response without grammar -> `Idle` after response publication;
- invalid candidate -> `CandidateInvalid`;
- preview preparation failure -> remain `ReadyForReview` with published preview restored;
- approval hash mismatch -> `Stale` or `Failed`, never `Publishing`;
- publication preparation failure -> remain approved/reviewable with source unchanged;
- superseded provider or generation completion -> discard as stale without user-state mutation.

## 7. End-to-End Request Lifecycle

### 7.1 Explanation or Diagnosis

1. User selects `ExplainSelection` or `DiagnoseGrammar`.
2. `AgentRequestContextAssemblyService` captures immutable source, selection, diagnostics, and source fingerprint.
3. `AgentRequestCoordinator` runs the provider call outside the UI thread.
4. `Progen3dDesignAgent::advancePendingWork()` publishes the latest non-stale response on the main thread.
5. Response text and advisory findings are displayed.
6. No `AgentProposalCandidate` is created unless the provider supplied grammar and the selected mode permits grammar proposals.
7. No publication controls are enabled.

### 7.2 Repair or Draft

1. User selects `RepairGrammar` or `DraftGrammar` and submits a request.
2. Immutable request context and base fingerprint are captured.
3. Provider returns a proposed grammar.
4. Client computes the candidate fingerprint; provider-supplied hashes are ignored.
5. `AgentProposalReviewSession` builds differences.
6. Exact full proposal is submitted automatically to candidate validation.
7. Candidate runtime compiles with the assigned candidate design nonce.
8. Failure publishes candidate diagnostics without changing editor diagnostics or preview.
9. Success creates `AgentCandidateScene` and deterministic evidence.
10. Review controls become available.

### 7.3 Selected Changes

1. User changes one difference selection.
2. Review session composes the selected source.
3. Client computes a new selected candidate fingerprint.
4. Previous candidate scene, validation report, evidence, preview state, and approval are discarded.
5. User requests validation, or validation starts automatically after a bounded debounce.
6. Only the newly validated selected source may be approved.

### 7.4 Preview

1. User requests candidate preview.
2. Agent verifies exact validation evidence and source fingerprint.
3. Candidate resources are prepared on the OpenGL thread.
4. Candidate geometry is displayed while `ScenePreviewSession` retains canonical ownership of the published snapshot.
5. Closing preview, changing selection, editing source, rejecting, or encountering failure restores published scene resources.

### 7.5 Approval and Publication

1. User presses `Approve Exact Candidate`.
2. `AgentProposalApprovalService` verifies current document, candidate, evidence, and nonce identities.
3. UI displays the complete candidate fingerprint and approval timestamp.
4. User presses `Publish Approved Candidate`.
5. `ApprovedAgentCandidatePublicationService` repeats all identity checks.
6. Exact candidate scene preparation occurs before source mutation.
7. Pending transaction is appended.
8. Exact validated source replaces the editor source without requesting a new stochastic candidate.
9. Exact validated scene snapshot becomes the canonical published scene.
10. Publication event is appended and rollback becomes available.

## 8. Provider Contract

### 8.1 Client-Owned Request Schema

The provider adapter will serialize these logical fields:

```text
schema_version
operation_mode
user_instruction
document_source
document_source_fingerprint
selected_source
selected_source_range
deterministic_diagnostics
conversation_history
document_metadata
candidate_design_nonce
```

The backend adapter may continue mapping these to existing fields during compatibility operation:

- `mode`
- `prompt` or `question`
- `grammar`
- `selection`
- `parserError`
- `history`
- `file_id`
- `title`

The backend prompt implementation must treat document source, selected source, diagnostics, conversation history, comments, and imported metadata as quoted untrusted data. Client code cannot enforce server-side prompt construction, so backend conformance must be a separately verified integration requirement.

### 8.2 Provider Response Schema

The preferred response schema is:

```text
schema_version
response_kind
assistant_text
proposed_grammar
summary
changes[]
warnings[]
unresolved_constraints[]
provider_request_id
conversation_id
model_name
usage
```

MVP compatibility mapping will use existing `BackendAiResult` fields:

- `grammar`
- `summary`
- `answer`
- `repair_summary`
- `diagnosis`
- `changes`
- `observations`
- `suggested_edits`
- `warnings`

All candidate and base fingerprints are computed locally after response parsing.

## 9. Publication and Rollback Transaction

### 9.1 Publication Preconditions

Publication must reject unless all conditions are true:

- review is pending;
- approval exists;
- approval candidate ID equals active candidate ID;
- approval candidate fingerprint equals selected-source fingerprint;
- validation report candidate fingerprint equals selected-source fingerprint;
- candidate scene fingerprint equals selected-source fingerprint;
- candidate design nonce equals validation design nonce;
- current document fingerprint equals proposal base fingerprint;
- provider request is the latest request;
- candidate validation request is the latest candidate request; and
- candidate scene snapshot exists.

### 9.2 Transaction Sequence

1. Recompute all fingerprints.
2. Deactivate any transient candidate preview and restore canonical resources.
3. Prepare the exact candidate scene again for publication.
4. Append `PublicationPrepared` transaction event.
5. Replace source through a new non-regenerating `EditorDocumentPublicationService::publishValidatedSource()` operation.
6. Publish the prepared candidate through `PreparedScenePreviewPublicationService`.
7. Append `PublicationCommitted` event.
8. Move the previous published scene into same-session rollback ownership when available.
9. Clear candidate ownership from the agent session.
10. Mark review decision `Published`.

If source replacement or scene publication fails after preparation:

- restore the previous source;
- restore published preview resources;
- append `PublicationFailed` with a deterministic failure code; and
- retain the proposal for review when safe.

### 9.3 Rollback Sequence

1. User requests rollback.
2. Load the latest committed, non-rolled-back agent transaction.
3. Verify current source still equals the transaction's published fingerprint.
4. Append `RollbackPrepared`.
5. Restore prior source through `EditorDocumentPublicationService`.
6. Republish the retained prior scene snapshot when it is available in the current session; otherwise request regeneration with the recorded prior design nonce.
7. Append `RollbackCommitted` after successful exact restoration or regeneration, or `RollbackFailed` if restoration fails.

Rollback does not delete, rewrite, or truncate transaction history.

## 10. Candidate Preview Resource Strategy

The current OpenGL resource service owns one active material library and one renderer upload set. The MVP will therefore use reversible activation rather than duplicate all GPU resources.

### 10.1 Candidate Activation

- `AgentCandidateScene` retains CPU-side generated scene ownership.
- `AgentCandidatePreviewActivationService` calls `prepareCandidateScene()` for the candidate.
- The renderer temporarily displays candidate geometry.
- `ScenePreviewSession` still owns the published scene snapshot and published source associations.
- Agent UI clearly labels the viewport `Candidate Preview`.

### 10.2 Published Scene Restoration

- Candidate preview exit calls `ScenePreviewResourceService::refreshSceneResources()` with the published generation context.
- Material synchronization returns to the published material set.
- Candidate preview state is cleared only after restoration succeeds or a recovery attempt is recorded.
- Application shutdown restores or releases resources through existing shutdown ownership.

### 10.3 Later Dual-Resource Optimization

A later performance release may introduce separate published and candidate GPU resource collections. It is not required for the MVP and must not be added until reversible single-resource activation is validated.

## 11. Implementation Milestones

## Milestone 0: Freeze Baseline and Add Agent Source Gates

### Tasks

- Capture `git status --short` and hashes for all files to be modified in the first slice.
- Run existing focused AI and generation gates before modification.
- Add an agent source-contract script before adding authority-bearing code.
- Make the source gate reject direct provider-to-document and presentation-to-document mutation.
- Keep existing source gates active.

### Files to Add

- `tests/run_progen3d_agent_source_contract_checks.sh`

### Files to Modify

- none outside test registration unless required by the current release-check structure

### Required Checks

```bash
./tests/run_authentication_ai_workflow_checks.sh
./tests/run_ai_grammar_proposal_review_checks.sh
./tests/run_completed_scene_generation_publication_checks.sh
./tests/run_editor_scene_generation_runtime_checks.sh
./tests/run_preview_session_checks.sh
./tests/run_progen3d_agent_source_contract_checks.sh
```

### Exit Gate

- Baseline outcomes are recorded without claiming they are current release certification.
- Source gate fails when a fixture introduces direct AI workflow document publication.

## Milestone 1: Add Domain Models and Exact Fingerprints

### Tasks

- Add provider-neutral request, response, proposal, validation, evidence, approval, and transaction models.
- Add `GrammarSourceFingerprintService` using OpenSSL EVP SHA-256.
- Add lifecycle methods that prevent direct state assignment.
- Add selected-source reconstruction with stable difference IDs.
- Preserve current line-difference behavior while migrating from `AiGrammarProposalReview`.

### Files to Add

- `include/editor/model/AgentOperationMode.h`
- `include/editor/model/GrammarSourceFingerprint.h`
- `include/editor/model/AgentDesignRequest.h`
- `include/editor/model/AgentConversation.h`
- `include/editor/model/AgentProviderResponse.h`
- `include/editor/model/AgentProposalCandidate.h`
- `include/editor/model/AgentProposalDifference.h`
- `include/editor/model/AgentProposalReviewSession.h`
- `include/editor/model/AgentCandidateValidationReport.h`
- `include/editor/model/AgentCandidateScene.h`
- `include/editor/model/AgentProposalApproval.h`
- `include/editor/model/AgentPublicationTransaction.h`
- `include/editor/model/AgentPublicationRollbackState.h`
- `include/editor/model/Progen3dAgentSession.h`
- `include/editor/service/GrammarSourceFingerprintService.h`
- corresponding `src/editor/model/*.cpp`
- `src/editor/service/GrammarSourceFingerprintService.cpp`
- `tests/grammar_source_fingerprint_harness.cpp`
- `tests/agent_proposal_review_session_harness.cpp`
- `tests/run_progen3d_agent_model_checks.sh`

### Files to Modify

- `include/editor/model/EditorWorkspaceSession.h`
- `Makefile`

### Required Tests

- known SHA-256 test vectors;
- byte-for-byte sensitivity, including final newline changes;
- stable difference IDs;
- selected-source reconstruction;
- evidence invalidation after selection changes;
- approval invalidation after selection changes;
- lifecycle transition rejection; and
- no backend DTO types in new canonical models.

### Exit Gate

- New domain model harness passes with no network, OpenGL, or backend dependency.
- `EditorWorkspaceSession` composes one canonical agent session.

## Milestone 2: Introduce Provider-Neutral Request Boundary

### Tasks

- Add `AgentRequestContextAssemblyService`.
- Add `DesignAgentProvider` and provider-neutral conversation models.
- Add `BackendDesignAgentProvider` as an adapter over existing AI transport.
- Move backend request/response translation out of `AiConversationWorkflowService`.
- Keep thread listing and loading behavior compatible.
- Add `ScriptedDesignAgentProvider` under test support.

### Files to Add

- `include/editor/service/AgentRequestContextAssemblyService.h`
- `include/editor/service/DesignAgentProvider.h`
- `include/editor/service/DesignAgentProviderFactory.h`
- `include/editor/service/BackendDesignAgentProvider.h`
- `include/editor/service/BackendDesignAgentProviderFactory.h`
- `src/editor/service/AgentRequestContextAssemblyService.cpp`
- `src/editor/service/BackendDesignAgentProvider.cpp`
- `src/editor/service/BackendDesignAgentProviderFactory.cpp`
- `tests/harness/ScriptedDesignAgentProvider.h`
- `tests/agent_request_context_assembly_harness.cpp`
- `tests/backend_design_agent_provider_harness.cpp`
- `tests/run_progen3d_agent_provider_checks.sh`

### Files to Modify

- `include/editor/service/AiConversationWorkflowService.h`
- `src/editor/service/AiConversationWorkflowService.cpp`
- `include/editor/application/Progen3dEditorApplication.h`
- `src/editor/application/Progen3dEditorApplication.cpp`
- `Makefile`
- `tests/run_authentication_ai_workflow_checks.sh`

### Required Tests

- immutable request captures source and fingerprint before provider call;
- complete diagnostics are captured deterministically;
- selected source and range are captured accurately;
- backend mode compatibility mapping;
- backend response translation;
- refreshed authentication session persistence;
- provider failure does not clear current proposal or document;
- explanation response without grammar does not create a proposal; and
- no provider adapter has document or preview publication access.

### Exit Gate

- `AiConversationWorkflowService` contains no direct `BackendAiGenerateRequest` construction.
- Existing backend behavior remains available through the new adapter.

## Milestone 3: Add Asynchronous Agent Orchestration

### Tasks

- Add `AgentRequestCoordinator` for provider worker ownership.
- Add `AgentCandidateGenerationRuntime` using existing compilation and regeneration classes.
- Add `GrammarGenerationExecutionGate` and route every production grammar compilation through it before two runtimes are enabled.
- Add `Progen3dAgentRuntimeContext`.
- Add `Progen3dDesignAgent` state-machine orchestration.
- Add a main-thread `advancePendingWork()` call to `EditorFrameCycle` before presentation rendering.
- Add deterministic shutdown of provider and candidate workers.

### Files to Add

- `include/editor/service/AgentRequestCoordinator.h`
- `src/editor/service/AgentRequestCoordinator.cpp`
- `include/editor/service/GrammarGenerationExecutionGate.h`
- `src/editor/service/GrammarGenerationExecutionGate.cpp`
- `include/editor/application/AgentCandidateGenerationRuntime.h`
- `src/editor/application/AgentCandidateGenerationRuntime.cpp`
- `include/editor/context/Progen3dAgentRuntimeContext.h`
- `src/editor/context/Progen3dAgentRuntimeContext.cpp`
- `include/editor/service/Progen3dDesignAgent.h`
- `src/editor/service/Progen3dDesignAgent.cpp`
- `tests/agent_request_coordinator_harness.cpp`
- `tests/grammar_generation_execution_gate_harness.cpp`
- `tests/agent_candidate_generation_runtime_harness.cpp`
- `tests/progen3d_design_agent_state_harness.cpp`
- `tests/run_progen3d_agent_runtime_checks.sh`

### Files to Modify

- `include/editor/application/EditorApplicationRuntimeContext.h`
- `src/editor/application/EditorApplicationRuntimeContext.cpp`
- `include/editor/service/GrammarCompilationService.h`
- `src/editor/service/GrammarCompilationService.cpp`
- `include/editor/application/EditorSceneGenerationRuntime.h`
- `src/editor/application/EditorSceneGenerationRuntime.cpp`
- `include/editor/application/Progen3dEditorApplication.h`
- `src/editor/application/Progen3dEditorApplication.cpp`
- `include/editor/application/EditorFrameCycle.h`
- `src/editor/application/EditorFrameCycle.cpp`
- application shutdown operations
- `Makefile`

### Required Tests

- provider call runs outside the calling thread;
- editor and candidate grammar compilation never overlap while global random streams remain;
- stale provider response is discarded;
- stale candidate result is discarded;
- cancellation prevents result publication;
- shutdown joins workers;
- normal editor generation and candidate generation use independent queues;
- candidate generation cannot call canonical scene publication; and
- frame-cycle advancement performs state publication on the main thread.

### Exit Gate

- Slow scripted provider does not block the frame-cycle harness.
- Competing editor and agent generation requests serialize on worker threads without random-stream races.
- Closing the application with provider or candidate work in progress exits cleanly.

## Milestone 4: Add Candidate Validation and Evidence

### Tasks

- Add `AgentCandidateValidationService`.
- Add `AgentProposalEvidenceCollectionService`.
- Automatically validate complete repair/draft proposals.
- Preserve candidate diagnostics separately from editor diagnostics.
- Create candidate scene only after successful generation.
- Bind validation report and evidence to exact source fingerprint and nonce.

### Files to Add

- `include/editor/service/AgentCandidateValidationService.h`
- `src/editor/service/AgentCandidateValidationService.cpp`
- `include/editor/service/AgentProposalEvidenceCollectionService.h`
- `src/editor/service/AgentProposalEvidenceCollectionService.cpp`
- `tests/agent_candidate_validation_harness.cpp`
- `tests/agent_proposal_evidence_harness.cpp`
- `tests/run_progen3d_agent_validation_checks.sh`

### Files to Modify

- `include/editor/service/Progen3dDesignAgent.h`
- `src/editor/service/Progen3dDesignAgent.cpp`
- `include/editor/model/Progen3dAgentSession.h`
- related model source files
- `Makefile`

### Required Tests

- invalid grammar creates candidate diagnostics and no candidate scene;
- valid grammar creates a candidate scene and validation report;
- editor diagnostics remain unchanged during candidate validation;
- published scene remains unchanged during candidate validation;
- source fingerprint, nonce, and result identity match;
- selected difference change destroys old candidate evidence;
- provider warnings cannot satisfy deterministic validation; and
- source-association and scene statistics are measured from the candidate.

### Exit Gate

- Invalid proposals cannot reach an approvable state.
- Valid proposals expose deterministic evidence without canonical mutation.

## Milestone 5: Add Exact Approval, Publication, and Rollback

### Tasks

- Extract successful canonical state projection into `PreparedScenePreviewPublicationService`.
- Preserve existing stale/failure behavior in `CompletedSceneGenerationPublicationService`.
- Add `AgentProposalApprovalService`.
- Add `ApprovedAgentCandidatePublicationService`.
- Add `AgentPublicationRollbackService`.
- Add non-regenerating validated-source publication to `EditorDocumentPublicationService`.
- Replace the current direct accepted-source scheduling path for agent proposals.

### Files to Add

- `include/editor/service/PreparedScenePreviewPublicationService.h`
- `src/editor/service/PreparedScenePreviewPublicationService.cpp`
- `include/editor/service/AgentProposalApprovalService.h`
- `src/editor/service/AgentProposalApprovalService.cpp`
- `include/editor/service/ApprovedAgentCandidatePublicationService.h`
- `src/editor/service/ApprovedAgentCandidatePublicationService.cpp`
- `include/editor/service/AgentPublicationRollbackService.h`
- `src/editor/service/AgentPublicationRollbackService.cpp`
- `tests/prepared_scene_preview_publication_harness.cpp`
- `tests/agent_proposal_approval_harness.cpp`
- `tests/approved_agent_candidate_publication_harness.cpp`
- `tests/agent_publication_rollback_harness.cpp`
- `tests/run_progen3d_agent_publication_checks.sh`

### Files to Modify

- `include/editor/service/CompletedSceneGenerationPublicationService.h`
- `src/editor/service/CompletedSceneGenerationPublicationService.cpp`
- `include/editor/service/EditorDocumentPublicationService.h`
- `src/editor/service/EditorDocumentPublicationService.cpp`
- `include/editor/model/ScenePreviewSession.h`
- `include/editor/service/AiGrammarProposalPublication.h`
- `include/editor/service/AiGrammarProposalPublicationService.h`
- `src/editor/service/AiGrammarProposalPublicationService.cpp`
- `include/editor/application/Progen3dEditorApplication.h`
- `src/editor/application/Progen3dEditorApplication.cpp`
- `Makefile`
- existing completed-publication and AI-publication harnesses

### Required Tests

- approval requires exact candidate validation;
- approval is rejected after document drift;
- approval is rejected after selected differences change;
- candidate preview preparation failure leaves source and scene unchanged;
- publication transfers the exact validated snapshot and design nonce;
- publication moves the previous published snapshot into same-session rollback ownership;
- publication does not request a new stochastic generation;
- current source and published scene correspond to the approved fingerprint;
- transaction contains previous source and exact approval identity;
- rollback restores previous source;
- rollback requires explicit action;
- rollback is rejected after unrelated source drift; and
- legacy completed-generation success and failure tests remain green.

### Exit Gate

- No agent acceptance path calls `requestSceneGeneration()` for the accepted candidate.
- Exact reviewed candidate identity becomes the published scene identity.

## Milestone 6: Add Candidate Preview and Review UI

### Tasks

- Add candidate preview activation service and state.
- Extend proposal panel with evidence, fingerprints, validation, preview, approval, publication, and rollback controls.
- Extend assistant panel with asynchronous busy/cancel state.
- Label provider claims and deterministic evidence separately.
- Add source-drift and candidate-drift status chips.
- Restore published preview when panel closes or candidate becomes invalid.

### Files to Add

- `include/editor/service/AgentCandidatePreviewWorkflow.h`
- `include/editor/service/AgentCandidatePreviewActivationService.h`
- `src/editor/service/AgentCandidatePreviewActivationService.cpp`
- `tests/agent_candidate_preview_activation_harness.cpp`
- `tests/run_progen3d_agent_preview_checks.sh`

### Files to Modify

- `include/editor/presentation/AiAssistantPanel.h`
- `src/editor/presentation/AiAssistantPanel.cpp`
- `include/editor/presentation/AiGrammarProposalPanel.h`
- `src/editor/presentation/AiGrammarProposalPanel.cpp`
- `include/editor/presentation/EditorPresentationComposition.h`
- `src/editor/presentation/EditorPresentationComposition.cpp`
- `include/editor/application/EditorApplicationRuntimeContext.h`
- `src/editor/application/EditorApplicationRuntimeContext.cpp`
- `include/editor/model/WorkspaceLayoutState.h` when a new agent pane state is required
- `Makefile`

### Required Tests

- preview requires valid exact candidate;
- candidate activation does not replace `lastValidSceneSnapshot()`;
- return-to-published restores published geometry and materials;
- selection change during preview restores published scene;
- document edit during preview marks proposal stale and restores published scene;
- preview preparation failure restores published resources;
- presentation code uses workflow interfaces only; and
- UI disables approval and publication when validation is stale.

### Exit Gate

- Candidate and published scene ownership are visibly and structurally distinct.
- GUI smoke demonstrates request, validation, preview, approval, publication, and rollback using a scripted provider.

## Milestone 7: Add Append-Only Transaction Persistence

### Tasks

- Add transaction repository contract and JSON Lines implementation.
- Add hash-chained event serialization.
- Add transaction summary projection.
- Integrate persistence with approval, publication, failure, and rollback events.
- Add privacy controls for retaining complete source and raw provider output.

### Files to Add

- `include/editor/service/AgentTransactionRepository.h`
- `include/editor/service/JsonAgentTransactionRepository.h`
- `src/editor/service/JsonAgentTransactionRepository.cpp`
- `include/editor/service/AgentTransactionHashService.h`
- `src/editor/service/AgentTransactionHashService.cpp`
- `tests/agent_transaction_repository_harness.cpp`
- `tests/agent_transaction_hash_chain_harness.cpp`
- `tests/run_progen3d_agent_transaction_checks.sh`

### Files to Modify

- `include/editor/service/ApprovedAgentCandidatePublicationService.h`
- `src/editor/service/ApprovedAgentCandidatePublicationService.cpp`
- `include/editor/service/AgentPublicationRollbackService.h`
- `src/editor/service/AgentPublicationRollbackService.cpp`
- `include/editor/application/Progen3dEditorApplication.h`
- `src/editor/application/Progen3dEditorApplication.cpp`
- `Makefile`

### Required Tests

- event hash changes when any event field changes;
- previous hash forms an unbroken chain;
- corrupted event is detected;
- appending never rewrites earlier events;
- projection can be rebuilt from ledger;
- secrets and authentication tokens are absent;
- temporary-file projection replacement is atomic; and
- transaction history survives application restart.

### Exit Gate

- Every accepted proposal has durable approval, publication, and rollback evidence.

## Milestone 8: Complete UI Migration and Compatibility Cleanup

### Tasks

- Make `Progen3dAgentSession` the only canonical agent state.
- Remove backend DTO ownership from editor models.
- Reduce `AiConversationWorkflowService` and `AiGrammarProposalPublicationService` to compatibility adapters or replace their interfaces in one reviewed slice.
- Update source gates to enforce the final object model.
- Update README and editor implementation documentation.
- Add agent tests to release checks only after all focused gates are stable.

### Files to Modify

- `include/editor/model/AiAssistantSession.h`
- `src/editor/model/AiAssistantSession.cpp`
- `include/editor/model/AiGrammarProposal.h`
- `include/editor/model/AiGrammarProposalReview.h`
- `src/editor/model/AiGrammarProposalReview.cpp`
- AI workflow and publication interfaces
- presentation composition and panels
- `README.md`
- `PROGEN3D_EDITOR_GUI_IMPLEMENTATION_PLAN.md` or a completion addendum
- `tests/run_release_checks.sh`
- `Makefile`

### Compatibility Rule

Do not remove old backend DTO or workflow types until:

- all live uses are migrated;
- backend thread loading still works;
- current AI source gates have replacements;
- scripted provider tests pass; and
- full GUI linking passes.

### Exit Gate

- The editor object model exposes a ProGen3D design agent rather than backend AI DTO state.

## Milestone 9: Later Capability Expansion

These capabilities are explicitly outside the MVP but must reuse the same candidate and approval model.

### `ExploreVariants`

- generate bounded candidate count;
- assign one deterministic nonce per candidate;
- validate every candidate independently;
- compare measured statistics and explicit user constraints;
- never rank solely from provider prose; and
- approve one exact candidate only.

### `AuditDesign`

- inspect source, diagnostics, generated scene facts, source associations, and declared constraints;
- identify contradictions and unknowns;
- produce no mutation unless the user separately requests a repair proposal.

### Reference-Driven Design

- retain reference identity and content hash;
- separate observed measurements from inferred measurements;
- keep unresolved constraints explicit;
- bind generated evidence to reference and grammar hashes; and
- require normal exact-candidate approval.

### Local Provider

An `OllamaDesignAgentProvider` may be added after the provider-neutral contract is stable. Local model availability, model tag, context size, and GPU state must be detected at runtime rather than embedded as permanent assumptions.

## 12. Concrete API Shape

The following signatures describe the intended ownership boundaries. Exact spelling may change only when repository compilation requires a clearer equivalent.

```cpp
class DesignAgentProvider
{
public:
    virtual ~DesignAgentProvider() = default;

    virtual bool listThreads(
        std::vector<AgentConversationThreadSummary> *threads,
        std::string *error_message) = 0;

    virtual bool loadThread(
        const std::string &thread_id,
        AgentConversation *conversation,
        std::string *error_message) = 0;

    virtual bool submitRequest(
        const AgentDesignRequest &request,
        AgentProviderResponse *response,
        std::string *error_message) = 0;
};
```

```cpp
class GrammarSourceFingerprintService
{
public:
    GrammarSourceFingerprint fingerprint(
        const std::string &source_text) const;
};
```

```cpp
class AgentCandidateValidationService
{
public:
    std::uint64_t requestValidation(
        AgentCandidateGenerationRuntime &generation_runtime,
        const AgentProposalCandidate &candidate,
        const std::string &selected_source);

    bool publishCompletedValidation(
        Progen3dAgentSession &agent_session,
        AgentCandidateGenerationRuntime &generation_runtime) const;
};
```

```cpp
class AgentProposalApprovalService
{
public:
    std::optional<AgentProposalApproval> approveExactCandidate(
        const Progen3dAgentSession &agent_session,
        const GrammarSourceFingerprint &current_document_fingerprint,
        bool explicit_human_action) const;
};
```

```cpp
class ApprovedAgentCandidatePublicationService
{
public:
    AgentCandidatePublicationOutcome publish(
        Progen3dAgentRuntimeContext &runtime_context,
        const AgentProposalApproval &approval);
};
```

```cpp
class Progen3dDesignAgent
{
public:
    bool submitCurrentRequest(Progen3dAgentRuntimeContext &runtime_context);
    void cancelCurrentRequest(Progen3dAgentRuntimeContext &runtime_context);
    void advancePendingWork(Progen3dAgentRuntimeContext &runtime_context);
    bool validateSelectedCandidate(Progen3dAgentRuntimeContext &runtime_context);
    bool previewCandidate(Progen3dAgentRuntimeContext &runtime_context);
    bool restorePublishedPreview(Progen3dAgentRuntimeContext &runtime_context);
    bool approveExactCandidate(Progen3dAgentRuntimeContext &runtime_context);
    bool publishApprovedCandidate(Progen3dAgentRuntimeContext &runtime_context);
    bool rejectProposal(Progen3dAgentRuntimeContext &runtime_context);
    bool rollbackLastPublication(Progen3dAgentRuntimeContext &runtime_context);
};
```

## 13. Existing File Change Map

| Existing file | Planned responsibility change |
|---|---|
| `include/editor/model/EditorWorkspaceSession.h` | Compose canonical `Progen3dAgentSession`. |
| `include/editor/model/AiAssistantSession.h` | Temporary compatibility shell, then remove backend DTO ownership. |
| `src/editor/model/AiAssistantSession.cpp` | Delegate lifecycle and proposal state to canonical agent models. |
| `include/editor/model/AiGrammarProposalReview.h` | Migrate or adapt line differences to `AgentProposalReviewSession`. |
| `src/editor/model/AiGrammarProposalReview.cpp` | Preserve tested difference behavior during migration. |
| `include/editor/service/AiConversationWorkflowService.h` | Delegate request submission to `Progen3dDesignAgent`. |
| `src/editor/service/AiConversationWorkflowService.cpp` | Remove backend request construction and direct proposal staging. |
| `include/editor/service/AiGrammarProposalPublicationService.h` | Become compatibility adapter over exact approval/publication authority. |
| `src/editor/service/AiGrammarProposalPublicationService.cpp` | Remove direct accepted-source scheduling path. |
| `include/editor/service/EditorDocumentPublicationService.h` | Add validated source publication without stochastic regeneration. |
| `src/editor/service/EditorDocumentPublicationService.cpp` | Preserve focus/source behavior while separating source commit from generation request. |
| `include/editor/model/ScenePreviewSession.h` | Add explicit transfer of the published snapshot for governed publication and same-session rollback. |
| `include/editor/service/CompletedSceneGenerationPublicationService.h` | Delegate successful state projection. |
| `src/editor/service/CompletedSceneGenerationPublicationService.cpp` | Retain stale/failure behavior; use prepared publication service for success. |
| `include/editor/application/EditorApplicationRuntimeContext.h` | Expose narrow agent workflow/context associations. |
| `src/editor/application/EditorApplicationRuntimeContext.cpp` | Store and return those associations. |
| `include/editor/application/Progen3dEditorApplication.h` | Compose agent services, runtimes, provider factory, and repository. |
| `src/editor/application/Progen3dEditorApplication.cpp` | Construct services in dependency order and shut them down deterministically. |
| `src/editor/application/EditorFrameCycle.cpp` | Advance provider and candidate completion before drawing. |
| `src/editor/presentation/AiAssistantPanel.cpp` | Present async request and provider-neutral response state. |
| `src/editor/presentation/AiGrammarProposalPanel.cpp` | Present validation, evidence, preview, exact approval, publication, and rollback. |
| `Makefile` | Register all production and harness sources. |
| `tests/run_authentication_ai_workflow_checks.sh` | Preserve transport/workflow boundaries and add new adapter checks. |
| `tests/run_release_checks.sh` | Add consolidated agent checks only after focused completion. |

## 14. Test and Verification Strategy

### 14.1 Focused Model Tests

```bash
./tests/run_progen3d_agent_model_checks.sh
```

Must cover fingerprints, lifecycle, differences, selected-source composition, validation invalidation, and approval invalidation.

### 14.2 Provider Tests

```bash
./tests/run_progen3d_agent_provider_checks.sh
```

Must use scripted or transport-stubbed providers and require no live network.

### 14.3 Runtime Tests

```bash
./tests/run_progen3d_agent_runtime_checks.sh
```

Must cover asynchronous provider work, independent candidate generation, stale results, cancellation, and shutdown.

### 14.4 Validation Tests

```bash
./tests/run_progen3d_agent_validation_checks.sh
```

Must compile representative valid and invalid grammar candidates with fixed nonces.

### 14.5 Publication Tests

```bash
./tests/run_progen3d_agent_publication_checks.sh
```

Must prove exact source/snapshot publication, source-drift rejection, preparation failure atomicity, and rollback.

### 14.6 Preview Tests

```bash
./tests/run_progen3d_agent_preview_checks.sh
```

Must prove candidate activation and restoration without canonical ownership replacement.

### 14.7 Transaction Tests

```bash
./tests/run_progen3d_agent_transaction_checks.sh
```

Must prove append-only hash-chain integrity and restart recovery.

### 14.8 Consolidated Agent Gate

Add:

```bash
./tests/run_progen3d_agent_checks.sh
```

It will run all focused agent scripts plus inherited AI, generation, preview, and source-contract checks.

### 14.9 Full Application Verification

After each authority-bearing milestone:

```bash
make -j2 progen3d-editor-gui
./tests/run_authentication_ai_workflow_checks.sh
./tests/run_completed_scene_generation_publication_checks.sh
./tests/run_editor_scene_generation_runtime_checks.sh
./tests/run_preview_session_checks.sh
./tests/run_editor_architecture_checks.sh
./tests/run_gui_smoke_check.sh
./tests/run_temporal_gui_smoke_check.sh
```

After MVP completion:

```bash
./tests/run_progen3d_agent_checks.sh
./tests/run_release_checks.sh
```

An interrupted or timed-out command has unknown final status and cannot be reported as passed.

## 15. Agent GUI Smoke Scenario

The GUI smoke must use `ScriptedDesignAgentProvider` and perform this deterministic scenario:

1. Open a known valid grammar.
2. Submit a repair request.
3. Receive a scripted two-difference proposal.
4. Observe asynchronous provider completion.
5. Validate the full proposal with fixed candidate nonce.
6. Confirm published scene identity has not changed.
7. Deselect one difference.
8. Confirm prior evidence and approval are cleared.
9. Validate selected source.
10. Activate candidate preview.
11. Confirm viewport is labeled as candidate and canonical snapshot owner is unchanged.
12. Return to published preview.
13. Approve exact candidate.
14. Publish approved candidate.
15. Confirm source fingerprint, published design nonce, and transaction identity match.
16. Roll back.
17. Confirm previous source is restored and normal generation completes.

The scenario must not require Firebase credentials or a live backend.

## 16. Requirement-to-Evidence Matrix

| Requirement | Primary evidence |
|---|---|
| Provider cannot mutate editor state | source-contract gate plus provider harness |
| Request context is immutable | request assembly harness |
| Exact source identity uses SHA-256 | fingerprint harness and known vectors |
| Provider work does not block UI | request coordinator/frame-cycle harness |
| Candidate generation is isolated | candidate runtime harness |
| Invalid proposal cannot be approved | validation and approval harnesses |
| Selected changes are revalidated | review-session and validation harnesses |
| Candidate preview preserves canonical ownership | preview harness and GUI smoke |
| Document drift rejects approval/publication | approval/publication harnesses |
| Published scene matches reviewed candidate | publication harness comparing fingerprint and nonce |
| Failure retains current source and scene | publication failure harness |
| Human action is required | approval service and presentation source gates |
| Rollback is explicit and reversible | rollback harness and GUI smoke |
| Transaction history is append-only | transaction repository harness |
| Existing editor behavior remains compatible | inherited focused checks, full build, GUI and temporal smokes |

## 17. Source-Contract Rules

The new source gate must reject:

- `publishEditedSource`, `publishValidatedSource`, or `replaceSourceText` calls from provider adapters;
- document publication calls from `AiAssistantPanel` or `AiGrammarProposalPanel`;
- `replacePublishedScene` calls from provider, request assembly, validation, or presentation code;
- approval creation outside `AgentProposalApprovalService`;
- agent source publication outside `ApprovedAgentCandidatePublicationService`;
- direct lifecycle-state assignments from presentation code;
- backend DTO members in `Progen3dAgentSession`;
- use of `std::hash` for candidate, approval, or transaction identity;
- candidate validation through the canonical editor generation queue;
- model-generated approval flags; and
- automatic cloud save or publish after agent acceptance.

The gate must require:

- `Progen3dDesignAgent` composition in `Progen3dEditorApplication`;
- `Progen3dAgentRuntimeContext` exposure through `EditorApplicationRuntimeContext`;
- independent `AgentCandidateGenerationRuntime` ownership;
- exact approval checks in `ApprovedAgentCandidatePublicationService`;
- successful publication delegation through `PreparedScenePreviewPublicationService`;
- source and candidate fingerprint comparison; and
- candidate preview restoration calls.

## 18. Release Sequence

### Release A: Read-Only Design Agent

Includes:

- Milestones 0-3;
- `ExplainSelection` and `DiagnoseGrammar`;
- asynchronous provider calls; and
- no publication controls.

### Release B: Validated Proposal Agent

Includes:

- Milestone 4;
- `RepairGrammar` and `DraftGrammar`;
- exact candidate compilation and evidence; and
- review without publication.

### Release C: Governed Authoring Agent

Includes:

- Milestones 5-7;
- exact approval;
- exact candidate publication;
- candidate preview;
- rollback; and
- append-only transaction persistence.

### Release D: Expanded Design Operations

Includes:

- Milestones 8-9;
- final compatibility cleanup;
- variant exploration;
- evidence-constrained reference workflows; and
- optional local provider support.

## 19. Completion Standard

The ProGen3D agent MVP is complete only when all statements below are supported by current evidence:

- `Progen3dDesignAgent` is composed by the application and used by the UI.
- Provider responses are represented by provider-neutral domain models.
- Provider calls and candidate generation do not block the interactive frame.
- Candidate compilation is isolated from canonical generation publication.
- Invalid candidates retain current source and the last valid scene.
- Complete and selected proposals are validated as exact source candidates.
- Candidate evidence includes local source fingerprint, nonce, diagnostics, and measured scene facts.
- Candidate preview is reversible and does not replace canonical scene ownership.
- Human approval is bound to the exact candidate fingerprint.
- Source drift, candidate drift, stale request results, and nonce mismatch fail closed.
- Publication transfers the exact reviewed candidate rather than generating an unreviewed stochastic replacement.
- Every accepted proposal has an append-only transaction record and explicit rollback path.
- Existing AI, generation, preview, editor architecture, GUI, and temporal checks pass.
- The complete `progen3d-editor-gui` build succeeds from current source.
- The requirement-to-evidence matrix has no unverified MVP requirement.

Focused green harnesses, provider success, or a visually plausible candidate do not independently establish completion.

## 20. Recommended First Implementation Slice

Begin with Milestones 0 and 1 only:

1. add the agent source-contract gate;
2. add SHA-256 grammar fingerprints;
3. add provider-neutral request, proposal, validation, approval, and transaction models;
4. add the proposal review session with evidence invalidation; and
5. add focused model harnesses.

Do not modify provider calls, scene generation, OpenGL preview, or document publication in this first slice. Establishing exact identity and state ownership first makes every later authority-bearing change testable and reviewable.
