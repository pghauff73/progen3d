#include "editor/presentation/SpatialObjectInspectorPanel.h"

#include <imgui.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {

void draw_vector3(const char *label, const std::array<float, 3> &value)
{
	ImGui::Text("%s: %.4f, %.4f, %.4f", label, value[0], value[1], value[2]);
}

void draw_matrix(const char *label, const std::array<float, 16> &matrix)
{
	if (!ImGui::TreeNode(label)) return;
	for (int row = 0; row < 4; ++row) {
		ImGui::Text("% .4f  % .4f  % .4f  % .4f",
		            matrix[static_cast<std::size_t>(row)],
		            matrix[static_cast<std::size_t>(4 + row)],
		            matrix[static_cast<std::size_t>(8 + row)],
		            matrix[static_cast<std::size_t>(12 + row)]);
	}
	ImGui::TreePop();
}

std::string hierarchy_label(const SpatialObjectSelectionEntry &entry)
{
	std::string label(static_cast<std::size_t>(entry.containment_depth * 2), ' ');
	label += entry.display_name.empty() ? entry.object_id : entry.display_name;
	label += " [" + entry.object_class + "]";
	if (entry.primitive_count == 0) label += " (semantic)";
	return label;
}

void draw_text_records(
	const char *tree_id,
	const char *label,
	const std::vector<std::string> &records)
{
	if (!ImGui::TreeNode(tree_id, "%s (%zu)", label, records.size())) return;
	for (const std::string &record : records) {
		ImGui::BulletText("%s", record.c_str());
	}
	ImGui::TreePop();
}

// Same screen scale on both axes before the optional displacement magnification.
void draw_member_profile(const MemberServiceResponse &response)
{
	const ImGuiID id = ImGui::GetID("MagnifyMemberDeflection");
	ImGuiStorage *settings = ImGui::GetStateStorage();
	bool magnify = settings->GetBool(id, true);
	if (ImGui::Checkbox("Magnify displacement (25x)", &magnify)) settings->SetBool(id, magnify);
	const double amplification = magnify ? 25.0 : 1.0;
	const ImVec2 start = ImGui::GetCursorScreenPos();
	const float width = std::max(10.f, ImGui::GetContentRegionAvail().x - 4);
	const double pixels = std::min(width / response.spanM, 60.0 / (amplification * response.midspanDeflectionM));
	const auto point = [&](double fraction) {
		return ImVec2(start.x + 2 + static_cast<float>(fraction * response.spanM * pixels),
		              start.y + 4 + static_cast<float>(response.deflectionM(fraction) * pixels * amplification));
	};
	ImDrawList *lines = ImGui::GetWindowDrawList();
	lines->AddLine(point(0), point(1), ImGui::GetColorU32(ImGuiCol_TextDisabled));
	ImVec2 previous = point(0);
	for (int i = 1; i <= 48; ++i) {
		const ImVec2 next = point(i / 48.0);
		lines->AddLine(previous, next, ImGui::GetColorU32(ImGuiCol_PlotLines), 2);
		previous = next;
	}
	ImGui::Dummy(ImVec2(width, 12 + static_cast<float>(response.midspanDeflectionM * pixels * amplification)));
}

void draw_coaxial_drive(const SpatialObjectInspection &inspection)
{
	if (!inspection.drive_response && inspection.drive_diagnostic.empty()) return;
	if (!ImGui::CollapsingHeader("Coaxial ring drive", ImGuiTreeNodeFlags_DefaultOpen)) return;
	if (!inspection.drive_response) {
		ImGui::TextWrapped("%s", inspection.drive_diagnostic.c_str());
		return;
	}
	const auto &r = *inspection.drive_response;
	constexpr double degrees = 57.29577951308232;
	ImGui::Text("Hoop inertia %.5g kg m^2", r.hoopInertiaKgM2);
	ImGui::Text("Parent-relative %.5g deg/s  /  world %.5g deg/s", r.relativeRadPerS*degrees, r.worldRadPerS*degrees);
	ImGui::Text("World acceleration %.5g deg/s^2", r.worldRadPerS2*degrees);
	ImGui::Text("Bearing %.5g rpm  /  motor %.5g rpm", std::abs(r.relativeRadPerS)*degrees/6, r.motorRpm);
	ImGui::Text("Hoop inertial torque %.5g N m", r.hoopJointTorqueNm);
	ImGui::Text("Gearbox output %.5g N m  /  motor %.5g N m", r.gearboxOutputTorqueNm, r.motorTorqueNm);
	ImGui::Text("Motor shaft power %.5g W", r.motorPowerW);
	ImGui::Text("This hoop %.5g J  /  all hoops %.5g J", r.hoopEnergyJ, r.chainHoopEnergyJ);
	ImGui::Text("All motor rotors %.5g J", r.chainRotorEnergyJ);
}

