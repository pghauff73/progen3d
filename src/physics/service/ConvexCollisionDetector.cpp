#include "physics/service/ConvexCollisionDetector.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace {

constexpr float kDirectionTolerance = 1.0e-6f;
constexpr float kSupportTolerance = 1.0e-5f;
constexpr float kTetrahedronInteriorTolerance = 1.0e-5f;
constexpr int kMaximumGjkIterations = 48;
constexpr int kMaximumEpaIterations = 64;

struct MinkowskiSupportPoint
{
	glm::vec3 difference{0.0f};
	glm::vec3 first_point{0.0f};
	glm::vec3 second_point{0.0f};
};

struct ExpandingPolytopeFace
{
	std::array<int, 3> vertex_indices{};
	glm::vec3 normal{0.0f};
	float distance = 0.0f;
	bool valid = false;
};

struct PolytopeBoundaryEdge
{
	int start = -1;
	int end = -1;
};

class OriginEnclosingTetrahedronBuilder
{
public:
	std::vector<MinkowskiSupportPoint> build(
		const ConvexCollisionShape &first,
		const ConvexCollisionShape &second,
		const std::vector<MinkowskiSupportPoint> &seed_points) const;

private:
	bool containsOrigin(const std::array<MinkowskiSupportPoint, 4> &vertices) const;
	void appendUniquePoint(std::vector<MinkowskiSupportPoint> *points,
	                       const MinkowskiSupportPoint &candidate) const;
};

class AxisProjectionContactBuilder
{
public:
	CollisionContact build(const ConvexCollisionShape &first,
	                       const ConvexCollisionShape &second) const;
};

MinkowskiSupportPoint create_support_point(const ConvexCollisionShape &first,
	                                        const ConvexCollisionShape &second,
	                                        const glm::vec3 &direction)
{
	MinkowskiSupportPoint support;
	support.first_point = first.supportPoint(direction);
	support.second_point = second.supportPoint(-direction);
	support.difference = support.first_point - support.second_point;
	return support;
}

bool OriginEnclosingTetrahedronBuilder::containsOrigin(
	const std::array<MinkowskiSupportPoint, 4> &vertices) const
{
	const glm::vec3 &first = vertices[0].difference;
	const glm::vec3 &second = vertices[1].difference;
	const glm::vec3 &third = vertices[2].difference;
	const glm::vec3 &fourth = vertices[3].difference;
	const glm::mat3 basis(first - fourth, second - fourth, third - fourth);
	const float determinant = glm::determinant(basis);
	if (std::fabs(determinant) <= kDirectionTolerance) {
		return false;
	}

	const glm::vec3 first_three_weights = glm::inverse(basis) * -fourth;
	const float fourth_weight =
		1.0f - first_three_weights.x - first_three_weights.y - first_three_weights.z;
	return first_three_weights.x > kTetrahedronInteriorTolerance &&
	       first_three_weights.y > kTetrahedronInteriorTolerance &&
	       first_three_weights.z > kTetrahedronInteriorTolerance &&
	       fourth_weight > kTetrahedronInteriorTolerance;
}

void OriginEnclosingTetrahedronBuilder::appendUniquePoint(
	std::vector<MinkowskiSupportPoint> *points,
	const MinkowskiSupportPoint &candidate) const
{
	if (points == nullptr) {
		return;
	}
	for (const MinkowskiSupportPoint &existing : *points) {
		if (glm::length(existing.difference - candidate.difference) <= kSupportTolerance) {
			return;
		}
	}
	points->push_back(candidate);
}

