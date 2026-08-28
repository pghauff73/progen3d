#include "vehicle/service/ModernEvFastbackBuilder.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "vehicle/model/SuspensionCornerSpecification.h"
#include "vehicle/model/WheelAssemblySpecification.h"
#include "vehicle/service/ModernEvFastbackBuilder.h"
#include "vehicle/service/SuspensionCornerAssemblyBuilder.h"
#include "vehicle/service/VehicleBodyAssemblyBuilder.h"
#include "vehicle/service/VehicleDatumDerivationService.h"
#include "vehicle/service/VehicleDeterministicHashService.h"
#include "vehicle/service/VehicleKinematicValidationService.h"
#include "vehicle/service/VehiclePackageValidationService.h"
#include "vehicle/service/VehicleValidationService.h"
#include "vehicle/service/WheelAssemblyBuilder.h"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

VehiclePackage create_acceptance_package()
{
	return VehiclePackage(
		4.75f, 1.90f, 1.45f,
		2.90f, 1.64f, 1.63f,
		0.15f, 0.95f, 0.90f,
		0.36f, 0.36f,
		VehiclePackageEnvelope(
			"cabin", glm::vec3(-0.78f, 0.42f, -2.55f),
			glm::vec3(0.78f, 1.42f, -0.45f)),
		VehiclePackageEnvelope(
			"front_powertrain", glm::vec3(-0.58f, 0.18f, -0.35f),
			glm::vec3(0.58f, 0.66f, 0.58f)),
		VehiclePackageEnvelope(
			"rear_powertrain", glm::vec3(-0.58f, 0.18f, -3.34f),
			glm::vec3(0.58f, 0.62f, -2.58f)),
		VehiclePackageEnvelope(
			"battery", glm::vec3(-0.68f, 0.16f, -2.60f),
			glm::vec3(0.68f, 0.34f, -0.42f)),
		VehiclePackageEnvelope(
			"luggage", glm::vec3(-0.70f, 0.42f, -3.55f),
			glm::vec3(0.70f, 0.95f, -2.54f)));
}

VehicleStyleState create_acceptance_style()
{
	return VehicleStyleState(
		0.78f, 0.34f, 0.50f, 0.35f, 0.82f, 0.56f, 0.74f, 0.86f,
		0.88f, VehicleFrontDesignLanguage::Technical,
		VehicleRearDesignLanguage::Fastback);
}

VehicleBodySection create_section(
	const std::string &identifier,
	float station_z,
	float half_width,
	float bottom_y,
	float shoulder_y,
	float belt_y,
	float roof_y,
	float greenhouse_width)
{
	return VehicleBodySection(
		identifier, station_z,
		{{0.0f, bottom_y},
		 {half_width * 0.58f, bottom_y + 0.018f},
		 {half_width, bottom_y + 0.075f},
		 {half_width, shoulder_y},
		 {half_width * 0.88f, belt_y},
		 {greenhouse_width, roof_y * 0.88f},
		 {0.0f, roof_y}});
}

