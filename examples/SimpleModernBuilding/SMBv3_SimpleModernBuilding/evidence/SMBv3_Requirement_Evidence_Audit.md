# SMBv3 Requirement-to-Evidence Audit

## Audit Control

- **Date:** August 27, 2026
- **Model:** `SimpleModernBuilding Version 3`
- **Schema:** `ProGen3D-SMBv3-SimpleModernBuilding-v1`
- **Root object:** `SMB_001`
- **Result:** Passed within the explicit implementation boundary below

## Requirement Matrix

| Requirement | Implemented authority | Deterministic evidence | Result |
| --- | --- | --- | --- |
| Preserve the complete taxonomy | One `BuildingObjectSpatialProfile` per stable object ID | 124 unique profiles and 124 passing coverage rows | Passed |
| Preserve manifestation semantics | `PhysicalLeaf`, `PhysicalAggregate`, `SpatialRegion`, `SystemAggregate`, and `SemanticOnly` | 80 direct, 28 hierarchy, 16 semantic; 108 spatial manifestations | Passed |
| Collision positioning awareness | Placement policy, collision behavior, collision extent, connection points, and graph membership on every profile | All 108 spatial profiles participate; all 16 semantic profiles are explicitly non-participating | Passed |
| Extent awareness | Eight explicit channels: visual, collision, occupancy, clearance, access, motion, growth, and aggregate | Every profile has all eight channels with applicability, representation, source, and provenance | Passed |
| Spatial awareness | Parent identity, local placement policy, derived manifestation, containment membership, and accepted spatial-model association | Runtime derives the V3 profile aggregate from the accepted SMB-OMv2 and SMB-OMv2.1 aggregate | Passed |
| Orientation awareness | Canonical front, right, and up axes; orientation policy; rotational symmetry; object-local cameras | Orthogonality validation and six-view coverage for every renderable object | Passed |
| Connection points | Purpose, family, region, compatibility, connectability, availability, and clearance metadata | Every spatial profile has at least one connectable non-inspection point; semantic profiles have none | Passed |
| Connection and dependency graphs | Independent containment, spatial connection, spatial constraint, service, function, requirement, and visual-reference authorities | 123, 28, 3, 11, 9, 351, and 648 edges respectively | Passed |
| Consolidate the folder without destroying compatibility | Canonical SMBv3 package embeds the accepted source payloads and inventories every pre-V3 file | 159 files inventoried; 148 authoritative; 11 generated caches excluded from authoritative hashing | Passed |
| Deterministic generation | Single generator owns all generated V3 artifacts | Two successive generations produced identical SHA-256 manifests; `--check` passed | Passed |
| Runtime publication | Scene finalization constructs V3 only after accepted OMv2.1 construction and publishes it through scene, context, and snapshot models | Architecture harness, SMB-OMv2 runtime harness, and full editor link passed | Passed |
| Fail-closed validation | Validation rejects missing coverage, invalid extents, fabricated semantic geometry, non-orthogonal frames, invalid points, missing graph membership, and missing hashes | Native positive and negative fixtures passed | Passed |
| Frozen compatibility | SMB-OMv1, SMB-OMv2, and SMB-OMv2.1 remain inputs rather than rewritten baselines | All three compatibility suites passed after active-path consolidation | Passed |

## Canonical Evidence

- `source/SMBv3_SimpleModernBuilding.model.json` is the canonical generated aggregate.
- `generated/SMBv3_SimpleModernBuilding.grammar` is the V3-named grammar preserving the accepted executable placement boundary.
- `generated/SMBv3_SimpleModernBuilding.taxonomy.json` is the all-object spatial and camera taxonomy.
- `generated/SMBv3_Spatial_Profile_Coverage.csv` proves exact one-to-one profile coverage.
- `source/SMBv3_Building_Object_Visual_Matching_Models.json` defines the visual
  contracts for all renderable taxonomy objects.
- `generated/SMBv3_Building_Object_Image_Matching_Coverage.csv` proves exact
  one-to-one visual model assignment coverage for 108 renderable objects.
- `evidence/SMBv3_Building_Object_Image_Matching_Baseline/` records the current
  per-object, per-view diagnostic comparison.
- `evidence/SMBv3_File_Inventory.json` records folder provenance and exclusions.
- `evidence/SMBv3_Migration_Report.json` records source hashes, output hashes, graph counts, and generated validation gates.
- `tests/smbv3_spatial_profile_harness.cpp` supplies native positive and negative fixtures.
- `tests/run_smbv3_checks.sh` validates generated artifacts and compiles the native V3 kernel with warnings treated as errors.

## Validation Commands

The following commands passed on August 27, 2026:

```bash
python3 examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/generators/generate_smbv3.py --check
./tests/run_smbv3_checks.sh
./tests/run_spatial_construction_architecture_checks.sh
./tests/run_smb_omv1_checks.sh
./tests/run_smb_omv2_checks.sh
./tests/run_smb_omv21_all_objects_checks.sh
./tests/run_comv1_chair_catalog_checks.sh
./tests/run_comv1_smb_omv21_integration_checks.sh
./tests/run_modern_luxury_townhouse_v1_checks.sh
./tests/run_preview_object_camera_view_checks.sh
./tests/run_building_object_image_matching_model_checks.sh
make -j2 progen3d-editor-gui
```

## Explicit Boundary

- `Touch`, `Gap`, and `Drop` remain the accepted executable positioning solvers.
- `Seat`, `Insert`, `Tangent`, `Between`, `CenterContact`, and new orientation solvers remain declared but fail closed until native algorithms and negative fixtures are accepted.
- Current authoritative collision resolution is AABB based. Higher-accuracy boundary representations are explicit profile metadata, not promoted runtime claims.
- The 648 visual-reference edges prove six object-local view assignments for 108 renderable entities. A separate pixel-level diagnostic baseline now exists,
  with 18 of 18 atlases and 108 of 108 references, but 0 objects pass every
  required freeform six-view test at 80%; it is not certification. The separate
  constrained front/right/top suite passes 108 of 108 objects and remains
  explicitly bounded to that construction contract.
- Consolidation is logical and provenance-preserving. Legacy catalogs, variants, image references, and compatibility fixtures remain at stable paths rather than being destructively flattened into one physical file.
- Semantic-only entities remain queryable taxonomy objects with explicit non-applicability; they never receive fabricated geometry, extents, or physical connection points.
