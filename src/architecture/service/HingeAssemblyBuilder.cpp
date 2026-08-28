#include "architecture/service/HingeAssemblyBuilder.h"

#include "architecture/service/ArchitecturalPartGeometryBuilder.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/RoundedBoxSpecification.h"
#include "geometry/service/GeneratedMeshComposer.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <string>
#include <utility>
#include <vector>

ArchitecturalAssemblyBuildResult HingeAssemblyBuilder::build(
	const HingeSpecification &specification,
	GeometryDetailLevel detail_level) const
{
	if (geometryDetailLevelRank(detail_level) <
	        geometryDetailLevelRank(GeometryDetailLevel::ConstructionDetail) ||
	    specification.objectIdentifier().empty() ||
	    specification.materialIdentifier().empty() ||
	    !std::isfinite(specification.cupDiameter()) ||
	    !std::isfinite(specification.cupDepth()) ||
	    !std::isfinite(specification.armLength()) ||
	    !std::isfinite(specification.armWidth()) ||
	    !std::isfinite(specification.armDepth()) ||
	    !std::isfinite(specification.plateWidth()) ||
	    !std::isfinite(specification.plateHeight()) ||
	    !std::isfinite(specification.plateDepth()) ||
	    specification.cupDiameter() <= 0.0f || specification.cupDepth() <= 0.0f ||
	    specification.armLength() <= 0.0f || specification.armWidth() <= 0.0f ||
	    specification.armDepth() <= 0.0f || specification.plateWidth() <= 0.0f ||
	    specification.plateHeight() <= 0.0f || specification.plateDepth() <= 0.0f) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"Hinge requires construction-detail LOD, an identifier, material, and finite positive component dimensions.");
	}

	ArchitecturalPartGeometryBuilder geometry_builder(complexity_limits_);
	std::string diagnostic;
	ArchitecturalPartGeometry cup = geometry_builder.buildCircularPrism(
		specification.cupDiameter(),
		specification.cupDepth(),
		24,
		detail_level,
		&diagnostic);
	ArchitecturalPartGeometry arm = geometry_builder.buildRoundedBox(
		RoundedBoxSpecification(
			specification.armLength(),
			specification.armWidth(),
			specification.armDepth(),
			std::min(specification.armWidth(), specification.armDepth()) * 0.2f),
		detail_level,
		&diagnostic);
	ArchitecturalPartGeometry plate = geometry_builder.buildRoundedBox(
		RoundedBoxSpecification(
			specification.plateWidth(),
			specification.plateHeight(),
			specification.plateDepth(),
			std::min(specification.plateWidth(), specification.plateHeight()) * 0.12f),
		detail_level,
		&diagnostic);
	if (!cup.isValid() || !arm.isValid() || !plate.isValid()) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	const float cup_radius = specification.cupDiameter() * 0.5f;
	const glm::mat4 cup_transform = glm::translate(
		glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -specification.cupDepth()));
	const glm::mat4 arm_transform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(
			cup_radius + specification.armLength() * 0.5f,
			0.0f,
			-specification.armDepth() * 0.5f));
	const glm::mat4 plate_transform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(
			cup_radius + specification.armLength() +
				specification.plateWidth() * 0.5f,
			0.0f,
			-specification.plateDepth() * 0.5f));

	const GeometryDetailRange detail_range(
		GeometryDetailLevel::ConstructionDetail,
		GeometryDetailLevel::FastenersAndSeals);
	std::vector<ArchitecturalAssemblyPart> parts;
	parts.emplace_back(
		"hinge_cup", "HingeCup", specification.materialIdentifier(),
		cup.shape(), cup.generatedMesh(), cup_transform, detail_range);
	parts.emplace_back(
		"hinge_arm", "HingeArm", specification.materialIdentifier(),
		arm.shape(), arm.generatedMesh(), arm_transform, detail_range);
	parts.emplace_back(
		"mounting_plate", "HingeMountingPlate", specification.materialIdentifier(),
		plate.shape(), plate.generatedMesh(), plate_transform, detail_range);

	GeometryBuildResult combined = GeneratedMeshComposer(complexity_limits_).compose({
		GeneratedMeshPlacement("hinge_cup", cup.generatedMesh(), cup_transform),
		GeneratedMeshPlacement("hinge_arm", arm.generatedMesh(), arm_transform),
		GeneratedMeshPlacement(
			"mounting_plate", plate.generatedMesh(), plate_transform)});
	if (!combined.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}

	const SpatialObjectId owner(specification.objectIdentifier());
	const float plate_center_x =
		cup_radius + specification.armLength() + specification.plateWidth() * 0.5f;
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("cup_mount"), owner, SpatialInterfaceType::Socket,
		SpatialInterfaceFrame(
			glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.cupDiameter(), specification.cupDiameter()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Circular,
			InterfaceGender::Male,
			specification.cupDiameter(),
			std::nullopt,
			std::nullopt,
			"cabinet-hinge-cup"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
	interfaces.emplace_back(
		SpatialInterfaceId("hinge_axis"), owner, SpatialInterfaceType::Hinge,
		SpatialInterfaceFrame(
			glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::axisSegment(specification.cupDiameter()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Axis,
			InterfaceGender::Neutral,
			std::nullopt,
			std::nullopt,
			std::nullopt,
			"cabinet-hinge-axis"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
	interfaces.emplace_back(
		SpatialInterfaceId("plate_mount"), owner, SpatialInterfaceType::Mate,
		SpatialInterfaceFrame(
			glm::vec3(plate_center_x, 0.0f, 0.0f),
			glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.plateWidth(), specification.plateHeight()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular,
			InterfaceGender::Neutral,
			std::nullopt,
			specification.plateWidth(),
			specification.plateHeight(),
			"cabinet-hinge-plate"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
	for (const auto &screw :
	     std::vector<std::pair<std::string, float>>{
		     {"screw_interface_left", -specification.plateHeight() * 0.25f},
		     {"screw_interface_right", specification.plateHeight() * 0.25f}}) {
		interfaces.emplace_back(
			SpatialInterfaceId(screw.first), owner, SpatialInterfaceType::Fastener,
			SpatialInterfaceFrame(
				glm::vec3(plate_center_x, screw.second, 0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::point(),
			InterfaceCompatibilityProfile(
				InterfaceShape::Point,
				InterfaceGender::Neutral,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				"cabinet-hinge-screw"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));
	}

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts),
			combined.generatedMesh(),
			std::move(interfaces),
			detail_level));
}
