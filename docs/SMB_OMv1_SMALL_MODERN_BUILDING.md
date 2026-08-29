# SMB-OMv1 Small Modern Building

The executable grammar is:

```text
examples/SimpleModernBuilding/SMB_OMv1_Instance_SMB_001_FiveDeep_Positioned.p3d
```

## Representation

- All 124 supplied SMB object names are defined exactly once.
- Container objects expand their declared direct child-object set.
- Physical and semantic leaves use 48 reusable, purpose-named L1-L5 chains.
- Geometry is emitted only below a physical L5 atomic feature.
- Relationship and semantic L5 rules terminate through `SMB_NoGeometry`.
- `SMB_NoGeometry` uses a zero-repeat rule, so relationship evidence never creates fake geometry.

The building root and ordinary containers use their real child-object calls as
the decomposition tree rather than inserting synthetic container geometry.

## Positioning

- The site, structure, envelope, fixed services, and installed fixtures use `!I()`.
- Loose furniture uses collision-active `I()` with explicit mass.
- Loose contents begin `ContactAllowance` above a supporting floor.
- Gravity advances those objects into collision contact with the fixed slabs.
- Structural elements are split around intermediate slabs where practical so fixed parts meet at authored boundaries instead of depending on static-static correction.

## Configuration Guards

The `Start` rule declares deterministic configuration values for:

```text
BuildingWidth
BuildingDepth
StoreyHeight
WallThickness
SlabThickness
ContactAllowance
SiteWidth
SiteDepth
```

Because the current conditional parser supports one comparison and no logical
`&&` or `||`, validation is implemented as a staged rule chain. The checks
cover minimum and maximum building dimensions, storey height, wall/slab
thickness, collision allowance, and site clearances.

Invalid input fails closed: the building tree is not expanded, and one crossed
red marker identifies the rejected configuration in the preview.

## Semantic Corrections

The supplied decomposition contained four evident template mismatches. The
grammar corrects them while preserving the original object names:

- Bedroom floor objects use `FLOOR_FINISH`, not `BED`.
- Bedroom wardrobe objects use `WARDROBE`, not `BED`.
- Bedroom window objects use `WINDOW`, not `BED`.
- `SMB_001_HVAC_BedroomIndoorUnit` uses `HVAC_UNIT`, not `BED`.

## Verification

Run the deterministic source and runtime acceptance gate:

```bash
tests/run_smb_omv1_checks.sh
```

The gate verifies:

- 124/124 SMB object rule coverage;
- exactly one definition for each SMB object name;
- complete L1-L5 coverage for all 48 reusable templates;
- parse and expansion without diagnostics;
- finite primitive and aggregate bounds;
- both fixed and collision-positioned scene populations;
- gravity-driven downward positioning without removed objects;
- invalid configuration isolation to the validation marker;
- zero geometry for relationship and semantic leaves.

Supervisor-mode GUI verification on Friday, August 21, 2026 used:

```bash
systemd-run --user --wait --collect --pipe \
  --unit=progen3d-smb-omv1-20260821 \
  /usr/bin/bash -lc 'cd /home/pamela/Projects/ProGen3d-main && \
    timeout 45s xvfb-run -a ./progen3d-editor-gui \
      --smoke-test \
      --open examples/SimpleModernBuilding/SMB_OMv1_Instance_SMB_001_FiveDeep_Positioned.p3d'
```

The supervised run parsed 399 rules, regenerated the scene, rendered a preview
frame, passed FOV smoke validation, and exited with status `0/SUCCESS`.
