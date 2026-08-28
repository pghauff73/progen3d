#include "geometry/service/MeshRayParityClassificationService.h"

#include <algorithm>
#include <cmath>
#include <vector>

MeshRayParityClassification MeshRayParityClassificationService::classify(
	const Mesh &closed_mesh,
	const glm::dvec3 &point,
	double surface_tolerance) const
{
	if (closed_mesh.faces.empty()) {
		return MeshRayParityClassification(
			MeshPointContainment::Indeterminate, 0u, false);
	}
	const double surface_tolerance_squared = surface_tolerance * surface_tolerance;
	for (const glm::ivec3 &face : closed_mesh.faces) {
		const glm::dvec3 a(closed_mesh.vertices[static_cast<std::size_t>(face.x)]);
		const glm::dvec3 b(closed_mesh.vertices[static_cast<std::size_t>(face.y)]);
		const glm::dvec3 c(closed_mesh.vertices[static_cast<std::size_t>(face.z)]);
		if (triangle_relationship_service_.pointSquaredDistance(point, a, b, c) <=
		    surface_tolerance_squared) {
			return MeshRayParityClassification(
				MeshPointContainment::OnSurface, 0u, true);
		}
	}
	const glm::dvec3 direction = glm::normalize(glm::dvec3(1.0, 0.3713906763541037, 0.2196152422706632));
	std::vector<double> intersections;
	intersections.reserve(closed_mesh.faces.size() / 4u);
	for (const glm::ivec3 &face : closed_mesh.faces) {
		const std::optional<double> distance =
			triangle_relationship_service_.rayIntersectionDistance(
				point, direction,
				glm::dvec3(closed_mesh.vertices[static_cast<std::size_t>(face.x)]),
				glm::dvec3(closed_mesh.vertices[static_cast<std::size_t>(face.y)]),
				glm::dvec3(closed_mesh.vertices[static_cast<std::size_t>(face.z)]));
		if (distance && *distance > surface_tolerance) intersections.push_back(*distance);
	}
	std::sort(intersections.begin(), intersections.end());
	std::vector<double> unique_intersections;
	for (double distance : intersections) {
		if (unique_intersections.empty() ||
		    std::abs(distance - unique_intersections.back()) > surface_tolerance) {
			unique_intersections.push_back(distance);
		}
	}
	return MeshRayParityClassification(
		unique_intersections.size() % 2u == 0u
			? MeshPointContainment::Outside
			: MeshPointContainment::Inside,
		unique_intersections.size(), true);
}
