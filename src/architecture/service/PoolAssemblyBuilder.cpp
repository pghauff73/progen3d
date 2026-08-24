#include "architecture/service/PoolAssemblyBuilder.h"

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/model/ShellLoftShapeSpecification.h"
#include "geometry/model/SweepDiskShapeSpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/LoftMeshGenerator.h"
#include "geometry/service/LoftSpecificationValidator.h"
#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepDiskMeshGenerator.h"
#include "geometry/service/SweepDiskSpecificationValidator.h"

#include "spatial/model/InterfaceCompatibilityProfile.h"
#include "spatial/model/SpatialClearanceRequirement.h"
#include "spatial/model/SpatialInterfaceFrame.h"
#include "spatial/model/SpatialInterfaceRegion.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

struct PoolPartGeometry
{
	std::shared_ptr<const ShapeSpecification> shape;
	GeneratedPrimitiveMesh mesh{std::make_shared<Mesh>(), {}};
};

bool finite_positive(float value)
{
	return std::isfinite(value) && value > 0.0f;
}

PoolPartGeometry build_extrusion(
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

ArchitecturalAssemblyBuildResult PoolAssemblyBuilder::build(
	const PoolAssemblySpecification &specification) const
{
	if (specification.objectIdentifier().empty() ||
	    specification.shellMaterialIdentifier().empty() ||
	    specification.copingMaterialIdentifier().empty() ||
	    specification.waterMaterialIdentifier().empty() ||
	    specification.drainMaterialIdentifier().empty() ||
	    !finite_positive(specification.width()) ||
	    !finite_positive(specification.length()) ||
	    !finite_positive(specification.depth()) ||
	    !finite_positive(specification.shellThickness()) ||
	    !finite_positive(specification.floorThickness()) ||
	    !finite_positive(specification.copingOverhang()) ||
	    !finite_positive(specification.copingThickness()) ||
	    !finite_positive(specification.waterFreeboard()) ||
	    specification.interiorWidth() <= 0.0f ||
	    specification.interiorLength() <= 0.0f ||
	    specification.waterDepth() <= 0.0f ||
	    specification.stepCount() > complexity_limits_.maximumInstanceArrayCount()) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile,
			"PoolAssembly requires finite positive shell, coping, and water dimensions.");
	}

	std::string diagnostic;
	Profile2DFactory profile_factory(complexity_limits_);
	const float outer_radius = std::min(specification.width(), specification.length()) * 0.05f;
	const float inner_radius = std::max(
		0.01f, outer_radius - specification.shellThickness());
	auto outer_profile = profile_factory.createRoundedRectangle(
		specification.width(), specification.length(), outer_radius, 2, &diagnostic);
	auto inner_profile = profile_factory.createRoundedRectangle(
		specification.interiorWidth(), specification.interiorLength(),
		inner_radius, 2, &diagnostic);
	auto drain_opening = profile_factory.createCircle(0.06f, 12, &diagnostic);
	if (!outer_profile || !inner_profile || !drain_opening) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	ShellLoftShapeSpecificationCandidate shell_candidate;
	ShellLoftSectionCandidate shell_bottom;
	shell_bottom.axial_position = 0.0f;
	shell_bottom.outer_loop = outer_profile->outerLoop().points();
	shell_bottom.inner_loop = drain_opening->outerLoop().points();
	ShellLoftSectionCandidate shell_floor;
	shell_floor.axial_position = specification.floorThickness();
	shell_floor.outer_loop = outer_profile->outerLoop().points();
	shell_floor.inner_loop = inner_profile->outerLoop().points();
	ShellLoftSectionCandidate shell_top = shell_floor;
	shell_top.axial_position = specification.depth();
	shell_candidate.sections = {shell_bottom, shell_floor, shell_top};
	auto shell_shape = LoftSpecificationValidator(complexity_limits_).validateShellLoft(
		std::move(shell_candidate), &diagnostic);
	if (!shell_shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	GeometryBuildResult shell_mesh =
		LoftMeshGenerator(complexity_limits_).build(*shell_shape);
	if (!shell_mesh.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			shell_mesh.status(), shell_mesh.firstDiagnostic());
	}

	Profile2DCandidate coping_profile;
	auto coping_outer = profile_factory.createRoundedRectangle(
		specification.width() + specification.copingOverhang() * 2.0f,
		specification.length() + specification.copingOverhang() * 2.0f,
		outer_radius + specification.copingOverhang(), 2, &diagnostic);
	if (!coping_outer) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	coping_profile.outer_loop = coping_outer->outerLoop().points();
	coping_profile.inner_loops.push_back(inner_profile->outerLoop().points());
	PoolPartGeometry coping = build_extrusion(
		std::move(coping_profile), specification.copingThickness(),
		complexity_limits_, &diagnostic);
	if (!coping.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	PoolPartGeometry water = build_extrusion(
		Profile2DCandidate{inner_profile->outerLoop().points(), {}},
		specification.waterDepth(), complexity_limits_, &diagnostic);
	if (!water.shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}

	SweepDiskShapeSpecificationCandidate drain_candidate;
	drain_candidate.path_points = {
		glm::vec3(0.0f, 0.0f, -0.30f),
		glm::vec3(0.0f, 0.0f, specification.floorThickness())};
	drain_candidate.up_hint = glm::vec3(1.0f, 0.0f, 0.0f);
	drain_candidate.radius = 0.06f;
	drain_candidate.circumferential_segments = 16;
	auto drain_shape = SweepDiskSpecificationValidator(complexity_limits_).validate(
		std::move(drain_candidate), &diagnostic);
	if (!drain_shape) {
		return ArchitecturalAssemblyBuildResult::failed(
			GeometryBuildStatus::InvalidProfile, diagnostic);
	}
	GeometryBuildResult drain_mesh =
		SweepDiskMeshGenerator(complexity_limits_).build(*drain_shape);
	if (!drain_mesh.succeeded()) {
		return ArchitecturalAssemblyBuildResult::failed(
			drain_mesh.status(), drain_mesh.firstDiagnostic());
	}

	PoolPartGeometry step;
	if (specification.stepCount() > 0u) {
		step = build_extrusion(
			rectangle_candidate(
				specification.interiorWidth() * 0.55f,
				std::min(0.18f, specification.depth() / 6.0f)),
			std::min(0.35f, specification.interiorLength() /
				static_cast<float>(specification.stepCount() + 1u)),
			complexity_limits_, &diagnostic);
		if (!step.shape) {
			return ArchitecturalAssemblyBuildResult::failed(
				GeometryBuildStatus::InvalidProfile, diagnostic);
		}
	}

	const glm::mat4 upright_transform = glm::rotate(
		glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	const glm::mat4 coping_transform = upright_transform * glm::translate(
		glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, specification.depth()));
	const glm::mat4 water_transform = upright_transform * glm::translate(
		glm::mat4(1.0f),
		glm::vec3(0.0f, 0.0f, specification.floorThickness()));

	std::vector<ArchitecturalAssemblyPart> parts;
	std::vector<GeneratedMeshPlacement> placements;
	parts.emplace_back(
		"pool_shell", "PoolShell", specification.shellMaterialIdentifier(),
		shell_shape, shell_mesh.generatedMesh(), upright_transform);
	placements.emplace_back("pool_shell", shell_mesh.generatedMesh(), upright_transform);
	parts.emplace_back(
		"coping", "PoolCoping", specification.copingMaterialIdentifier(),
		coping.shape, coping.mesh, coping_transform);
	placements.emplace_back("coping", coping.mesh, coping_transform);
	parts.emplace_back(
		"water_volume", "WaterVolume", specification.waterMaterialIdentifier(),
		water.shape, water.mesh, water_transform);
	placements.emplace_back("water_volume", water.mesh, water_transform);
	parts.emplace_back(
		"main_drain", "MainDrain", specification.drainMaterialIdentifier(),
		drain_shape, drain_mesh.generatedMesh(), upright_transform);
	placements.emplace_back("main_drain", drain_mesh.generatedMesh(), upright_transform);

	for (std::size_t step_index = 0; step_index < specification.stepCount(); ++step_index) {
		const float step_height =
			specification.depth() - 0.22f * static_cast<float>(step_index + 1u);
		const float step_z = -specification.interiorLength() * 0.5f +
		                     0.32f * static_cast<float>(step_index);
		const glm::mat4 transform = glm::translate(
			glm::mat4(1.0f), glm::vec3(0.0f, step_height, step_z));
		const std::string identifier = "entry_step_" + std::to_string(step_index);
		parts.emplace_back(
			identifier, "PoolEntryStep", specification.copingMaterialIdentifier(),
			step.shape, step.mesh, transform);
		placements.emplace_back(identifier, step.mesh, transform);
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
		SpatialInterfaceId("ground_support"), owner, SpatialInterfaceType::Support,
		SpatialInterfaceFrame(
			glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.width(), specification.length()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.0f));
	interfaces.emplace_back(
		SpatialInterfaceId("water_surface"), owner, SpatialInterfaceType::InspectionInterface,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, specification.depth() - specification.waterFreeboard(), 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::planeRectangle(
			specification.interiorWidth(), specification.interiorLength()),
		InterfaceCompatibilityProfile(
			InterfaceShape::Rectangular, InterfaceGender::Neutral),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.01f));
	interfaces.emplace_back(
		SpatialInterfaceId("main_drain"), owner, SpatialInterfaceType::DrainPort,
		SpatialInterfaceFrame(
			glm::vec3(0.0f, -0.30f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f),
			glm::vec3(1.0f, 0.0f, 0.0f)),
		SpatialInterfaceRegion::axisSegment(0.30f),
		InterfaceCompatibilityProfile(
			InterfaceShape::Circular, InterfaceGender::Male,
			0.12f, std::nullopt, std::nullopt, "pool-waste"),
		SpatialClearanceRequirement(0.0f, 0.0f, 0.003f));

	return ArchitecturalAssemblyBuildResult::succeeded(
		ArchitecturalAssemblyGeometry(
			std::move(parts), combined.generatedMesh(), std::move(interfaces)));
}
