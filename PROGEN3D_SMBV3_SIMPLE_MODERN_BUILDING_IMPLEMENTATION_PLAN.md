# ProGen3D SMBv3 SimpleModernBuilding Version 3 Implementation Plan

## Document Control

- **Date:** August 27, 2026
- **Canonical model name:** `SimpleModernBuilding Version 3`
- **Canonical abbreviation:** `SMBv3`
- **Canonical instance:** `SMB_001`
- **Target schema:** `ProGen3D-SMBv3-SimpleModernBuilding-v1`
- **Compatibility inputs:** frozen SMB-OMv1, SMB-OMv2, and SMB-OMv2.1 artifacts

## 1. Objective

Consolidate the material in `examples/SimpleModernBuilding` behind one governed
SMBv3 package and give every authoritative taxonomy entity an explicit contract
for:

- collision positioning;
- visual, collision, clearance, access, motion, and aggregate extents;
- parent-relative and world spatial awareness;
- canonical local orientation;
- connection points;
- containment, connection, constraint, service, functional, requirement, and
  visual-reference graph membership.

SMBv3 must preserve all 124 stable identities. It must not invent collision
geometry for semantic assertions or convert rooms into solid obstacles.

## 2. Compatibility Boundary

SMB-OMv1, SMB-OMv2, and SMB-OMv2.1 remain immutable compatibility inputs.
SMBv3 is additive and generated from those inputs. Existing paths stay usable
until an explicit reviewed migration removes them.

The canonical V3 label is `SimpleModernBuilding`. The legacy
`SmallModernBuilding` label remains a compatibility alias. Stable object IDs,
including `SMB_001`, do not change.

## 3. Current Authoritative Scope

The current visual taxonomy defines:

- 124 total taxonomy entities;
- 80 direct-geometry objects;
- 28 hierarchy-geometry objects;
- 16 non-renderable semantic entities;
- 108 renderable entities;
- six object-local comparison views and 648 expected comparisons.

SMB-OMv2 additionally supplies 123 containment relationships, 28 typed spatial
connections, and three executable P0 positioning constraints. SMB-OMv2.1
supplies the classification, role, function, service, requirement, scenario,
state, and evidence layers.

## 4. Canonical Entity Categories

Every object receives exactly one `BuildingObjectSpatialProfile` and exactly one
manifestation category:

1. `PhysicalLeaf` — directly owns physical geometry.
2. `PhysicalAggregate` — derives its extent from physical descendants.
3. `SpatialRegion` — represents occupancy, circulation, or usable space.
4. `SystemAggregate` — derives spatial coverage from system members.
5. `SemanticOnly` — has explicit spatial non-applicability and no fabricated
   geometry.

The V3 registry contains all 124 entities. The spatial manifestation view
contains the 108 renderable physical, aggregate, and spatial-region entities.
A compatibility view may expose all 124 through the historical spatial-object
interface, but semantic entities remain non-colliding and boundary-free.

## 5. Universal Spatial Profile

Each `BuildingObjectSpatialProfile` contains:

- stable object identity and taxonomy path;
- manifestation kind and spatial applicability;
- explicit placement policy;
- explicit collision behavior;
- authored, resolved, or descendant-derived extent descriptors;
- canonical front, right, and up axes;
- orientation policy and rotational symmetry;
- named connection points and connection compatibility;
- graph memberships;
- source provenance, validation status, revision, and deterministic hash.

### 5.1 Placement Policies

- `AuthoredFixed`
- `ConstraintSolved`
- `HostedByObject`
- `DerivedFromParent`
- `AggregateFromChildren`
- `NotApplicable`

### 5.2 Collision Behaviors

- `HardBoundary`
- `SoftBoundary`
- `OccupancyRegion`
- `VoidBoundary`
- `AggregateBoundary`
- `NonBlockingAggregate`
- `NonParticipating`

### 5.3 Extent Channels

- `Visual`
- `Collision`
- `Occupancy`
- `Clearance`
- `Access`
- `Motion`
- `Growth`
- `Aggregate`

An extent records its applicability, representation, source, and provenance.
Numeric bounds are never inferred from taxonomy names. Current P0 AABBs remain
authoritative where available; higher-accuracy representations stay explicit
and pending until implemented and validated.

## 6. Connection Points

Every spatially applicable object has named reference points derived from its
local frame or boundary. Physical connection points additionally declare their
purpose, region, family, compatibility, clearance, and availability.

Connection-point families include:

- support and bearing;
- floor, wall, roof, and ceiling mounting;
- seat, insert, opening, seal, and penetration;
- hinge, swing, slide, and articulation;
- water, waste, drainage, air, electrical, data, and control;
- inspection, maintenance, operation, and emergency access;
- soil, root, trunk support, and canopy clearance.

Semantic-only entities have an empty physical connection-point collection and
an explicit `NotApplicable` status.

## 7. Independent Graph Authorities

SMBv3 keeps each semantic fact in one canonical graph:

- containment tree;
- physical spatial-connection graph;
- positioning-constraint dependency graph;
- service-flow graph;
- access and egress graph;
- functional-dependency graph;
- requirement-trace graph;
- visual-reference graph.

A derived cross-graph index may answer combined queries but cannot become a
second relationship authority.

## 8. Collision and Orientation Resolution

The deterministic positioning transaction is:

