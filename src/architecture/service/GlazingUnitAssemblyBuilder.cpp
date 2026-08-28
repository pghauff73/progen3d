#include "architecture/service/GlazingUnitAssemblyBuilder.h"

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
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
#include <vector>

namespace {

struct GlazingPartGeometry
{
	std::shared_ptr<const ShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};
};

GlazingPartGeometry build_extrusion(
	Profile2DCandidate profile,
	float depth,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	ExtrudeProfileShapeSpecificationCandidate candidate;
	candidate.profile = std::move(profile);
	candidate.depth = depth;
	auto shape = ExtrudeProfileSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult generated =
		ExtrudeProfileMeshGenerator(complexity_limits).build(*shape);
	if (!generated.succeeded()) {
		if (diagnostic != nullptr) *diagnostic = generated.firstDiagnostic();
		return {};
	}
	return {shape, generated.generatedMesh()};
}

Profile2DCandidate rectangle_candidate(float width, float height)
{
	const float half_width = width * 0.5f;
	const float half_height = height * 0.5f;
	return {{{-half_width, -half_height},
	         {half_width, -half_height},
	         {half_width, half_height},
	         {-half_width, half_height}},
	        {}};
}

}

ArchitecturalAssemblyBuildResult GlazingUnitAssemblyBuilder::build(
	const GlazingUnitSpecification &specification) const
{
	if (specification.objectIdentifier().empty() ||
	    specification.glassMaterialIdentifier().empty() ||
	    specification.spacerMaterialIdentifier().empty() ||
	    !std::isfinite(specification.width()) ||
	    !std::isfinite(specification.height()) ||
	    !std::isfinite(specification.glassThickness()) ||
	    !std::isfinite(specification.airGap()) ||
	    !std::isfinite(specification.spacerWidth()) ||
	    specification.width() <= specification.spacerWidth() * 2.0f ||
	    specification.height() <= specification.spacerWidth() * 2.0f ||
	    specification.glassThickness() <= 0.0f ||
	    specification.airGap() <= 0.0f ||
	    specification.spacerWidth() <= 0.0f) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"GlazingUnit requires finite positive pane, gap, and spacer dimensions.");
	}

	std::string diagnostic;
	GlazingPartGeometry pane = build_extrusion(
		rectangle_candidate(specification.width(), specification.height()),
		specification.glassThickness(), complexity_limits_, &diagnostic);
	if (!pane.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	Profile2DCandidate spacer_profile =
		rectangle_candidate(specification.width(), specification.height());
	spacer_profile.inner_loops.push_back(
		rectangle_candidate(
			specification.width() - specification.spacerWidth() * 2.0f,
			specification.height() - specification.spacerWidth() * 2.0f)
			.outer_loop);
	GlazingPartGeometry spacer = build_extrusion(
		std::move(spacer_profile), specification.airGap(), complexity_limits_,
		&diagnostic);
	if (!spacer.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	const glm::mat4 exterior_transform(1.0f);
	const glm::mat4 spacer_transform = glm::translate(
		glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, specification.glassThickness()));
	const glm::mat4 interior_transform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(
			0.0f, 0.0f,
			specification.glassThickness() + specification.airGap()));

	std::vector<ArchitecturalAssemblyPart> parts;
	parts.emplace_back(
		"exterior_glass", "GlassOuter", specification.glassMaterialIdentifier(),
		pane.shape, pane.mesh, exterior_transform);
	parts.emplace_back(
		"spacer", "GlazingSpacer", specification.spacerMaterialIdentifier(),
		spacer.shape, spacer.mesh, spacer_transform);
	parts.emplace_back(
		"interior_glass", "GlassInner", specification.glassMaterialIdentifier(),
		pane.shape, pane.mesh, interior_transform);

	GeometryBuildResult combined = GeneratedMeshComposer(complexity_limits_).compose({
		GeneratedMeshPlacement("exterior_glass", pane.mesh, exterior_transform),
		GeneratedMeshPlacement("spacer", spacer.mesh, spacer_transform),
		GeneratedMeshPlacement("interior_glass", pane.mesh, interior_transform)});
	if (!combined.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}

	const SpatialObjectId owner(specification.objectIdentifier());
	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("exterior_face"), owner, SpatialInterfaceType::Seal,
		SpatialInterfaceFrame(
			glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.height()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.0f));
	interfaces.emplace_back(
		SpatialInterfaceId("interior_face"), owner, SpatialInterfaceType::Seal,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, 0.0f, specification.totalDepth()),
			glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.height()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.0f));

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces)));
}
