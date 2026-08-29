# Progen3D P2 Temporal Grammar and Bounded Rotation

P2 adds deterministic time to the P1 semantic engine. A grammar can now describe a scene at an explicit instant, compose time through checked motion functions, and constrain axis-angle rotations without accumulating hidden animation state.

## 1. Investigation result: `t` is a scene input, not mutable state

The most reliable temporal model is:

\[
\mathcal{S}=F(G,\sigma,t)
\]

where:

- \(G\) is the grammar source;
- \(\sigma\) is the stochastic design seed;
- \(t\) is evaluation time in seconds;
- \(\mathcal{S}\) is the complete generated scene snapshot.

This model was selected instead of incrementally changing a global time variable. It gives five useful invariants:

1. Evaluating the same source, design seed, and time produces the same action stream.
2. Scrubbing backward requires no undo operation.
3. A frame can be regenerated independently of every earlier frame.
4. Random declarations remain stable while time changes.
5. Failed time samples cannot corrupt the last valid scene.

`dt` is deliberately not exposed as a grammar variable in P2. A mutable `dt` would make output depend on frame rate, dropped frames, playback history, and the order in which samples were requested. The preview clock advances `t`, while the grammar itself remains a pure snapshot description.

## 2. Built-in time variable

`t` is an immutable built-in value measured in seconds:

```text
Start ->
    T(0 sin(t) 0)
    I(Cube material(plaster))
```

Properties:

- default value: `0`;
- negative time: supported;
- storage: per `GrammarDocument`;
- precision domain: finite values representable by the grammar's `float` action layer;
- lexical visibility: every rule;
- mutability: none;
- random resampling: `&t` is rejected;
- reserved name: `t` cannot be a rule, parameter, reroll binding, `R`, or `R*` declaration.

The public evaluation interface is in `src/GrammarRuntime.h`:

```cpp
setGrammarEvaluationTime(grammar, seconds);
getGrammarEvaluationTime(grammar);
grammarUsesTime(grammar);
getGrammarEvaluationState(grammar, &state);
```

P2 also records `t` in the immutable editor/debug snapshot registry with defining rule `<builtin-time>`.

## 3. Stable randomness through time

A normal **Run** creates a new design nonce and therefore rerolls stochastic design choices. Temporal samples reuse the current design nonce:

\[
\sigma_{\text{sample}}=\sigma_{\text{active design}}
\]

Consequently:

```text
Start ->
    R Width(2 4)
    S(Width+0.2*sin(t) 1 1)
    I(Cube material(plaster))
```

keeps `Width` fixed while the sinusoidal component moves. Running the grammar again intentionally generates another design. Physics and collision effects continue to use independent random streams inherited from P0.

## 4. Temporal and interpolation functions

Function arguments use commas. Action and rule-call arguments remain whitespace-separated.

| Function | Contract | Result |
|---|---|---|
| `cycle(t, period[, phase])` | `period > 0`; phase is in cycles | Repeating sawtooth in `[0, 1)` |
| `pingpong(t, period[, phase])` | `period > 0` | Out-and-back triangle in `[0, 1]` |
| `pulse(t, period, duty[, phase])` | `period > 0`, `duty ∈ [0,1]` | `0` or `1` |
| `accelerate(u[, exponent])` | `exponent > 0`; clamps `u` | Ease-in in `[0,1]` |
| `decelerate(u[, exponent])` | `exponent > 0`; clamps `u` | Ease-out in `[0,1]` |
| `ease(u)` | clamps `u` | Cubic smoothstep |
| `smoother(u)` | clamps `u` | Quintic smootherstep |
| `swing(t, min, max, period[, phase])` | `max >= min`, `period > 0` | Bounded triangle motion |
| `oscillate(t, min, max, period[, phase])` | `max >= min`, `period > 0` | Bounded sinusoidal motion |
| `spin(t, rate[, offset])` | finite arguments | `offset + t·rate` |
| `lerp(a, b, u)` | finite arguments | Linear interpolation, with extrapolation allowed |
| `clamp(x, min, max)` | `max >= min` | Inclusive clamp |
| `wrap(x, min, max)` | `max > min` | Value in `[min,max)` |
| `radians(degrees)` | one argument | Degrees to radians |
| `degrees(radians)` | one argument | Radians to degrees |

Existing checked functions remain available:

```text
sin cos tan abs sqrt floor ceil min max
```

Trigonometric functions consume radians. `A()` consumes degrees.

### Cycle model

For period \(P>0\) and phase \(\phi\) measured in cycles:

\[
c(t)=\operatorname{frac}\left(\frac{t}{P}+\phi\right)
\]

P2 reduces `t` with `fmod` before normalization to preserve useful phase precision over long runs and handles negative time without producing negative cycle values.

### Ping-pong model

\[
p(t)=1-|2c(t)-1|
\]

