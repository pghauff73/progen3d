# SMBv3 Building Object Image Matching Requirement Audit

## Audit Control

- **Date:** August 27, 2026
- **Program:** `Building Object Image Matching V1`
- **Taxonomy:** SMBv3 renderable building objects
- **Minimum view acceptance:** 80%
- **Construction target:** 84%
- **Overall result:** matching architecture passed; visual certification pending

## Requirement Matrix

| Requirement | Implemented Authority | Evidence | Result |
|---|---|---|---|
| Compare individual objects rather than only the whole scene | Object isolation hard gate, one taxonomy assignment per ID, one atlas crop per referenced ID, and one object render directory per ID | Duplicate-object synthetic test and deterministic analyzer validation | Passed |
| Compare front, back, left, right, top, and bottom | Exactly six `BuildingCameraViewMatchContract` records per model | Catalog validation and 648 comparison slots | Passed |
| Keep tests independent | One observation owns one object, view, and test kind; hard gates and numeric scores are evaluated separately | C++ harness proves a 59% silhouette failure cannot be hidden by a composite above 90% | Passed |
| Isolate unobservable views | Required and optional-degenerate semantics are explicit | C++ positive and negative fixtures; 48 current six-view failures remain isolated instead of aborting the suite | Passed |
| Model the requested building object families | 38 specialist `BuildingVisualObjectModel` records | Exact specialist-ID assertion in the acceptance runner | Passed |
| Cover the current renderable taxonomy | Generated one-to-one model assignment map | 108 unique assignments and 108 passing coverage rows | Passed |
| Supply grammar improvement directives | Each model composes ordered `BuildingGrammarImageConstructionDirective` records | Generator validation rejects models without directives | Passed |
| Supply named feature and topology expectations | Each model composes `BuildingVisualFeatureRequirement` records | Unique feature validation and generated catalog | Passed for contract; executable topology metric pending |
| Measure deterministic shape agreement | Silhouette, edge, projection, aspect, occupancy, component count, and symmetry observations | Eight synthetic metric tests and committed per-view baseline | Passed |
| Measure repetition, transparency, materials, color, and texture | Explicit planned test specifications | Catalog records implementation state as `Planned` with zero active weight | Pending |
| Validate cross-view dimensions and part identity | Explicit planned cross-view hard gate | Contract exists; executable reconstruction is not implemented | Pending |
| Validate spatial extents and connection points against images | Explicit planned extent and connection hard gates | Contracts exist; executable projection is not implemented | Pending |
| Complete all ImageGen references | Atlas manifest expects 18 atlases | 18 atlases and 108 of 108 object references are available | Passed |
| Achieve at least 80% for every required view | Fail-closed per-view and per-object evaluator | Current baseline has 0 of 108 objects passing | Failed |
| Preserve a margin above the release gate | Every model has an 84% construction target | Constrained front/right/top suite passes 108 of 108 objects with a 96.63% minimum; freeform six-view baseline remains below gate | Passed for constrained three-view scope; pending for six-view certification |
| Produce deterministic evidence | Regenerate catalog and baseline, then compare byte-for-byte | Acceptance runner passes deterministic replay | Passed |
| Require final semantic approval | Planned human-review hard gate | No certification review has been recorded | Pending |

## Requested Specialist Models

The exact specialist set is:

`vegetation`, `chair`, `exterior_wall`, `structural_floor`, `window`,
`balcony`, `door`, `couch`, `stair`, `light`, `table`, `kitchen`, `bed`,
`interior_wall`, `television`, `artwork`, `fireplace`, `pool_shell`,
`pool_water`, `paving`, `privacy_screen`, `fence`, `study`, `toilet`,
`vanity`, `shower`, `curtain`, `curtain_rod`, `cushion`, `vase`, `cabinet`,
`tap`, `gutter`, `downpipe`, `ground_drain`, `sink`, `desk`, and `wardrobe`.

The catalog additionally supplies 11 generic models so every current renderable
SMBv3 taxonomy object has exactly one visual contract. A generic model is not a
substitute for a requested specialist model.

## Validation Command

```bash
./tests/run_building_object_image_matching_model_checks.sh
```

## Certification Boundary

This audit accepts the object model, test architecture, complete reference
coverage, deterministic six-view baseline, and constrained front/right/top
construction evidence. The constrained suite passes 108 of 108 objects, but it
preserves ProGen3D geometry and transfers ImageGen appearance inside that
geometry. It therefore does not certify freeform six-view equivalence.

Full visual certification still requires correction of all 48 six-view
observability failures, activation of the planned semantic tests, all 108
objects passing every required six-view test, deterministic rerun evidence, and
human semantic approval.
