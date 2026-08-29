# SMB-OMv2.1 All Building Objects Verification

## Document Control

- **Verification date:** Monday, August 24, 2026
- **Implementation plan:** `PROGEN3D_SMB_OMV21_ALL_BUILDING_OBJECTS_IMPLEMENTATION_PLAN.md`
- **Completion evidence:** `tests/evidence/smb_omv21_all_objects_completion_2026-08-24.json`
- **Final status:** Passed
- **Release command:** `./tests/run_release_checks.sh`
- **Release marker:** `All ProGen3D editor release checks passed.`

## Result

SMB-OMv2.1 is implemented as an additive building-knowledge model over the
accepted SMB-OMv2 spatial model. The frozen SMB-OMv1 grammar remains the source
fixture, SMB-OMv2 remains the spatial authority, and SMB-OMv2.1 adds explicit
classification, role, function, service, relationship, requirement, state,
scenario, evidence, deterministic-hash, inspection, and overlay models without
changing the accepted spatial object set.

The completed aggregate contains:

| Model dimension | Verified count |
| --- | ---: |
| Source spatial objects | 124 |
| Semantic profiles | 124 |
| Concept definitions | 81 |
| Role definitions | 27 |
| Role assignments | 261 |
| Functions | 39 |
| Function allocations | 149 |
| Functional dependencies | 9 |
| Service systems | 5 |
| Service ports | 24 |
| Service flows | 11 |
| Canonical relationship assertions | 12 |
| Hard requirements | 527 |
| Passed hard requirements | 527 |
| Failed or unknown hard requirements | 0 |
| Scenarios | 11 |
| Scenario participations | 623 |
| Immutable state snapshots | 2 |
| Authoritative evidence records | 3 |

## Object-Model Implementation

The native implementation is organized into UML-readable responsibilities:

- `include/building/model/` owns immutable model records and real requirement
  subclasses such as identity, classification, containment, connection,
  clearance, collision, function, service, state, scenario, and boundary
  requirements.
- `include/building/relationship/` owns allocation, dependency, and canonical
  assertion relationships rather than hiding edges inside procedural code.
- `include/building/graph/` owns the independent function, service-flow, and
  requirement-dependency graphs.
- `src/building/service/` owns classification, function coverage, service
  topology, requirement evaluation, relationship assertion, scenario
  transition, evidence hashing, snapshot hashing, construction, validation,
  and aggregate deterministic hashing.
- `src/building/generated/SmallModernBuildingKnowledgeCatalog.cpp` is the typed
  native catalog generated from reviewed Python authoring modules.
- `src/Context.cpp` constructs and publishes the immutable building model only
  after the SMB-OMv2 spatial model is finalized and validated.
- `src/editor/service/BuildingKnowledgeInspectionService.cpp` and
  `src/editor/service/SpatialOverlayEvidenceService.cpp` expose inspection and
  opt-in visual evidence without becoming mutation authorities.

The generator remains data-driven and preserves the existing SMB-OMv2
generator. Reviewed modules separately own concept, role, function, service,
relationship, requirement, and scenario definitions. The generator emits the
JSON knowledge model, CSV coverage audit, and typed C++ catalog twice and the
acceptance gate requires byte identity.

## All-Object Coverage

`SMB_OMv21_Object_Coverage.csv` contains 124 rows and every row has
`coverage_passed=true`. The required branch counts are exact:

| Root branch | Objects |
| --- | ---: |
| `SMB_001` | 1 |
| `SMB_001_Site` | 10 |
| `SMB_001_Structure` | 11 |
| `SMB_001_GroundFloor` | 32 |
| `SMB_001_UpperFloor` | 21 |
| `SMB_001_Envelope` | 7 |
| `SMB_001_RoofZone` | 5 |
| `SMB_001_Services` | 18 |
| `SMB_001_ExternalWorks` | 5 |
| `SMB_001_Relationships` | 14 |

Every accepted object ID appears once, has one validated concept assignment,
has a complete applicability record, and has a nonzero deterministic profile
hash. Relationship and semantic objects remain geometry-free.

## Requirement-to-Evidence Audit

| Definition-of-complete clause | Evidence owner | Result |
| --- | --- | --- |
| SMB-OMv1 source remains unchanged | `run_smb_omv1_checks.sh`, frozen source SHA-256 | Passed |
| SMB-OMv1 native checks pass | `run_smb_omv1_checks.sh` | Passed |
| SMB-OMv2 generated output remains compatible | `run_smb_omv2_checks.sh` | Passed |
| SMB-OMv2 native and visual checks pass | native, visual, and supervisor evidence | Passed |
| Exactly 124 complete semantic profiles exist | JSON, CSV, native catalog harness | Passed |
| Concepts, roles, and applicability are valid | classification validation service | Passed |
| Applicable functions have allocation coverage | function coverage service | Passed |
| Functional dependencies are cycle-free | function graph validation | Passed |
| Executable service topology is valid | service topology validation | Passed |
| All hard requirements pass | 527 native re-evaluations | Passed |
| Passed requirements do not depend on unknown evidence | requirement dependency audit | Passed |
| Priority SMB-OMv2 validity is reproduced | eight first-class parity predicates | Passed |
| Relationship assertions cite canonical facts | 12 assertion records and exact diagnostics | Passed |
| State transitions are legal and service-controlled | scenario transition service | Passed |
| State snapshots are immutable and deterministic | two 124-object snapshot hashes | Passed |
| Evidence records cite explicit authority | evidence ledger and hash service | Passed |
| Required hashes repeat exactly | double generation and double construction | Passed |
| Invalid fixtures fail with exact diagnostics | native negative harness | Passed |
| All profiles are inspectable | spatial editor and knowledge inspection gates | Passed |
| Visual overlays are deterministic | two byte-identical captures | Passed |
| Visual evidence remains supporting | authority note in visual evidence | Passed |
| Hard requirement coverage has no gaps | acceptance audit reports 527/527 | Passed |
| Supervisor matrix passes | six collected cases | Passed |
| Full editor release gate passes | `run_release_checks.sh` | Passed |