VehicleBodySpecification create_body_specification(
	const VehicleDefinition &definition,
	GeometryDetailLevel detail_level)
{
	const VehiclePackage &package = definition.package();
	std::vector<VehicleBodySection> sections = {
		create_section("Tail", package.rearBumperStation(), 0.76f, 0.40f, 0.77f, 0.82f, 0.91f, 0.34f),
		create_section("RearAxle", package.rearAxleStation(), 0.94f, 0.43f, 0.95f, 1.00f, 1.15f, 0.57f),
		create_section("RearCabin", -2.20f, 0.95f, 0.25f, 0.98f, 1.03f, 1.36f, 0.69f),
		create_section("CabinCentre", -1.45f, 0.95f, 0.24f, 0.96f, 1.01f, 1.45f, 0.72f),
		create_section("FrontCabin", -0.78f, 0.94f, 0.26f, 0.93f, 0.98f, 1.38f, 0.69f),
		create_section("FrontAxle", 0.0f, 0.93f, 0.43f, 0.86f, 0.91f, 0.99f, 0.49f),
		create_section("Nose", package.frontBumperStation(), 0.73f, 0.45f, 0.67f, 0.72f, 0.81f, 0.32f)};

	std::vector<VehicleSurfaceGuide> guides;
	guides.emplace_back(
		"roofLine",
		Curve3D(
			Curve3DType::CatmullRom,
			{{0.0f, 0.91f, package.rearBumperStation()},
			 {0.0f, 1.36f, -2.20f},
			 {0.0f, 1.45f, -1.45f},
			 {0.0f, 1.38f, -0.78f},
			 {0.0f, 0.81f, package.frontBumperStation()}},
			"roofLine"));
	guides.emplace_back(
		"beltLine",
		Curve3D(
			Curve3DType::CatmullRom,
			{{0.67f, 0.82f, package.rearBumperStation()},
			 {0.84f, 1.03f, -2.20f},
			 {0.84f, 1.01f, -1.45f},
			 {0.83f, 0.98f, -0.78f},
			 {0.64f, 0.72f, package.frontBumperStation()}},
			"beltLine"));
	guides.emplace_back(
		"rockerLine",
		Curve3D(
			Curve3DType::Bezier,
			{{0.70f, 0.40f, package.rearBumperStation()},
			 {0.91f, 0.24f, -2.40f},
			 {0.91f, 0.24f, -0.55f},
			 {0.68f, 0.45f, package.frontBumperStation()}},
			"rockerLine"));

	std::vector<WheelArchSpecification> arches;
	for (const auto &datum : std::vector<std::pair<std::string, VehicleDatumType>>{
		     {"front_left_arch", VehicleDatumType::FrontWheelCentreLeft},
		     {"front_right_arch", VehicleDatumType::FrontWheelCentreRight},
		     {"rear_left_arch", VehicleDatumType::RearWheelCentreLeft},
		     {"rear_right_arch", VehicleDatumType::RearWheelCentreRight}}) {
		const VehicleDatum *wheel = definition.datums().find(datum.second);
		arches.emplace_back(
			datum.first, wheel->origin(), 0.36f, 0.055f, 0.26f, 0.018f);
	}

	const float left_x = -package.overallWidth() * 0.5f - 0.002f;
	const float right_x = package.overallWidth() * 0.5f + 0.002f;
	std::vector<VehiclePanelCutSpecification> panel_cuts;
	auto vertical_seam = [](float x, float station, const std::string &identifier) {
		return VehiclePanelCutSpecification(
			identifier,
			Curve3D(
				Curve3DType::Bezier,
				{{x, 0.43f, station}, {x, 0.70f, station - 0.015f},
				 {x, 1.02f, station + 0.015f}, {x, 1.20f, station}},
				identifier),
			0.006f, 0.004f);
	};
	panel_cuts.push_back(vertical_seam(left_x, -0.75f, "front_door_left"));
	panel_cuts.push_back(vertical_seam(right_x, -0.75f, "front_door_right"));
	panel_cuts.push_back(vertical_seam(left_x, -1.85f, "rear_door_left"));
	panel_cuts.push_back(vertical_seam(right_x, -1.85f, "rear_door_right"));
	panel_cuts.emplace_back(
		"hood",
		Curve3D(
			Curve3DType::Bezier,
			{{-0.65f, 0.82f, 0.08f}, {-0.25f, 0.91f, 0.62f},
			 {0.25f, 0.91f, 0.62f}, {0.65f, 0.82f, 0.08f}},
			"hood"),
		0.005f, 0.004f);
	panel_cuts.emplace_back(
		"tailgate",
		Curve3D(
			Curve3DType::Bezier,
			{{-0.64f, 0.81f, -3.48f}, {-0.24f, 1.03f, -3.35f},
			 {0.24f, 1.03f, -3.35f}, {0.64f, 0.81f, -3.48f}},
			"tailgate"),
		0.006f, 0.004f);
	return VehicleBodySpecification(
		"BodyShell", "paint-deep-blue", std::move(sections),
		std::move(guides), std::move(arches), std::move(panel_cuts),
		0.018f, detail_level);
}

