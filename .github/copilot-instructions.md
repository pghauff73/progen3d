# ProGen3D repository instructions

## Mission and authority

ProGen3D is a procedural 3D engineering authoring and runtime platform: semantic grammars, parametric geometry, spatial object models, materials, rendering, physics and evidence-oriented validation. The modern desktop editor uses Dear ImGui, GLFW and OpenGL, not GTK. Read root and applicable nested AGENTS.md, then inspect the active branch, README.md, BUILDING.md, actual source, build definitions and tests. Executable contracts outrank stale descriptions; report conflicts rather than silently changing requirements. Keep proposals distinct from implemented functionality.

## Architecture and C++ design

- Use C++17 and the purpose-driven UML-readable object model mandated by AGENTS.md. Inheritance means genuine is-a; use composition, aggregation and association for other relationships. PascalCase noun-based classes, explicit verb-based methods and role-based variables. Avoid vague Helper/Manager/Util classes, procedural blobs and hidden responsibilities.
- Follow current ownership in src/ and include/. Inspect src/imgui_main.cpp, src/imgui_render.cpp, src/editor/, src/grammar/, src/geometry/, src/spatial/, src/building/, src/physics/, src/lighting/, src/electrical/, src/vegetation/ and src/vehicle/ as relevant. Grammar.cpp, Context.cpp and Scope.cpp remain integration points; do not assume historical token architecture is the whole runtime.
- Keep parser, semantic validation, expansion, spatial resolution, geometry generation, rendering and presentation responsibilities separate. Use explicit interfaces and application-owned state; avoid hidden process-global state and direct canonical mutation from panels. Preserve last accepted scene when candidate regeneration fails. Keep preview snapshots immutable where required.
- Inspect examples/ for executable .p3d and .grammar syntax; grammar/ is a catalog, not automatically the canonical generator input. Preserve compatibility entry points, units, transform order, seed/time behavior and diagnostic contracts. Do not invent operators or implement reference syntax accidentally. Register new sources in applicable build and test definitions.

## Maximum Information Conveyance Principle (MICP)

Preserve 100% of intended information and intrinsic design complexity as an objective, not an unverified claim. Optimise information received accurately per representation cost and human review effort; never confuse fewer lines, more polygons or visual noise with improved understanding. Apply recursively to code, grammar, geometry, UI, diagnostics and reports.

- Represent repetition through named reusable rules, shared definitions, parameters, counts and controlled variations. Keep exceptions explicit. Maintain one canonical owner for each semantic fact; derive views and reports from it. Preserve recoverability, provenance and access to underlying detail.
- Prioritise large-scale identity and spatial composition, medium-scale functional assemblies, then small-scale construction detail. A recognizable silhouette alone is insufficient; fine detail must not obscure missing functional structure. Use contextual, engineering-meaningful surface relief, curvature, seams, fillets and material transitions. Do not add arbitrary decoration or idealize real surfaces without justification. Distinguish manufacturing tolerance from visual detail.
- Use minimal sufficient planar tessellation and curvature/error-adaptive sampling for curved surfaces; preserve logical boundaries and topology. Retain analytic/parametric authority; render meshes are derived representations. Separate rendering LOD from collision and numerical simulation discretisation. Do not replace required physics resolution with visual simplification.
- In the UI expose meaningful summary first, with drill-down to exact evidence, units, parameters, object relationships and exceptions. Avoid duplicated labels, controls, reports or geometry that convey no additional information. In handoffs report unique changes and missing evidence rather than enumerating identical instances.

## Semantic object models

An object model defines a family of objects with shared meaning and structure, not just a mesh or a single arrangement of primitives. Represent variants through validated parameters and composable subobjects. E.g. Door owns a leaf, frame, handle, hinges and closure mechanism with appropriate variants, dimensions, materials, mass, use, loads and connection interfaces. Distinguish an object class from a particular instance and from its geometry.

