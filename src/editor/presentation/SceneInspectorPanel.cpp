#include "editor/presentation/SceneInspectorPanel.h"

#include <imgui.h>

namespace {

void draw_vector3(const char *label, const std::array<float, 3> &value)
{
	ImGui::Text("%s: %.3f, %.3f, %.3f", label, value[0], value[1], value[2]);
}

}

void SceneInspectorPanel::draw(const ScenePrimitiveInspection &inspection,
	                           const PreviewStatistics &statistics) const
{
	ImGui::TextUnformatted("Scene Inspector");
	ImGui::Separator();
	ImGui::Text("Rules: %zu", statistics.rule_count);
	ImGui::SameLine();
	ImGui::Text("Tokens: %zu", statistics.token_count);
	ImGui::Text("Primitives: %zu", statistics.primitive_count);
	ImGui::SameLine();
	ImGui::Text("Materials: %zu", statistics.material_count);
	ImGui::Text("Generation: %.2f ms", statistics.generation_milliseconds);
	ImGui::SameLine();
	ImGui::Text("Render: %.2f ms", statistics.render_milliseconds);
	ImGui::Separator();

	if (!inspection.available) {
		ImGui::TextDisabled("Select a preview primitive to inspect its immutable scene snapshot.");
		return;
	}
	ImGui::Text("Primitive #%d: %s", inspection.instance_index, inspection.primitive_type.c_str());
	ImGui::Text("Material: %s",
	            inspection.material_name.empty() ? "<none>" : inspection.material_name.c_str());
	ImGui::Text("Mass: %.3f", inspection.mass);
	ImGui::SameLine();
	ImGui::Text("Estimated density: %.3f", inspection.estimated_density);
	ImGui::Text("Mobility: %s", inspection.immovable ? "Immovable" : "Dynamic");
	ImGui::Text("Watertight: %s", inspection.watertight ? "Yes" : "No");
	ImGui::Text("Volume evidence: %s", inspection.volume_evidence.c_str());
	ImGui::Text("Collision policy: %s", inspection.collision_policy.c_str());
	if (!inspection.topology_hash.empty()) {
		ImGui::Text("Topology hash: %s", inspection.topology_hash.c_str());
	}
	if (!inspection.shape_specification.empty()) {
		ImGui::TextWrapped("Shape: %s", inspection.shape_specification.c_str());
	}
	if (inspection.profile_count > 0) {
		ImGui::Text("Profiles: %zu", inspection.profile_count);
		ImGui::SameLine();
		ImGui::Text("Levels: %zu", inspection.level_count);
		ImGui::Text("Transitions: %zu hold, %zu linear, %zu step",
		            inspection.hold_transition_count,
		            inspection.linear_transition_count,
		            inspection.step_transition_count);
		ImGui::Text("Generated mesh: %zu vertices, %zu triangles",
		            inspection.generated_vertex_count,
		            inspection.generated_triangle_count);
		ImGui::Text("Closed profile geometry: %s",
		            inspection.closed_profile_geometry ? "Yes" : "No");
	}
	if (inspection.vegetation_geometry) {
		ImGui::Separator();
		ImGui::Text("Vegetation geometry: %s",
		            inspection.vegetation_geometry_kind.c_str());
		if (inspection.vegetation_path_sample_count > 0) {
			ImGui::Text("Path samples: %zu", inspection.vegetation_path_sample_count);
			ImGui::Text("Radius: %.4f base, %.4f tip",
			            inspection.vegetation_base_radius,
			            inspection.vegetation_tip_radius);
			ImGui::Text("Radial segments: %d", inspection.vegetation_radial_segments);
			ImGui::Text("Caps: %s", inspection.vegetation_cap_policy.c_str());
		}
		if (!inspection.botanical_blade_profile.empty()) {
			ImGui::Text("Blade profile: %s",
			            inspection.botanical_blade_profile.c_str());
			ImGui::Text("Blade size: %.4f length, %.4f width, %.4f thickness",
			            inspection.botanical_blade_length,
			            inspection.botanical_blade_width,
			            inspection.botanical_blade_thickness);
			ImGui::Text("Blade form: %.4f curvature, %.4f camber, %.2f deg twist",
			            inspection.botanical_blade_curvature,
			            inspection.botanical_blade_camber,
			            inspection.botanical_blade_twist_degrees);
			ImGui::Text("Blade tessellation: %d longitudinal, %d lateral",
			            inspection.botanical_blade_longitudinal_segments,
			            inspection.botanical_blade_lateral_segments);
			ImGui::Text("Width power: %.3f", inspection.botanical_blade_width_power);
		}
		if (!inspection.plant_species.empty()) {
			ImGui::Text("Species: %s", inspection.plant_species.c_str());
			ImGui::Text("Architecture: %s", inspection.plant_architecture.c_str());
			ImGui::Text("Development: %s at age %.3f",
			            inspection.plant_development_state.c_str(),
			            inspection.plant_age);
			ImGui::Text("Deterministic seed: %llu",
			            static_cast<unsigned long long>(inspection.plant_seed));
			ImGui::Text("Vegetation LOD: %s",
			            inspection.plant_detail_level.c_str());
		}
		if (!inspection.vine_growth_mode.empty()) {
			ImGui::Text("Vine growth: %s", inspection.vine_growth_mode.c_str());
			ImGui::Text("Collision: %s", inspection.vine_collision_behavior.c_str());
			ImGui::Text("Attachment: %s", inspection.vine_attachment_mode.c_str());
			ImGui::Text("Target: %s", inspection.vine_target_identifier.c_str());
			ImGui::Text("Growth path: %zu segments at %.4f step",
			            inspection.vine_segment_count,
			            inspection.vine_step_length);
			ImGui::Text("Surface offset: %.4f; obstacles: %zu",
			            inspection.vine_attachment_distance,
			            inspection.vine_obstacle_count);
		}
		if (!inspection.scatter_region_identifier.empty()) {
			ImGui::Text("Scatter region: %s",
			            inspection.scatter_region_identifier.c_str());
			ImGui::Text("Surface: %s / %s",
			            inspection.scatter_surface_identifier.c_str(),
			            inspection.scatter_surface_face.c_str());
			ImGui::Text("Density %.3f; separation %.3f",
			            inspection.scatter_density,
			            inspection.scatter_minimum_distance);
			ImGui::Text("Scale range: %.3f to %.3f",
			            inspection.scatter_scale_range[0],
			            inspection.scatter_scale_range[1]);
			ImGui::Text("Orientation: %s",
			            inspection.scatter_orientation_mode.c_str());
			ImGui::Text("Layer: %s; mask bits: 0x%X",
			            inspection.scatter_placement_layer.c_str(),
			            inspection.scatter_collision_mask_bits);
			ImGui::Text("Scatter seed: %llu; obstacles: %zu",
			            static_cast<unsigned long long>(inspection.scatter_seed),
			            inspection.scatter_obstacle_count);
		}
	}
	draw_vector3("Position", inspection.position);
	draw_vector3("Velocity", inspection.velocity);
	draw_vector3("Rotational velocity", inspection.rotational_velocity);
	draw_vector3("Rotation degrees", inspection.rotation_degrees);
	draw_vector3("Bounds min", inspection.bounds_min);
	draw_vector3("Bounds max", inspection.bounds_max);
	if (inspection.source_range.isValid()) {
		ImGui::Text("Source: %d:%d to %d:%d",
		            inspection.source_range.start_line + 1,
		            inspection.source_range.start_column + 1,
		            inspection.source_range.end_line + 1,
		            inspection.source_range.end_column + 1);
	}
	ImGui::TextWrapped("Connections: %s", inspection.connection_evidence.c_str());
}
