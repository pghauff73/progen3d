# Progen3D Example Grammar Corpus

Corpus status reviewed: 2026-11-27. The validation report below is historical evidence from 2026-08-21.

This directory is a partial archive of grammar documentation, diagnostic fixtures, and proposed shape examples. The historical `current/` example tree and several files listed in the validation report are not included in this checkout.

## Directory status

- `diagnostic_fixtures/` — intentionally invalid `.p3d` examples for parser and semantic diagnostics.
- `proposed_axialprofilev1/` — design fixtures; these are not current executable grammars.
- `proposed_future_curved/` — design notes for curved shape families, not current executable examples.
- `reference/` — the parser-oriented grammar writing guide.

## Current language highlights represented here

The source grammar includes the built-in box names `Cube`, `CubeX`, `CubeY`, and `CubeZ`, `Cylinder` and `Sphere`, and additional shape specifications recognized by the parser. Other implemented syntax includes rule parameters and repetition, up to three `|` sections, one alternate production, `R` and `R*`, expression-level `&name` resampling, conditionals, bracket scopes, transforms, time functions, materials, and physics actions. Parser recognition of a shape name does not guarantee complete geometry, rendering, collision, or physics support for that family; check the current implementation.

### Important current-source caveats

1. `{...}` is not used as source-level scoping in this corpus. Use `[...]`.
2. Every intended-valid example contains runtime `I()` geometry. `!I()` is reserved for one explicit support such as a floor, wall, foundation, slab, plaza, or anchor.
3. Cube uses box collision geometry. Closed convex Cylinder and Sphere forms use generated convex collision geometry. Open, concave, and AxialProfile procedural meshes remain immovable triangle-mesh collision targets, so their `I()` examples demonstrate runtime geometry but not dynamic contact response.
4. Preview, export, picking, bounds, and collision all resolve Cylinder, Sphere, and AxialProfile through the shared procedural geometry path.
5. `P()` density-derived mass requires closed watertight geometry. Open surfaces reject density-derived mass with a diagnostic.

## Recommended reading order

1. `reference/Progen3D_Grammar_Writing_Guide.md` for current syntax guidance.
2. `diagnostic_fixtures/README.md` for intentionally invalid source cases.
3. `proposed_axialprofilev1/README.md` and `proposed_future_curved/README.md` for explicitly proposed designs.

`VALIDATION.md` preserves results recorded on 2026-08-21. It describes files and a tested binary from that historical run; it is not evidence that the referenced fixture files or results are present or current in this checkout.
