#include "architecture/service/DrawerAssemblyBuilder.h"

#include "architecture/service/ArchitecturalPartGeometryBuilder.h"
#include "geometry/model/GeneratedMeshPlacement.h"
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

ArchitecturalAssemblyBuildResult DrawerAssemblyBuilder::build(
	const DrawerSpecification &specification,
	GeometryDetailLevel detail_level) const
{
	if (specification.objectIdentifier().empty() ||
	    specification.panelMaterialIdentifier().empty() ||
	    specification.runnerMaterialIdentifier().empty() ||
	    !std::isfinite(specification.width()) ||
	    !std::isfinite(specification.height()) ||
	    !std::isfinite(specification.depth()) ||
	    !std::isfinite(specification.panelThickness()) ||
	    !std::isfinite(specification.frontThickness()) ||
	    !std::isfinite(specification.runnerWidth()) ||
	    !std::isfinite(specification.runnerHeight()) ||
	    specification.width() <= specification.panelThickness() * 2.0f ||
	    specification.height() <= specification.panelThickness() * 2.0f ||
	    specification.depth() <= specification.panelThickness() ||
	    specification.panelThickness() <= 0.0f ||
	    specification.frontThickness() <= 0.0f ||
	    specification.runnerWidth() <= 0.0f ||
	    specification.runnerHeight() <= 0.0f) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"Drawer requires finite positive dimensions that contain its panels and runners.");
	}

	ArchitecturalPartGeometryBuilder geometry_builder(complexity_limits_);
	std::string diagnostic;
	std::vector<ArchitecturalAssemblyPart> parts;
	std::vector<GeneratedMeshPlacement> placements;
	const GeometryDetailRange detail_range(
		GeometryDetailLevel::Component,
		GeometryDetailLevel::FastenersAndSeals);
	auto append_part = [
		&parts,
		&placements,
		&diagnostic,
		detail_range](
		std::string identifier,
		std::string role,
		const std::string &material,
		ArchitecturalPartGeometry geometry,
		const glm::vec3 &translation) -> bool {
		if (!geometry.isValid()) return false;
		const glm::mat4 transform =
			glm::translate(glm::mat4(1.0f), translation);
		placements.emplace_back(identifier, geometry.generatedMesh(), transform);
		parts.emplace_back(
			std::move(identifier),
			std::move(role),
			material,
			geometry.shape(),
			geometry.generatedMesh(),
			transform,
			detail_range);
		return true;
	};

	const float panel = specification.panelThickness();
	const float half_width = specification.width() * 0.5f;
	const float half_height = specification.height() * 0.5f;
	const float internal_width = specification.width() - panel * 2.0f;
	const float internal_height = specification.height() - panel;
	if (!append_part(
			"drawer_side_left",
			"DrawerSideLeft",
			specification.panelMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				panel,
				specification.height(),
				specification.depth(),
				detail_level,
				&diagnostic),
			glm::vec3(-half_width + panel * 0.5f, 0.0f, 0.0f)) ||
	    !append_part(
			"drawer_side_right",
			"DrawerSideRight",
			specification.panelMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				panel,
				specification.height(),
				specification.depth(),
				detail_level,
				&diagnostic),
			glm::vec3(half_width - panel * 0.5f, 0.0f, 0.0f)) ||
	    !append_part(
			"drawer_bottom",
			"DrawerBottom",
			specification.panelMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				internal_width,
				panel,
				specification.depth(),
				detail_level,
				&diagnostic),
			glm::vec3(0.0f, -half_height + panel * 0.5f, 0.0f)) ||
	    !append_part(
			"drawer_back",
			"DrawerBack",
			specification.panelMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				internal_width,
				internal_height,
				panel,
				detail_level,
				&diagnostic),
			glm::vec3(0.0f, panel * 0.5f, specification.depth() - panel)) ||
	    !append_part(
			"drawer_front",
			"DrawerFront",
			specification.panelMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				specification.width(),
				specification.height(),
				specification.frontThickness(),
				detail_level,
				&diagnostic),
			glm::vec3(0.0f, 0.0f, -specification.frontThickness())) ||
	    !append_part(
			"drawer_runner_left",
			"DrawerRunnerLeft",
			specification.runnerMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				specification.runnerWidth(),
				specification.runnerHeight(),
				specification.depth(),
				detail_level,
				&diagnostic),
			glm::vec3(
				-half_width - specification.runnerWidth() * 0.5f,
				-half_height + specification.runnerHeight(),
				0.0f)) ||
	    !append_part(
			"drawer_runner_right",
			"DrawerRunnerRight",
			specification.runnerMaterialIdentifier(),
			geometry_builder.buildRectangularPrism(
				specification.runnerWidth(),
				specification.runnerHeight(),
				specification.depth(),
				detail_level,
				&diagnostic),
			glm::vec3(
				half_width + specification.runnerWidth() * 0.5f,
				-half_height + specification.runnerHeight(),
				0.0f))) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	GeometryBuildResult combined =
		GeneratedMeshComposer(complexity_limits_).compose(placements);
	if (!combined.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}

	const SpatialObjectId owner(specification.objectIdentifier());
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("cabinet_insert"), owner, SpatialInterfaceType::Insert,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, 0.0f, specification.depth()),
			glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.height()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular,
			InterfaceGender::Male,
			std::nullopt,
			specification.width(),
			specification.height(),
			"cabinet-drawer-opening"),
		SpatialClearanceRequirement(0.003f, 0.001f, 0.006f));
	for (const auto &runner :
	     std::vector<std::pair<std::string, float>>{
		     {"runner_left", -half_width - specification.runnerWidth() * 0.5f},
		     {"runner_right", half_width + specification.runnerWidth() * 0.5f}}) {
		interfaces.emplace_back(
			SpatialInterfaceId(runner.first), owner, SpatialInterfaceType::Slide,
			SpatialInterfaceFrame(
				glm::vec3(
					runner.second,
					-half_height + specification.runnerHeight(),
					0.0f),
				glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
			SpatialInterfaceRegion::axisSegment(specification.depth()),
			InterfaceCompatibilityProfile(
				InterfaceShape::Axis,
				InterfaceGender::Neutral,
				std::nullopt,
				std::nullopt,
				std::nullopt,
				"drawer-runner"),
			SpatialClearanceRequirement(0.001f, 0.0f, 0.003f));
	}
	interfaces.emplace_back(
		SpatialInterfaceId("front_hardware_mount"),
		owner,
		SpatialInterfaceType::Fastener,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, 0.0f, -specification.frontThickness()),
			glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width() * 0.4f, specification.height() * 0.4f),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular,
			InterfaceGender::Neutral,
			std::nullopt,
			specification.width() * 0.4f,
			specification.height() * 0.4f,
			"drawer-front-hardware"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.001f));

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts),
			combined.generatedMesh(),
			std::move(interfaces),
			detail_level));
}
