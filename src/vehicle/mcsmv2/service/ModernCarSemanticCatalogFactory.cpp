#include "vehicle/mcsmv2/service/ModernCarSemanticCatalogFactory.h"

#include "vehicle/mcsmv2/generated/GeneratedMcsMv2Catalog.h"

#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

VehicleSemanticStationRole stationRole(std::string_view role)
{
	if (role == "tail_face") return VehicleSemanticStationRole::TailFace;
	if (role == "rear_bumper") return VehicleSemanticStationRole::RearBumper;
	if (role == "rear_axle") return VehicleSemanticStationRole::RearAxle;
	if (role == "rear_door") return VehicleSemanticStationRole::RearDoor;
	if (role == "b_pillar") return VehicleSemanticStationRole::BPillar;
	if (role == "front_door") return VehicleSemanticStationRole::FrontDoor;
	if (role == "a_pillar") return VehicleSemanticStationRole::APillar;
	if (role == "hood_rear") return VehicleSemanticStationRole::HoodRear;
	if (role == "front_axle") return VehicleSemanticStationRole::FrontAxle;
	if (role == "nose") return VehicleSemanticStationRole::Nose;
	if (role == "front_face") return VehicleSemanticStationRole::FrontFace;
	throw std::invalid_argument("Unknown MCSMv2 semantic station role.");
}

glm::dvec3 vectorFrom(const std::array<double, 3> &values)
{
	return glm::dvec3(values[0], values[1], values[2]);
}

VehicleReferenceFrameDefinition referenceFrame(
	const GeneratedMcsMv2VariantRecord &record)
{
	VehicleReferenceFrameDefinition::TransformationMatrix matrix{};
	for (std::size_t row = 0u; row < matrix.size(); ++row) {
		for (std::size_t column = 0u; column < matrix[row].size(); ++column) {
			matrix[row][column] = record.reference_frame.source_to_progen3d_matrix[row][column];
		}
	}
	const double package_center =
		(record.package.front_overhang - record.package.rear_overhang) * 0.5;
	return VehicleReferenceFrameDefinition(
		std::string(record.reference_frame.schema),
		std::string(record.reference_frame.name),
		vectorFrom(record.reference_frame.origin),
		glm::dvec3(package_center, 0.0, 0.0),
		vectorFrom(record.reference_frame.source_x_axis),
		vectorFrom(record.reference_frame.source_y_axis),
		vectorFrom(record.reference_frame.source_z_axis),
		std::string(record.reference_frame.handedness),
		std::string(record.reference_frame.length_unit),
		std::string(record.reference_frame.angle_unit),
		matrix,
		0.0);
}

ModernCarParameterDependencyGraph parameterDependencyGraph(
	const GeneratedMcsMv2VariantRecord &record)
{
	std::vector<ModernCarParameterDependencyNode> nodes;
	nodes.reserve(record.parameter_dependencies.size());
	for (const GeneratedMcsMv2ParameterDependencyRecord &source_node :
	     record.parameter_dependencies) {
		std::vector<std::string> dependencies;
		dependencies.reserve(source_node.dependency_count);
		for (std::size_t index = 0u; index < source_node.dependency_count; ++index) {
			dependencies.emplace_back(source_node.dependencies[index]);
		}
		const std::optional<double> uncertainty = source_node.evidence.has_uncertainty
			? std::optional<double>(source_node.evidence.uncertainty)
			: std::nullopt;
		nodes.emplace_back(
			std::string(source_node.identifier),
			std::string(source_node.serialized_value),
			std::string(source_node.unit),
			std::string(source_node.value_type),
			std::string(source_node.formula),
			std::move(dependencies),
			ModernCarParameterEvidence(
				std::string(source_node.evidence.status),
				std::string(source_node.evidence.source),
				source_node.evidence.confidence,
				uncertainty,
				std::string(source_node.evidence.note)));
	}
	return ModernCarParameterDependencyGraph(
		"MCSMv2." + std::string(record.key) + ".ParameterDependencyGraph",
		std::move(nodes));
}

ImplicitFieldCalibration fieldCalibration(
	const GeneratedMcsMv2VariantRecord &record)
{
	return ImplicitFieldCalibration(
		std::string(record.field_calibration.schema),
		std::string(record.field_calibration.method),
		record.field_calibration.calibration_iterations,
		vectorFrom(record.field_calibration.field_prewarp_scale),
		vectorFrom(record.field_calibration.field_prewarp_translation),
		record.field_calibration.maximum_field_prewarp_fraction,
		record.field_calibration.post_mesh_affine_correction_applied,
		record.field_calibration.maximum_post_mesh_correction_fraction,
		vectorFrom(record.field_calibration.target_bounds_minimum),
		vectorFrom(record.field_calibration.target_bounds_maximum),
		vectorFrom(record.field_calibration.accepted_final_bounds_minimum),
		vectorFrom(record.field_calibration.accepted_final_bounds_maximum),
		record.field_calibration.package_tolerance);
}

ModernCarSourceAssurance sourceAssurance(
	const GeneratedMcsMv2VariantRecord &record)
{
	std::vector<ModernCarAssuranceLevel> levels;
	levels.reserve(record.source_assurance.levels.size());
	for (const GeneratedMcsMv2AssuranceLevelRecord &source_level :
	     record.source_assurance.levels) {
		levels.emplace_back(
			std::string(source_level.identifier),
			source_level.passed,
			source_level.partial_static_envelopes,
			std::string(source_level.claim),
			std::string(source_level.note));
	}
	std::vector<std::string> known_limitations;
	known_limitations.reserve(record.source_assurance.known_limitations.size());
	for (std::string_view source_limitation :
	     record.source_assurance.known_limitations) {
		known_limitations.emplace_back(source_limitation);
	}
	return ModernCarSourceAssurance(
		std::string(record.source_assurance.schema),
		std::string(record.source_assurance.achieved_level),
		std::string(record.source_assurance.achieved_label),
		record.source_assurance.release_gate_passed,
		std::move(levels),
		std::move(known_limitations));
}

