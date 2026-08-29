# Validation

```json
{
  "grammar_file": "Complex_Modern_Courtyard_Building_OM20.p3d",
  "lines": 32751,
  "rules_including_Start": 4905,
  "reachable_rules": 4905,
  "unreachable_rules": [],
  "taxonomy_level_rule_counts": {
    "L1": 9,
    "L2": 43,
    "L3": 140,
    "L4": 193,
    "L5": 280,
    "L6": 280,
    "L7": 280,
    "L8": 280,
    "L9": 280,
    "L10": 280,
    "L11": 280,
    "L12": 280,
    "L13": 280,
    "L14": 280,
    "L15": 280,
    "L16": 280,
    "L17": 280,
    "L18": 280,
    "L19": 280,
    "L20": 280
  },
  "functional_leaf_rows": 280,
  "additional_L6_to_L20_nodes": 4200,
  "deduplicated_taxonomy_node_registry_rows": 4866,
  "tail_templates": 39,
  "geometry_leaves": 258,
  "semantic_leaves": 22,
  "relationship_records": 22,
  "collision_positioning_constraints": 6,
  "executable_omv2_objects": 666,
  "executable_omv2_interfaces": 291,
  "executable_p0_position_constraints": 2,
  "executable_connections": 2,
  "canonical_omv2_object_records": 666,
  "runtime_interface_ids_in_object_records": 291,
  "runtime_resolution_record_contracts": 2,
  "deterministic_package_archive": true,
  "error_codes": 25,
  "estimated_expanded_primitive_instances": 274,
  "selected_geometry_detail_counts": {
    "CoarseShape": 252,
    "FastenersAndSeals": 6
  },
  "lod5_geometry_leaves": 6,
  "arbv1_profile_bindings": 4,
  "arbv1_reference_bound_objects": 6,
  "undefined_rule_references": [],
  "arity_mismatches": [],
  "delimiter_errors": [],
  "duplicate_taxonomy_paths": 0,
  "maximum_rule_name_length": 147,
  "all_functional_leaves_reach_L20": true
}
```

All L0-L5 taxonomy nodes use executable OMv2 Object scopes. L20 geometry is localized under L5 object frames. Linked runtime validation is performed by `tests/run_courtyard_omv2_checks.sh`; deterministic supervisor rendering is checked by `tests/run_courtyard_omv2_visual_checks.sh`.
