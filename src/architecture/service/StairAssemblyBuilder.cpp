#include "architecture/service/StairAssemblyBuilder.h"

#include "architecture/model/ArchitecturalProfileKind.h"
#include "architecture/service/ArchitecturalProfileLibrary.h"
#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/SweepProfileShapeSpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepProfileMeshGenerator.h"
#include "geometry/service/SweepProfileSpecificationValidator.h"

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

struct StairPartGeometry
{
	std::shared_ptr<const ShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};
};

StairPartGeometry build_extrusion(
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

StairPartGeometry build_sweep(
	const Profile2D &profile,
	std::vector<glm::vec3> path,
	glm::vec3 up_hint,
	const GeometryComplexityLimits &complexity_limits,
	std::string *diagnostic)
{
	SweepProfileShapeSpecificationCandidate candidate;
	candidate.profile.outer_loop = profile.outerLoop().points();
	for (const ProfileLoop2D &inner_loop : profile.innerLoops()) {
		candidate.profile.inner_loops.push_back(inner_loop.points());
	}
	candidate.path_points = std::move(path);
	candidate.up_hint = up_hint;
	auto shape = SweepProfileSpecificationValidator(complexity_limits).validate(
		std::move(candidate), diagnostic);
	if (!shape) return {};
	GeometryBuildResult generated =
		SweepProfileMeshGenerator(complexity_limits).build(*shape);
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

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

}

ArchitecturalAssemblyBuildResult StairAssemblyBuilder::build(
	const StairAssemblySpecification &specification) const
{
	if (specification.objectIdentifier().empty() || specification.stepCount() < 2u ||
	    specification.stepCount() > complexity_limits_.maximumInstanceArrayCount() ||
	    !finite_positive(specification.width()) ||
	    !finite_positive(specification.totalRise()) ||
	    !finite_positive(specification.totalRun()) ||
	    !finite_positive(specification.treadThickness()) ||
	    !std::isfinite(specification.nosingProjection()) ||
	    specification.nosingProjection() < 0.0f ||
	    !finite_positive(specification.stringerWidth()) ||
	    !finite_positive(specification.stringerThickness()) ||
	    !finite_positive(specification.balustradeHeight()) ||
	    !finite_positive(specification.glazingThickness()) ||
	    specification.treadMaterialIdentifier().empty() ||
	    specification.stringerMaterialIdentifier().empty() ||
	    specification.glazingMaterialIdentifier().empty() ||
	    specification.handrailMaterialIdentifier().empty()) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"StairAssembly requires finite positive dimensions, at least two steps, and materials.");
	}

	std::string diagnostic;
	const float tread_depth =
		specification.goingPerStep() + specification.nosingProjection();
	StairPartGeometry tread = build_extrusion(
		rectangle_candidate(specification.width(), specification.treadThickness()),
		tread_depth, complexity_limits_, &diagnostic);
	if (!tread.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	std::shared_ptr<const Profile2D> stringer_profile =
		Profile2DFactory(complexity_limits_).createRectangle(
			specification.stringerThickness(), specification.stringerWidth(),
			&diagnostic);
	if (!stringer_profile) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	const float stringer_x = specification.width() * 0.5f -
	                         specification.stringerThickness() * 0.5f;
	StairPartGeometry left_stringer = build_sweep(
		*stringer_profile,
		{glm::vec3(-stringer_x, 0.0f, 0.0f),
		 glm::vec3(-stringer_x, specification.totalRise(), specification.totalRun())},
		glm::vec3(1.0f, 0.0f, 0.0f), complexity_limits_, &diagnostic);
	if (!left_stringer.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	StairPartGeometry right_stringer = build_sweep(
		*stringer_profile,
		{glm::vec3(stringer_x, 0.0f, 0.0f),
		 glm::vec3(stringer_x, specification.totalRise(), specification.totalRun())},
		glm::vec3(1.0f, 0.0f, 0.0f), complexity_limits_, &diagnostic);
	if (!right_stringer.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	Profile2DCandidate glass_profile;
	glass_profile.outer_loop = {
		glm::vec2(0.0f, 0.0f),
		glm::vec2(specification.totalRun(), specification.totalRise()),
		glm::vec2(
			specification.totalRun(),
			specification.totalRise() + specification.balustradeHeight()),
		glm::vec2(0.0f, specification.balustradeHeight())};
	StairPartGeometry glass = build_extrusion(
		std::move(glass_profile), specification.glazingThickness(),
		complexity_limits_, &diagnostic);
	if (!glass.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	const glm::mat4 glass_transform = glm::translate(
		glm::mat4(1.0f),
		glm::vec3(
			specification.width() * 0.5f + specification.glazingThickness(),
			0.0f, 0.0f)) *
		glm::rotate(
			glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	std::shared_ptr<const ArchitecturalProfileDefinition> handrail_profile =
		ArchitecturalProfileLibrary(complexity_limits_).find(
			ArchitecturalProfileKind::SteelHandrail, &diagnostic);
	if (!handrail_profile) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	const float handrail_x = specification.width() * 0.5f + 0.02f;
	StairPartGeometry handrail = build_sweep(
		handrail_profile->crossSection(),
		{glm::vec3(handrail_x, specification.balustradeHeight(), 0.0f),
		 glm::vec3(
			handrail_x,
			specification.totalRise() + specification.balustradeHeight(),
			specification.totalRun())},
		glm::vec3(1.0f, 0.0f, 0.0f), complexity_limits_, &diagnostic);
	if (!handrail.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	std::vector<ArchitecturalAssemblyPart> parts;
	std::vector<GeneratedMeshPlacement> placements;
	parts.reserve(specification.stepCount() + 4u);
	placements.reserve(specification.stepCount() + 4u);
	for (std::size_t step_index = 0; step_index < specification.stepCount(); ++step_index) {
		const glm::mat4 transform = glm::translate(
			glm::mat4(1.0f),
			glm::vec3(
				0.0f,
				specification.risePerStep() * static_cast<float>(step_index + 1u),
				specification.goingPerStep() * static_cast<float>(step_index) -
					specification.nosingProjection()));
		const std::string identifier = "tread_" + std::to_string(step_index);
		parts.emplace_back(
			identifier, "StairTread", specification.treadMaterialIdentifier(),
			tread.shape, tread.mesh, transform);
		placements.emplace_back(identifier, tread.mesh, transform);
	}
	parts.emplace_back(
		"stringer_left", "StairStringer", specification.stringerMaterialIdentifier(),
		left_stringer.shape, left_stringer.mesh, glm::mat4(1.0f));
	placements.emplace_back("stringer_left", left_stringer.mesh, glm::mat4(1.0f));
	parts.emplace_back(
		"stringer_right", "StairStringer", specification.stringerMaterialIdentifier(),
		right_stringer.shape, right_stringer.mesh, glm::mat4(1.0f));
	placements.emplace_back("stringer_right", right_stringer.mesh, glm::mat4(1.0f));
	parts.emplace_back(
		"glass_balustrade", "GlazingPanel", specification.glazingMaterialIdentifier(),
		glass.shape, glass.mesh, glass_transform);
	placements.emplace_back("glass_balustrade", glass.mesh, glass_transform);
	parts.emplace_back(
		"handrail", "StairHandrail", specification.handrailMaterialIdentifier(),
		handrail.shape, handrail.mesh, glm::mat4(1.0f));
	placements.emplace_back("handrail", handrail.mesh, glm::mat4(1.0f));

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
			glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.goingPerStep()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.0f));
	interfaces.emplace_back(
		SpatialInterfaceId("top_bearing"), owner, SpatialInterfaceType::Bearing,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, specification.totalRise(), specification.totalRun()),
			glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.goingPerStep()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.0f));

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces)));
}
