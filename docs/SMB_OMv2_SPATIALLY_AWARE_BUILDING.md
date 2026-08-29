# SMB-OMv2 Spatially Aware Small Modern Building

## Purpose

SMB-OMv2 promotes the 124-object Small Modern Building from a positioned
geometry grammar into an executable spatial object model. The generated scene
retains the SMB-OMv1 hierarchy and geometry while adding stable identity, local
frames, typed interfaces, connection evidence, collision policies, deterministic
P0 placement constraints, editor inspection, and spatial preview overlays.

The authoritative SMB-OMv1 grammar remains unchanged:

```text
examples/SimpleModernBuilding/SMB_OMv1_Instance_SMB_001_FiveDeep_Positioned.p3d
```

The OMv2 generator reads that file and writes:

```text
examples/SimpleModernBuilding/SMB_OMv2_Instance_SMB_001_SpatiallyAware.p3d
examples/SimpleModernBuilding/SMB_OMv2_Instance_SMB_001/SMB_OMv2_Spatial_Object_Model.json
```

Regenerate the fixture with:

```bash
python3 examples/SimpleModernBuilding/SMB_OMv2_Instance_SMB_001/generate_smb_omv2.py
```

## Authoritative Counts

| Evidence | Count |
|---|---:|
| Stable spatial objects | 124 |
| Containment relationships | 123 |
| Primitive instances | 143 |
| Primitive-to-object bindings | 143 |
| Typed interfaces | 178 |
| Interface-level connections | 28 |
| Executable P0 constraints | 3 |
| Resolution records | 3 |
| `CubeY` instances | 105 |
| Movable legacy instances | 11 |
| Plain `Cube` instances | 0 |

Every object exposes an `inspection` interface, including containers and
geometry-free semantic relationship objects. Semantic leaves do not receive
placeholder geometry.

## CubeY Positioning Contract

SMB-OMv1 authored rectangular solids with centered `Cube` geometry. `CubeY`
instead occupies local Y from `0` to `1`, so replacing the name without changing
the transform moves every part upward by half its height and makes nested
rotation pivots appear upside down.

SMB-OMv2 preserves the original center-coordinate meaning with one explicit
conversion in the primitive helper rules:

```p3d
SMB_StaticCube(x y z width height depth materialIndex alpha textureScale) ->
    [ T(x y-height/2 z) S(width height depth) !I(CubeY index(materialIndex) alpha textureScale) ]

SMB_DynamicCube(x y z width height depth materialIndex alpha textureScale mass) ->
    [ M(mass) T(x y-height/2 z) S(width height depth) I(CubeY index(materialIndex) alpha textureScale) ]
```

The red fail-closed validation marker uses the same base-origin correction inside
its rotated child scope. Tests reject any remaining `I(Cube ...)` or
`!I(Cube ...)` form in the generated grammar.

## Object and Geometry Ownership

Each original SMB object rule becomes one `Object(...) [ ... ]` scope. Physical
leaf geometry is translated into the object's local frame before the scope is
emitted. The runtime therefore records one stable object identity independently
of whether the object owns zero, one, or several primitives.

Repeated implementation helpers are represented as aggregate objects rather
than duplicate identities:

| Aggregate object | Owned geometry |
|---|---:|
| `SMB_StairStep` | 12 stair-step primitives |
| `SMB_SolarPanel` | 8 photovoltaic panel primitives |
| `SMB_CeilingLight` | 8 ceiling-light primitives |
| `SMB_ExternalLight` | 4 external-light primitives |

This preserves the source model's 124 identities while retaining all 143
primitive instances.

## Connection Graph

The 28 typed connections consist of:

- 12 original SMB semantic relationships converted from zero-geometry rules;
- 2 support connections used by executable cabinet and outdoor-HVAC placement;
- sink support, cold-water, hot-water, and waste connections;
- 3 window-to-opening interface connections;
- 8 loose-furniture support connections.

The priority validity matrix proves:

```text
cabinet supported and wall-cleared
sink supported and connected to cold, hot, and waste plumbing
water heater connected to the hot-water riser
both indoor HVAC units served by the outdoor unit
solar array connected to the electrical system
both smoke alarms connected to their monitored floors
three windows seated in authored facade opening interfaces
all 11 movable furniture primitives supported by the correct floor object
```

Window openings are currently semantic facade interfaces. P0 does not claim
Boolean wall cutouts or exact surface-tag boundaries.

## Resolved Placement

Three constraints are solved deterministically from interface and boundary
evidence:

| Constraint | Mode | Result |
|---|---|---|
| `CP_CabinetGroundSlab` | `Drop` | Cabinet base contacts ground-slab top at Y `0.81` |
| `CP_CabinetFrontWallGap` | `Gap` | Cabinet keeps a nominal `0.01` gap from the front wall |
| `CP_HVACRoofSlab` | `Drop` | Outdoor HVAC base contacts roof-slab top at Y `7.435` |

The cabinet constraints run in priority order. The horizontal wall placement is
committed only after the floor-support residual remains valid. The downpipes are
classified as `Plumbing`, not `Envelope`, so they do not falsely participate in
the cabinet's envelope-only wall search.

Other support connections retain explicit authored placement. Their acceptance
predicate checks the object-to-floor vertical clearance against the source
grammar's fixed `ContactAllowance` of `0.03` scene units.

## Deterministic Evidence

Run the complete source and runtime acceptance harness with:

```bash
./tests/run_smb_omv2_checks.sh
```

It writes:

```text
tests/evidence/smb_omv2_acceptance_2026-08-21.json
```

The report contains object, graph, interface, geometry-binding, state, collision
query, broad-phase, refinement, clearance, residual, evidence-hash, and aggregate
topology-hash fields. The scene is generated twice; resolution and topology
hashes must match exactly.

Visual evidence is produced with spatial overlays enabled:

```bash
./tests/run_smb_omv2_visual_checks.sh
```

Two independent preview captures must be byte-identical. The resulting hash and
GUI smoke markers are written to:

```text
tests/evidence/smb_omv2_visual_2026-08-21.json
```

The collected supervisor matrix is:

```bash
./tests/run_smb_omv2_supervisor_matrix.sh
```

It covers the spatial model, queries, positioning, invalid grammar fixtures,
editor inspection, SMB-OMv1 compatibility, full SMB-OMv2 acceptance, and the GUI
visual capture.

## Editor Evidence

Open the fixture with spatial overlays enabled:

```bash
./progen3d-editor-gui \
    --spatial-overlays \
    --open examples/SimpleModernBuilding/SMB_OMv2_Instance_SMB_001_SpatiallyAware.p3d
```

The editor exposes geometry-free and geometric objects through the spatial
hierarchy, shows authored/resolution/world transforms, lists interfaces,
connections, constraints, query counts, contacts, clearances, and immutable
evidence hashes, and renders object-frame/interface/constraint/bounds overlays.

## P0 Boundaries

SMB-OMv2 P0 deliberately reports its limitations:

- placement uses axis-aligned aggregate boundaries;
- window openings are interface regions, not mesh Boolean cuts;
- only `Touch`, `Gap`, and `Drop` translation modes are authoritative;
- loose-furniture support outside the three solved constraints remains authored;
- `validated_object_count` remains zero because P0 publishes validity predicates
  in the acceptance report rather than promoting every object to a new runtime
  state;
- orientation solving, insertion, seating depth, exact contact patches, OBBs,
  convex boundaries, and triangle-boundary placement remain P1/P2 work.

These limits prevent the generated building from claiming exact geometric or
physics behavior that the P0 runtime does not implement.
