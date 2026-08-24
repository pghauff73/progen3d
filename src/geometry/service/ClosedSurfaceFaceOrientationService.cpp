#include "geometry/service/ClosedSurfaceFaceOrientationService.h"

#include "Mesh.h"

#include <glm/geometric.hpp>

#include <array>
#include <cmath>
#include <map>
#include <queue>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace {

struct MeshEdgeKey
{
	int first_vertex = 0;
	int second_vertex = 0;

	bool operator<(const MeshEdgeKey &other) const
	{
		return std::tie(first_vertex, second_vertex) <
			std::tie(other.first_vertex, other.second_vertex);
	}
};

struct MeshFaceEdgeIncident
{
	std::size_t face_index = 0u;
	bool traverses_first_to_second = false;
};

MeshEdgeKey createEdgeKey(int start_vertex, int end_vertex)
{
	return start_vertex < end_vertex
		? MeshEdgeKey{start_vertex, end_vertex}
		: MeshEdgeKey{end_vertex, start_vertex};
}

bool traversesFirstToSecond(int start_vertex, int end_vertex)
{
	return start_vertex < end_vertex;
}

std::array<std::pair<int, int>, 3> faceEdges(const glm::ivec3 &face)
{
	return {{{face.x, face.y}, {face.y, face.z}, {face.z, face.x}}};
}

double signedFaceVolume(
	const Mesh &mesh,
	const glm::ivec3 &face,
	bool flipped)
{
	const int second_index = flipped ? face.z : face.y;
	const int third_index = flipped ? face.y : face.z;
	const glm::dvec3 first(mesh.vertices[static_cast<std::size_t>(face.x)]);
	const glm::dvec3 second(
		mesh.vertices[static_cast<std::size_t>(second_index)]);
	const glm::dvec3 third(
		mesh.vertices[static_cast<std::size_t>(third_index)]);
	return glm::dot(first, glm::cross(second, third)) / 6.0;
}

} // namespace

ClosedSurfaceFaceOrientationReport
ClosedSurfaceFaceOrientationService::orientOutward(Mesh &mesh) const
{
	if (mesh.vertices.empty() || mesh.faces.empty()) {
		return ClosedSurfaceFaceOrientationReport::createFailure(
			"Closed-surface orientation requires vertices and faces.");
	}

	std::map<MeshEdgeKey, std::vector<MeshFaceEdgeIncident>> edge_incidents;
	for (std::size_t face_index = 0u; face_index < mesh.faces.size(); ++face_index) {
		const glm::ivec3 &face = mesh.faces[face_index];
		if (face.x < 0 || face.y < 0 || face.z < 0 ||
		    static_cast<std::size_t>(face.x) >= mesh.vertices.size() ||
		    static_cast<std::size_t>(face.y) >= mesh.vertices.size() ||
		    static_cast<std::size_t>(face.z) >= mesh.vertices.size() ||
		    face.x == face.y || face.y == face.z || face.z == face.x) {
			return ClosedSurfaceFaceOrientationReport::createFailure(
				"Closed-surface orientation found an invalid face index.");
		}
		for (const std::pair<int, int> &edge : faceEdges(face)) {
			edge_incidents[createEdgeKey(edge.first, edge.second)].push_back({
				face_index,
				traversesFirstToSecond(edge.first, edge.second)});
		}
	}
	for (const auto &edge_record : edge_incidents) {
		if (edge_record.second.size() != 2u) {
			return ClosedSurfaceFaceOrientationReport::createFailure(
				"Closed-surface orientation requires exactly two faces per edge.");
		}
	}

	std::vector<int> face_flip_state(mesh.faces.size(), -1);
	std::vector<std::vector<std::size_t>> connected_components;
	for (std::size_t seed_face = 0u; seed_face < mesh.faces.size(); ++seed_face) {
		if (face_flip_state[seed_face] >= 0) continue;
		connected_components.emplace_back();
		std::queue<std::size_t> pending_faces;
		face_flip_state[seed_face] = 0;
		pending_faces.push(seed_face);
		while (!pending_faces.empty()) {
			const std::size_t face_index = pending_faces.front();
			pending_faces.pop();
			connected_components.back().push_back(face_index);
			const glm::ivec3 &face = mesh.faces[face_index];
			for (const std::pair<int, int> &edge : faceEdges(face)) {
				const MeshEdgeKey edge_key = createEdgeKey(edge.first, edge.second);
				const std::vector<MeshFaceEdgeIncident> &incidents =
					edge_incidents.at(edge_key);
				const MeshFaceEdgeIncident &current_incident =
					incidents[0].face_index == face_index
						? incidents[0]
						: incidents[1];
				const MeshFaceEdgeIncident &neighbour_incident =
					incidents[0].face_index == face_index
						? incidents[1]
						: incidents[0];
				const int required_neighbour_flip =
					face_flip_state[face_index] ^
					(current_incident.traverses_first_to_second ==
					 neighbour_incident.traverses_first_to_second);
				int &neighbour_flip_state =
					face_flip_state[neighbour_incident.face_index];
				if (neighbour_flip_state < 0) {
					neighbour_flip_state = required_neighbour_flip;
					pending_faces.push(neighbour_incident.face_index);
				}
				else if (neighbour_flip_state != required_neighbour_flip) {
					return ClosedSurfaceFaceOrientationReport::createFailure(
						"Closed-surface face adjacency contains an orientation conflict.");
				}
			}
		}
	}

	for (const std::vector<std::size_t> &component : connected_components) {
		double signed_volume = 0.0;
		for (std::size_t face_index : component) {
			signed_volume += signedFaceVolume(
				mesh,
				mesh.faces[face_index],
				face_flip_state[face_index] != 0);
		}
		if (!std::isfinite(signed_volume) || std::fabs(signed_volume) <= 1.0e-12) {
			return ClosedSurfaceFaceOrientationReport::createFailure(
				"Closed-surface orientation found a component with zero signed volume.");
		}
		if (signed_volume < 0.0) {
			for (std::size_t face_index : component) {
				face_flip_state[face_index] ^= 1;
			}
		}
	}

	std::size_t flipped_face_count = 0u;
	for (std::size_t face_index = 0u; face_index < mesh.faces.size(); ++face_index) {
		if (face_flip_state[face_index] == 0) continue;
		std::swap(mesh.faces[face_index].y, mesh.faces[face_index].z);
		++flipped_face_count;
	}
	mesh.clearCollisionAcceleration();
	mesh.calc_normals();
	return ClosedSurfaceFaceOrientationReport::createSuccess(
		connected_components.size(), flipped_face_count);
}