## Exact Negative Coverage

The native harness requires exact validation codes and diagnostics for missing,
duplicate, and unknown profiles; undefined concepts and roles; concept,
function, service, and requirement cycles; invented geometry; uncovered hard
functions; invalid service owners, media, directions, and orphan ports; unknown
hard requirements and targets; contradictory requirement evaluations;
dependency-depth overflow; missing evaluations; non-finite evidence; invalid
scenario participants and transitions; invalid snapshot hashes; duplicate or
undefined canonical relationship facts; source-string and diagnostic ceilings;
and source-object-set mismatch.

Publication is fail-closed: a candidate aggregate is returned only after
normalization, native re-evaluation, complete validation, and deterministic
hashing succeed. Failed acceptance runs keep candidate evidence in a temporary
directory and do not replace authoritative evidence.

## Runtime and Editor Integration

The spatial model remains the canonical owner of identity, containment,
interfaces, connections, constraints, geometry bindings, and resolution
evidence. After spatial finalization, `SmallModernBuildingModelConstructionService`
constructs the additive knowledge aggregate and `GeneratedSceneSnapshot`
retains the immutable model beside the spatial snapshot.

The editor inspector can display classification, roles, function allocations,
service ports and flows, requirement status, relationship assertions, state,
scenario participation, evidence authority, and hashes for a selected spatial
object. Knowledge overlays are explicitly opt-in and can show function,
service-flow, requirement, and pending-evidence information.

The retained visual evidence selects
`SMB_001_Ground_Kitchen_Sink`, enables the knowledge overlays, captures the
preview twice, and proves byte identity. The reviewed capture is coherent and
the service-flow overlay is visible. Native graph and requirement validation,
not pixel classification, remains authoritative.

## Deterministic Evidence

| Artifact | SHA-256 |
| --- | --- |
| SMB-OMv1 frozen source | `bb2694ae73804699288adbebf17ac3c34456ebd0942f512a6806a55b5c26152d` |
| SMB-OMv2 generated grammar | `8a5f7e09e47fb77424e29033f9379c6e44142631f589420d15b7475a85052418` |
| SMB-OMv2 spatial manifest | `71649b811c78d84e61a6920eb081500dfd77a9d28947f425007e2046247021aa` |
| SMB-OMv2.1 knowledge JSON | `836ce05804ebbd1327ef93fc582fe9b6572d05dd7e124bedc82399f979a003a9` |
| SMB-OMv2.1 coverage CSV | `0c45cac88a5d6b3da1664aa30a7267643c9048dd29f7bdd7f63e249a1e995f32` |
| SMB-OMv2.1 typed C++ catalog | `0896bf38df3d70aa5ec28026144431c61b2f9ac06dac8c26eff6466f5ebb5f2c` |
| SMB-OMv2 native acceptance | `82c38950516407a2fedbd9a9354441b2acc55ef96bf4eb95ff9e0ce8800de3a0` |
| SMB-OMv2.1 all-object acceptance | `3a20afd58122c621e72c33815746b86bab3882505b008d743d94421fa5e96d26` |
| SMB-OMv2.1 visual evidence | `900cc3f3cf8e6ab50375a7b3741efa5d92dd5cfb49318d574af3cc62043841ce` |
| SMB-OMv2.1 supervisor evidence | `19f75d2d92399fb25296f38d58d0808f65b1929bf2ee25f29f84952cc1204eb5` |
| Retained reviewed PPM | `d63fd049041da1f759b60a8f461327ac35bcd40bb0ab9d9ddf3c991909c28691` |

Native deterministic hashes include aggregate
`f24124cfae08ff5e`, source object set `78141e7d236a5f00`, service topology
`48fa25057968df34`, requirements `ceff6198458dbedc`, requirement evaluations
`94935d1c4b081057`, relationships `d37887128063a872`, and snapshots
`3c195e06df86c873`.

## Validation Commands

The following gates passed on Monday, August 24, 2026:

```bash
./tests/run_building_knowledge_model_checks.sh
./tests/run_smb_omv1_checks.sh
./tests/run_smb_omv2_checks.sh
./tests/run_smb_omv21_all_objects_checks.sh
./tests/run_smb_omv2_visual_checks.sh
./tests/run_smb_omv21_visual_checks.sh
./tests/run_smb_omv21_supervisor_matrix.sh
./tests/run_release_checks.sh
```

The final `run_release_checks.sh` execution began with `make clean`, rebuilt the
GUI, ran P0/P1/P2 language gates, spatial and compatibility gates, the full
editor architecture suite, vegetation gates, GUI smoke tests, visual and
supervisor matrices, `git diff --check`, and ended successfully.

## Explicit Limits

Two service-output ports remain deliberately `PendingEvidence` and are not
treated as executable continuity claims:

- `Port.BathroomExhaust.AirOut` on `SMB_001_HVAC_BathroomExhaust`.
- `Port.Downpipes.RainwaterOut` on `SMB_001_Envelope_Downpipes`.

The release does not infer building-code compliance, energy performance,
hydraulic capacity, fire-engineering adequacy, structural performance, product
certification, or other external-domain claims. Those require new authoritative
evidence and explicit reviewed requirements. Unknown and pending evidence stay
visible and fail closed rather than being promoted from names, geometry, or
visual plausibility.
