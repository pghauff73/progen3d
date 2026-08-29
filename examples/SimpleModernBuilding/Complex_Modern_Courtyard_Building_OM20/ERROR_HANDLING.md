# OM20 Error Handling

## Transactional invariant

\[
\boxed{\text{Failed positioning or validation must not alter the last valid building state}}
\]

Every object-placement operation is staged in a `SpatialTransaction`:

```text
capture initial transforms and connection states
→ validate objects, geometry and interfaces
→ solve in temporary state
→ validate required contacts, clearances and forbidden collisions
→ commit all changes, or roll back all changes
```

## Error result model

Collision and distance queries must not return only a Boolean. A query distinguishes:

```text
Separated
Touching
Penetrating
InvalidMovingGeometry
InvalidTargetGeometry
UnsupportedPair
NumericalFailure
```

`Separated` is a valid geometric result. Invalid or unsupported geometry is an error and must never be silently interpreted as separation.

## Severity

| Severity | Meaning | Default response |
|---|---|---|
| Information | Resolution fact | Continue |
| Warning | Usable approximation or uncertainty | Commit with visible warning |
| Recoverable | Declared retry or alternative may work | Apply named recovery only |
| Blocking | Required object/constraint invalid | Roll back transaction |
| Internal | Engine invariant failed | Abort affected generation stage |

## Dependency propagation

A root error blocks dependent interfaces, connections, and subassemblies. The diagnostic panel should show one root cause plus a summarized blocked-dependency set rather than a waterfall of duplicate messages.

## Default recovery policy

- Preserve the last valid scene snapshot.
- Never move only part of a required assembly.
- Never enlarge tolerances, change targets, reverse directions, or substitute AABB collision silently.
- Preview approximations must be labelled and must not pass strict export validation.
- Record initial pose, proposed pose, geometry versions, search interval, iterations, residual, contact normal, and recovery decision.

The complete stable error-code catalog is supplied in `OM20_Error_Catalog.csv`.