std::vector<MinkowskiSupportPoint> OriginEnclosingTetrahedronBuilder::build(
	const ConvexCollisionShape &first,
	const ConvexCollisionShape &second,
	const std::vector<MinkowskiSupportPoint> &seed_points) const
{
	std::vector<MinkowskiSupportPoint> candidates;
	candidates.reserve(seed_points.size() + 14u);
	for (const MinkowskiSupportPoint &seed_point : seed_points) {
		appendUniquePoint(&candidates, seed_point);
	}

	static const std::array<glm::vec3, 14> kSearchDirections = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(-1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, -1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f),
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(1.0f, 1.0f, 1.0f),
		glm::vec3(-1.0f, 1.0f, 1.0f),
		glm::vec3(1.0f, -1.0f, 1.0f),
		glm::vec3(1.0f, 1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f, 1.0f),
		glm::vec3(-1.0f, 1.0f, -1.0f),
		glm::vec3(1.0f, -1.0f, -1.0f),
		glm::vec3(-1.0f, -1.0f, -1.0f)};
	for (const glm::vec3 &direction : kSearchDirections) {
		appendUniquePoint(&candidates, create_support_point(first, second, direction));
	}

	for (std::size_t first_index = 0; first_index + 3u < candidates.size(); ++first_index) {
		for (std::size_t second_index = first_index + 1u;
		     second_index + 2u < candidates.size();
		     ++second_index) {
			for (std::size_t third_index = second_index + 1u;
			     third_index + 1u < candidates.size();
			     ++third_index) {
				for (std::size_t fourth_index = third_index + 1u;
				     fourth_index < candidates.size();
				     ++fourth_index) {
					const std::array<MinkowskiSupportPoint, 4> tetrahedron = {
						candidates[first_index],
						candidates[second_index],
						candidates[third_index],
						candidates[fourth_index]};
					if (containsOrigin(tetrahedron)) {
						return std::vector<MinkowskiSupportPoint>(
							tetrahedron.begin(), tetrahedron.end());
					}
				}
			}
		}
	}
	return {};
}

CollisionContact AxisProjectionContactBuilder::build(
	const ConvexCollisionShape &first,
	const ConvexCollisionShape &second) const
{
	static const std::array<glm::vec3, 3> kProjectionAxes = {
		glm::vec3(1.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)};

	float minimum_penetration = std::numeric_limits<float>::max();
	glm::vec3 minimum_translation_normal(0.0f);
	for (const glm::vec3 &axis : kProjectionAxes) {
		const float first_minimum = glm::dot(first.supportPoint(-axis), axis);
		const float first_maximum = glm::dot(first.supportPoint(axis), axis);
		const float second_minimum = glm::dot(second.supportPoint(-axis), axis);
		const float second_maximum = glm::dot(second.supportPoint(axis), axis);
		if (first_maximum <= second_minimum || second_maximum <= first_minimum) {
			return CollisionContact::createSeparated();
		}

		const float positive_penetration = first_maximum - second_minimum;
		if (positive_penetration < minimum_penetration) {
			minimum_penetration = positive_penetration;
			minimum_translation_normal = axis;
		}
		const float negative_penetration = second_maximum - first_minimum;
		if (negative_penetration < minimum_penetration) {
			minimum_penetration = negative_penetration;
			minimum_translation_normal = -axis;
		}
	}

	if (!std::isfinite(minimum_penetration) ||
	    minimum_penetration <= kSupportTolerance ||
	    glm::length(minimum_translation_normal) <= kDirectionTolerance) {
		return CollisionContact::createSeparated();
	}
	const glm::vec3 first_point = first.supportPoint(minimum_translation_normal);
	const glm::vec3 second_point = second.supportPoint(-minimum_translation_normal);
	return CollisionContact::createIntersecting(
		minimum_translation_normal,
		minimum_penetration,
		(first_point + second_point) * 0.5f);
}

bool points_in_same_direction(const glm::vec3 &first, const glm::vec3 &second)
{
	return glm::dot(first, second) > 0.0f;
}

glm::vec3 perpendicular_direction(const glm::vec3 &axis)
{
	glm::vec3 reference = std::fabs(axis.x) < 0.8f
		? glm::vec3(1.0f, 0.0f, 0.0f)
		: glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 perpendicular = glm::cross(axis, reference);
	if (glm::length(perpendicular) <= kDirectionTolerance) {
		reference = glm::vec3(0.0f, 0.0f, 1.0f);
		perpendicular = glm::cross(axis, reference);
	}
	return glm::length(perpendicular) > kDirectionTolerance
		? glm::normalize(perpendicular)
		: glm::vec3(1.0f, 0.0f, 0.0f);
}

