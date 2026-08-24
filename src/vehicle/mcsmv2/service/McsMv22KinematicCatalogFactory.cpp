#include "vehicle/mcsmv2/service/McsMv22KinematicCatalogFactory.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv22Catalog.h"

#include <glm/glm.hpp>

#include <array>
#include <string>
#include <vector>

namespace {

glm::dvec3 createVector(const GeneratedMcsMv22Vector3Record &record)
{
	return glm::dvec3(record.x, record.y, record.z);
}

glm::dmat4 createMatrix(const GeneratedMcsMv22Matrix4Record &record)
{
	glm::dmat4 matrix(1.0);
	for (std::size_t row = 0u; row < 4u; ++row) {
		for (std::size_t column = 0u; column < 4u; ++column) {
			matrix[column][row] = record.row_major_values[row * 4u + column];
		}
	}
	return matrix;
}

} // namespace

McsMv22KinematicFamilyDefinition
McsMv22KinematicCatalogFactory::createFamilyDefinition() const
{
	const GeneratedMcsMv22SourceReleaseRecord &source_record =
		generatedMcsMv22SourceReleaseRecord();
	std::vector<McsMv22KinematicVariantDefinition> variants;
	variants.reserve(generatedMcsMv22VariantRecords().size());
	for (const GeneratedMcsMv22VariantRecord &record :
	     generatedMcsMv22VariantRecords()) {
		std::vector<McsMv22SuspensionHardpointDefinition> hardpoints;
		hardpoints.reserve(record.hardpoints.size());
		for (const GeneratedMcsMv22HardpointRecord &hardpoint : record.hardpoints) {
			hardpoints.emplace_back(
				std::string(hardpoint.name), std::string(hardpoint.axle),
				std::string(hardpoint.side), std::string(hardpoint.role),
				createVector(hardpoint.position), std::string(hardpoint.evidence_status),
				hardpoint.confidence);
		}

		std::vector<McsMv22WheelPoseEvidence> wheel_poses;
		wheel_poses.reserve(record.wheel_poses.size());
		for (const GeneratedMcsMv22WheelPoseRecord &pose : record.wheel_poses) {
			wheel_poses.emplace_back(
				std::string(pose.wheel), pose.steer_degrees, pose.travel_metres,
				pose.camber_degrees, pose.toe_degrees,
				createVector(pose.centre_metres), createMatrix(pose.transform));
		}

		std::vector<McsMv22ClosureHingeDefinition> hinges;
		hinges.reserve(record.hinges.size());
		for (const GeneratedMcsMv22HingeRecord &hinge : record.hinges) {
			hinges.emplace_back(
				std::string(hinge.name), std::string(hinge.closure),
				createVector(hinge.point), createVector(hinge.axis),
				hinge.maximum_angle_degrees, hinge.direction, hinge.rise_metres,
				createVector(hinge.translation_axis), std::string(hinge.joint_type));
		}

		std::vector<McsMv22HelicalGlassDefinition> glass_systems;
		glass_systems.reserve(record.glass_systems.size());
		for (const GeneratedMcsMv22GlassRecord &glass : record.glass_systems) {
			glass_systems.emplace_back(
				std::string(glass.name), std::string(glass.parent_closure),
				std::string(glass.side), glass.travel_metres, glass.inward_metres,
				glass.longitudinal_metres, glass.rotation_degrees,
				createVector(glass.pivot));
		}

		std::vector<McsMv22SurfaceOwnerDefinition> panel_owners;
		panel_owners.reserve(record.panel_owners.size());
		for (const GeneratedMcsMv22SurfaceOwnerRecord &owner : record.panel_owners) {
			panel_owners.emplace_back(
				std::string(owner.name), std::string(owner.kind), owner.closure,
				owner.face_count);
		}
		std::vector<McsMv22SurfaceOwnerDefinition> aperture_owners;
		aperture_owners.reserve(record.aperture_owners.size());
		for (const GeneratedMcsMv22SurfaceOwnerRecord &owner : record.aperture_owners) {
			aperture_owners.emplace_back(
				std::string(owner.name), std::string(owner.kind), owner.closure,
				owner.face_count);
		}

		variants.emplace_back(
			std::string(record.identifier), std::string(record.display_name),
			std::string(record.model_schema), std::move(hardpoints),
			std::move(wheel_poses), std::move(hinges), std::move(glass_systems),
			std::move(panel_owners), std::move(aperture_owners),
			record.wheel_radius_metres, record.wheel_width_metres,
			record.minimum_steer_degrees, record.maximum_steer_degrees,
			record.minimum_travel_metres, record.maximum_travel_metres,
			record.declared_tyre_clearance_metres,
			record.accepted_minimum_tyre_clearance_metres,
			record.tyre_tessellation_tolerance_metres,
			record.source_release_gate_pass, std::string(record.assurance_level));
	}

	return McsMv22KinematicFamilyDefinition(
		McsMv220SourceRelease(
			std::string(source_record.version), std::string(source_record.model_schema),
			std::string(source_record.manifest_schema),
			std::string(source_record.release_manifest_sha256),
			source_record.signed_artifact_count,
			source_record.signed_total_size_bytes,
			std::string(source_record.reference_frame_schema),
			createMatrix(source_record.to_progen3d_matrix),
			std::string(source_record.assurance_boundary)),
		std::move(variants));
}
