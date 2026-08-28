#include "vehicle/service/VehicleExteriorAssemblyBuilder.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"
#include "vehicle/service/VehicleAssemblyCompositionService.h"
#include "vehicle/service/VehiclePrimitivePartFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<glm::vec2> chamfered_profile(
	float width,
	float height,
	float chamfer,
	float vertical_offset = 0.0f)
{
	const float half_width = width * 0.5f;
	const float half_height = height * 0.5f;
	return {
		{-half_width + chamfer, vertical_offset - half_height},
		{half_width - chamfer, vertical_offset - half_height},
		{half_width, vertical_offset - half_height + chamfer},
		{half_width, vertical_offset + half_height - chamfer},
		{half_width - chamfer, vertical_offset + half_height},
		{-half_width + chamfer, vertical_offset + half_height},
		{-half_width, vertical_offset + half_height - chamfer},
		{-half_width, vertical_offset - half_height + chamfer}};
}

VehicleAssemblyBuildResult compose_assembly(
	std::vector<VehicleAssemblyPart> parts,
	std::vector<SpatialInterface> interfaces,
	GeometryDetailLevel detail_level,
	const GeometryComplexityLimits &complexity_limits)
{
	if (parts.empty()) {
		return VehicleAssemblyBuildResult::succeeded(VehicleAssemblyGeometry(
			{}, GeneratedPrimitiveMesh(std::make_shared<Mesh>(), {}),
			std::move(interfaces), detail_level));
	}
	GeometryBuildResult combined =
		VehicleAssemblyCompositionService(complexity_limits).compose(parts);
	if (!combined.succeeded()) {
		return VehicleAssemblyBuildResult::failed(combined.firstDiagnostic());
	}
	return VehicleAssemblyBuildResult::succeeded(VehicleAssemblyGeometry(
		std::move(parts), combined.generatedMesh(), std::move(interfaces),
		detail_level));
}

