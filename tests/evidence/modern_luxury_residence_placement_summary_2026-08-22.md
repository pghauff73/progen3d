# Modern Luxury Residence Placement Evidence

- Evidence date: `2026-08-22`
- Grammar SHA-256: `8cde0c9cf9721f0959152bf67430e4e5afbaacce8369ccc533b46268ae67faae`
- Aggregate placement hash: `9412778286926412700`
- Spatial object placements: `12`
- Primitive placements: `568`
- Connections: `1`
- Constraints/resolutions: `1` / `1`
- Validation: `PASS`

## Spatial Object Placements

| Object ID | Class | State | Parent | Primitive Count | World Origin | World Bounds | Placement Hash |
|---|---|---|---|---:|---|---|---:|
| `MLR_001` | ResidentialProperty | Approximate | `<root>` | 70 | 0.000, 0.000, 0.000 | (-12.541, -0.420, -12.820) to (12.200, 10.160, 12.820) | `8881475684800593470` |
| `MLR_SITE` | BuildingSite | Approximate | `MLR_001` | 10 | 0.000, 0.000, 0.000 | (-12.541, -0.420, -12.820) to (12.200, 0.070, 12.820) | `13583482558099469046` |
| `MLR_STRUCTURE` | StructuralSystem | Approximate | `MLR_001` | 15 | 0.000, 0.000, 0.000 | (-8.950, -0.240, -9.984) to (8.950, 9.720, 9.984) | `4705698339923618161` |
| `MLR_GROUND_SLAB` | FoundationSlab | Approximate | `MLR_STRUCTURE` | 1 | 0.000, -0.240, 0.000 | (-8.612, -0.240, -9.646) to (8.612, 0.000, 9.646) | `12034134887730238682` |
| `MLR_ENVELOPE` | BuildingEnvelope | Approximate | `MLR_001` | 243 | 0.000, 0.000, 0.000 | (-9.152, 0.000, -10.166) to (9.152, 10.160, 10.166) | `11697792422702321561` |
| `MLR_GROUND_INTERIOR` | InteriorStorey | Approximate | `MLR_001` | 89 | 0.000, 0.000, 0.000 | (-8.071, 0.000, -9.105) to (8.071, 6.250, 9.105) | `5887469605484267696` |
| `MLR_KITCHEN_ISLAND` | KitchenIsland | Contact resolved | `MLR_GROUND_INTERIOR` | 4 | 1.280, 0.000, 1.981 | (-0.144, 0.000, -0.017) to (2.703, 1.100, 3.979) | `7292498966101262290` |
| `MLR_UPPER_INTERIORS` | InteriorStoreyGroup | Approximate | `MLR_001` | 37 | 0.000, 0.000, 0.000 | (-8.259, 3.220, -9.458) to (8.259, 9.500, 9.458) | `17299062445628339912` |
| `MLR_CIRCULATION` | VerticalCirculationSystem | Approximate | `MLR_001` | 34 | 0.000, 0.000, 0.000 | (-3.773, 0.100, -6.854) to (1.109, 7.570, -1.863) | `5888695853258486483` |
| `MLR_REAR_AMENITY` | OutdoorAmenitySystem | Approximate | `MLR_001` | 33 | 0.000, 0.000, 0.000 | (-10.355, -0.300, -4.040) to (-1.176, 2.130, 12.092) | `16773409490106469893` |
| `MLR_SWIMMING_POOL` | SwimmingPool | Approximate | `MLR_REAR_AMENITY` | 14 | -6.298, -0.300, 7.709 | (-9.895, -0.300, 2.573) to (-1.176, 1.250, 12.092) | `3718927986826704141` |
| `MLR_FRONT_LANDSCAPE` | LandscapeSystem | Approximate | `MLR_001` | 18 | 0.000, 0.000, 0.000 | (1.458, -0.170, -10.369) to (9.546, 2.380, 3.986) | `13129663774466727481` |

## Runtime Placement Relationships

| Connection | Type | Source | Target | State |
|---|---|---|---|---|
| `CONN_KitchenIslandSupportedByGroundSlab` | SupportedBy | `MLR_KITCHEN_ISLAND.bottomSupport` | `MLR_GROUND_SLAB.upperSurface` | Proposed |

## Validation Gates

- `PASS` `all_connection_endpoints_and_states_are_valid`
- `PASS` `all_constraints_use_supported_runtime_types`
- `PASS` `all_containment_parents_exist`
- `PASS` `all_primitive_bounds_are_valid`
- `PASS` `all_primitive_sources_are_traceable`
- `PASS` `all_primitive_transforms_are_finite`
- `PASS` `all_primitives_are_bound_to_spatial_objects`
- `PASS` `all_resolution_records_succeeded_with_evidence`
- `PASS` `all_spatial_object_frames_are_finite`
- `PASS` `all_spatial_objects_have_hierarchical_placement_envelopes`
- `PASS` `every_constraint_has_exactly_one_resolution_record`
- `PASS` `generation_has_no_diagnostics`
- `PASS` `missing_runtime_boundaries_are_container_only`
- `PASS` `no_spatial_object_is_invalid`
- `PASS` `primitive_bindings_are_unique`
- `PASS` `primitive_placements_exist`
- `PASS` `scene_bounds_are_valid`
- `PASS` `scene_context_exists`
- `PASS` `spatial_building_model_exists`
- `PASS` `spatial_object_ids_are_unique`
- `PASS` `spatial_object_placements_exist`
- `PASS` `pool_footprint_is_not_covered_by_lawn`
- `PASS` `stair_stringers_align_with_both_flights`
- `PASS` `upper_levels_preserve_stair_openings`
- `PASS` `round_white_table_top_is_horizontal_and_connected`
- `PASS` `living_sofas_do_not_intersect_coffee_table`
- `PASS` `kitchen_basin_is_on_island_side`

## Named Placement Contracts

- `PASS` `pool_footprint_is_not_covered_by_lawn` — Two lawn strips terminate outside the 7.20 x 3.24 pool-water footprint.
- `PASS` `stair_stringers_align_with_both_flights` — Each 3.20 m stringer uses the stair-flight heading and terminates beneath its final tread.
- `PASS` `upper_levels_preserve_stair_openings` — Each upper slab and finish is four perimeter segments, not a full floor plate.
- `PASS` `round_white_table_top_is_horizontal_and_connected` — The circular top is rotated 90 degrees about X and seats on the vertical pedestal.
- `PASS` `living_sofas_do_not_intersect_coffee_table` — The two sofa footprints have positive plan clearance from the coffee-table top.
- `PASS` `kitchen_basin_is_on_island_side` — The basin is offset +0.82 m from the island centre while remaining within the worktop.

## Primitive Inventory

The complete `568`-row primitive placement inventory is in `modern_luxury_residence_primitive_placements_2026-08-22.csv`.
Each row records its owning OMv2 object, runtime type, material, source rule and line, world bounds, and deterministic placement hash.

## Broad-Phase Notice

The JSON contains `43` blocking-policy AABB overlap candidates.
They are evidence for review, not automatic errors: architectural assemblies intentionally intersect at supports, envelopes, slabs, and finishes.
