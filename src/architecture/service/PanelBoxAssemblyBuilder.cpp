#include "architecture/service/PanelBoxAssemblyBuilder.h"

#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/Profile2DFactory.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>
#include <memory>
#include <string>
#include <utility>

namespace {

struct PanelBuild
{
	std::shared_ptr<const ShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};
};

PanelBuild build_panel(
	float width,
	float height,
	float depth,
	const GeometryComplexityLimits &limits,
	std::string *diagnostic)
{
	auto profile = Profile2DFactory(limits).createRectangle(width, height, diagnostic);
	if (!profile) return {};
	ExtrudeProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile->outerLoop().points();
	candidate.depth = depth;
	auto shape = ExtrudeProfileSpecificationValidator(limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult built = ExtrudeProfileMeshGenerator(limits).build(*shape);
	if (!built.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = built.firstDiagnostic();
		return {};
	}
	return {shape, built.generatedMesh()};
}

}

ArchitecturalAssemblyBuildResult PanelBoxAssemblyBuilder::build(
	const PanelBoxSpecification &specification) const
{
	if (specification.objectIdentifier().empty() ||
	    specification.materialIdentifier().empty() ||
	    !std::isfinite(specification.width()) ||
	    !std::isfinite(specification.height()) ||
	    !std::isfinite(specification.depth()) ||
	    !std::isfinite(specification.panelThickness()) ||
	    !std::isfinite(specification.backPanelThickness()) ||
	    specification.width() <= specification.panelThickness() * 2.0f ||
	    specification.height() <= specification.panelThickness() * 2.0f ||
	    specification.depth() <= specification.backPanelThickness() ||
	    specification.panelThickness() <= 0.0f ||
	    specification.backPanelThickness() <= 0.0f) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"PanelBox requires finite positive dimensions larger than its panel thicknesses.");
	}

	std::vector<ArchitecturalAssemblyPart> parts;
	std::vector<GeneratedMeshPlacement> placements;
	std::string diagnostic;
	auto append_panel = [&parts, &placements, &diagnostic, &specification, this](
		const std::string &identifier,
		const std::string &role,
		float width,
		float height,
		float depth,
		const glm::vec3 &translation) -> bool {
		PanelBuild panel = build_panel(width, height, depth, complexity_limits_, &diagnostic);
		if (!panel.shape) return false;
		const glm::mat4 transform = glm::translate(glm::mat4(1.0f), translation);
		placements.emplace_back(identifier, panel.mesh, transform);
		parts.emplace_back(
			identifier, role, specification.materialIdentifier(),
			panel.shape, panel.mesh, transform);
		return true;
	};

	const float panel = specification.panelThickness();
	const float half_width = specification.width() * 0.5f;
	const float half_height = specification.height() * 0.5f;
	if (!append_panel(
			"left_side", "CabinetSide", panel, specification.height(),
			specification.depth(), glm::vec3(-half_width + panel * 0.5f, 0.0f, 0.0f)) ||
	    !append_panel(
			"right_side", "CabinetSide", panel, specification.height(),
			specification.depth(), glm::vec3(half_width - panel * 0.5f, 0.0f, 0.0f)) ||
	    !append_panel(
			"bottom", "CabinetBottom", specification.width() - panel * 2.0f,
			panel, specification.depth(), glm::vec3(0.0f, -half_height + panel * 0.5f, 0.0f)) ||
	    !append_panel(
			"top", "CabinetTop", specification.width() - panel * 2.0f,
			panel, specification.depth(), glm::vec3(0.0f, half_height - panel * 0.5f, 0.0f)) ||
	    !append_panel(
			"back", "CabinetBack", specification.width() - panel * 2.0f,
			specification.height() - panel * 2.0f,
			specification.backPanelThickness(),
			glm::vec3(0.0f, 0.0f, 0.0f))) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	if (specification.includesFrontPanel() &&
	    !append_panel(
			"front", "CabinetFront", specification.width(), specification.height(),
			panel,
			glm::vec3(0.0f, 0.0f, specification.depth() - panel))) {
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
		SpatialInterfaceId("bottom_support"), owner, SpatialInterfaceType::Support,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, -half_height, specification.depth() * 0.5f),
			glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.depth()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.0f));
	interfaces.emplace_back(
		SpatialInterfaceId("back_clearance"), owner, SpatialInterfaceType::Mate,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.height()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.01f, 0.008f, 0.015f));

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces)));
}
