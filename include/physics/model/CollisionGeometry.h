#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "physics/model/CollisionGeometryKind.h"
#include "physics/model/CollisionOrientedBox.h"
#include "physics/model/CollisionTriangle.h"

#include <array>
#include <vector>

#include <glm/glm.hpp>

class Mesh;

struct CollisionGeometry {
	std::array<glm::vec3, 8> vertices{};
	std::vector<glm::vec3> convex_vertices;
	std::vector<glm::vec3> face_axes;
	std::vector<glm::vec3> edge_directions;
	std::vector<CollisionTriangle> triangles;
	AxisAlignedBounds bounds;
	glm::vec3 centroid{0.0f};
	glm::vec3 center_of_mass{0.0f};
	float average_radial_distance_to_vertex = 0.0f;
	CollisionGeometryKind kind = CollisionGeometryKind::BoxHull;
	const Mesh *source_mesh = nullptr;
	glm::mat4 source_transform{1.0f};
	CollisionOrientedBox obb;
	bool valid = false;
};
