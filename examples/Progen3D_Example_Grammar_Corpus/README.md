# Progen3D Example Grammar Corpus

Corpus status reviewed: 2026-11-27. The validation report below is historical evidence from 2026-08-21.

This directory is a partial archive of grammar documentation, diagnostic fixtures, and proposed shape examples. The historical `current/` example tree and several files listed in the validation report are not included in this checkout.

## Directory status

- `diagnostic_fixtures/` — intentionally invalid `.p3d` examples for parser and semantic diagnostics.
- `proposed_axialprofilev1/` — design fixtures; these are not current executable grammars.
- `proposed_future_curved/` — design notes for curved shape families, not current executable examples.
- `reference/` — the parser-oriented grammar writing guide.
- [`docs/ENGINEERING_GRAMMAR.md`](../../docs/ENGINEERING_GRAMMAR.md) — exhaustive implementation-oriented grammar and descriptor reference.

## Language coverage described by the guide

For the complete source syntax, recognized shape descriptor names and option arities, and implementation boundaries, see the [engineering grammar reference](../../../docs/ENGINEERING_GRAMMAR.md). The parser-oriented [writing guide](reference/Progen3D_Grammar_Writing_Guide.md) provides authoring explanations and examples. Parser recognition of a shape name does not guarantee complete geometry, rendering, collision, or physics support for that family; check the current implementation.

### Historical corpus notes

The following caveats describe the 2026-08-21 validation corpus, not examples included in this checkout:

1. `{...}` was not used as source-level scoping in that corpus; use `[...]`.
2. Every intended-valid example in that historical corpus contained runtime `I()` geometry. `!I()` was reserved for one explicit support such as a floor, wall, foundation, slab, plaza, or anchor.
3. Cube uses box collision geometry. Closed convex Cylinder and Sphere forms use generated convex collision geometry. Open, concave, and AxialProfile procedural meshes remain immovable triangle-mesh collision targets, so their `I()` examples demonstrate runtime geometry but not dynamic contact response.
4. Preview, export, picking, bounds, and collision all resolve Cylinder, Sphere, and AxialProfile through the shared procedural geometry path.
5. `P()` density-derived mass requires closed watertight geometry. Open surfaces reject density-derived mass with a diagnostic.

## Recommended reading order

1. `../../docs/ENGINEERING_GRAMMAR.md` for the implementation-oriented source language reference.
2. `reference/Progen3D_Grammar_Writing_Guide.md` for authoring explanations and examples.
3. `diagnostic_fixtures/README.md` for intentionally invalid source cases.
4. `proposed_axialprofilev1/README.md` and `proposed_future_curved/README.md` for explicitly proposed designs.

`VALIDATION.md` preserves results recorded on 2026-08-21. It describes files and a tested binary from that historical run; it is not evidence that the referenced fixture files or results are present or current in this checkout.