- Assign stable IDs, class, parent, local coordinate frame, parameter ranges, units, functional roles, materials and provenance. Model containment separately from typed interface/connection, dependency, service-flow, constraint and state graphs. Represent exact interface locations, directions, support/contact and clearance; use collision policy and resolution evidence when applicable.
- Keep semantic leaves geometry-free; attach physical geometry only to its owner. Use local frames and derive world placement. Do not substitute aggregate bounds for a missing exact target or silently resolve unsupported constraints. Invalid candidates fail closed; pending evidence remains explicit.
- Use SMB-OMv1/OMv2/OMv2.1 and OM20 examples as design precedents, not universal fixed taxonomies. For generated packages inspect declared ownership before changes: OMv2 generator, OMv2.1 catalog modules and generator, or OM20 generator when present. Preserve frozen inputs; regenerate derivatives twice and compare hashes. Do not hand-edit generated output as the primary source.
- Make LOD explicit: bounds, coarse shape, assembly, component, construction detail, fasteners/seals as applicable. Avoid fake solids for taxonomy depth. Engineering certification cannot be inferred from aesthetics, collision clearance or a passing software test.

## ICO-index objectives and experiments

Use the repository/user-approved ICO definition, scale and weights if available. Search for its implementation, protocol and prior baselines before scoring. Never invent a canonical formula or report unmeasured scores. If none is available, propose a versioned experimental protocol for approval and report raw dimensions independently.

Suggested experimental dimensions: semantic coverage, geometric fidelity (multi-view silhouette, surface distances and dimensions), functional/interface correctness, multiscale detail coverage, representation redundancy/size, and human recognition or comprehension effort. Measure complexity and runtime separately. Require hard correctness, topology, safety and dimensional constraints before optimizing any scalar score. A provisional comparison can use a Pareto frontier of higher fidelity/coverage and lower redundancy/review effort, with no arbitrary aggregation. Record reference provenance, tolerances, weights if approved, views, lighting, seeds, time, hardware, sample size and holdout split. Compare baseline and candidate under identical conditions; report per-object and per-scale weaknesses, uncertainty and trade-offs. No claim of improvement without measured evidence. Keep human comprehension separate from computational proxies. Repeat across diverse grammars and unseen parameter combinations to detect overfitting. Do not optimize metrics by removing intended information, adding redundant triangles or weakening tests.

## Branch-aware builds

On Linux main, inspect Makefile and .github/workflows/ci.yml. Current documented baseline: GNU Make, C++17, Dear ImGui 1.90.1, GLFW, OpenGL, GLM, curl/OpenSSL, Native File Dialog Extended 1.3.0 portal backend and D-Bus. CI also installs Eigen, jsoncpp, libxml2 and X11 development dependencies. Use the workflow dependency list and third_party layout; do not revive GTK dependencies or edit flags merely to hide failures.

```sh
make clean
make -j2 all
make -j2 airflow-check
```

The documented outputs are progen3d-editor-gui, compatibility link progen3d, p3d_to_3dtexmesh and p3d_to_3view; verify actual targets. Use -Wall -Wextra -Wpedantic for application sources. Clean builds are mandatory after public header, source list, compiler flag or linkage changes. Never rely on retained objects as evidence.

Windows support is branch-specific: codex/windows-ucrt64 and codex/windows-lyra-greeting contain .github/workflows/windows-ucrt64.yml. On a checkout that actually provides its CMake configuration, use MSYS2 UCRT64 (not MSVC or Linux binaries) and the workflow's UCRT64 dependencies:

```sh
cmake -S . -B build/ucrt64 -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build/ucrt64 --parallel 2
ctest --test-dir build/ucrt64 --output-on-failure
```

The workflow verifies progen3d.exe, Open Lyra.cmd and staged Lyra_Greeting.p3d byte identity, then packages DLLs, cacert.pem and runtime asset directories. Keep Windows paths, Unicode, working-directory independence and runtime dependency staging correct. Do not assume Windows support or scripts already exist on main. Do not merge platform branches without approval.

## Regression and evidence contract

