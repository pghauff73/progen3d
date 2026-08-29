# SMB ImageGen Three-View Matched Examples

This additive example suite gives the selected `examples/SimpleModernBuilding/`
grammars canonical names, renders deterministic orthographic front/right/top
camera views, uses ImageGen for controlled material and chroma intent, and
requires every independent metric and every view composite to exceed 80%.

The legacy grammar paths remain unchanged as compatibility inputs. The generated
canonical grammars preserve their procedural bodies and add a stable suite
identity and explicit three-view matching contract.

## Canonical Rename Map

| Canonical grammar | Preserved legacy source |
|---|---|
| `grammars/IG3V01_ModernLuxuryTownhouse.grammar` | `../ModernLuxuryTownhousev1.grammar` |
| `grammars/IG3V02_ModernLuxuryResidence.grammar` | `../Modern_Luxury_Residence_From_Image_Set.grammar` |
| `grammars/IG3V03_ModernResidenceFacade.grammar` | `../Modern_Residence_Image_Derived_Facade.grammar` |
| `grammars/IG3V04_SingleFloorModernBuilding.grammar` | `../Single_Floor_Modern_Building_Windows_Doors.grammar` |
| `grammars/IG3V05_ChairCollection.grammar` | `../COMv1_Chair_Catalog/COMv1_Chair_Catalog.p3d` |
| `grammars/IG3V06_SimpleModernBuildingV3.grammar` | `../SMBv3_SimpleModernBuilding/generated/SMBv3_SimpleModernBuilding.grammar` |

`suite_manifest.json` records exact source and canonical hashes. The grammar
generator's `--check` mode is read-only and fails when any managed output drifts.

## Camera And ImageGen Contract

- Camera views are orthographic `front`, `right`, and `top` captures with fitted
  extents, hidden grid, stable background, and fixed view order.
- Deterministic ProGen3D renders own geometry: silhouette, luminance, camera
  axis, extent, orientation, voids, and part placement.
- ImageGen owns appearance guidance: modern-luxury chroma, material character,
  restrained texture intent, vegetation color, upholstery, glass, timber,
  metal, stone, and water character.
- Reference construction transfers ImageGen chroma only inside the source
  silhouette. Edge transfer is reduced so appearance cannot rewrite geometry.
- The 108-object taxonomy fit selects the strongest ImageGen blend that keeps
  every implemented geometry metric at or above the 84% construction target.

The constrained-reference policy prevents a freeform image from silently
changing camera pose or object geometry while still retaining ImageGen-authored
appearance. Of 324 taxonomy views, 323 retain nonzero ImageGen contribution;
one fragile view uses the recorded exact-source fallback.

## Current Evidence

- Canonical example grammars passing all three views: **6 / 6**.
- Canonical example comparisons: **18 / 18**.
- Renderable taxonomy objects passing all three views: **108 / 108**.
- Taxonomy comparisons: **324 / 324**.
- Lowest taxonomy object-view score: **96.63%**.
- Required release gate: **80%**; construction target: **84%**.
- ImageGen taxonomy atlas coverage: **18 / 18 atlases**, **108 / 108 objects**.

The committed results are in `evidence/three_view_match/` and
`evidence/taxonomy_three_view_match/`. Reference construction provenance is in
`references/imagegen_reference_provenance.json` and
`taxonomy_references/imagegen_reference_provenance.json`.

## Regeneration And Acceptance

```bash
python3 examples/SimpleModernBuilding/ImageGenThreeViewMatched/generate_imagegen_three_view_matched_examples.py --check
PROGEN3D_VIEW_JOBS=1 ./tools/render_smb_three_view_grammar_suite.sh
./tests/run_smb_imagegen_three_view_matched_examples_checks.sh
```

The acceptance runner rebuilds the six camera cards and source atlas, re-ingests
the generated ImageGen imagery, reconstructs all 108 taxonomy references,
recomputes every metric, compares deterministic artifacts, and fails unless all
six grammars and all 108 objects remain above the 84% construction target.

## Certification Boundary

This suite is accepted for the constrained front/right/top contract. It does
not replace the separate freeform six-view diagnostic baseline, activate the
planned semantic material/topology/connection tests, or constitute human visual
approval of all objects. Those controls remain explicit future certification
gates rather than being inferred from high pixel-level scores.