glm::vec3 line_search_direction(const glm::vec3 &line, const glm::vec3 &toward_origin)
{
	glm::vec3 direction = glm::cross(glm::cross(line, toward_origin), line);
	if (glm::length(direction) <= kDirectionTolerance) {
		direction = perpendicular_direction(line);
	}
	return direction;
}

bool update_line_simplex(std::vector<MinkowskiSupportPoint> *simplex,
	                     glm::vec3 *search_direction)
{
	const MinkowskiSupportPoint newest = simplex->back();
	const MinkowskiSupportPoint previous = (*simplex)[simplex->size() - 2u];
	const glm::vec3 toward_origin = -newest.difference;
	const glm::vec3 line = previous.difference - newest.difference;
	if (points_in_same_direction(line, toward_origin)) {
		*simplex = {previous, newest};
		*search_direction = line_search_direction(line, toward_origin);
	} else {
		*simplex = {newest};
		*search_direction = toward_origin;
	}
	return false;
}

bool update_triangle_simplex(std::vector<MinkowskiSupportPoint> *simplex,
	                         glm::vec3 *search_direction)
{
	const MinkowskiSupportPoint newest = simplex->back();
	const MinkowskiSupportPoint middle = (*simplex)[simplex->size() - 2u];
	const MinkowskiSupportPoint oldest = (*simplex)[simplex->size() - 3u];
	const glm::vec3 toward_origin = -newest.difference;
	const glm::vec3 edge_to_middle = middle.difference - newest.difference;
	const glm::vec3 edge_to_oldest = oldest.difference - newest.difference;
	const glm::vec3 triangle_normal = glm::cross(edge_to_middle, edge_to_oldest);

	const glm::vec3 outside_oldest_edge = glm::cross(triangle_normal, edge_to_oldest);
	if (points_in_same_direction(outside_oldest_edge, toward_origin)) {
		if (points_in_same_direction(edge_to_oldest, toward_origin)) {
			*simplex = {oldest, newest};
			*search_direction = line_search_direction(edge_to_oldest, toward_origin);
		} else {
			*simplex = {middle, newest};
			return update_line_simplex(simplex, search_direction);
		}
		return false;
	}

	const glm::vec3 outside_middle_edge = glm::cross(edge_to_middle, triangle_normal);
	if (points_in_same_direction(outside_middle_edge, toward_origin)) {
		*simplex = {middle, newest};
		return update_line_simplex(simplex, search_direction);
	}

	if (points_in_same_direction(triangle_normal, toward_origin)) {
		*simplex = {oldest, middle, newest};
		*search_direction = triangle_normal;
	} else {
		*simplex = {middle, oldest, newest};
		*search_direction = -triangle_normal;
	}
	return false;
}

bool update_tetrahedron_simplex(std::vector<MinkowskiSupportPoint> *simplex,
	                            glm::vec3 *search_direction)
{
	const MinkowskiSupportPoint newest = simplex->back();
	const MinkowskiSupportPoint second = (*simplex)[2];
	const MinkowskiSupportPoint third = (*simplex)[1];
	const MinkowskiSupportPoint fourth = (*simplex)[0];
	const glm::vec3 toward_origin = -newest.difference;

	auto face_points_toward_origin = [&](const MinkowskiSupportPoint &first,
	                                    const MinkowskiSupportPoint &second_point,
	                                    const MinkowskiSupportPoint &opposite) {
		glm::vec3 normal = glm::cross(first.difference - newest.difference,
		                              second_point.difference - newest.difference);
		if (glm::dot(normal, opposite.difference - newest.difference) > 0.0f) {
			normal = -normal;
		}
		return std::pair<bool, glm::vec3>(points_in_same_direction(normal, toward_origin), normal);
	};

	const auto first_face = face_points_toward_origin(second, third, fourth);
	if (first_face.first) {
		*simplex = {third, second, newest};
		*search_direction = first_face.second;
		return update_triangle_simplex(simplex, search_direction);
	}
	const auto second_face = face_points_toward_origin(third, fourth, second);
	if (second_face.first) {
		*simplex = {fourth, third, newest};
		*search_direction = second_face.second;
		return update_triangle_simplex(simplex, search_direction);
	}
	const auto third_face = face_points_toward_origin(fourth, second, third);
	if (third_face.first) {
		*simplex = {second, fourth, newest};
		*search_direction = third_face.second;
		return update_triangle_simplex(simplex, search_direction);
	}
	return true;
}

