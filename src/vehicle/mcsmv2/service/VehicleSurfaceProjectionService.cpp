#include "vehicle/mcsmv2/service/VehicleSurfaceProjectionService.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <stdexcept>
#include <utility>

namespace {

struct ClosestTrianglePoint
{
	glm::dvec3 point{0.0};
	glm::dvec3 barycentric{1.0, 0.0, 0.0};
};

ClosestTrianglePoint closestPointOnTriangle(
	const glm::dvec3 &point,
	const glm::dvec3 &first,
	const glm::dvec3 &second,
	const glm::dvec3 &third)
{
	const glm::dvec3 first_edge = second - first;
	const glm::dvec3 second_edge = third - first;
	const glm::dvec3 from_first = point - first;
	const double first_projection = glm::dot(first_edge, from_first);
	const double second_projection = glm::dot(second_edge, from_first);
	if (first_projection <= 0.0 && second_projection <= 0.0) {
		return {first, glm::dvec3(1.0, 0.0, 0.0)};
	}

	const glm::dvec3 from_second = point - second;
	const double third_projection = glm::dot(first_edge, from_second);
	const double fourth_projection = glm::dot(second_edge, from_second);
	if (third_projection >= 0.0 && fourth_projection <= third_projection) {
		return {second, glm::dvec3(0.0, 1.0, 0.0)};
	}

	const double first_edge_region =
		first_projection * fourth_projection - third_projection * second_projection;
	if (first_edge_region <= 0.0 && first_projection >= 0.0 && third_projection <= 0.0) {
		const double parameter = first_projection / (first_projection - third_projection);
		return {
			first + parameter * first_edge,
			glm::dvec3(1.0 - parameter, parameter, 0.0)};
	}

	const glm::dvec3 from_third = point - third;
	const double fifth_projection = glm::dot(first_edge, from_third);
	const double sixth_projection = glm::dot(second_edge, from_third);
	if (sixth_projection >= 0.0 && fifth_projection <= sixth_projection) {
		return {third, glm::dvec3(0.0, 0.0, 1.0)};
	}

	const double second_edge_region =
		fifth_projection * second_projection - first_projection * sixth_projection;
	if (second_edge_region <= 0.0 && second_projection >= 0.0 && sixth_projection <= 0.0) {
		const double parameter = second_projection / (second_projection - sixth_projection);
		return {
			first + parameter * second_edge,
			glm::dvec3(1.0 - parameter, 0.0, parameter)};
	}

	const double opposite_edge_region =
		third_projection * sixth_projection - fifth_projection * fourth_projection;
	if (opposite_edge_region <= 0.0 &&
	    (fourth_projection - third_projection) >= 0.0 &&
	    (fifth_projection - sixth_projection) >= 0.0) {
		const double parameter =
			(fourth_projection - third_projection) /
			((fourth_projection - third_projection) +
			 (fifth_projection - sixth_projection));
		return {
			second + parameter * (third - second),
			glm::dvec3(0.0, 1.0 - parameter, parameter)};
	}

	const double denominator =
		1.0 / (first_edge_region + second_edge_region + opposite_edge_region);
	const double second_weight = second_edge_region * denominator;
	const double third_weight = first_edge_region * denominator;
	const double first_weight = 1.0 - second_weight - third_weight;
	return {
		first_weight * first + second_weight * second + third_weight * third,
		glm::dvec3(first_weight, second_weight, third_weight)};
}

double squaredDistanceToBounds(
	const Mesh::CollisionBounds &bounds,
	const glm::dvec3 &point)
{
	if (!bounds.valid) return std::numeric_limits<double>::infinity();
	const glm::dvec3 minimum(bounds.min);
	const glm::dvec3 maximum(bounds.max);
	const glm::dvec3 nearest = glm::clamp(point, minimum, maximum);
	const glm::dvec3 delta = point - nearest;
	return glm::dot(delta, delta);
}

std::optional<VehicleSurfaceProjection> projectByFaceScan(
	const Mesh &mesh,
	const glm::dvec3 &query_point,
	const std::vector<int> *face_indices,
	int first_index,
	int face_count,
	double initial_squared_distance)
{
	double best_squared_distance = initial_squared_distance;
	std::size_t best_face_index = 0u;
	ClosestTrianglePoint best_point;
	bool found = false;
	for (int offset = 0; offset < face_count; ++offset) {
		const int face_index = face_indices == nullptr
			? first_index + offset
			: (*face_indices)[static_cast<std::size_t>(first_index + offset)];
		if (face_index < 0 || static_cast<std::size_t>(face_index) >= mesh.faces.size()) {
			continue;
		}
		const glm::ivec3 &face = mesh.faces[static_cast<std::size_t>(face_index)];
		const ClosestTrianglePoint candidate = closestPointOnTriangle(
			query_point,
			glm::dvec3(mesh.vertices[static_cast<std::size_t>(face.x)]),
			glm::dvec3(mesh.vertices[static_cast<std::size_t>(face.y)]),
			glm::dvec3(mesh.vertices[static_cast<std::size_t>(face.z)]));
		const glm::dvec3 delta = query_point - candidate.point;
		const double squared_distance = glm::dot(delta, delta);
		if (!found || squared_distance < best_squared_distance ||
		    (squared_distance == best_squared_distance &&
		     static_cast<std::size_t>(face_index) < best_face_index)) {
			found = true;
			best_squared_distance = squared_distance;
			best_face_index = static_cast<std::size_t>(face_index);
			best_point = candidate;
		}
	}
	if (!found) return std::nullopt;
	return VehicleSurfaceProjection(
		best_point.point,
		best_face_index,
		best_point.barycentric,
		std::sqrt(std::max(0.0, best_squared_distance)));
}

} // namespace

