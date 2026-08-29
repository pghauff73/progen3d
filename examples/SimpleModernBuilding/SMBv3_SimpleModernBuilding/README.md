# SMBv3 SimpleModernBuilding Version 3

This directory is the canonical consolidated package for the 124-object Simple
Modern Building. It is generated additively from the frozen SMB-OMv1,
SMB-OMv2, and SMB-OMv2.1 inputs in the parent directory.

## Canonical Artifacts

- `source/SMBv3_SimpleModernBuilding.model.json` embeds the legacy spatial,
  knowledge, and visual-taxonomy payloads and adds the 124 V3 spatial profiles.
- `generated/SMBv3_SimpleModernBuilding.grammar` is the V3-named grammar derived
  from the accepted SMB-OMv2 grammar.
- `generated/SMBv3_SimpleModernBuilding.taxonomy.json` is the object-local
  camera and spatial-profile taxonomy.
- `generated/SMBv3_Spatial_Profile_Coverage.csv` proves one profile per object.
- `source/SMBv3_Building_Object_Visual_Matching_Models.json` defines 49
  purpose-specific and fallback visual object models, six-view contracts,
  independent tests, feature requirements, and grammar construction directives.
- `generated/SMBv3_Building_Object_Image_Matching_Coverage.csv` proves one
  visual object model assignment for every renderable taxonomy object.
- `evidence/SMBv3_File_Inventory.json` classifies and hashes the pre-V3 folder.
- `evidence/SMBv3_Migration_Report.json` records counts, graph preservation,
  output hashes, validation gates, and explicit limits.
- `evidence/SMBv3_Requirement_Evidence_Audit.md` maps every SMBv3 requirement
  to current deterministic and compatibility evidence.
- `evidence/SMBv3_Building_Object_Image_Matching_Requirement_Audit.md`
  separates accepted matching architecture from pending visual certification.
- `evidence/SMBv3_Building_Object_Image_Matching_Baseline/` records the current
  independent six-view ImageGen comparison without promoting it to certification.
- `../ImageGenThreeViewMatched/` contains the additive renamed grammar suite and
  constrained front/right/top ImageGen matching evidence.

## Regeneration

```bash
python3 examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/generators/generate_smbv3.py
```

## Deterministic Check

```bash
python3 examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/generators/generate_smbv3.py --check
python3 examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/generators/generate_smbv3_visual_matching_models.py --check
```

## Acceptance

```bash
./tests/run_smbv3_checks.sh
./tests/run_building_object_image_matching_model_checks.sh
./tests/run_smb_imagegen_three_view_matched_examples_checks.sh
```

The current executable positioning authority remains `Touch`, `Gap`, and
`Drop`. The V3 model records the broader extent, orientation, connection-point,
and graph contracts without promoting unimplemented geometry or physics claims.

The image-matching layer uses independent hard gates and numeric observations.
Every required object-local view must score at least 80%, and grammar
construction targets 84% for regression margin. The current freeform six-view
diagnostic baseline has all 18 atlases and all 108 object references, but still
has 0 certified objects and 48 isolated observability failures. The separate
constrained front/right/top suite passes 108 of 108 objects with a 96.63% minimum
score; it does not replace six-view semantic certification.
