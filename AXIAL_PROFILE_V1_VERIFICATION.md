# AxialProfilev1 Verification

Date: August 21, 2026

## Status

```text
implementation_state: candidate_complete
external_validation_state: awaiting_approved_source_asset
promotion_state: candidate_complete_validation_pending
```

The source tree now contains the AxialProfile object model, validator, deterministic mesh generator, procedural repository dispatch, nested grammar AST/parser/evaluator, scene integration, editor catalog/autocomplete/inspection/overlay support, fixtures, benchmarks, COLLADA tooling, and release gates.

Core promotion is not claimed because the requested Coruscant `Component_24 / ID3050` source geometry and provenance are absent from this checkout.

## Focused Evidence

| Gate | Result |
| --- | --- |
| `run_axial_profile_geometry_checks.sh` | Pass |
| `run_axial_profile_grammar_scene_checks.sh` | Pass |
| `run_axial_profile_editor_checks.sh` | Pass |
| Full warning-enabled GUI build | Pass |
| Synthetic fixture set | Pass |
| Temporal `t` fixture | Pass |
| COLLADA tool syntax/CLI checks | Pass |
| `run_release_checks.sh` | Pass |
| Supervisor launch and observation | Pass |
| External Component 24 comparison | Pending approved source asset |

## Release Evidence

The complete release suite passed on August 21, 2026:

```text
./tests/run_release_checks.sh
All ProGen3D editor release checks passed.
```

Validated artifacts:

```text
progen3d-editor-gui
37d93ab4055c819ef175038f07ffeae2efc88c39ce41566ba102d0ef9cbde685

examples/axial_profile_v1_showcase.p3d
58f0996d8fb929b23af230dd9f0a70541cab99a7d902f188f591146e232142e1
```

The user supervisor unit is:

```text
/home/pamela/.config/systemd/user/progen3d-editor-gui.service
```

Observed supervisor state after launch:

```text
ActiveState=active
SubState=running
MainPID=501431
NRestarts=0
ExecMainStatus=0
```

The executable exposed by `/proc/501431/exe` matched the validated binary
SHA-256 exactly. Recent supervisor logs showed successful grammar validation and
scene regeneration for `examples/axial_profile_v1_showcase.p3d`, with no runtime
geometry error.

## Measured Benchmark

Evidence file: `tests/evidence/axial_profile_benchmark_2026-08-21.json`

```text
parse 1000 descriptors                 1.14531 ms
evaluate 1000 descriptors              7.91472 ms
construct one mesh                     0.018314 ms
first repository resolution            0.025838 ms
1000 cached repository lookups         0.053699 ms
100 unique repository resolutions      0.957411 ms
120 time-sampled mesh generations      0.690906 ms
one BVH rebuild                        0.004698 ms
1000 preview triangle traversals       0.267699 ms
```

These figures are local measurements, not portable latency guarantees. The evidence hash is:

```text
0b5a0ed07ef26da9a81ddb6aa0bde504a6024580da03fea6261cffad47ededc8
```

## Explicit Exclusions

- holes and inner profiles;
- different profile vertex counts or automatic runtime resampling;
- curved interpolation;
- arbitrary topology change;
- general CSG;
- per-surface materials;
- dynamic convex or concave collision;
- tetrahedral inertia;
- automatic certification of fitted COLLADA candidates.

## External Gate

When the approved source asset is supplied:

1. Record its exact path and SHA-256 in the validation manifest.
2. Analyze the selected geometry in local space.
3. Generate and human-review an AxialProfile candidate.
4. Export the generated preview mesh.
5. Compare cross-section RMS, maximum section deviation, volume, boundary, degeneracy, and manifold evidence.
6. Promote only if every threshold in the manifest passes and the exact evidence hashes receive human approval.