1. Inspect git status, commit, branch and actual test inventory. Establish a baseline; use a small failing regression or explicit acceptance case before changing production code. Isolate changes and keep existing tests intact.
2. Run relevant actual implementation harnesses, parser/grammar/runtime integration, invalid/boundary tests, and application build. Test deterministic and stochastic parameters, coordinate transforms, finite values, topology, units, collision and temporal limits as applicable. Use ASan/UBSan when supported; report environment limitations. Include GUI and visual tests for presentation changes. Exercise actual code rather than only string/source assertions or mocks.
3. Main currently contains tests/run_building_vegetation_triangle_distribution_checks.sh and MCSMv2.2 run_mcsmv22_* checks. Inspect tests/README.md, tests/fixtures/, tools/verify_mcsmv220_source_contract.py and relevant harness sources. Vegetation tests distinguish measured evidence and geometric proxies; retain this boundary. Linux CI runs make -j2 all airflow-check. Anatomical-arm CI uses python -m unittest discover -s examples/AnatomicalArm -p 'test_*.py' -v plus sequence, geometry, junction and solver audits; inspect .github/workflows/anatomical-arm.yml before use.
4. Documented but not guaranteed present: tests/run_p2_checks.sh, run_release_checks.sh, run_gui_smoke_check.sh, run_editor_architecture_checks.sh, run_document_persistence_checks.sh, run_local_file_dialog_workflow_checks.sh, SMB and courtyard gates. Check existence and content before invoking; restore missing gates only as a separately scoped change. A successful build is not a substitute for missing tests. Package gates (make package and run_conversion_package_checks.sh) are likewise conditional on actual availability.
5. Where supported, use --smoke-test or --visual-test --capture-preview /tmp/progen3d-preview.ppm --open <grammar>, under Xvfb if needed. Fix camera, fit, resolution, seed and time; capture twice, verify dimensions, visible silhouette and hashes. Compare multiple orthographic and isometric views. Within a fixed environment require repeatability; cross-driver comparisons may need declared numerical/pixel tolerances rather than byte identity.
6. Windows UCRT64 branch provides tests/run_windows_headless_checks.ps1: inspect CLI target and run with -CliPath pointing to the actual executable. It checks doctor/validate/render JSON, exit codes, same-seed repeatability, changed-seed variation, strict/permissive diagnostics, PPM payload, relative manifest paths and nonpublication on invalid/collapsed/empty output. Preserve these contracts. Do not claim headless validation ran when unavailable.
7. Record commit, exact commands, environment, exit codes, baseline/candidate measurements, changed paths, evidence artifacts and hashes. Classify each gate PASS, FAIL, BLOCKED or NOT RUN; distinguish expected negative-test exits from failures. Never fabricate evidence, suppress diagnostics, loosen thresholds to pass or update golden data without explaining and reviewing the intended change. Do not claim completion while mandatory gates remain unresolved.

## Operating boundaries and handoff

Use local/offline validation and credential-free fakes where possible. Never expose secrets, tokens or private data. Do not launch paid agent sessions, incur costs, dispatch remote CI, push, merge, publish or change subscriptions without explicit approval. Avoid unrelated formatting, generated churn and build artifacts. Summarise problem, semantic owner, implementation, tests, actual results, assumptions, limitations and next action; keep detailed evidence accessible.

## Three practical Copilot task examples

1. **Reusable door object family:** Inspect existing SMB/OM20 object and interface contracts. Extend a canonical door family with parameterized leaf/frame, hinge and handle variants, material and mass constraints, support and clearance interfaces. Preserve stable IDs and geometry-free semantic leaves. Add valid/invalid parameter fixtures, transform and collision checks, regenerate twice and compare hashes. Report multiscale fidelity and redundancy versus baseline; do not claim certification.
2. **Meridian robot ICO improvement:** Locate the authoritative Meridian grammar (ask for its path if unavailable). Establish a component-level baseline across silhouette, major assemblies, functional interfaces and meaningful smaller features. Prioritize low-scoring large/medium-scale components; use reusable grammar objects and realistic engineering relief rather than arbitrary detail. Preserve articulation limits, clearances and pilot configuration. Render fixed multiview comparisons, test parameter variants and report raw ICO dimensions or the approved index with evidence. No invented scores.
3. **Windows/Linux grammar regression:** Reproduce a malformed grammar or collapsed geometry failure with a minimal fixture. Follow parser-to-scene ownership to implement the smallest fix. Confirm structured diagnostics and last-valid-scene preservation in the editor; confirm strict headless failure publishes no image on the Windows branch where supported. Run actual platform build/test gates, repeat valid renders, record hashes and exit codes, and mark unavailable gates BLOCKED. Do not weaken acceptance criteria, push or start remote sessions.
