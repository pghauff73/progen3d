#include "vehicle/service/SuspensionCornerAssemblyBuilder.h"

#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/service/SweepDiskMeshGenerator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"
#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "vehicle/service/VehicleAssemblyCompositionService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>

#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

struct BuiltSweep
{
	std::shared_ptr<const SweepDiskShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};
};

BuiltSweep build_sweep(
	std::vector<glm::vec3> path,
	float radius,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	SweepDiskShapeSpecificationCandidate candidate;
	candidate.path_points = std::move(path);
	candidate.radius = radius;
	candidate.circumferential_segments = 12;
	candidate.detail_level = detail_level;
	auto shape = SweepDiskSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = SweepDiskMeshGenerator(complexity_limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

std::vector<glm::vec3> create_spring_path(
	glm::vec3 lower,
	glm::vec3 upper,
	float coil_radius)
{
	const glm::vec3 axis = upper - lower;
	const float axis_length = glm::length(axis);
	const glm::vec3 direction = axis / axis_length;
	glm::vec3 first_normal = glm::cross(direction, glm::vec3(1.0f, 0.0f, 0.0f));
	if (glm::length(first_normal) <= 1.0e-5f) {
		first_normal = glm::cross(direction, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	first_normal = glm::normalize(first_normal);
	const glm::vec3 second_normal = glm::normalize(glm::cross(direction, first_normal));
	std::vector<glm::vec3> path;
	path.reserve(49u);
	for (int index = 0; index <= 48; ++index) {
		const float parameter = static_cast<float>(index) / 48.0f;
		const float angle = glm::two_pi<float>() * 6.0f * parameter;
		path.push_back(
			lower + axis * parameter +
			coil_radius *
				(first_normal * std::cos(angle) + second_normal * std::sin(angle)));
	}
	return path;
}

} // namespace

VehicleAssemblyBuildResult SuspensionCornerAssemblyBuilder::build(
	const SuspensionCornerSpecification &specification) const
{
	if (specification.objectIdentifier().empty() ||
	    !finite(specification.wheelCenter()) ||
	    !finite(specification.upperBodyMount()) ||
	    !finite(specification.lowerBodyMount()) ||
	    !std::isfinite(specification.uprightHeight()) ||
	    !std::isfinite(specification.memberRadius()) ||
	    !std::isfinite(specification.compressionTravel()) ||
	    !std::isfinite(specification.reboundTravel()) ||
	    specification.uprightHeight() <= 0.0f ||
	    specification.memberRadius() <= 0.0f ||
	    specification.compressionTravel() < 0.0f ||
	    specification.reboundTravel() < 0.0f) {
		return VehicleAssemblyBuildResult::failed(
			"SuspensionCorner requires finite mount positions, positive member dimensions, and nonnegative travel." );
	}

	const glm::vec3 lower_hub = specification.wheelCenter() -
		glm::vec3(0.0f, specification.uprightHeight() * 0.5f, 0.0f);
	const glm::vec3 upper_hub = specification.wheelCenter() +
		glm::vec3(0.0f, specification.uprightHeight() * 0.5f, 0.0f);
	std::string diagnostic;
	const GeometryDetailLevel detail_level = specification.detailLevel();
	BuiltSweep upright = build_sweep(
		{lower_hub, upper_hub}, specification.memberRadius() * 1.35f,
		detail_level, complexity_limits_, &diagnostic);
	BuiltSweep lower_arm = build_sweep(
		{specification.lowerBodyMount(), lower_hub}, specification.memberRadius(),
		detail_level, complexity_limits_, &diagnostic);
	BuiltSweep damper = build_sweep(
		{upper_hub, specification.upperBodyMount()}, specification.memberRadius(),
		detail_level, complexity_limits_, &diagnostic);
	if (!upright.shape || !lower_arm.shape || !damper.shape) {
		return VehicleAssemblyBuildResult::failed(diagnostic);
	}
	std::vector<VehicleAssemblyPart> parts;
	parts.emplace_back(
		"upright", "SuspensionUpright", "suspension-metal", upright.shape,
		upright.mesh, glm::mat4(1.0f),
		GeometryDetailRange(
			GeometryDetailLevel::Assembly,
			GeometryDetailLevel::FastenersAndSeals));
	parts.emplace_back(
		"lower_control_arm", "LowerControlArm", "suspension-metal",
		lower_arm.shape, lower_arm.mesh, glm::mat4(1.0f),
		GeometryDetailRange(
			GeometryDetailLevel::Assembly,
			GeometryDetailLevel::FastenersAndSeals));
	parts.emplace_back(
		"damper", "Damper", "damper-metal", damper.shape, damper.mesh,
		glm::mat4(1.0f),
		GeometryDetailRange(
			GeometryDetailLevel::Assembly,
			GeometryDetailLevel::FastenersAndSeals));

	if (geometryDetailLevelRank(detail_level) >=
	    geometryDetailLevelRank(GeometryDetailLevel::Component)) {
		BuiltSweep spring = build_sweep(
			create_spring_path(
				upper_hub, specification.upperBodyMount(),
				specification.memberRadius() * 2.2f),
			specification.memberRadius() * 0.35f, detail_level,
			complexity_limits_, &diagnostic);
		if (!spring.shape) return VehicleAssemblyBuildResult::failed(diagnostic);
		parts.emplace_back(
			"spring", "CoilSpring", "spring-steel", spring.shape, spring.mesh,
			glm::mat4(1.0f),
			GeometryDetailRange(
				GeometryDetailLevel::Component,
				GeometryDetailLevel::FastenersAndSeals));
	}

	if (specification.location() == VehicleCornerLocation::FrontLeft ||
	    specification.location() == VehicleCornerLocation::FrontRight) {
		const float inward = specification.wheelCenter().x < 0.0f ? 0.20f : -0.20f;
		BuiltSweep tie_rod = build_sweep(
			{specification.wheelCenter() + glm::vec3(inward, 0.0f, -0.05f),
			 specification.wheelCenter() + glm::vec3(0.0f, 0.0f, -0.05f)},
			specification.memberRadius() * 0.55f, detail_level,
			complexity_limits_, &diagnostic);
		if (!tie_rod.shape) return VehicleAssemblyBuildResult::failed(diagnostic);
		parts.emplace_back(
			"tie_rod", "SteeringTieRod", "suspension-metal", tie_rod.shape,
			tie_rod.mesh, glm::mat4(1.0f),
			GeometryDetailRange(
				GeometryDetailLevel::Assembly,
				GeometryDetailLevel::FastenersAndSeals));
	}

	GeometryBuildResult combined =
		VehicleAssemblyCompositionService(complexity_limits_).compose(parts);
	if (!combined.succeeded()) {
		return VehicleAssemblyBuildResult::failed(combined.firstDiagnostic());
	}

	const SpatialObjectId owner(specification.objectIdentifier());
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("wheel_axis"), owner, SpatialInterfaceType::Shaft,
		SpatialInterfaceFrame(
			specification.wheelCenter(), glm::vec3(1.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f)),
		SpatialInterfaceRegion::axisSegment(specification.memberRadius() * 4.0f),
		InterfaceCompatibilityProfile(
			InterfaceShape::Axis, InterfaceGender::Male,
			std::nullopt, std::nullopt, std::nullopt, "vehicle-wheel-axis"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
	interfaces.emplace_back(
		SpatialInterfaceId("upper_body_mount"), owner, SpatialInterfaceType::Mate,
		SpatialInterfaceFrame(
			specification.upperBodyMount(), glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::point(),
		InterfaceCompatibilityProfile(
			InterfaceShape::Point, InterfaceGender::Male,
			std::nullopt, std::nullopt, std::nullopt,
			"vehicle-suspension-upper-mount"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));
	interfaces.emplace_back(
		SpatialInterfaceId("lower_body_mount"), owner, SpatialInterfaceType::Mate,
		SpatialInterfaceFrame(
			specification.lowerBodyMount(), glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::point(),
		InterfaceCompatibilityProfile(
			InterfaceShape::Point, InterfaceGender::Male,
			std::nullopt, std::nullopt, std::nullopt,
			"vehicle-suspension-lower-mount"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));

	return VehicleAssemblyBuildResult::succeeded(
		VehicleAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces),
			detail_level));
}
