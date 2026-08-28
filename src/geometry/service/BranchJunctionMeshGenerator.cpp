#include "geometry/service/BranchJunctionMeshGenerator.h"

#include "geometry/model/BranchJunctionShapeSpecification.h"
#include "geometry/model/GeneratedMeshPlacement.h"
#include "geometry/service/GeneratedMeshComposer.h"
#include "geometry/service/ShapeSpecificationValidator.h"
#include "geometry/service/SphereMeshGenerator.h"
#include "geometry/service/TaperedSweepMeshGenerator.h"
#include "geometry/service/TaperedSweepSpecificationValidator.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <memory>
#include <string>
#include <vector>

namespace {

glm::vec3 perpendicular_hint(const glm::vec3 &direction)
{
	const glm::vec3 absolute = glm::abs(direction);
	const glm::vec3 reference = absolute.x <= absolute.y && absolute.x <= absolute.z
		? glm::vec3(1.0f, 0.0f, 0.0f)
		: absolute.y <= absolute.z
			? glm::vec3(0.0f, 1.0f, 0.0f)
			: glm::vec3(0.0f, 0.0f, 1.0f);
	return glm::normalize(glm::cross(direction, reference));
}

GeneratedPrimitiveMesh retag_as_junction(const GeneratedPrimitiveMesh &source)
{
	return GeneratedPrimitiveMesh(
		source.mesh(),
		std::vector<MeshSurfaceTag>(
			source.mesh() ? source.mesh()->faces.size() : 0u,
			MeshSurfaceTag(MeshSurfaceRole::BotanicalJunction)));
}

} // namespace

GeometryBuildResult BranchJunctionMeshGenerator::build(
	const ShapeSpecification &specification) const
{
	const auto *junction =
		dynamic_cast<const BranchJunctionShapeSpecification *>(&specification);
	if (junction == nullptr) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			"BranchJunctionMeshGenerator requires a BranchJunctionShapeSpecification.");
	}

	std::string diagnostic;
	SphereShapeSpecificationCandidate sphere_candidate;
	sphere_candidate.azimuth_segments = junction->circumferentialSegments() * 2;
	sphere_candidate.polar_segments = junction->circumferentialSegments();
	const auto sphere_shape = ShapeSpecificationValidator().validateSphere(
		std::move(sphere_candidate), &diagnostic);
	if (!sphere_shape) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology, diagnostic);
	}
	const GeneratedPrimitiveMesh sphere =
		SphereMeshGenerator().generate(*sphere_shape, &diagnostic);
	if (!sphere.mesh() || sphere.mesh()->faces.empty()) {
		return GeometryBuildResult::createFailure(
			GeometryBuildStatus::UnsupportedTopology,
			diagnostic.empty() ? "BranchJunction core generation failed." : diagnostic);
	}

	std::vector<GeneratedMeshPlacement> placements;
	const float core_diameter =
		junction->coreRadius() * junction->bulgeScale() * 2.0f;
	placements.emplace_back(
		"junction_core", retag_as_junction(sphere),
		glm::scale(glm::mat4(1.0f), glm::vec3(core_diameter)));

	for (std::size_t arm_index = 0u; arm_index < junction->arms().size();
	     ++arm_index) {
		const BranchJunctionArmSpecification &arm = junction->arms()[arm_index];
		TaperedSweepShapeSpecificationCandidate arm_candidate;
		arm_candidate.path_points = {
			glm::vec3(0.0f), arm.direction() * arm.transitionLength()};
		arm_candidate.radii = {
			junction->coreRadius() * junction->bulgeScale() * 0.92f,
			arm.radius()};
		arm_candidate.up_hint = perpendicular_hint(arm.direction());
		arm_candidate.circumferential_segments =
			junction->circumferentialSegments();
		arm_candidate.detail_level = junction->detailLevel();
		const auto arm_shape =
			TaperedSweepSpecificationValidator(complexity_limits_).validate(
				std::move(arm_candidate), &diagnostic);
		if (!arm_shape) {
			return GeometryBuildResult::createFailure(
				GeometryBuildStatus::UnsupportedTopology, diagnostic);
		}
		const GeometryBuildResult arm_mesh =
			TaperedSweepMeshGenerator(complexity_limits_).build(*arm_shape);
		if (!arm_mesh.succeeded()) return arm_mesh;
		placements.emplace_back(
			"junction_arm_" + std::to_string(arm_index),
			retag_as_junction(arm_mesh.generatedMesh()), glm::mat4(1.0f));
	}

	return GeneratedMeshComposer(complexity_limits_).compose(placements);
}

GeneratedPrimitiveMesh BranchJunctionMeshGenerator::generate(
	const ShapeSpecification &specification,
	std::string *diagnostic) const
{
	const GeometryBuildResult result = build(specification);
	if (!result.succeeded() && diagnostic != nullptr) {
		*diagnostic = result.firstDiagnostic();
	}
	return result.generatedMesh();
}