std::optional<VehicleSurfaceProjection> VehicleSurfaceProjectionService::projectPoint(
	const Mesh &target_surface,
	const glm::dvec3 &query_point) const
{
	if (target_surface.faces.empty() || target_surface.vertices.empty() ||
	    !std::isfinite(query_point.x) || !std::isfinite(query_point.y) ||
	    !std::isfinite(query_point.z)) {
		return std::nullopt;
	}
	const std::vector<Mesh::CollisionBvhNode> &nodes =
		target_surface.getCollisionBvhNodes();
	const std::vector<int> &triangle_indices =
		target_surface.getCollisionBvhTriangleIndices();
	if (nodes.empty() || triangle_indices.empty()) {
		return projectByFaceScan(
			target_surface,
			query_point,
			nullptr,
			0,
			static_cast<int>(target_surface.faces.size()),
			std::numeric_limits<double>::infinity());
	}

	using QueueEntry = std::pair<double, int>;
	std::priority_queue<QueueEntry, std::vector<QueueEntry>, std::greater<QueueEntry>>
		pending_nodes;
	pending_nodes.emplace(squaredDistanceToBounds(nodes.front().bounds, query_point), 0);
	std::optional<VehicleSurfaceProjection> best_projection;
	double best_squared_distance = std::numeric_limits<double>::infinity();
	while (!pending_nodes.empty()) {
		const QueueEntry entry = pending_nodes.top();
		pending_nodes.pop();
		if (entry.first > best_squared_distance) break;
		if (entry.second < 0 || static_cast<std::size_t>(entry.second) >= nodes.size()) {
			continue;
		}
		const Mesh::CollisionBvhNode &node = nodes[static_cast<std::size_t>(entry.second)];
		if (node.isLeaf()) {
			const std::optional<VehicleSurfaceProjection> candidate = projectByFaceScan(
				target_surface,
				query_point,
				&triangle_indices,
				node.start,
				node.count,
				best_squared_distance);
			if (candidate &&
			    (!best_projection || candidate->distance() < best_projection->distance() ||
			     (candidate->distance() == best_projection->distance() &&
			      candidate->faceIndex() < best_projection->faceIndex()))) {
				best_squared_distance = candidate->distance() * candidate->distance();
				best_projection = candidate;
			}
			continue;
		}
		for (const int child_index : {node.left, node.right}) {
			if (child_index < 0 || static_cast<std::size_t>(child_index) >= nodes.size()) {
				continue;
			}
			const double lower_bound = squaredDistanceToBounds(
				nodes[static_cast<std::size_t>(child_index)].bounds, query_point);
			if (lower_bound <= best_squared_distance) {
				pending_nodes.emplace(lower_bound, child_index);
			}
		}
	}
	return best_projection;
}

std::vector<VehicleSurfaceProjection> VehicleSurfaceProjectionService::projectPoints(
	const Mesh &target_surface,
	const std::vector<glm::dvec3> &query_points) const
{
	std::vector<VehicleSurfaceProjection> projections;
	projections.reserve(query_points.size());
	for (const glm::dvec3 &query_point : query_points) {
		const std::optional<VehicleSurfaceProjection> projection =
			projectPoint(target_surface, query_point);
		if (!projection) {
			throw std::runtime_error("Vehicle surface projection failed.");
		}
		projections.push_back(*projection);
	}
	return projections;
}
