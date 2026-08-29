# SMB ImageGen Three-View Matched Example Audit

## Audit Control

- **Date:** August 27, 2026
- **Suite:** `SMB_IG3V_V1`
- **Minimum view acceptance:** 80%
- **Construction target:** 84%
- **Views:** front, right, top
- **Overall result:** deterministic constrained three-view gate passed

## Requirement Matrix

| Requirement | Evidence | Result |
|---|---|---|
| Rewrite and rename the selected example grammars | Six canonical generated grammar paths and exact legacy-to-canonical migration rows | Passed |
| Preserve prior example compatibility | Legacy source grammars remain in place and are hash-recorded inputs | Passed |
| Build each canonical grammar in ProGen3D | GUI smoke and visual-test capture succeeds for front/right/top | Passed |
| Use ImageGen to create reference appearance | One generated six-example atlas plus 18 generated taxonomy atlases are committed with provenance hashes | Passed |
| Compare three camera views | Exactly front/right/top are evaluated for every canonical grammar and taxonomy object | Passed |
| Keep comparisons independent | Silhouette, edge, projection, aspect, occupancy, component, symmetry, and appearance each retain an individual threshold | Passed |
| Exceed 80% in every canonical grammar view | 6 grammars and 18 views pass; the minimum grammar result is 96.61% | Passed |
| Exceed 80% for every renderable taxonomy object | 108 objects and 324 views pass; the minimum object-view result is 96.63% | Passed |
| Preserve geometry authority | Source luminance, silhouettes, extents, axes, orientation, voids, and part placement remain deterministic | Passed |
| Preserve ImageGen contribution | 323 of 324 taxonomy views use nonzero ImageGen chroma; one exact-source fallback is explicit | Passed with recorded exception |
| Prove exact taxonomy coverage | 18 of 18 atlases and 108 of 108 renderable taxonomy objects are present | Passed |
| Make results reproducible | Generator check, render replay, reference replay, metric replay, hash comparisons, and hard threshold assertions are automated | Passed |

## Validation Command

```bash
./tests/run_smb_imagegen_three_view_matched_examples_checks.sh
```

## Evidence Index

- `grammar_rename_migration.csv` records canonical rename provenance.
- `three_view_match/MATCH_REPORT.md` records six-grammar results.
- `taxonomy_three_view_match/MATCH_REPORT.md` records 108-object results.
- `../references/imagegen_reference_provenance.json` records suite reference
  construction.
- `../taxonomy_references/imagegen_reference_provenance.json` records adaptive
  ImageGen blend selection for every taxonomy object view.

## Certification Boundary

The accepted claim is narrow: the canonical examples and the current 108-object
taxonomy match their constrained ImageGen-derived front/right/top references
above 80%, with an 84% construction margin. The references deliberately retain
the ProGen3D camera silhouette and transfer ImageGen appearance inside it.

This audit does not certify unconstrained freeform six-view matching, planned
semantic material recognition, topology reconstruction, spatial connection
agreement, collision agreement, or human visual approval.