bool update_simplex(std::vector<MinkowskiSupportPoint> *simplex,
	                glm::vec3 *search_direction)
{
	switch (simplex->size()) {
	case 2u:
		return update_line_simplex(simplex, search_direction);
	case 3u:
		return update_triangle_simplex(simplex, search_direction);
	case 4u:
		return update_tetrahedron_simplex(simplex, search_direction);
	default:
		*search_direction = -simplex->back().difference;
		return false;
	}
}

bool find_intersection_simplex(const ConvexCollisionShape &first,
	                           const ConvexCollisionShape &second,
	                           std::vector<MinkowskiSupportPoint> *simplex)
{
	glm::vec3 search_direction = second.center() - first.center();
	if (glm::length(search_direction) <= kDirectionTolerance) {
		search_direction = glm::vec3(1.0f, 0.0f, 0.0f);
	}
	simplex->clear();
	simplex->push_back(create_support_point(first, second, search_direction));
	search_direction = -simplex->back().difference;

	for (int iteration = 0; iteration < kMaximumGjkIterations; ++iteration) {
		if (glm::length(search_direction) <= kDirectionTolerance) {
			return simplex->size() >= 2u;
		}
		const MinkowskiSupportPoint support =
			create_support_point(first, second, search_direction);
		if (glm::dot(support.difference, search_direction) <= kSupportTolerance) {
			return false;
		}
		simplex->push_back(support);
		if (update_simplex(simplex, &search_direction)) {
			return true;
		}
	}
	return false;
}

ExpandingPolytopeFace create_polytope_face(
	const std::vector<MinkowskiSupportPoint> &vertices,
	int first,
	int second,
	int third)
{
	ExpandingPolytopeFace face;
	face.vertex_indices = {first, second, third};
	const glm::vec3 first_edge = vertices[static_cast<std::size_t>(second)].difference -
	                             vertices[static_cast<std::size_t>(first)].difference;
	const glm::vec3 second_edge = vertices[static_cast<std::size_t>(third)].difference -
	                              vertices[static_cast<std::size_t>(first)].difference;
	glm::vec3 normal = glm::cross(first_edge, second_edge);
	const float normal_length = glm::length(normal);
	if (normal_length <= kDirectionTolerance) {
		return face;
	}
	normal /= normal_length;
	float distance = glm::dot(normal, vertices[static_cast<std::size_t>(first)].difference);
	if (distance < 0.0f) {
		std::swap(face.vertex_indices[1], face.vertex_indices[2]);
		normal = -normal;
		distance = -distance;
	}
	face.normal = normal;
	face.distance = distance;
	face.valid = true;
	return face;
}

void append_boundary_edge(std::vector<PolytopeBoundaryEdge> *edges, int start, int end)
{
	for (auto iterator = edges->begin(); iterator != edges->end(); ++iterator) {
		if (iterator->start == end && iterator->end == start) {
			edges->erase(iterator);
			return;
		}
	}
	edges->push_back({start, end});
}

CollisionContact create_contact_from_face(const ConvexCollisionShape &first,
	                                       const ConvexCollisionShape &second,
	                                       const ExpandingPolytopeFace &face)
{
	glm::vec3 normal = face.normal;
	const glm::vec3 center_direction = second.center() - first.center();
	if (glm::dot(normal, center_direction) < 0.0f) {
		normal = -normal;
	}
	const glm::vec3 first_point = first.supportPoint(normal);
	const glm::vec3 second_point = second.supportPoint(-normal);
	return CollisionContact::createIntersecting(
		normal,
		std::max(face.distance, 0.0f),
		(first_point + second_point) * 0.5f);
}