void draw_member_study(const SpatialObjectInspection &inspection)
{
	if (!inspection.member_response && inspection.member_diagnostic.empty()) return;
	if (!ImGui::CollapsingHeader("Member service analysis", ImGuiTreeNodeFlags_DefaultOpen)) return;
	ImGui::Text("Pin %s  /  roller %s", inspection.member_pin.c_str(), inspection.member_roller.c_str());
	ImGui::Text("Material: %s", inspection.member_material.c_str());
	if (!inspection.member_response) {
		ImGui::TextWrapped("Study unavailable: %s", inspection.member_diagnostic.c_str());
		return;
	}
	const auto &r = *inspection.member_response;
	ImGui::Text("Span %.5g m  /  mass %.5g kg", r.spanM, r.massKg);
	ImGui::Text("Line load %.5g N/m  /  centre load %.5g N", r.totalLineLoadNPerM, r.centreLoadN);
	ImGui::Text("Reaction at each seat: %.5g N", r.reactionPerSeatN);
	ImGui::Text("Pin pressure %.5g MPa  /  roller %.5g MPa", r.pinPressurePa / 1e6, r.rollerPressurePa / 1e6);
	ImGui::Text("Peak moment %.5g N m  /  bending stress %.5g MPa", r.peakMomentNm, r.bendingStressPa / 1e6);
	ImGui::Text("Midspan deflection: %.5g mm", 1000 * r.midspanDeflectionM);
	ImGui::TextDisabled("Undeformed and loaded centreline");
	draw_member_profile(r);
}

} // namespace