SuspensionCornerSpecification create_suspension_corner(
	const std::string &identifier,
	VehicleCornerLocation location,
	const glm::vec3 &wheel_center,
	GeometryDetailLevel detail_level)
{
	const float inward = wheel_center.x < 0.0f ? 0.28f : -0.28f;
	return SuspensionCornerSpecification(
		identifier, location, VehicleSuspensionType::MacPherson, wheel_center,
		wheel_center + glm::vec3(inward, 0.52f, 0.03f),
		wheel_center + glm::vec3(inward, -0.12f, 0.02f),
		0.30f, 0.025f, 0.045f, 0.045f, detail_level);
}

std::vector<VehicleJoint> create_joint_graph()
{
	std::vector<VehicleJoint> joints;
	for (const std::string wheel :
	     {"FrontWheelLeft", "FrontWheelRight", "RearWheelLeft", "RearWheelRight"}) {
		joints.emplace_back(
			wheel + ".rotation", VehicleJointType::Revolute, wheel, "axis",
			wheel + ".Suspension", "wheel_axis", glm::vec3(1.0f, 0.0f, 0.0f),
			-360000.0f, 360000.0f, 0.0f);
		joints.emplace_back(
			wheel + ".travel", VehicleJointType::Prismatic,
			wheel + ".Suspension", "upper_body_mount", "BodyShell",
			"chassis_mount", glm::vec3(0.0f, 1.0f, 0.0f), -0.045f, 0.045f,
			0.0f);
	}
	for (const std::string wheel : {"FrontWheelLeft", "FrontWheelRight"}) {
		joints.emplace_back(
			wheel + ".steering", VehicleJointType::Revolute,
			wheel + ".Suspension", "wheel_axis", wheel, "axis",
			glm::vec3(0.0f, 1.0f, 0.0f), -30.0f, 30.0f, 0.0f);
	}
	for (const std::string panel :
	     {"FrontDoorLeft", "FrontDoorRight", "RearDoorLeft", "RearDoorRight"}) {
		joints.emplace_back(
			panel + ".hinge", VehicleJointType::Revolute, panel, "hinge",
			"BodyShell", "chassis_mount", glm::vec3(0.0f, 1.0f, 0.0f),
			0.0f, 70.0f, 0.0f);
	}
	joints.emplace_back(
		"Hood.hinge", VehicleJointType::Revolute, "Hood", "hinge",
		"BodyShell", "chassis_mount", glm::vec3(1.0f, 0.0f, 0.0f),
		0.0f, 65.0f, 0.0f);
	joints.emplace_back(
		"Tailgate.hinge", VehicleJointType::Revolute, "Tailgate", "hinge",
		"BodyShell", "chassis_mount", glm::vec3(1.0f, 0.0f, 0.0f),
		0.0f, 75.0f, 0.0f);
	return joints;
}

} // namespace

