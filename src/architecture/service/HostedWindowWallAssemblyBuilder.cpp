#include "architecture/service/HostedWindowWallAssemblyBuilder.h"

#include "architecture/service/WindowFrameAssemblyBuilder.h"
#include "architecture/service/WindowOpeningCollisionPositioningService.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/Profile2DFactory.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"

#include <cmath>
#include <memory>
#include <utility>

HostedWindowWallAssemblyBuildResult HostedWindowWallAssemblyBuilder::build(
	const WindowOpeningSpecification &opening,
	const WindowFrameSpecification &frame,
	GeometryDetailLevel detail_level) const
{
	if (opening.objectIdentifier().empty() ||
	    !std::isfinite(opening.wallWidth()) ||
	    !std::isfinite(opening.wallHeight()) ||
	    !std::isfinite(opening.wallDepth()) ||
	    !std::isfinite(opening.openingCenter().x) ||
	    !std::isfinite(opening.openingCenter().y) ||
	    opening.wallWidth() <= 0.0f || opening.wallHeight() <= 0.0f ||
	    opening.wallDepth() <= 0.0f || opening.openingWidth() <= 0.0f ||
	    opening.openingHeight() <= 0.0f ||
	    std::fabs(opening.openingCenter().x) + opening.openingWidth() * 0.5f >=
		    opening.wallWidth() * 0.5f ||
	    std::fabs(opening.openingCenter().y) + opening.openingHeight() * 0.5f >=
		    opening.wallHeight() * 0.5f) {
		return HostedWindowWallAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidHole,
			"Hosted wall opening must be finite, positive, and strictly inside the wall profile.");
	}

	std::string diagnostic;
	std::shared_ptr<const Profile2D> wall_profile =
		Profile2DFactory(complexity_limits_).createRectangle(
			opening.wallWidth(), opening.wallHeight(), &diagnostic);
	std::shared_ptr<const Profile2D> opening_profile =
		Profile2DFactory(complexity_limits_).createRectangle(
			opening.openingWidth(), opening.openingHeight(), &diagnostic);
	if (!wall_profile || !opening_profile) {
		return HostedWindowWallAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	ExtrudeProfileShapeSpecificationCandidate wall_candidate;
	wall_candidate.profile.outer_loop = wall_profile->outerLoop().points();
	std::vector<glm::vec2> translated_opening;
	translated_opening.reserve(opening_profile->outerLoop().points().size());
	for (const glm::vec2 &point : opening_profile->outerLoop().points()) {
		translated_opening.push_back(point + opening.openingCenter());
	}
	wall_candidate.profile.inner_loops.push_back(std::move(translated_opening));
	wall_candidate.depth = opening.wallDepth();
	auto wall_shape = ExtrudeProfileSpecificationValidator(complexity_limits_).validate(
		std::move(wall_candidate), &diagnostic);
	if (!wall_shape) {
		return HostedWindowWallAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidHole, diagnostic);
	}
	GeometryBuildResult wall_mesh =
		ExtrudeProfileMeshGenerator(complexity_limits_).build(*wall_shape);
	if (!wall_mesh.succeeded()) {
		return HostedWindowWallAssemblyBuildResult::failed(
			wall_mesh.status(), wall_mesh.firstDiagnostic());
	}

	ArchitecturalAssemblyBuildResult frame_result =
		WindowFrameAssemblyBuilder(
			complexity_limits_, reference_binding_catalog_)
			.build(frame, detail_level);
	if (!frame_result.succeeded()) {
		return HostedWindowWallAssemblyBuildResult::failed(
			frame_result.status(), frame_result.diagnostic());
	}
	OpeningBoundaryPlacementResult placement =
		WindowOpeningCollisionPositioningService().position(frame, opening);
	if (!placement.succeeded()) {
		return HostedWindowWallAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidHole, placement.diagnostic());
	}

	std::vector<ArchitecturalAssemblyPart> parts;
	std::vector<GeneratedMeshPlacement> placements;
	parts.emplace_back(
		"host_wall", "HostedWall", "warmwhitematteplaster",
		wall_shape, wall_mesh.generatedMesh(), glm::mat4(1.0f));
	placements.emplace_back(
		"host_wall", wall_mesh.generatedMesh(), glm::mat4(1.0f));
	for (const ArchitecturalAssemblyPart &window_part :
	     frame_result.geometry()->parts()) {
		const glm::mat4 transform =
			*placement.localTransform() * window_part.localTransform();
		parts.emplace_back(
			window_part.partIdentifier(),
			window_part.semanticRole(),
			window_part.materialIdentifier(),
			window_part.shape(),
			window_part.generatedMesh(),
			transform,
			window_part.detailRange());
		placements.emplace_back(
			window_part.partIdentifier(),
			window_part.generatedMesh(),
			transform);
	}
	GeometryBuildResult combined =
		GeneratedMeshComposer(complexity_limits_).compose(placements);
	if (!combined.succeeded()) {
		return HostedWindowWallAssemblyBuildResult::failed(
			combined.status(), combined.firstDiagnostic());
	}

	std::vector<SpatialInterface> interfaces;
	interfaces.emplace_back(
		SpatialInterfaceId("opening_boundary"),
		SpatialObjectId(opening.objectIdentifier()),
		SpatialInterfaceType::Socket,
		SpatialInterfaceFrame(
			glm::vec3(opening.openingCenter(), opening.wallDepth() * 0.5f),
			glm::vec3(0.0f, 0.0f, -1.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			opening.openingWidth(), opening.openingHeight()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular,
			InterfaceGender::Female,
			std::nullopt,
			opening.openingWidth(),
			opening.openingHeight(),
			"HostedWindowOpening"),
		SpatialClearanceRequirement(
			frame.perimeterClearance(),
			frame.perimeterClearance(),
			frame.perimeterClearance()));
	for (const SpatialInterface &window_interface :
	     frame_result.geometry()->interfaces()) {
		const glm::vec3 transformed_origin = glm::vec3(
			*placement.localTransform() *
			glm::vec4(window_interface.localFrame().localOrigin(), 1.0f));
		interfaces.push_back(window_interface.withLocalFrame(
			SpatialInterfaceFrame(
				transformed_origin,
				window_interface.localFrame().localNormal(),
				window_interface.localFrame().localTangent())));
	}

	ArchitecturalAssemblyGeometry geometry(
		std::move(parts),
		combined.generatedMesh(),
		std::move(interfaces),
		detail_level);
	return HostedWindowWallAssemblyBuildResult::succeeded(
		HostedWindowWallAssembly(std::move(geometry), std::move(placement)));
}
