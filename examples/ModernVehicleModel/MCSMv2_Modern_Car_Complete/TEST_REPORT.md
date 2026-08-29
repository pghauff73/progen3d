# MCSMv2 Test Report

The reference source was tested with Python's `unittest` runner.

```text
test_inverse_fit_reduces_residual (test_mcsmv2.HybridModelTests.test_inverse_fit_reduces_residual) ... ok
test_low_resolution_body_is_watertight_and_deterministic (test_mcsmv2.HybridModelTests.test_low_resolution_body_is_watertight_and_deterministic) ... ok
test_parameter_graph_is_acyclic (test_mcsmv2.HybridModelTests.test_parameter_graph_is_acyclic) ... ok
test_wheel_envelopes_are_side_specific (test_mcsmv2.HybridModelTests.test_wheel_envelopes_are_side_specific) ... ok
test_detects_cycle (test_mcsmv2.ParameterGraphTests.test_detects_cycle) ... ok
test_evaluates_in_dependency_order (test_mcsmv2.ParameterGraphTests.test_evaluates_in_dependency_order) ... ok
test_package_width_and_height_are_built_into_sections (test_mcsmv2.SemanticSectionTests.test_package_width_and_height_are_built_into_sections) ... ok
test_sections_are_ordered_and_nonnegative (test_mcsmv2.SemanticSectionTests.test_sections_are_ordered_and_nonnegative) ... ok
test_semantic_surface_is_watertight (test_mcsmv2.SemanticSectionTests.test_semantic_surface_is_watertight) ... ok
test_station_roles_and_order (test_mcsmv2.SemanticSectionTests.test_station_roles_and_order) ... ok

----------------------------------------------------------------------
Ran 10 tests in 0.384s

OK
```

The suite covers dependency evaluation and cycle rejection, semantic station ordering and package-constrained dimensions, watertight semantic-surface generation, side-specific wheel envelopes, deterministic watertight body generation, and inverse-fit residual reduction.

## Native ProGen3D acceptance

The native implementation additionally passes source-contract, section parity,
generic implicit meshing, four-variant body topology, semantic curve/surface,
engineering object graph, executable grammar, and twelve-view silhouette gates.
The minimum native silhouette IoU is `0.967752`, above the required `0.80`.

See `MCSMv2_NATIVE_IMPLEMENTATION_VERIFICATION.md` and
`tests/evidence/mcsmv2_native_implementation_2026-08-24.json` for exact hashes,
commands, compatibility results, and the inherited MCP_OMv1 visual exception.
