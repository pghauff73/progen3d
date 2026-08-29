# Validation report

- Current-syntax examples: **57**
- Expected-invalid diagnostic fixtures: **15**
- Proposed AxialProfilev1 examples: **13**
- Proposed future curved examples: **6**

The corpus passed the generated static checks and was executed through the full
Progen3D GUI on **Friday, August 21, 2026**. Each file ran in its own transient
user-systemd unit with the XDG portal backend initialized. Time-driven files used
`--temporal-smoke-test`; other intended-valid files used `--smoke-test`.

- Intended-valid examples accepted: **80 / 80**
- Runtime `I()` coverage: **80 / 80 intended-valid examples**
- Explicit immovable supports: **17 examples with one `!I()` anchor**
- Expected-invalid diagnostic fixtures rejected: **15 / 15**
- XDG portal initialization: **95 / 95**
- Total supervised verdicts: **95 / 95 PASS**
- Tested binary SHA-256: `98ad9fc09a53c6fa8aa72806a5cc9561535d278cc2d9ec212accc40540d1811e`
- Machine-readable evidence: `tests/evidence/examples_supervisor_matrix_2026-08-21.json`
- Tabular evidence: `tests/evidence/examples_supervisor_matrix_2026-08-21.tsv`

The `diagnostic_fixtures/` files are intentionally invalid; their PASS verdict
means Progen3D returned a blocking grammar diagnostic and exit code 2.

- PASS `current/00_basics/001_hello_cube.p3d`
- PASS `current/00_basics/002_primitive_gallery.p3d`
- PASS `current/00_basics/003_translate_scale.p3d`
- PASS `current/00_basics/004_rotations.p3d`
- PASS `current/00_basics/005_rotation_bounds.p3d`
- PASS `current/00_basics/006_scopes.p3d`
- PASS `current/01_rules/010_parameterized_rule.p3d`
- PASS `current/01_rules/011_nested_rules.p3d`
- PASS `current/01_rules/012_repeat_single_section.p3d`
- PASS `current/01_rules/013_setup_body_tail.p3d`
- PASS `current/01_rules/014_two_sections_setup_body.p3d`
- PASS `current/01_rules/015_probability_alternate.p3d`
- PASS `current/01_rules/016_header_reroll.p3d`
- PASS `current/02_random/020_random_continuous.p3d`
- PASS `current/02_random/021_random_integer.p3d`
- PASS `current/02_random/022_ampersand_resample.p3d`
- PASS `current/02_random/023_random_tower_field.p3d`
- PASS `current/03_expressions/030_arithmetic.p3d`
- PASS `current/03_expressions/031_trigonometry.p3d`
- PASS `current/03_expressions/032_min_max_abs_sqrt.p3d`
- PASS `current/03_expressions/033_clamp_wrap_lerp.p3d`
- PASS `current/03_expressions/034_easing_functions.p3d`
- PASS `current/04_conditionals/040_numeric_conditional.p3d`
- PASS `current/04_conditionals/041_comparison_conditional.p3d`
- PASS `current/04_conditionals/042_iteration_conditional.p3d`
- PASS `current/05_time/050_spin.p3d`
- PASS `current/05_time/051_swing.p3d`
- PASS `current/05_time/052_oscillate.p3d`
- PASS `current/05_time/053_cycle_pingpong.p3d`
- PASS `current/05_time/054_pulse.p3d`
- PASS `current/05_time/055_accelerate_decelerate.p3d`
- PASS `current/05_time/056_animated_tower.p3d`
- PASS `current/06_materials/060_named_materials.p3d`
- PASS `current/06_materials/061_material_index.p3d`
- PASS `current/06_materials/062_alpha_texscale.p3d`
- PASS `current/06_materials/063_material_name_composition.p3d`
- PASS `current/07_physics/070_gravity_cube.p3d`
- PASS `current/07_physics/071_linear_velocity.p3d`
- PASS `current/07_physics/072_rotational_velocity.p3d`
- PASS `current/07_physics/073_mass.p3d`
- PASS `current/07_physics/074_density_cube.p3d`
- PASS `current/07_physics/075_immovable_ignores_velocity.p3d`
- PASS `current/07_physics/076_drop_scene.p3d`
- PASS `current/08_deformation/080_secondary_scale_D.p3d`
- PASS `current/08_deformation/081_cubeX_split.p3d`
- PASS `current/08_deformation/082_cubeY_split.p3d`
- PASS `current/08_deformation/083_cubeZ_split.p3d`
- PASS `current/08_deformation/084_dual_face_scale.p3d`
- PASS `current/08_deformation/085_dual_face_translate.p3d`
- PASS `current/09_architecture/090_simple_building.p3d`
- PASS `current/09_architecture/091_colonnade.p3d`
- PASS `current/09_architecture/092_tower_field.p3d`
- PASS `current/09_architecture/093_coruscant_tower24_current_proxy.p3d`
- PASS `current/09_architecture/094_skybridge.p3d`
- PASS `current/09_architecture/095_city_block.p3d`
- PASS `current/10_comprehensive/100_feature_showcase.p3d`
- PASS `current/10_comprehensive/101_procedural_city.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/400_unsupported_primitive.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/401_unclosed_scope.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/402_undefined_rule.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/403_undefined_variable.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/404_bad_rotation_axis.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/405_random_min_greater_max.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/406_reserved_time_variable.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/407_too_many_sections.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/408_too_many_alternates.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/409_division_by_zero.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/410_rule_arity_mismatch.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/411_entry_rule_has_parameters.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/412_invalid_probability.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/413_instance_missing_material.p3d`
- EXPECTED-FAIL `diagnostic_fixtures/414_invalid_integer_range.p3d`