1. preserve authored local transforms;
2. resolve parent and host frames;
3. derive the applicable extent channels;
4. perform AABB broad-phase candidate discovery;
5. solve orientation constraints;
6. solve translation and contact constraints;
7. run the declared narrow-phase boundary test;
8. validate collision, clearance, access, motion, growth, and service rules;
9. commit only when every hard predicate passes;
10. otherwise preserve authored state and publish failure evidence.

`Touch`, `Gap`, and `Drop` remain the accepted translation baseline. `Seat`,
`Insert`, `Tangent`, `Between`, and `CenterContact` remain declared but are not
promoted to authoritative behavior until their native solvers and negative
fixtures pass. Orientation operations follow the same fail-closed rule.

## 9. Object-Local Camera Contract

The six views are derived from each object's canonical orientation rather than
from building-global axes:

- front and back use the local forward axis;
- left and right use the local right axis;
- top and bottom use the local up axis;
- hierarchy views frame the deterministic descendant visual extent;
- spatial regions frame their occupancy extent;
- semantic-only entities publish a non-renderable evidence record.

Rotational symmetry is recorded so equivalent views do not generate false
comparison failures.

## 10. Canonical Package

```text
examples/SimpleModernBuilding/SMBv3_SimpleModernBuilding/
├── source/
│   └── SMBv3_SimpleModernBuilding.model.json
├── generators/
│   └── generate_smbv3.py
├── generated/
│   ├── SMBv3_SimpleModernBuilding.grammar
│   ├── SMBv3_SimpleModernBuilding.taxonomy.json
│   └── SMBv3_Spatial_Profile_Coverage.csv
├── evidence/
│   ├── SMBv3_File_Inventory.json
│   ├── SMBv3_Migration_Report.json
│   └── SMBv3_Requirement_Evidence_Audit.md
└── README.md
```

The canonical model embeds the frozen spatial, knowledge, and visual-taxonomy
payloads together with the new V3 profiles and graph registry. Generated files
are never hand-edited.

The file inventory classifies every pre-V3 file as a compatibility fixture,
catalog, design variant, reference/evidence asset, generator, generated scene,
or excluded generated cache. This provides consolidation and provenance without
duplicating large image collections or breaking compatibility paths.

## 11. C++ Object Model

The native V3 layer uses purpose-specific classes:

- `BuildingExtentDescriptor`
- `BuildingObjectExtentSet`
- `BuildingOrientationProfile`
- `BuildingConnectionPoint`
- `BuildingObjectSpatialProfile`
- `BuildingSpatialProfileModel`
- `SimpleModernBuildingV3Model`
- `BuildingSpatialProfileValidationService`
- `SimpleModernBuildingV3DeterministicHashService`

`SimpleModernBuildingV3Model` composes the accepted
`SmallModernBuildingModel` compatibility aggregate and the new spatial-profile
model. It does not duplicate classification, service, requirement, or scenario
ownership.

## 12. Implementation Milestones

### Milestone 1 — Package and Inventory

- create the canonical SMBv3 directory;
- inventory and hash the existing folder;
- classify caches as non-authoritative;
- record frozen source hashes.

### Milestone 2 — All-Object Spatial Profiles

- generate exactly 124 profiles;
- preserve the 80/28/16 mode partition;
- define placement, collision, extent, orientation, and connection-point
  applicability for every entity;
- add deterministic profile hashes.

### Milestone 3 — Graph Consolidation

- import containment, connection, and constraint graphs from SMB-OMv2;
- import service, functional, requirement, scenario, and evidence relationships
  from SMB-OMv2.1;
- generate the six-view reference graph.

### Milestone 4 — Native V3 Kernel

- add the UML-readable C++ model classes;
- add exact-coverage and invariant validation;
- add deterministic aggregate hashing;
- provide positive and negative native fixtures.

### Milestone 5 — Generated Grammar

- publish one V3-named grammar derived from the accepted OMv2 grammar;
- preserve object IDs, geometry ownership, and current executable constraints;
- identify unsupported V3 solver features as pending, never as resolved.

### Milestone 6 — Acceptance and Compatibility

- regenerate artifacts twice and prove byte identity;
- run the V3 artifact and native harnesses;
- rerun OMv1, OMv2, and OMv2.1 acceptance scripts;
- audit every requirement against current evidence.

## 13. Acceptance Gates

SMBv3 is accepted only when:

- all 124 expected IDs appear exactly once;
- 80 direct-geometry, 28 hierarchy-geometry, and 16 semantic profiles remain;
- all 108 renderable entities have spatial manifestations and six views;
- every physical profile has collision, extent, placement, orientation, and at
  least one non-inspection connection point;
- every semantic profile is explicitly non-participating and boundary-free;
- all current containment, spatial-connection, constraint, service-flow, and
  functional-dependency edges are preserved;
- unresolved constraints remain fail-closed;
- no generated cache contributes to the authoritative model hash;
- generator output is deterministic;
- native positive and negative fixtures pass;
- OMv1, OMv2, and OMv2.1 compatibility gates remain green.

## 14. Completion Evidence

Required completion evidence consists of:

- the canonical V3 model JSON;
- V3 grammar and taxonomy artifacts;
- exact object-profile coverage CSV;
- file inventory and migration report;
- native V3 model and validation sources;
- deterministic generator check;
- focused native harness output;
- legacy compatibility command output;
- a final requirement-to-evidence audit.
