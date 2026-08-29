# Complex Modern Courtyard Building — OM20

This package rewrites the existing courtyard-building example as a **20-level executable SMB-OMv2 spatial object model** with localized procedural geometry and evidence-backed positioning.

## Version meaning

`OM20` names the **twenty-level taxonomy depth** retained by this example. `SMB-OMv2` names the **spatial object-model runtime** used by the grammar: executable objects, interfaces, typed connections, collision-position constraints, state, and deterministic resolution evidence.

## Scale

- **280** functional L5 objects
- **4,200** added L6–L20 taxonomy nodes
- **4,866** deduplicated taxonomy nodes from L0 through L20
- **4,905** executable grammar rules including `Start` and geometry helpers
- **274** estimated expanded primitive instances
- **666** executable L0-L5 `Object(...)` scopes
- **666** canonical L0-L5 runtime object records
- **291** executable inspection, support, seal, anchor, seat, and bearing interfaces
- **2** executable P0 collision-position constraints and **2** typed connections
- **39** detailed tail templates
- **22** relationship records
- **6** collision-positioning constraints
- **25** stable error codes

## Files

- `Complex_Modern_Courtyard_Building_OM20.p3d` — executable OMv2 grammar.
- `../Complex_Modern_Courtyard_Building_OM20.p3d` — byte-identical top-level compatibility entry for the same executable OMv2 grammar.
- `OM20_Taxonomy_Manifest.csv` — one row per complete L0–L20 path.
- `OM20_Node_Registry.csv` — every deduplicated taxonomy node with parent, level, rule and path.
- `OM20_Spatial_Object_Model.json` — structured spatial/object/interface/constraint/error model.
- `OM20_ARBv1_Profile_Bindings.json` — exact reference-bound window profile dimensions and source-hash provenance.
- `OM20_Relationships.csv` — typed containment, service, power, support and validation relations.
- `OM20_Collision_Positioning.csv` — moving/target interfaces, modes, direction, clearance and transaction policy.
- `OM20_Error_Catalog.csv` — stable object, geometry, interface, graph, collision and validation errors.
- `OBJECT_MODEL_SCHEMA.md` — level semantics and spatial object schema.
- `ERROR_HANDLING.md` — transactional failure, rollback and diagnostic policy.
- `VALIDATION.md` — generated static validation results.
- `generate_om20.py` — reproducible package generator.
- `../Complex_Modern_Courtyard_Building_OM20.zip` — byte-deterministic package archive with normalized timestamps and permissions.

## JSON runtime contract

`OM20_Spatial_Object_Model.json` declares schema `Progen3D-SMB-OMv2-SpatialObjectModel-v2`.
The `objects` array contains **666 canonical L0-L5 runtime object records** with explicit
identity, hierarchy, spatial frame, geometry, boundary, executable runtime interfaces,
connections, constraints, collision policy, state, resolution-evidence contracts, and error
policy. `functional_leaf_object_ids` identifies the **280** L5 functional leaves
without reducing the complete registry to geometry-bearing objects only.

## Verification

```bash
./tests/run_courtyard_omv2_checks.sh
./tests/run_courtyard_omv2_visual_checks.sh
```

The source gate regenerates both grammar entry points, the object model, and the package archive twice; it requires byte-identical hashes and rejects any compatibility-path drift. The visual gate runs two GUI captures under a user-systemd transient unit when available, requires byte-identical output, and rejects a collapsed or missing building silhouette.

Open the executable OMv2 model with spatial frames, interfaces, constraints, and bounds visible:

```bash
./progen3d-editor-gui     --spatial-overlays     --open examples/Complex_Modern_Courtyard_Building_OM20/Complex_Modern_Courtyard_Building_OM20.p3d
```

The compiled runtime evidence is written to `tests/evidence/courtyard_omv2_runtime_2026-08-22.json`. The current deterministic
GUI evidence is written to `tests/evidence/courtyard_omv2_visual_2026-08-22.json`.

## Geometry detail and references

The detailed left-front window is explicitly selected at **LOD5 FastenersAndSeals** and declares LOD0-L5 availability. Retained OM5 visual-baseline geometry is selected at **LOD1 CoarseShape**. The object-model JSON records each leaf's selected detail, bounds contract, and ARBv1 profile-binding identifiers. `OM20_ARBv1_Profile_Bindings.json` records the frame, mullion, gasket, and flashing dimensions together with the exact SHA-256 hash of `generate_om20.py`.

## Core architecture

```text
Containment tree
+
Connection graph
+
Spatial constraint graph
+
Geometry / boundary hierarchy
+
Transactional diagnostics and evidence
```

The model treats position as a derived result:

```text
Position = parent frame + interfaces + connections + constraints + collision evidence
```

All L0-L5 nodes are runtime objects. Each L5 geometry leaf owns localized L20 geometry, exposes an inspection interface, and binds generated primitives to its object identity. Existing structural and envelope boxes retain the bottom-anchored `CubeY` path so the rewritten object model preserves the proven visual baseline. The left front window is the first procedural vertical slice and now expands into double glazing, spacer seals, chamfered frame members, jamb anchors, head and sill flashings, and mullion seals using deterministic `Extrude(...)` meshes. The rear cabinet run and outdoor heat pump start from deliberately separated poses and are collision-positioned onto the kitchen floor and service pad. The cabinet-wall `Gap`, sink `Seat`, window `CenterContact`, and grouped solar-array `Drop` remain documentary sidecar constraints until their exact target geometry can be resolved without aggregate or inferred substitutes.
