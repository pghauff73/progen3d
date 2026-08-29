# Progen3D P0 Hardening

This patch implements the immediate safety gates identified in the grammar analysis while preserving the current grammar syntax.

## Implemented gates

1. **Validation before execution**
   - Performs delimiter and parser-hostile character validation before parsing and recursive expansion.
   - Treats parser diagnostics and undefined rule references as blocking errors.
   - Runs the editor's structural validator before a scene context is created.
   - Performs a complete action and scope preflight before the first scene mutation.

2. **Scope-stack protection**
   - Rejects unmatched and mismatched scope delimiters before expansion.
   - Guards `SceneGenerationContext::popScope()` against an empty stack.
   - Refuses to publish a partially generated context after a runtime diagnostic.

3. **Bounded expansion**
   - Adds recursion-depth, rule-invocation, expansion-work, action, primitive, and repeat-count limits.
   - Aborts the entire expanded action stream rather than executing a partial result.
   - Includes the recent rule-call path in expansion failure diagnostics.

4. **Correct alternate productions**
   - Parses one-, two-, and three-section alternates using the same setup/body/tail model as primary productions.
   - Applies the parent rule's repeat count and `<Rule>_count` iteration variable to either selected production.

5. **RAII variable cleanup**
   - Restores the variable stack on every return and exception path.
   - Owns parameter bindings and rerolled variables with `std::unique_ptr`.
   - Owns and removes the iteration counter without leaking it.

6. **Undefined-rule errors**
   - Reports undefined references during semantic preflight.
   - Retains a defensive expansion-time check if malformed state bypasses preflight.

7. **Independent random streams**
   - Uses `grammar_rng` for grammar sampling and stochastic productions.
   - Uses `effects_rng` for collision particles and visual effects.
   - Seeds the grammar stream for each generation before source parsing, so physics history cannot perturb later grammar sampling.

## Limits

| Limit | Value |
|---|---:|
| Recursion depth | 256 |
| Rule invocations | 250,000 |
| Expansion work units | 1,000,000 |
| Expanded actions | 250,000 |
| Generated primitives | 50,000 |
| Repeat count per rule | 100,000 |

## Verification

Run the complete verification suite:

```bash
./tests/run_p0_checks.sh
```

The suite performs 15 source-level regression tests, syntax-checks the actual patched `Grammar.cpp`, executes its expansion and safety harness, then extracts and executes the exact patched scope-stack and random-seeding function bodies. AddressSanitizer and UndefinedBehaviorSanitizer are enabled automatically when supported. The executable checks cover primary and alternate section ordering, recursion and multiplicative budget failures, undefined rules, malformed scopes, early-return cleanup, iteration-counter cleanup, complete execution preflight, safe scope restoration, and random-stream isolation.

The supplied archive contains `.cpp` implementation files only. A complete application compile still requires the project's matching headers, third-party dependencies, and build files. These focused integration tests validate the changed grammar core and extracted safety-critical functions, but do not substitute for the complete application build.
