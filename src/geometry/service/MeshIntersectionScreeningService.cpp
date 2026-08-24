#include "geometry/service/MeshIntersectionScreeningService.h"

#include <utility>
#include <vector>

namespace {

bool boundsOverlap(
	const Mesh::CollisionBounds &first,
	const Mesh::CollisionBounds &second,
	double tolerance)
{
	for (int axis = 0; axis < 3; ++axis) {
		if (static_cast<double>(first.max[axis]) + tolerance < second.min[axis] ||
		    static_cast<double>(second.max[axis]) + tolerance < first.min[axis]) {
			return false;
		}
	}
	return true;
}

glm::dvec3 vertex(const Mesh &mesh, int index)
{
	return glm::dvec3(mesh.vertices[static_cast<std::size_t>(index)]);
}

} // namespace

MeshIntersectionScreeningReport MeshIntersectionScreeningService::screen(
	const Mesh &first_mesh,
	const Mesh &second_mesh,
	double tolerance) const
{
	if (first_mesh.faces.empty() || second_mesh.faces.empty()) {
		return MeshIntersectionScreeningReport(0u, 0u, 0u, false);
	}
	Mesh first_acceleration(first_mesh);
	Mesh second_acceleration(second_mesh);
	first_acceleration.buildCollisionAccel();
	second_acceleration.buildCollisionAccel();
	const auto &first_nodes = first_acceleration.getCollisionBvhNodes();
	const auto &second_nodes = second_acceleration.getCollisionBvhNodes();
	const auto &first_indices = first_acceleration.getCollisionBvhTriangleIndices();
	const auto &second_indices = second_acceleration.getCollisionBvhTriangleIndices();
	if (first_nodes.empty() || second_nodes.empty()) {
		return MeshIntersectionScreeningReport(0u, 0u, 0u, false);
	}
	std::vector<std::pair<int, int>> node_pairs = {{0, 0}};
	std::size_t candidate_count = 0u;
	std::size_t checked_count = 0u;
	std::size_t intersection_count = 0u;
	while (!node_pairs.empty()) {
		const auto node_pair = node_pairs.back();
		node_pairs.pop_back();
		const Mesh::CollisionBvhNode &first_node =
			first_nodes[static_cast<std::size_t>(node_pair.first)];
		const Mesh::CollisionBvhNode &second_node =
			second_nodes[static_cast<std::size_t>(node_pair.second)];
		if (!boundsOverlap(first_node.bounds, second_node.bounds, tolerance)) continue;
		if (first_node.isLeaf() && second_node.isLeaf()) {
			for (int first_offset = 0; first_offset < first_node.count; ++first_offset) {
				const int first_face_index = first_indices[static_cast<std::size_t>(
					first_node.start + first_offset)];
				const glm::ivec3 first_face =
					first_acceleration.faces[static_cast<std::size_t>(first_face_index)];
				for (int second_offset = 0; second_offset < second_node.count; ++second_offset) {
					const int second_face_index = second_indices[static_cast<std::size_t>(
						second_node.start + second_offset)];
					const glm::ivec3 second_face =
						second_acceleration.faces[static_cast<std::size_t>(second_face_index)];
					if (!boundsOverlap(
							first_acceleration.triangle_bounds[static_cast<std::size_t>(first_face_index)],
							second_acceleration.triangle_bounds[static_cast<std::size_t>(second_face_index)],
							tolerance)) continue;
					++candidate_count;
					++checked_count;
					if (triangle_relationship_service_.intersects(
							vertex(first_acceleration, first_face.x),
							vertex(first_acceleration, first_face.y),
							vertex(first_acceleration, first_face.z),
							vertex(second_acceleration, second_face.x),
							vertex(second_acceleration, second_face.y),
							vertex(second_acceleration, second_face.z), tolerance)) {
						++intersection_count;
					}
				}
			}
			continue;
		}
		if (first_node.isLeaf()) {
			node_pairs.emplace_back(node_pair.first, second_node.left);
			node_pairs.emplace_back(node_pair.first, second_node.right);
		} else if (second_node.isLeaf()) {
			node_pairs.emplace_back(first_node.left, node_pair.second);
			node_pairs.emplace_back(first_node.right, node_pair.second);
		} else {
			node_pairs.emplace_back(first_node.left, second_node.left);
			node_pairs.emplace_back(first_node.left, second_node.right);
			node_pairs.emplace_back(first_node.right, second_node.left);
			node_pairs.emplace_back(first_node.right, second_node.right);
		}
	}
	return MeshIntersectionScreeningReport(
		candidate_count, checked_count, intersection_count, true);
}
