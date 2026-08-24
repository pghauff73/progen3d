#include "geometry/service/ConvexHullMeshGenerationService.h"

#include "geometry/service/MeshTopologyAnalyzer.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <stdexcept>
#include <vector>

ConvexHullMeshGenerationResult
ConvexHullMeshGenerationService::generateConservativeEnvelope(
	const std::vector<glm::dvec3> &source_points,
	double quantization_metres) const
{
	if (source_points.empty() || !std::isfinite(quantization_metres) ||
	    quantization_metres <= 0.0) {
		throw std::invalid_argument(
			"Convex hull generation requires finite source points and positive quantization.");
	}
	glm::dvec3 minimum(std::numeric_limits<double>::infinity());
	glm::dvec3 maximum(-std::numeric_limits<double>::infinity());
	for (const glm::dvec3 &point : source_points) {
		for (int axis = 0; axis < 3; ++axis) {
			if (!std::isfinite(point[axis])) {
				throw std::invalid_argument("Convex hull source point is not finite.");
			}
			const double quantized =
				std::round(point[axis] / quantization_metres) * quantization_metres;
			minimum[axis] = std::min(minimum[axis], quantized - quantization_metres);
			maximum[axis] = std::max(maximum[axis], quantized + quantization_metres);
		}
	}
	if (maximum.x <= minimum.x || maximum.y <= minimum.y || maximum.z <= minimum.z) {
		throw std::invalid_argument("Convex hull source points do not span a volume.");
	}
	auto mesh = std::make_shared<Mesh>();
	mesh->vertices = {
		glm::vec3(minimum.x, minimum.y, minimum.z),
		glm::vec3(maximum.x, minimum.y, minimum.z),
		glm::vec3(maximum.x, maximum.y, minimum.z),
		glm::vec3(minimum.x, maximum.y, minimum.z),
		glm::vec3(minimum.x, minimum.y, maximum.z),
		glm::vec3(maximum.x, minimum.y, maximum.z),
		glm::vec3(maximum.x, maximum.y, maximum.z),
		glm::vec3(minimum.x, maximum.y, maximum.z)};
	mesh->faces = {
		{0, 2, 1}, {0, 3, 2}, {4, 5, 6}, {4, 6, 7},
		{0, 1, 5}, {0, 5, 4}, {1, 2, 6}, {1, 6, 5},
		{2, 3, 7}, {2, 7, 6}, {3, 0, 4}, {3, 4, 7}};
	mesh->calc_normals();
	const std::vector<MeshSurfaceTag> tags(
		mesh->faces.size(), MeshSurfaceTag(MeshSurfaceRole::Outer));
	const MeshTopologyReport topology = MeshTopologyAnalyzer().analyze(*mesh, tags);
	bool contains_all_source_points = true;
	for (const glm::dvec3 &point : source_points) {
		for (int axis = 0; axis < 3; ++axis) {
			contains_all_source_points = contains_all_source_points &&
				point[axis] >= minimum[axis] && point[axis] <= maximum[axis];
		}
	}
	const glm::dvec3 extent = maximum - minimum;
	return ConvexHullMeshGenerationResult(
		GeneratedPrimitiveMesh(mesh, tags),
		ConvexHullMeshGenerationReport(
			"axis_aligned_conservative_envelope", source_points.size(),
			mesh->vertices.size(), mesh->faces.size(), extent.x * extent.y * extent.z,
			contains_all_source_points, topology.isWatertight()));
}