void SpatialObjectInspectorPanel::draw(
	const SpatialObjectInspection &inspection,
	const std::vector<SpatialObjectSelectionEntry> &selection_entries,
	std::string *selected_object_id,
	int *selected_primitive_instance_index) const
{
	ImGui::Separator();
	ImGui::TextUnformatted("Spatial Object Inspector");
	if (selection_entries.empty() || selected_object_id == nullptr) {
		ImGui::TextDisabled("The current scene has no SMB-OMv2 spatial model.");
		return;
	}

	const char *preview = selected_object_id->empty()
		? "Select spatial object"
		: selected_object_id->c_str();
	if (ImGui::BeginCombo("Object", preview)) {
		for (const SpatialObjectSelectionEntry &entry : selection_entries) {
			const bool selected = entry.object_id == *selected_object_id;
			const std::string label = hierarchy_label(entry);
			if (ImGui::Selectable(label.c_str(), selected)) {
				*selected_object_id = entry.object_id;
				if (selected_primitive_instance_index != nullptr) {
					*selected_primitive_instance_index = -1;
				}
			}
			if (selected) ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	if (!inspection.available) {
		ImGui::TextDisabled("Select a spatial object or a bound preview primitive.");
		return;
	}

	ImGui::Text("ID: %s", inspection.object_id.c_str());
	ImGui::Text("Name: %s", inspection.object_name.c_str());
	ImGui::Text("Class: %s", inspection.object_class.c_str());
	ImGui::TextWrapped("Taxonomy: %s", inspection.taxonomy_path.c_str());
	ImGui::Text("Container: %s", inspection.container_object_id.c_str());
	ImGui::Text("State: %s", inspection.state.c_str());
	ImGui::Text("Parent frame: %s", inspection.parent_frame.c_str());
	ImGui::Text("Collision: %s", inspection.collision_layer.c_str());
	ImGui::TextWrapped("Mask: %s", inspection.collision_mask.c_str());
	ImGui::Text("Primitive bindings: %zu", inspection.primitive_instance_indices.size());
	ImGui::SameLine();
	ImGui::Text("Boundaries: %zu", inspection.boundary_representation_count);
	if (!inspection.primitive_instance_indices.empty()) {
		ImGui::TextUnformatted("Primitive indices:");
		ImGui::SameLine();
		for (std::size_t index = 0; index < inspection.primitive_instance_indices.size(); ++index) {
			if (index > 0) ImGui::SameLine(0.0f, 4.0f);
			ImGui::Text("%zu", inspection.primitive_instance_indices[index]);
		}
	}
	draw_coaxial_drive(inspection);
	draw_member_study(inspection);
	draw_vector3("Translation uncertainty", inspection.translation_uncertainty);
	draw_vector3("Rotation uncertainty", inspection.rotation_uncertainty_degrees);
	draw_matrix("Authored local transform", inspection.authored_local_transform);
	draw_matrix("Resolution local transform", inspection.resolution_local_transform);
	draw_matrix("Resolved world transform", inspection.resolved_world_transform);

	if (ImGui::TreeNode("Interfaces", "Interfaces (%zu)", inspection.interfaces.size())) {
		for (const SpatialInterfaceInspection &interface : inspection.interfaces) {
			if (ImGui::TreeNode(interface.interface_id.c_str(), "%s — %s",
			                    interface.interface_id.c_str(), interface.interface_type.c_str())) {
				ImGui::Text("State: %s", interface.state.c_str());
				ImGui::Text("Region: %s", interface.region.c_str());
				draw_vector3("Origin", interface.origin);
				draw_vector3("Normal", interface.normal);
				draw_vector3("Tangent", interface.tangent);
				ImGui::Text("Nominal clearance: %.4f", interface.nominal_clearance);
				ImGui::TreePop();
			}
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Connections", "Connections (%zu)", inspection.connections.size())) {
		for (const SpatialConnectionInspection &connection : inspection.connections) {
			ImGui::BulletText("%s: %s | %s -> %s | %s",
			                  connection.connection_id.c_str(),
			                  connection.connection_type.c_str(),
			                  connection.source_reference.c_str(),
			                  connection.target_reference.c_str(),
			                  connection.state.c_str());
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Constraints", "Constraints (%zu)", inspection.constraints.size())) {
		for (const SpatialConstraintInspection &constraint : inspection.constraints) {
			ImGui::BulletText("%s: %s %s -> %s | %s | priority %d",
			                  constraint.constraint_id.c_str(),
			                  constraint.constraint_type.c_str(),
			                  constraint.source_reference.c_str(),
			                  constraint.target_reference.c_str(),
			                  constraint.mode.c_str(),
			                  constraint.priority);
		}
		ImGui::TreePop();
	}

	if (ImGui::TreeNode("Resolution", "Resolution records (%zu)",
	                    inspection.resolution_records.size())) {
		for (const SpatialResolutionInspection &record : inspection.resolution_records) {
			if (ImGui::TreeNode(record.constraint_id.c_str(), "%s — %s",
			                    record.constraint_id.c_str(), record.status.c_str())) {
				ImGui::Text("Target: %s", record.target_object_id.c_str());
				ImGui::Text("Algorithm: %s", record.algorithm.c_str());
					ImGui::Text("Iterations: %d broad, %d refinement",
					            record.broad_phase_step_count,
					            record.refinement_iteration_count);
					ImGui::Text("Spatial queries: %d", record.collision_query_count);
				ImGui::Text("Clearance: %.6f", record.resulting_clearance);
				ImGui::Text("Residual: %.6f", record.residual_error);
				draw_vector3("Contact point", record.contact_point);
				draw_vector3("Contact normal", record.contact_normal);
				ImGui::Text("Evidence hash: %016llx",
				            static_cast<unsigned long long>(record.evidence_hash));
				ImGui::TreePop();
			}
		}
		ImGui::Text("Aggregate evidence hash: %016llx",
		            static_cast<unsigned long long>(inspection.aggregate_evidence_hash));
		ImGui::TreePop();
	}

	if (inspection.building_knowledge_available) {
		ImGui::Separator();
		ImGui::TextUnformatted("SMB-OMv2.1 Building Knowledge");
		ImGui::Text("Concept: %s", inspection.canonical_concept_name.c_str());
		ImGui::TextWrapped("Concept ID: %s", inspection.canonical_concept_id.c_str());
		ImGui::TextWrapped(
			"Semantic taxonomy: %s", inspection.semantic_taxonomy_path.c_str());
		ImGui::Text("Profile hash: %016llx",
		            static_cast<unsigned long long>(inspection.semantic_profile_hash));
		ImGui::Text("Building model hash: %016llx",
		            static_cast<unsigned long long>(inspection.building_model_hash));
		ImGui::Text("Placement: %s", inspection.placement_state.c_str());
		ImGui::Text("Operational: %s", inspection.operational_state.c_str());
		ImGui::Text("Condition: %s", inspection.condition_state.c_str());
		ImGui::Text("Compliance: %s", inspection.compliance_state.c_str());
		ImGui::Text(
			"Service availability: %s", inspection.service_availability_state.c_str());
		draw_text_records(
			"BuildingApplicability", "Applicability", inspection.building_applicability);
		draw_text_records("BuildingRoles", "Roles", inspection.building_roles);
		draw_text_records(
			"BuildingFunctions", "Allocated functions", inspection.building_functions);
		draw_text_records(
			"BuildingServicePorts", "Service ports", inspection.building_service_ports);
		draw_text_records(
			"IncomingServiceFlows", "Incoming service flows",
			inspection.incoming_service_flows);
		draw_text_records(
			"OutgoingServiceFlows", "Outgoing service flows",
			inspection.outgoing_service_flows);
		draw_text_records(
			"BuildingRelationshipAssertions", "Canonical relationship facts",
			inspection.building_relationship_assertions);
		draw_text_records(
			"BuildingRequirements", "Requirements", inspection.building_requirements);
		draw_text_records(
			"BuildingScenarios", "Scenario participation", inspection.building_scenarios);
		draw_text_records(
			"BuildingEvidence", "Evidence references", inspection.building_evidence);
	}

	if (inspection.source_range.isValid()) {
		ImGui::Text("Source: %d:%d to %d:%d",
		            inspection.source_range.start_line + 1,
		            inspection.source_range.start_column + 1,
		            inspection.source_range.end_line + 1,
		            inspection.source_range.end_column + 1);
	}
}