CollisionContact expand_intersection_polytope(
	const ConvexCollisionShape &first,
	const ConvexCollisionShape &second,
	std::vector<MinkowskiSupportPoint> vertices)
{
	if (vertices.size() != 4u) {
		return CollisionContact::createSeparated();
	}
	std::vector<ExpandingPolytopeFace> faces = {
		create_polytope_face(vertices, 0, 1, 2),
		create_polytope_face(vertices, 0, 3, 1),
		create_polytope_face(vertices, 0, 2, 3),
		create_polytope_face(vertices, 1, 3, 2)};

	for (int iteration = 0; iteration < kMaximumEpaIterations; ++iteration) {
		auto closest = std::min_element(
			faces.begin(), faces.end(),
			[](const ExpandingPolytopeFace &left, const ExpandingPolytopeFace &right) {
				if (!left.valid) return false;
				if (!right.valid) return true;
				return left.distance < right.distance;
			});
		if (closest == faces.end() || !closest->valid) {
			return CollisionContact::createSeparated();
		}

		const MinkowskiSupportPoint support =
			create_support_point(first, second, closest->normal);
		const float support_distance = glm::dot(support.difference, closest->normal);
		if (support_distance - closest->distance <= kSupportTolerance) {
			return create_contact_from_face(first, second, *closest);
		}

		bool duplicate_support = false;
		for (const MinkowskiSupportPoint &existing : vertices) {
			if (glm::length(existing.difference - support.difference) <= kSupportTolerance) {
				duplicate_support = true;
				break;
			}
		}
		if (duplicate_support) {
			return create_contact_from_face(first, second, *closest);
		}

		const int support_index = static_cast<int>(vertices.size());
		vertices.push_back(support);
		std::vector<PolytopeBoundaryEdge> boundary_edges;
		for (ExpandingPolytopeFace &face : faces) {
			if (!face.valid) continue;
			const glm::vec3 face_vertex =
				vertices[static_cast<std::size_t>(face.vertex_indices[0])].difference;
			if (glm::dot(face.normal, support.difference - face_vertex) <= kSupportTolerance) {
				continue;
			}
			append_boundary_edge(&boundary_edges, face.vertex_indices[0], face.vertex_indices[1]);
			append_boundary_edge(&boundary_edges, face.vertex_indices[1], face.vertex_indices[2]);
			append_boundary_edge(&boundary_edges, face.vertex_indices[2], face.vertex_indices[0]);
			face.valid = false;
		}
		faces.erase(
			std::remove_if(faces.begin(), faces.end(),
			               [](const ExpandingPolytopeFace &face) { return !face.valid; }),
			faces.end());
		for (const PolytopeBoundaryEdge &edge : boundary_edges) {
			ExpandingPolytopeFace face =
				create_polytope_face(vertices, edge.start, edge.end, support_index);
			if (face.valid) {
				faces.push_back(face);
			}
		}
	}
	return CollisionContact::createSeparated();
}

}

CollisionContact ConvexCollisionDetector::detect(
	const ConvexCollisionShape &first,
	const ConvexCollisionShape &second) const
{
	if (!first.isValid() || !second.isValid()) {
		return CollisionContact::createSeparated();
	}
	std::vector<MinkowskiSupportPoint> simplex;
	if (!find_intersection_simplex(first, second, &simplex)) {
		return CollisionContact::createSeparated();
	}
	std::vector<MinkowskiSupportPoint> tetrahedron =
		OriginEnclosingTetrahedronBuilder().build(first, second, simplex);
	if (tetrahedron.size() != 4u) {
		return AxisProjectionContactBuilder().build(first, second);
	}
	const CollisionContact polytope_contact =
		expand_intersection_polytope(first, second, std::move(tetrahedron));
	return polytope_contact.intersects()
		? polytope_contact
		: AxisProjectionContactBuilder().build(first, second);
}
