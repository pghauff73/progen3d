#pragma once

#include "editor/relationship/SourceRange.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

class ScenePrimitiveInspection
{
public:
	bool available = false;
	int instance_index = -1;
	std::string primitive_type;
	std::string material_name;
	std::array<float, 3> bounds_min{};
	std::array<float, 3> bounds_max{};
	std::array<float, 3> bounds_center{};
	std::array<float, 3> position{};
	std::array<float, 3> velocity{};
	std::array<float, 3> rotational_velocity{};
	std::array<float, 3> rotation_degrees{};
	float mass = 0.0f;
	float estimated_density = 0.0f;
	bool immovable = false;
	bool watertight = false;
	std::string volume_evidence;
	std::string collision_policy;
	std::string topology_hash;
	std::string shape_specification;
	std::size_t profile_count = 0;
	std::size_t level_count = 0;
	std::size_t hold_transition_count = 0;
	std::size_t linear_transition_count = 0;
	std::size_t step_transition_count = 0;
	std::size_t generated_vertex_count = 0;
	std::size_t generated_triangle_count = 0;
	bool closed_profile_geometry = false;
	bool vegetation_geometry = false;
	std::string vegetation_geometry_kind;
	std::size_t vegetation_path_sample_count = 0;
	float vegetation_base_radius = 0.0f;
	float vegetation_tip_radius = 0.0f;
	int vegetation_radial_segments = 0;
	std::string vegetation_cap_policy;
	std::string botanical_blade_profile;
	float botanical_blade_length = 0.0f;
	float botanical_blade_width = 0.0f;
	float botanical_blade_curvature = 0.0f;
	float botanical_blade_camber = 0.0f;
	float botanical_blade_twist_degrees = 0.0f;
	float botanical_blade_thickness = 0.0f;
	float botanical_blade_width_power = 0.0f;
	int botanical_blade_longitudinal_segments = 0;
	int botanical_blade_lateral_segments = 0;
	std::string plant_species;
	std::string plant_architecture;
	std::string plant_development_state;
	float plant_age = 0.0f;
	std::uint64_t plant_seed = 0u;
	std::string plant_detail_level;
	std::string vine_growth_mode;
	std::string vine_collision_behavior;
	std::string vine_attachment_mode;
	std::string vine_target_identifier;
	std::size_t vine_obstacle_count = 0u;
	std::size_t vine_segment_count = 0u;
	float vine_step_length = 0.0f;
	float vine_attachment_distance = 0.0f;
	std::string scatter_region_identifier;
	std::string scatter_surface_identifier;
	std::string scatter_surface_face;
	std::string scatter_orientation_mode;
	std::string scatter_placement_layer;
	std::uint32_t scatter_collision_mask_bits = 0u;
	std::uint64_t scatter_seed = 0u;
	float scatter_density = 0.0f;
	float scatter_minimum_distance = 0.0f;
	std::array<float, 2> scatter_scale_range{};
	std::size_t scatter_obstacle_count = 0u;
	SourceRange source_range;
	std::string connection_evidence;
};
