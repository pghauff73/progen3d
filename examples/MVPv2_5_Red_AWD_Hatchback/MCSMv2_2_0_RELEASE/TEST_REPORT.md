# MCSMv2.2 Test Report

## Result

```text
34 tests
34 passed
0 failures
0 errors
```

Runtime in the release environment was approximately 4 seconds for the test bodies and 5.6 seconds including interpreter startup and reporting.

## Test groups

### Geometry helpers

- vector normalization and fallback behavior;
- arbitrary-axis rigid transforms;
- transformed-point correspondence;
- watertight tyre geometry and outer-radius check.

### Suspension and wheel poses

- 32 hardpoints;
- exact left/right mirroring;
- valid rigid-transform determinants;
- front steer preservation;
- rear steer suppression;
- deterministic pose-grid cardinality.

### Semantic surface and partition

- radial semantic projection with no failed vertices;
- persistent UV bounds and one UV per vertex;
- 100% exclusive ownership;
- six glass apertures;
- six movable closure surfaces.

### Closure and glass kinematics

- six hinge systems and four glass systems;
- identity transforms at zero state;
- rigid transforms at full state;
- downward glass travel;
- direct closure and glass validation passes.

### Release acceptance

- four variants present;
- V3 assurance and release gates;
- complete partitions;
- 36 actual tyre poses per variant;
- closure, glass and hardpoint evidence;
- semantic/implicit alignment within declared thresholds;
- watertight final body STL files;
- loadable reference GLB scenes;
- complete kinematic JSON records;
- structurally balanced target grammar with no `Object(...)` call.

See [TEST_RESULTS.txt](TEST_RESULTS.txt) for the exact console transcript.
