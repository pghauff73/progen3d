#include "architecture/service/SurfaceTilingGeometryBuilder.h"

#include "geometry/model/InstanceArraySpecification.h"
#include "geometry/service/ExtrudeProfileMeshGenerator.h"
#include "geometry/service/ExtrudeProfileSpecificationValidator.h"
#include "geometry/service/InstanceArrayGeometryBuilder.h"
#include "geometry/service/Profile2DFactory.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

SurfaceTilingBuildResult SurfaceTilingGeometryBuilder::build(
	const SurfaceTilingSpecification &specification) const
{
	SurfaceTilingBuildResult result;
	if (specification.materialIdentifier().empty() ||
	    specification.surfaceWidth() <= 0.0f ||
	    specification.surfaceHeight() <= 0.0f ||
	    specification.tileWidth() <= 0.0f ||
	    specification.tileHeight() <= 0.0f ||
	    specification.tileDepth() <= 0.0f ||
	    specification.jointWidth() < 0.0f) {
		result.status = GeometryBuildStatus::InvalidProfile;
		result.diagnostic = "SurfaceTiling requires positive finite dimensions and nonnegative joints.";
		return result;
	}
	const std::size_t columns = static_cast<std::size_t>(std::floor(
		(specification.surfaceWidth() + specification.jointWidth()) /
		(specification.tileWidth() + specification.jointWidth())));
	const std::size_t rows = static_cast<std::size_t>(std::floor(
		(specification.surfaceHeight() + specification.jointWidth()) /
		(specification.tileHeight() + specification.jointWidth())));
	const std::size_t tile_count = columns * rows;
	if (tile_count == 0u ||
	    tile_count > complexity_limits_.maximumInstanceArrayCount()) {
		result.status = GeometryBuildStatus::TriangleLimitExceeded;
		result.diagnostic = "SurfaceTiling produces zero tiles or exceeds the instance limit.";
		return result;
	}

	std::string diagnostic;
	auto profile = Profile2DFactory(complexity_limits_).createRectangle(
		specification.tileWidth(), specification.tileHeight(), &diagnostic);
	ExtrudeProfileShapeSpecificationCandidate tile_candidate;
	if (profile) tile_candidate.profile.outer_loop = profile->outerLoop().points();
	tile_candidate.depth = specification.tileDepth();
	auto tile_shape = profile
		? ExtrudeProfileSpecificationValidator(complexity_limits_).validate(
			std::move(tile_candidate), &diagnostic)
		: std::shared_ptr<const ExtrudeProfileShapeSpecification>();
	if (!tile_shape) {
		result.status = GeometryBuildStatus::InvalidProfile;
		result.diagnostic = diagnostic;
		return result;
	}
	GeometryBuildResult tile_mesh =
		ExtrudeProfileMeshGenerator(complexity_limits_).build(*tile_shape);
	if (!tile_mesh.succeeded()) {
		result.status = tile_mesh.status();
		result.diagnostic = tile_mesh.firstDiagnostic();
		return result;
	}

	const float used_width =
		static_cast<float>(columns) * specification.tileWidth() +
		static_cast<float>(columns - 1u) * specification.jointWidth();
	const float used_height =
		static_cast<float>(rows) * specification.tileHeight() +
		static_cast<float>(rows - 1u) * specification.jointWidth();
	std::vector<glm::mat4> transforms;
	transforms.reserve(tile_count);
	for (std::size_t row = 0; row < rows; ++row) {
		for (std::size_t column = 0; column < columns; ++column) {
			const float x = -used_width * 0.5f + specification.tileWidth() * 0.5f +
			                static_cast<float>(column) *
				                (specification.tileWidth() + specification.jointWidth());
			const float y = -used_height * 0.5f + specification.tileHeight() * 0.5f +
			                static_cast<float>(row) *
				                (specification.tileHeight() + specification.jointWidth());
			transforms.push_back(glm::translate(
				glm::mat4(1.0f), glm::vec3(x, y, 0.0f)));
		}
	}
	GeometryBuildResult tiled = InstanceArrayGeometryBuilder(complexity_limits_).build(
		tile_mesh.generatedMesh(), InstanceArraySpecification(std::move(transforms)));
	if (!tiled.succeeded()) {
		result.status = tiled.status();
		result.diagnostic = tiled.firstDiagnostic();
		return result;
	}
	result.status = GeometryBuildStatus::Success;
	result.geometry.emplace(
		tile_count, specification.materialIdentifier(), tiled.generatedMesh());
	return result;
}