std::array<VehicleSectionLandmark, kVehicleSectionLandmarkCount> landmarks(
	const GeneratedMcsMv2StationRecord &record)
{
	return {{
		{VehicleSectionLandmarkRole::Underbody,
		 record.underbody_halfwidth, record.underbody_z},
		{VehicleSectionLandmarkRole::Rocker,
		 record.rocker_halfwidth, record.rocker_z},
		{VehicleSectionLandmarkRole::LowerBody,
		 record.lower_halfwidth, record.lower_z},
		{VehicleSectionLandmarkRole::Shoulder,
		 record.shoulder_halfwidth, record.shoulder_z},
		{VehicleSectionLandmarkRole::Belt,
		 record.belt_halfwidth, record.belt_z},
		{VehicleSectionLandmarkRole::GlassShoulder,
		 record.glass_shoulder_halfwidth, record.glass_shoulder_z},
		{VehicleSectionLandmarkRole::RoofRail,
		 record.roof_rail_halfwidth, record.roof_rail_z},
		{VehicleSectionLandmarkRole::RoofCrown,
		 0.0, record.roof_crown_z}
	}};
}

ModernCarSemanticVariant createVariant(const GeneratedMcsMv2VariantRecord &record)
{
	std::vector<VehicleSemanticSectionStation> stations;
	stations.reserve(record.stations.size());
	for (const GeneratedMcsMv2StationRecord &station : record.stations) {
		stations.emplace_back(
			static_cast<std::size_t>(station.index),
			stationRole(station.role),
			station.normalized_station,
			station.source_x,
			landmarks(station),
			station.confidence);
	}

	return ModernCarSemanticVariant(
		std::string(record.key),
		std::string(record.label),
		std::string(record.description),
		std::string(record.package_basis),
		referenceFrame(record),
		fieldCalibration(record),
		sourceAssurance(record),
		VehiclePackageParameters(
			"MCSMv2." + std::string(record.key) + ".Package",
			record.package.length,
			record.package.width,
			record.package.height,
			record.package.wheelbase,
			record.package.ground_clearance,
			record.package.front_overhang,
			record.package.rear_overhang),
		ModernCarPlatformDefinition(
			record.platform.floor_height,
			record.platform.front_hpoint_x,
			record.platform.rear_hpoint_x,
			record.platform.hpoint_z,
			record.platform.eye_z,
			record.platform.head_clearance,
			record.platform.seat_lateral),
		ModernCarSemanticStyleDefinition(
			record.style.roof_scale,
			record.style.greenhouse_front_factor,
			record.style.greenhouse_rear_factor,
			record.style.nose_taper,
			record.style.tail_taper,
			record.style.hood_wedge,
			record.style.roof_crown,
			record.style.tumblehome,
			record.style.belt_rise,
			record.style.shoulder_strength,
			record.style.front_fender_amplitude,
			record.style.rear_haunch_amplitude,
			record.style.rocker_tuck,
			record.style.door_scallop,
			record.style.spoiler_scale,
			record.style.splitter_scale,
			record.style.grille_scale),
		VehicleWheelMotionParameters(
			VehicleWheelParameters(
				record.wheels.radius,
				record.wheels.width,
				record.wheels.front_track,
				record.wheels.rear_track),
			record.wheels.steer_minimum_degrees,
			record.wheels.steer_maximum_degrees,
			record.wheels.travel_minimum,
			record.wheels.travel_maximum,
			record.wheels.wheelhouse_clearance),
		ModernCarPowertrainDefinition(
			std::string(record.powertrain.architecture),
			std::string(record.powertrain.drive_layout),
			record.powertrain.has_battery_pack,
			record.powertrain.battery_thickness,
			record.powertrain.engine_envelope_length,
			record.powertrain.engine_envelope_width,
			record.powertrain.engine_envelope_height,
			record.powertrain.exhaust_count),
		ModernCarClosureDefinition(
			record.closures.front_door_maximum_degrees,
			record.closures.rear_door_maximum_degrees,
			record.closures.bonnet_maximum_degrees,
			record.closures.hatch_maximum_degrees,
			record.closures.side_glass_travel,
			record.closures.nominal_panel_gap),
		VehicleBodyColor(
			record.body_color.red,
			record.body_color.green,
				record.body_color.blue),
		record.has_crossover_cladding,
		parameterDependencyGraph(record),
		VehicleSemanticSectionField(
			"MCSMv2." + std::string(record.key) + ".SemanticSections",
			std::move(stations)));
}

} // namespace

ModernCarSemanticFamily ModernCarSemanticCatalogFactory::createFamily() const
{
	const GeneratedMcsMv2SourceReleaseRecord &source_release =
		generatedMcsMv2SourceReleaseRecord();
	std::vector<ModernCarSemanticVariant> variants;
	variants.reserve(generatedMcsMv2VariantRecords().size());
	for (const GeneratedMcsMv2VariantRecord &record : generatedMcsMv2VariantRecords()) {
		variants.push_back(createVariant(record));
	}
	return ModernCarSemanticFamily(
		"MCSMv2.ModernCarSemanticFamily",
		McsMv201SourceRelease(
			std::string(source_release.version),
			std::string(source_release.model_schema),
			std::string(source_release.manifest_schema),
			std::string(source_release.release_manifest_sha256),
			source_release.signed_artifact_count),
		std::move(variants));
}
