# Progen3D Example Grammar Corpus

Generated: 2026-08-21

This archive is a source-grounded example corpus for the Progen3D grammar implementation represented by the uploaded C++ snapshot.

## Directory status

- `current/` — examples intended to conform to the current parser/runtime syntax.
- `diagnostic_fixtures/` — intentionally invalid regression grammars for diagnostics.
- `proposed_axialprofilev1/` — executable design and acceptance examples for **AxialProfilev1**.
- `proposed_future_curved/` — executable examples for partial, hollow, chord-cut, clipped Cylinder, and Sphere families.

## Current language highlights represented here

The current grammar supports the built-ins `Cube`, `CubeX`, `CubeY`, `CubeZ`, `Cylinder`, and `Sphere`; rule parameters; rule repetition; up to three `|` sections; one alternate production; `R` and `R*`; expression-level `&name` resampling; conditionals; bracket scopes; transforms; time functions; materials; and the current physics actions.

### Important current-source caveats

1. `{...}` is not used as source-level scoping in this corpus. Use `[...]`.
2. Every intended-valid example contains runtime `I()` geometry. `!I()` is reserved for one explicit support such as a floor, wall, foundation, slab, plaza, or anchor.
3. Cube uses box collision geometry. Closed convex Cylinder and Sphere forms use generated convex collision geometry. Open, concave, and AxialProfile procedural meshes remain immovable triangle-mesh collision targets, so their `I()` examples demonstrate runtime geometry but not dynamic contact response.
4. Preview, export, picking, bounds, and collision all resolve Cylinder, Sphere, and AxialProfile through the shared procedural geometry path.
5. `P()` density-derived mass requires closed watertight geometry. Open surfaces reject density-derived mass with a diagnostic.

## Recommended reading order

1. `current/00_basics`
2. `current/01_rules`
3. `current/02_random`
4. `current/03_expressions`
5. `current/04_conditionals`
6. `current/05_time`
7. `current/06_materials`
8. `current/07_physics`
9. `current/08_deformation`
10. `current/09_architecture`
11. `current/10_comprehensive`
12. `diagnostic_fixtures`
13. `proposed_axialprofilev1`
14. `proposed_future_curved`

See `MANIFEST.csv` and `VALIDATION.md` for per-file status.