SpatialInterface body_mount_interface(
	const std::string &owner_identifier,
	glm::vec3 origin,
	glm::vec3 normal)
{
	return SpatialInterface(
		SpatialInterfaceId("body_mount"), SpatialObjectId(owner_identifier),
		SpatialInterfaceType::Mate,
		SpatialInterfaceFrame(origin, normal, glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::point(),
		InterfaceCompatibilityProfile(
			InterfaceShape::Point, InterfaceGender::Male,
			std::nullopt, std::nullopt, std::nullopt,
			"vehicle-body-mount"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));
}

std::vector<SpatialInterface> panel_interfaces(
	const std::string &owner_identifier,
	glm::vec3 hinge_origin,
	glm::vec3 hinge_axis,
	glm::vec3 latch_origin)
{
	const SpatialObjectId owner(owner_identifier);
	return {
		SpatialInterface(
			SpatialInterfaceId("hinge_axis"), owner,
			SpatialInterfaceType::Hinge,
			SpatialInterfaceFrame(
				hinge_origin, hinge_axis, glm::vec3(0.0f, 1.0f, 0.0f)),
			SpatialInterfaceRegion::axisSegment(0.36f),
			InterfaceCompatibilityProfile(
				InterfaceShape::Axis, InterfaceGender::Male,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-panel-hinge"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.002f)),
		SpatialInterface(
			SpatialInterfaceId("latch"), owner,
			SpatialInterfaceType::Mate,
			SpatialInterfaceFrame(
				latch_origin, glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f)),
			SpatialInterfaceRegion::point(),
			InterfaceCompatibilityProfile(
				InterfaceShape::Point, InterfaceGender::Male,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-panel-latch"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.003f)),
		SpatialInterface(
			SpatialInterfaceId("seal_path"), owner,
			SpatialInterfaceType::Seal,
			SpatialInterfaceFrame(
				glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f),
				glm::vec3(0.0f, 1.0f, 0.0f)),
			SpatialInterfaceRegion::objectBoundaryFace("panel_perimeter"),
			InterfaceCompatibilityProfile(
				InterfaceShape::Rectangular, InterfaceGender::Neutral,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-weather-seal"),
			SpatialClearanceRequirement(0.003f, 0.004f, 0.006f))};
}

std::vector<SpatialInterface> glazing_interfaces(
	const std::string &owner_identifier)
{
	const SpatialObjectId owner(owner_identifier);
	return {
		SpatialInterface(
			SpatialInterfaceId("frame_seat"), owner,
			SpatialInterfaceType::Seat,
			SpatialInterfaceFrame(
				glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::objectBoundaryFace("glass_perimeter"),
			InterfaceCompatibilityProfile(
				InterfaceShape::Rectangular, InterfaceGender::Male,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-glazing-seat"),
			SpatialClearanceRequirement(0.003f, 0.004f, 0.007f)),
		SpatialInterface(
			SpatialInterfaceId("seal_path"), owner,
			SpatialInterfaceType::Seal,
			SpatialInterfaceFrame(
				glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::objectBoundaryFace("glass_perimeter"),
			InterfaceCompatibilityProfile(
				InterfaceShape::Rectangular, InterfaceGender::Neutral,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-weather-seal"),
			SpatialClearanceRequirement(0.002f, 0.004f, 0.006f))};
}

bool append_part(
	const VehiclePrimitivePartBuildResult &result,
	std::vector<VehicleAssemblyPart> *parts,
	std::string *diagnostic)
{
	if (!result.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = result.diagnostic();
		return false;
	}
	parts->push_back(*result.part());
	return true;
}

} // namespace

VehicleSubsystemAssemblyBuildResult VehicleExteriorAssemblyBuilder::build(
	const VehicleDefinition &definition,
	const VehicleBodySpecification &body_specification,
	GeometryDetailLevel detail_level) const
{
	if (body_specification.panelCuts().size() < 6u) {
		return VehicleSubsystemAssemblyBuildResult::failed(
			"Vehicle exterior requires four door, hood, and tailgate panel cuts.");
	}
	const VehiclePackage &package = definition.package();
	VehiclePrimitivePartFactory part_factory(complexity_limits_);
	std::vector<VehiclePlacedAssembly> assemblies;
	std::string diagnostic;

	auto add_glazing = [&](
		const std::string &identifier,
		glm::vec3 position,
		std::vector<VehicleLoftPanelSection> sections) -> bool {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::CoarseShape)) {
			if (!append_part(
					part_factory.buildLoftedSolid(
						"glass", "CurvedGlazingPanel", "vehicle-glass",
						std::move(sections), glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::CoarseShape,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) return false;
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), glazing_interfaces(identifier), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), position));
		return true;
	};

	if (!add_glazing(
			"Windshield", glm::vec3(0.0f, 1.08f, -0.54f),
			{{-0.040f, chamfered_profile(1.46f, 0.58f, 0.08f, -0.025f)},
			 {0.000f, chamfered_profile(1.43f, 0.60f, 0.08f, 0.000f)},
			 {0.040f, chamfered_profile(1.39f, 0.58f, 0.08f, 0.028f)}}) ||
	    !add_glazing(
			"RearWindow", glm::vec3(0.0f, 1.10f, -2.48f),
			{{-0.040f, chamfered_profile(1.28f, 0.49f, 0.07f, 0.020f)},
			 {0.000f, chamfered_profile(1.34f, 0.52f, 0.07f, 0.000f)},
			 {0.040f, chamfered_profile(1.30f, 0.49f, 0.07f, -0.020f)}})) {
		return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
	}

	auto add_side_glass = [&](
		const std::string &identifier,
		float x,
		float z,
		float length) -> bool {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::CoarseShape)) {
			glm::mat4 orientation = glm::rotate(
				glm::mat4(1.0f), glm::radians(90.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"glass", "CurvedDoorGlazing", "vehicle-glass",
						glm::vec3(length, 0.43f, 0.012f), 0.055f,
						orientation,
						GeometryDetailRange(
							GeometryDetailLevel::CoarseShape,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) return false;
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), glazing_interfaces(identifier), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), glm::vec3(x, 1.12f, z)));
		return true;
	};
	if (!add_side_glass("FrontDoorGlassLeft", -0.805f, -1.02f, 0.76f) ||
	    !add_side_glass("FrontDoorGlassRight", 0.805f, -1.02f, 0.76f) ||
	    !add_side_glass("RearDoorGlassLeft", -0.805f, -1.86f, 0.72f) ||
	    !add_side_glass("RearDoorGlassRight", 0.805f, -1.86f, 0.72f)) {
		return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
	}

	auto add_side_panel = [&](
		const std::string &identifier,
		float x,
		float z,
		float length,
		bool left_side) -> bool {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			glm::mat4 orientation = glm::rotate(
				glm::mat4(1.0f), glm::radians(90.0f),
				glm::vec3(0.0f, 1.0f, 0.0f));
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"outer_skin", "HostedBodyPanel", "vehicle-body-paint",
						glm::vec3(length, 0.72f, 0.022f), 0.065f,
						orientation,
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) return false;
		}
		const float hinge_z = -length * 0.5f;
		const float latch_z = length * 0.5f;
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts),
			panel_interfaces(
				identifier, glm::vec3(0.0f, 0.0f, hinge_z),
				glm::vec3(0.0f, 1.0f, 0.0f),
				glm::vec3(0.0f, 0.0f, latch_z)),
			detail_level, complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(
				glm::mat4(1.0f),
				glm::vec3(x, 0.69f, z)));
		(void)left_side;
		return true;
	};
	if (!add_side_panel("FrontDoorLeft", -0.942f, -1.02f, 0.80f, true) ||
	    !add_side_panel("FrontDoorRight", 0.942f, -1.02f, 0.80f, false) ||
	    !add_side_panel("RearDoorLeft", -0.942f, -1.88f, 0.76f, true) ||
	    !add_side_panel("RearDoorRight", 0.942f, -1.88f, 0.76f, false)) {
		return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
	}

	auto add_panel = [&](
		const std::string &identifier,
		glm::vec3 dimensions,
		glm::vec3 position,
		glm::mat4 orientation,
		glm::vec3 hinge_axis) -> bool {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"outer_skin", "HostedBodyPanel", "vehicle-body-paint",
						dimensions, 0.065f, orientation,
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) return false;
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts),
			panel_interfaces(
				identifier, glm::vec3(0.0f, 0.0f, -dimensions.y * 0.5f),
				hinge_axis, glm::vec3(0.0f, 0.0f, dimensions.y * 0.5f)),
			detail_level, complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), position));
		return true;
	};
	const glm::mat4 horizontal_panel = glm::rotate(
		glm::mat4(1.0f), glm::radians(90.0f),
		glm::vec3(1.0f, 0.0f, 0.0f));
	if (!add_panel(
			"Hood", glm::vec3(1.45f, 0.70f, 0.024f),
			glm::vec3(0.0f, 0.76f, 0.30f), horizontal_panel,
			glm::vec3(1.0f, 0.0f, 0.0f)) ||
	    !add_panel(
			"Tailgate", glm::vec3(1.36f, 0.70f, 0.024f),
			glm::vec3(0.0f, 0.91f, package.rearBumperStation() + 0.035f),
			glm::mat4(1.0f), glm::vec3(1.0f, 0.0f, 0.0f))) {
		return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
	}

	auto add_lamp = [&](
		const std::string &identifier,
		glm::vec3 position,
		const std::string &lens_material) -> bool {
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"housing", "LampHousing", "lamp-housing",
						glm::vec3(0.42f, 0.14f, 0.075f), 0.035f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic) ||
			    !append_part(
					part_factory.buildCenteredRoundedBox(
						"lens", "CurvedLampLens", lens_material,
						glm::vec3(0.38f, 0.10f, 0.018f), 0.025f,
						glm::translate(
							glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.045f)),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) return false;
		}
		std::vector<SpatialInterface> interfaces;
		interfaces.push_back(body_mount_interface(
			identifier, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
		interfaces.emplace_back(
			SpatialInterfaceId("electrical_port"), SpatialObjectId(identifier),
			SpatialInterfaceType::ElectricalPort,
			SpatialInterfaceFrame(
				glm::vec3(0.0f, 0.0f, -0.04f),
				glm::vec3(0.0f, 0.0f, -1.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::point(),
			InterfaceCompatibilityProfile(
				InterfaceShape::Point, InterfaceGender::Female,
				std::nullopt, std::nullopt, std::nullopt,
				"vehicle-light-power"),
			SpatialClearanceRequirement(0.0f, 0.0f, 0.002f));
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts), std::move(interfaces), detail_level,
			complexity_limits_);
		if (!built.succeeded()) {
			diagnostic = built.diagnostic();
			return false;
		}
		assemblies.emplace_back(
			identifier, *built.geometry(),
			glm::translate(glm::mat4(1.0f), position));
		return true;
	};
	if (!add_lamp("HeadlampLeft", glm::vec3(-0.56f, 0.70f, 0.885f), "headlamp-lens") ||
	    !add_lamp("HeadlampRight", glm::vec3(0.56f, 0.70f, 0.885f), "headlamp-lens") ||
	    !add_lamp("TailLampLeft", glm::vec3(-0.55f, 0.77f, -3.765f), "taillamp-lens") ||
	    !add_lamp("TailLampRight", glm::vec3(0.55f, 0.77f, -3.765f), "taillamp-lens")) {
		return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
	}

	{
		std::vector<VehicleAssemblyPart> parts;
		if (geometryDetailLevelRank(detail_level) >=
		    geometryDetailLevelRank(GeometryDetailLevel::Assembly)) {
			if (!append_part(
					part_factory.buildCenteredRoundedBox(
						"grille", "SurfacePatternGrid", "dark-grille",
						glm::vec3(0.78f, 0.18f, 0.032f), 0.028f,
						glm::mat4(1.0f),
						GeometryDetailRange(
							GeometryDetailLevel::Assembly,
							GeometryDetailLevel::FastenersAndSeals),
						detail_level),
					&parts, &diagnostic)) {
				return VehicleSubsystemAssemblyBuildResult::failed(diagnostic);
			}
		}
		VehicleAssemblyBuildResult built = compose_assembly(
			std::move(parts),
			{body_mount_interface(
				"FrontGrille", glm::vec3(0.0f),
				glm::vec3(0.0f, 0.0f, -1.0f))},
			detail_level, complexity_limits_);
		if (!built.succeeded()) {
			return VehicleSubsystemAssemblyBuildResult::failed(built.diagnostic());
		}
		assemblies.emplace_back(
			"FrontGrille", *built.geometry(),
			glm::translate(
				glm::mat4(1.0f), glm::vec3(0.0f, 0.59f, 0.922f)));
	}

	return VehicleSubsystemAssemblyBuildResult::succeeded(
		VehicleSubsystemAssemblySet(std::move(assemblies)));
}
