# OM20 Spatial Object Model Schema

## Core equation

\[\boxed{Object = Identity + Taxonomy + Hierarchy + SpatialFrame + Geometry + Boundary + Interfaces + Connections + Constraints + State + Evidence}\]

The model combines a containment tree, a typed connection graph, and a spatial-constraint graph. Geometry remains independent from object identity.

The JSON document declares schema `Progen3D-SMB-OMv2-SpatialObjectModel-v2`. Its `objects` array records every executable L0-L5 runtime object with explicit identity, hierarchy, spatial frame, geometry, boundary, runtime interfaces, connections, constraints, state, resolution-evidence contracts, and error policy. `functional_leaf_object_ids` identifies the 280 L5 records without weakening the complete runtime registry.

## Runtime evidence

Executable constraints declare an evidence contract that is verified by the compiled grammar-scene harness and written to `tests/evidence/courtyard_omv2_runtime_2026-08-22.json`. Static generation never claims that an executable declaration has already been solved.

## Geometry detail model

`geometry_detail.selected_level` uses LOD0 bounds, LOD1 coarse shape, LOD2 assembly, LOD3 component, LOD4 construction detail, and LOD5 fasteners/seals. The selected detail level participates in mesh-cache identity; object identity and world transform do not.

## ARBv1 profile references

`reference_binding_ids` links detailed objects to `OM20_ARBv1_Profile_Bindings.json`. Each binding carries purpose, dimensions, revision, source location, and an exact SHA-256 source hash.

## Taxonomy levels

| Level | Role | Definition |
|---:|---|---|
| L00 | `BuildingInstance` | Complete modern courtyard-building object instance. |
| L01 | `System` | Major building, site, service, amenity, or relationship system. |
| L02 | `Assembly` | Constructible, operational, or semantic assembly. |
| L03 | `Component` | Individually identifiable component within an assembly. |
| L04 | `FeatureOrInterface` | Geometric feature, connection interface, or policy feature. |
| L05 | `FunctionalLeafObject` | OM5 terminal object retained as the functional leaf identity. |
| L06 | `PartFamily` | Detailed part family beneath the functional leaf. |
| L07 | `PartAssembly` | Assembly of parts that realizes the leaf object. |
| L08 | `Subassembly` | Subassembly separating structure, skin, mechanism, or information. |
| L09 | `Element` | Primary element of the subassembly. |
| L10 | `Subelement` | Replaceable or independently meaningful portion of the element. |
| L11 | `DetailedFeature` | Edge, opening, surface, device, or functional feature. |
| L12 | `Interface` | Connection, support, seal, control, flow, optical, or spatial interface. |
| L13 | `LayerOrRegion` | Material layer, functional region, or metadata layer. |
| L14 | `MaterialOrInformationSystem` | Material system or information system governing the object. |
| L15 | `ConstituentOrDataField` | Material constituent, signal field, parameter, or evidence field. |
| L16 | `GeometryOrLogicPrimitive` | Lowest reusable geometry primitive or logical predicate. |
| L17 | `ConnectionOrBoundaryDetail` | Joint, seal, contact, boundary, or cardinality detail. |
| L18 | `StateOrControlPoint` | Operational, structural, environmental, or solver state point. |
| L19 | `EvidenceOrInspectionPoint` | Inspection, assurance, provenance, or calibration evidence point. |
| L20 | `AtomicRegionOrAssertion` | Atomic rendered region or atomic semantic assertion. |

## Scale

- Functional L5 objects: **280**
- Additional L6–L20 nodes: **4200**
- Deduplicated taxonomy nodes L0–L20: **4866**
- Canonical runtime object records L0–L5: **666**
- Tail templates: **39**

## Spatial object record

```text
SpatialBuildingObject
├── identity { object_id, object_name, object_class, taxonomy_path, revision, provenance }
├── hierarchy { parent_object_id, spatial_container_id, child_object_ids }
├── spatial_frame { authored pose, initial pose, parent frame, uncertainty, transform rule }
├── geometry_model { representation, constructors, selected detail, mesh/surface authority }
├── boundary_model { broad phase, positioning boundary, exact boundary }
├── interface_model { catalog ids, executable runtime ids, declaration authority }
├── connection_ids[]
├── spatial_constraint_ids[]
├── collision_policy { runtime layer, mask, query and rollback policy }
├── state { spatial, connection, operational }
├── resolution_evidence { constraint ids, evidence path, required fields }
└── error_policy { transaction, fallback, diagnostics }
```

## Deep path example

```text
L00 OM20_Building_001
    └── L01 SiteSystem
        └── L02 GroundAssembly
            └── L03 SiteGroundComponent
                └── L04 GroundSurfaceFeature
                    └── L05 GroundPlaneLeaf
                        └── L06 SiteWorkPartFamily
                            └── L07 SurfaceConstructionAssembly
                                └── L08 PavementSubassembly
                                    └── L09 SurfaceElement
                                        └── L10 BaseCourseSubelement
                                            └── L11 EdgeJointFeature
                                                └── L12 GroundInterface
                                                    └── L13 WearingLayer
                                                        └── L14 CivilMaterialSystem
                                                            └── L15 AggregateBinderConstituent
                                                                └── L16 PlanarSolidPrimitive
                                                                    └── L17 DrainageBoundaryDetail
                                                                        └── L18 SurfaceConditionPoint
                                                                            └── L19 SiteInspectionPoint
                                                                                └── L20 AtomicSiteSurfaceRegion
```

## Grammar realization

L0-L5 taxonomy nodes execute as nested `Object(...) [ ... ]` scopes. L5 geometry owns localized L20 `GEO_*` constructor calls, so primitive bindings and object frames remain independent. Semantic L20 nodes execute `OM20_SemanticLeaf()` without manufacturing fake solids.

The rear cabinet run and outdoor heat pump use executable P0 `Drop` constraints. The remaining four collision records stay authored and fail-closed until the cabinet-wall `Gap`, sink `Seat`, window `CenterContact`, and grouped solar-array `Drop` targets can be resolved without substituting aggregate or inferred geometry.
