# Progen3D P1 Semantic Isolation

P1 builds on the P0 safety gates and separates parsing, semantic binding, expansion-time values, and scene metadata. The grammar syntax remains broadly compatible, while ambiguous or invalid programs now stop with diagnostics rather than producing plausible but incorrect geometry.

## 1. Random declarations execute during expansion

`R` and `R*` now parse into declaration tokens without sampling or mutating a process-global variable pool. A declaration is evaluated only when its production is reached in expansion order.

Consequences:

- unreachable rules no longer consume random numbers or create variables;
- a declaration cannot affect tokens that precede it;
- ranges may depend on values already visible in the lexical environment;
- random declarations never enter the scene-action VM;
- failed expansion publishes neither a partial action stream nor partial runtime snapshots.

Example:

```text
Start ->
    R Height(3 5)
    S(1 Height 1)
    I(Cube material(plaster))
```

## 2. Per-grammar lexical environments

The former process-global `variable_list` and `full_variable_list` have been removed. Each active grammar expansion owns a stack of lexical frames:

```text
grammar root frame
  └─ rule-call frame
       └─ nested rule-call frame
```

Rule parameters, rerolled values, random declarations, and `<Rule>_count` are bound in the current frame. RAII removes the complete frame on every normal return, diagnostic abort, and exception path. Two `GrammarDocument` instances can no longer read or clear one another's values.

A mutex-protected, per-grammar snapshot registry exposes the last successfully generated random values to the editor without exposing mutable runtime storage. The public contract is in `src/GrammarRuntime.h`.

## 3. Semantic symbol tables and preflight

Before expansion, P1 constructs tables for:

- rule definitions;
- parameters;
- random declarations;
- reroll bindings;
- call-graph edges;
- potentially visible lexical names.

The semantic pass blocks:

- duplicate rule definitions;
- duplicate parameters and reroll bindings;
- reserved iteration-counter collisions;
- parameter/reroll/random-binding collisions;
- undefined rule calls;
- undefined variables and unsupported functions in expressions;
- rule-call arity mismatches;
- entry rules that require arguments;
- probabilities outside `[0, 1]`;
- invalid literal and evaluated random ranges.

Runtime checks remain in place as a defensive second gate for path-dependent values.

## 4. Checked expression engine

The old substitution-and-zero fallback has been replaced with a recursive-descent evaluator supporting:

```text
+  -  *  /  ^
unary + and -
parentheses
sin cos tan abs sqrt floor ceil min max
variable lookup
&Variable random resampling
scientific notation
```

The parser reports an explicit diagnostic for:

- empty or malformed expressions;
- undefined variables;
- unsupported functions;
- missing parentheses;
- division by zero;
- negative `sqrt` arguments;
- non-finite power results;
- values outside the engine's finite `float` range, including silent underflow to zero.

Errors return `NaN` internally and abort expansion. Zero is no longer used as an error sentinel.

## 5. Range and conversion safety

P1 validates random ranges both statically when literal and dynamically when expression-based:

```text
minimum <= maximum
finite minimum and maximum
integer range contains at least one integer
integer range fits the supported C++ int domain
```

Real random sampling uses a double-precision interval calculation before conversion to `float`, avoiding overflow in `max - min`. Material-index expressions are also checked before conversion to `int`.

## 6. Explicit material intent

Instance materials now support explicit forms:

```text
I(Cube material(plaster))
I(Cube material("plaster"))
I(Cube index(FloorTex))
```

Legacy forms remain accepted:

```text
I(Cube plaster)
I(Cube FloorTex)
```

A bare identifier is resolved against semantic lexical visibility instead of whichever values happened to have been sampled while parsing. `material(...)` and `index(...)` are the deterministic forms and should be used in new grammars.

## 7. Authoritative transform metadata

The transform matrices are now the source of truth for spatial state. `Scope.cpp` derives:

- position from the transform translation column;
- primary and secondary scale magnitudes from matrix basis columns;
- a cumulative orthonormal basis using Gram-Schmidt rejection;
- Euler display metadata from a normalized quaternion.

Translation, scaling, rotation, scope copying, and `setPosition` all resynchronize metadata after modifying a matrix. `Context.cpp` derives primitive position, size, and orientation from the emitted transform, not separately accumulated scope fields.

Physics translations and rotations also recompute pose metadata from the resulting matrices. This prevents rendered geometry, collision geometry, and stored primitive pose from drifting apart.

## Compatibility notes

- P0 limits and pre-execution safety gates remain active.
- Bare material identifiers remain available for older grammars, but explicit `material(...)` or `index(...)` is recommended.
- Variables declared in a rule are lexical and do not leak into sibling calls.
- A variable must exist on the executed path before it is read.
- The uploaded source snapshot does not include the matching project headers, third-party dependencies, or complete build configuration. The included contract harnesses compile and execute the changed grammar and scope cores, plus exact extracted Context transform functions.

## Verification

Run all inherited P0 and new P1 gates:

```bash
./tests/run_p1_checks.sh
```

The script uses Clang by default and automatically enables AddressSanitizer and UndefinedBehaviorSanitizer when available. GCC can be selected with:

```bash
CXX=g++ ./tests/run_p1_checks.sh
```