This traverses `0 → 1 → 0` over one complete period.

### Acceleration and deceleration

With \(u=\operatorname{clamp}(x,0,1)\) and exponent \(q>0\):

\[
\operatorname{accelerate}(u,q)=u^q
\]

\[
\operatorname{decelerate}(u,q)=1-(1-u)^q
\]

The default exponent is `2`.

### Smooth arrival and departure

Cubic easing:

\[
\operatorname{ease}(u)=u^2(3-2u)
\]

Quintic easing:

\[
\operatorname{smoother}(u)=u^3\left(u(6u-15)+10\right)
\]

Both functions clamp input to `[0,1]`. `ease` has zero first derivative at both endpoints. `smoother` also has zero second derivative there.

## 5. Composition patterns

### Eased out-and-back motion

```text
Start ->
    T(lerp(-4,4,ease(pingpong(t,3))) 0 0)
    I(Cube material(plaster))
```

### Continuous wrapped rotation

```text
Start ->
    A(wrap(spin(t,90),-180,180) 2 -180 180)
    I(Cube material(plaster))
```

### Bounded smooth rotation

```text
Start ->
    A(oscillate(t,-35,50,4) 1 -35 50)
    I(Cube material(plaster))
```

### Accelerating one-way interval

```text
Start ->
    T(0 lerp(0,8,accelerate(clamp(t/2,0,1),3)) 0)
    I(Cube material(plaster))
```

### Time-selected structure

```text
Start ->
    ?(pulse(t,2,0.25) == 1) Visible : Hidden

Visible -> I(Cube material(plaster))
Hidden -> S(0.001 0.001 0.001) I(Cube material(plaster))
```

A conditional may alter topology between snapshots. Every selected snapshot still passes the P0/P1 validation and generation gates.

## 6. Bounded angle rotations

The original form remains valid:

```text
A(angle axis)
```

P2 adds:

```text
A(angle axis lower upper)
```

The evaluated rotation is:

\[
\theta_{\text{applied}}=
\operatorname{clamp}(\theta,\theta_{\min},\theta_{\max})
\]

Rules:

- axis must be exactly `0`, `1`, or `2`, corresponding to X, Y, or Z;
- bounds are inclusive;
- `lower <= upper`;
- all four values may be checked expressions, including expressions using `t`;
- invalid axis or bounds abort expansion before geometry execution.

Example with moving bounds:

```text
Start ->
    A(spin(t,100) 0 -20+t 20+t)
    I(Cube material(plaster))
```

Bounds are a safety envelope, not a cycle operator. Use `swing`, `oscillate`, or `wrap` to define motion shape, then use bounded `A()` as the final invariant gate.

## 7. Preview clock and editor integration

The preview uses one authoritative clock at a time:

- a grammar that references `t` is regenerated as deterministic temporal snapshots;
- a grammar that does not reference `t` retains the existing physics simulation clock.

This avoids applying both procedural time and physics integration to the same scene during preview, which would otherwise create two competing definitions of state.

Temporal preview controls include:

- Play/Pause;
- one-frame Step;
- Reset Time;
- signed time scrubber;
- speed control;
- `grammar t` status display.

Snapshot requests are asynchronous, coalesced to the latest request, and capped at a 60 Hz wall-clock request cadence. The target `t` advances at the selected speed. Slow motion therefore remains smooth rather than reducing the rebuild cadence. Routine success diagnostics are quiet during playback.

If a temporal sample fails semantic validation, expansion, context creation, or the generation gates, playback stops and the last valid scene remains active.

## 8. Compatibility and limits

- Existing grammars that do not mention `t` retain P1 behavior.
- Existing `A(angle axis)` syntax remains valid.
- A normal Run still rerolls random design variables.
- Temporal sampling keeps random design variables stable.
- P0 recursion, action, primitive, repeat, and scope gates remain active for every sample.
- P1 lexical environments, semantic preflight, checked expressions, and matrix-authoritative transforms remain active.
- P2 performs full grammar regeneration for a temporal snapshot. This favors correctness and topology-changing animation over maximum frame throughput. A later typed scene IR could cache invariant subgraphs and evaluate only time-dependent expressions.
- The supplied project snapshot does not contain the matching application headers, complete dependency tree, or full build configuration. The actual grammar core is compiled and executed by the included contract harnesses; UI integration is covered by source-level structural regression gates.

## 9. Verification

Run all inherited and P2 gates:

```bash
./tests/run_p2_checks.sh
```

The suite includes:

- 15 P0 source/behavior gates;
- 11 P1 semantic gates;
- 14 P2 temporal source/mathematical gates;
- actual `Grammar.cpp` compilation;
- a runtime P2 grammar harness;
- AddressSanitizer and UndefinedBehaviorSanitizer when supported;
- shell and Python syntax checks.

The executable example is in:

```text
examples/P2_time_showcase.p3d
```