std::optional<ModernVehicleAssembly>
ModernEvFastbackBuilder::buildAcceptanceVehicle(
	GeometryDetailLevel detail_level,
	std::string *diagnostic) const
{
	VehicleIntent intent(
		VehicleBodyType::Fastback, VehiclePowertrainType::BatteryElectric,
		VehicleDriveLayout::AllWheelDrive, 4, 5,
		VehiclePerformanceClass::SportLuxury, VehicleSizeClass::Medium);
	VehiclePackage package = create_acceptance_package();
	VehicleStyleState style = create_acceptance_style();
	VehicleValidationReport package_report =
		VehiclePackageValidationService().validate(intent, package);
	if (!package_report.isValid()) {
		if (diagnostic != nullptr) *diagnostic = package_report.issues().front().message();
		return std::nullopt;
	}
	VehicleDatumSet datums = VehicleDatumDerivationService().derive(package, style);
	VehicleDefinition definition(
		"MVG_Test_EV_001", std::move(intent), std::move(package),
		std::move(datums), std::move(style));
	VehicleBodySpecification body_specification =
		create_body_specification(definition, detail_level);
	VehicleAssemblyBuildResult body_result =
		VehicleBodyAssemblyBuilder(complexity_limits_).build(body_specification);
	if (!body_result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = body_result.diagnostic();
		return std::nullopt;
	}

	WheelAssemblySpecification wheel_specification(
		"SharedWheel", 0.36f, 0.24f, 0.245f, 0.20f, 0.070f, 0.185f, 7,
		"tire-rubber", "machined-alloy", "brake-steel", detail_level);
	VehicleAssemblyBuildResult wheel_result =
		WheelAssemblyBuilder(complexity_limits_).build(wheel_specification);
	if (!wheel_result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = wheel_result.diagnostic();
		return std::nullopt;
	}

	std::vector<VehiclePlacedAssembly> assemblies;
	assemblies.emplace_back(
		"BodyShell", *body_result.geometry(), glm::mat4(1.0f));
	const std::array<std::pair<const char *, VehicleDatumType>, 4> wheel_datums = {{
		{"FrontWheelLeft", VehicleDatumType::FrontWheelCentreLeft},
		{"FrontWheelRight", VehicleDatumType::FrontWheelCentreRight},
		{"RearWheelLeft", VehicleDatumType::RearWheelCentreLeft},
		{"RearWheelRight", VehicleDatumType::RearWheelCentreRight}}};
	for (const auto &wheel : wheel_datums) {
		const VehicleDatum *datum = definition.datums().find(wheel.second);
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), datum->origin());
		transform = glm::rotate(
			transform, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		assemblies.emplace_back(wheel.first, *wheel_result.geometry(), transform);
	}

	if (geometryDetailLevelRank(detail_level) >=
	    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
		const std::array<VehicleCornerLocation, 4> locations = {
			VehicleCornerLocation::FrontLeft, VehicleCornerLocation::FrontRight,
			VehicleCornerLocation::RearLeft, VehicleCornerLocation::RearRight};
		for (std::size_t index = 0u; index < wheel_datums.size(); ++index) {
			const VehicleDatum *datum = definition.datums().find(wheel_datums[index].second);
			const std::string identifier =
				std::string(wheel_datums[index].first) + ".Suspension";
			VehicleAssemblyBuildResult suspension =
				SuspensionCornerAssemblyBuilder(complexity_limits_).build(
					create_suspension_corner(
						identifier, locations[index], datum->origin(), detail_level));
			if (!suspension.succeeded()) {
				if (diagnostic != nullptr) *diagnostic = suspension.diagnostic();
				return std::nullopt;
			}
			assemblies.emplace_back(
				identifier, *suspension.geometry(), glm::mat4(1.0f));
		}
	}

	std::vector<GeneratedMeshPlacement> placements;
	placements.reserve(assemblies.size());
	for (const VehiclePlacedAssembly &assembly : assemblies) {
		placements.emplace_back(
			assembly.objectIdentifier(), assembly.geometry().combinedPreviewMesh(),
			assembly.localTransform());
	}
	GeometryBuildResult combined = GeneratedMeshComposer(complexity_limits_).compose(
		placements);
	if (!combined.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = combined.firstDiagnostic();
		return std::nullopt;
	}
	std::vector<VehicleJoint> joints = create_joint_graph();
	VehicleValidationReport validation = VehicleValidationService().validate(
		definition, body_specification, wheel_specification, assemblies, joints,
		combined.generatedMesh());
	if (!validation.isValid()) {
		if (diagnostic != nullptr) {
			*diagnostic = std::string(vehicleDiagnosticCodeName(
				validation.issues().front().code())) + ": " +
				validation.issues().front().message();
		}
		return std::nullopt;
	}
	const std::uint64_t deterministic_hash =
		VehicleDeterministicHashService().calculate(
			definition, assemblies, joints, combined.generatedMesh());
	return ModernVehicleAssembly(
		std::move(definition), std::move(body_specification),
		std::move(assemblies), std::move(joints), combined.generatedMesh(),
		std::move(validation), deterministic_hash);
}
