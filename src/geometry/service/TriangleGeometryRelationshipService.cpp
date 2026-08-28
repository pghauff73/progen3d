#include "geometry/service/TriangleGeometryRelationshipService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>

namespace {

double segmentSquaredDistance(
	const glm::dvec3 &first_start,
	const glm::dvec3 &first_end,
	const glm::dvec3 &second_start,
	const glm::dvec3 &second_end)
{
	const glm::dvec3 first_direction = first_end - first_start;
	const glm::dvec3 second_direction = second_end - second_start;
	const glm::dvec3 separation = first_start - second_start;
	const double first_length_squared = glm::dot(first_direction, first_direction);
	const double second_length_squared = glm::dot(second_direction, second_direction);
	const double cross_direction = glm::dot(first_direction, second_direction);
	const double first_projection = glm::dot(first_direction, separation);
	const double second_projection = glm::dot(second_direction, separation);
	double first_parameter = 0.0;
	double second_parameter = 0.0;
	const double denominator =
		first_length_squared * second_length_squared - cross_direction * cross_direction;
	if (denominator > 1.0e-24) {
		first_parameter = std::clamp(
			(cross_direction * second_projection - first_projection * second_length_squared) /
				denominator,
			0.0, 1.0);
	}
	if (second_length_squared > 1.0e-24) {
		second_parameter =
			std::clamp((cross_direction * first_parameter + second_projection) /
				second_length_squared, 0.0, 1.0);
	}
	if (first_length_squared > 1.0e-24) {
		first_parameter =
			std::clamp((cross_direction * second_parameter - first_projection) /
				first_length_squared, 0.0, 1.0);
	}
	const glm::dvec3 closest_separation =
		separation + first_parameter * first_direction -
		second_parameter * second_direction;
	return glm::dot(closest_separation, closest_separation);
}

glm::dvec2 projectToPlane(const glm::dvec3 &point, int omitted_axis)
{
	if (omitted_axis == 0) return {point.y, point.z};
	if (omitted_axis == 1) return {point.x, point.z};
	return {point.x, point.y};
}

double cross2d(const glm::dvec2 &first, const glm::dvec2 &second)
{
	return first.x * second.y - first.y * second.x;
}

bool pointInsideTriangle2d(
	const glm::dvec2 &point,
	const glm::dvec2 &a,
	const glm::dvec2 &b,
	const glm::dvec2 &c,
	double tolerance)
{
	const double first = cross2d(b - a, point - a);
	const double second = cross2d(c - b, point - b);
	const double third = cross2d(a - c, point - c);
	const bool has_negative = first < -tolerance || second < -tolerance || third < -tolerance;
	const bool has_positive = first > tolerance || second > tolerance || third > tolerance;
	return !(has_negative && has_positive);
}

bool segmentsIntersect2d(
	const glm::dvec2 &first_start,
	const glm::dvec2 &first_end,
	const glm::dvec2 &second_start,
	const glm::dvec2 &second_end,
	double tolerance)
{
	const glm::dvec2 first_direction = first_end - first_start;
	const glm::dvec2 second_direction = second_end - second_start;
	const double denominator = cross2d(first_direction, second_direction);
	if (std::abs(denominator) <= tolerance) {
		return pointInsideTriangle2d(first_start, second_start, second_end, second_end, tolerance) ||
		       pointInsideTriangle2d(second_start, first_start, first_end, first_end, tolerance) ||
		       segmentSquaredDistance(
			       glm::dvec3(first_start, 0.0), glm::dvec3(first_end, 0.0),
			       glm::dvec3(second_start, 0.0), glm::dvec3(second_end, 0.0)) <=
			       tolerance * tolerance;
	}
	const glm::dvec2 separation = second_start - first_start;
	const double first_parameter = cross2d(separation, second_direction) / denominator;
	const double second_parameter = cross2d(separation, first_direction) / denominator;
	return first_parameter >= -tolerance && first_parameter <= 1.0 + tolerance &&
	       second_parameter >= -tolerance && second_parameter <= 1.0 + tolerance;
}

bool coplanarTrianglesIntersect(
	const std::array<glm::dvec3, 3> &first,
	const std::array<glm::dvec3, 3> &second,
	const glm::dvec3 &normal,
	double tolerance)
{
	int omitted_axis = 0;
	if (std::abs(normal.y) > std::abs(normal.x)) omitted_axis = 1;
	if (std::abs(normal.z) > std::abs(normal[omitted_axis])) omitted_axis = 2;
	std::array<glm::dvec2, 3> first_2d;
	std::array<glm::dvec2, 3> second_2d;
	for (int index = 0; index < 3; ++index) {
		first_2d[index] = projectToPlane(first[index], omitted_axis);
		second_2d[index] = projectToPlane(second[index], omitted_axis);
	}
	for (int first_edge = 0; first_edge < 3; ++first_edge) {
		for (int second_edge = 0; second_edge < 3; ++second_edge) {
			if (segmentsIntersect2d(
					first_2d[first_edge], first_2d[(first_edge + 1) % 3],
					second_2d[second_edge], second_2d[(second_edge + 1) % 3],
					tolerance)) return true;
		}
	}
	return pointInsideTriangle2d(
		first_2d[0], second_2d[0], second_2d[1], second_2d[2], tolerance) ||
	       pointInsideTriangle2d(
		second_2d[0], first_2d[0], first_2d[1], first_2d[2], tolerance);
}

} // namespace

double TriangleGeometryRelationshipService::pointSquaredDistance(
	const glm::dvec3 &point,
	const glm::dvec3 &triangle_a,
	const glm::dvec3 &triangle_b,
	const glm::dvec3 &triangle_c) const
{
	const glm::dvec3 edge_ab = triangle_b - triangle_a;
	const glm::dvec3 edge_ac = triangle_c - triangle_a;
	const glm::dvec3 offset_a = point - triangle_a;
	const double ab_projection = glm::dot(edge_ab, offset_a);
	const double ac_projection = glm::dot(edge_ac, offset_a);
	if (ab_projection <= 0.0 && ac_projection <= 0.0) return glm::dot(offset_a, offset_a);

	const glm::dvec3 offset_b = point - triangle_b;
	const double bb_projection = glm::dot(edge_ab, offset_b);
	const double bc_projection = glm::dot(edge_ac, offset_b);
	if (bb_projection >= 0.0 && bc_projection <= bb_projection) return glm::dot(offset_b, offset_b);

	const double vertex_c_region = ab_projection * bc_projection - bb_projection * ac_projection;
	if (vertex_c_region <= 0.0 && ab_projection >= 0.0 && bb_projection <= 0.0) {
		const double edge_parameter = ab_projection / (ab_projection - bb_projection);
		const glm::dvec3 separation = point - (triangle_a + edge_parameter * edge_ab);
		return glm::dot(separation, separation);
	}

	const glm::dvec3 offset_c = point - triangle_c;
	const double cb_projection = glm::dot(edge_ab, offset_c);
	const double cc_projection = glm::dot(edge_ac, offset_c);
	if (cc_projection >= 0.0 && cb_projection <= cc_projection) return glm::dot(offset_c, offset_c);

	const double vertex_b_region = cb_projection * ac_projection - ab_projection * cc_projection;
	if (vertex_b_region <= 0.0 && ac_projection >= 0.0 && cc_projection <= 0.0) {
		const double edge_parameter = ac_projection / (ac_projection - cc_projection);
		const glm::dvec3 separation = point - (triangle_a + edge_parameter * edge_ac);
		return glm::dot(separation, separation);
	}

	const double edge_bc_region = bb_projection * cc_projection - cb_projection * bc_projection;
	if (edge_bc_region <= 0.0 && (bc_projection - bb_projection) >= 0.0 &&
	    (cb_projection - cc_projection) >= 0.0) {
		const double edge_parameter =
			(bc_projection - bb_projection) /
			((bc_projection - bb_projection) + (cb_projection - cc_projection));
		const glm::dvec3 separation =
			point - (triangle_b + edge_parameter * (triangle_c - triangle_b));
		return glm::dot(separation, separation);
	}

	const double denominator = 1.0 /
		(vertex_c_region + vertex_b_region + edge_bc_region);
	const double barycentric_b = vertex_b_region * denominator;
	const double barycentric_c = vertex_c_region * denominator;
	const glm::dvec3 separation = point -
		(triangle_a + barycentric_b * edge_ab + barycentric_c * edge_ac);
	return glm::dot(separation, separation);
}

std::optional<double> TriangleGeometryRelationshipService::rayIntersectionDistance(
	const glm::dvec3 &origin,
	const glm::dvec3 &direction,
	const glm::dvec3 &triangle_a,
	const glm::dvec3 &triangle_b,
	const glm::dvec3 &triangle_c,
	double tolerance) const
{
	const glm::dvec3 edge_ab = triangle_b - triangle_a;
	const glm::dvec3 edge_ac = triangle_c - triangle_a;
	const glm::dvec3 perpendicular = glm::cross(direction, edge_ac);
	const double determinant = glm::dot(edge_ab, perpendicular);
	if (std::abs(determinant) <= tolerance) return std::nullopt;
	const double inverse_determinant = 1.0 / determinant;
	const glm::dvec3 offset = origin - triangle_a;
	const double barycentric_b = glm::dot(offset, perpendicular) * inverse_determinant;
	if (barycentric_b < -tolerance || barycentric_b > 1.0 + tolerance) return std::nullopt;
	const glm::dvec3 cross_offset = glm::cross(offset, edge_ab);
	const double barycentric_c = glm::dot(direction, cross_offset) * inverse_determinant;
	if (barycentric_c < -tolerance || barycentric_b + barycentric_c > 1.0 + tolerance) {
		return std::nullopt;
	}
	const double distance = glm::dot(edge_ac, cross_offset) * inverse_determinant;
	if (distance < -tolerance) return std::nullopt;
	return distance;
}

bool TriangleGeometryRelationshipService::intersects(
	const glm::dvec3 &first_a,
	const glm::dvec3 &first_b,
	const glm::dvec3 &first_c,
	const glm::dvec3 &second_a,
	const glm::dvec3 &second_b,
	const glm::dvec3 &second_c,
	double tolerance) const
{
	const std::array<glm::dvec3, 3> first = {first_a, first_b, first_c};
	const std::array<glm::dvec3, 3> second = {second_a, second_b, second_c};
	for (int edge = 0; edge < 3; ++edge) {
		const glm::dvec3 start = first[edge];
		const glm::dvec3 end = first[(edge + 1) % 3];
		const glm::dvec3 direction = end - start;
		const std::optional<double> distance = rayIntersectionDistance(
			start, direction, second_a, second_b, second_c, tolerance);
		if (distance && *distance >= -tolerance && *distance <= 1.0 + tolerance) return true;
	}
	for (int edge = 0; edge < 3; ++edge) {
		const glm::dvec3 start = second[edge];
		const glm::dvec3 end = second[(edge + 1) % 3];
		const glm::dvec3 direction = end - start;
		const std::optional<double> distance = rayIntersectionDistance(
			start, direction, first_a, first_b, first_c, tolerance);
		if (distance && *distance >= -tolerance && *distance <= 1.0 + tolerance) return true;
	}
	const glm::dvec3 first_normal = glm::cross(first_b - first_a, first_c - first_a);
	const glm::dvec3 second_normal = glm::cross(second_b - second_a, second_c - second_a);
	const double first_normal_length = glm::length(first_normal);
	const double second_normal_length = glm::length(second_normal);
	if (first_normal_length <= tolerance || second_normal_length <= tolerance) return false;
	const bool normals_parallel =
		glm::length(glm::cross(first_normal, second_normal)) <=
		tolerance * first_normal_length * second_normal_length;
	const double plane_separation =
		std::abs(glm::dot(first_normal / first_normal_length, second_a - first_a));
	if (normals_parallel && plane_separation <= tolerance) {
		return coplanarTrianglesIntersect(first, second, first_normal, tolerance);
	}
	return false;
}

double TriangleGeometryRelationshipService::squaredDistance(
	const glm::dvec3 &first_a,
	const glm::dvec3 &first_b,
	const glm::dvec3 &first_c,
	const glm::dvec3 &second_a,
	const glm::dvec3 &second_b,
	const glm::dvec3 &second_c) const
{
	if (intersects(
			first_a, first_b, first_c, second_a, second_b, second_c)) return 0.0;
	double minimum = pointSquaredDistance(first_a, second_a, second_b, second_c);
	minimum = std::min(minimum, pointSquaredDistance(first_b, second_a, second_b, second_c));
	minimum = std::min(minimum, pointSquaredDistance(first_c, second_a, second_b, second_c));
	minimum = std::min(minimum, pointSquaredDistance(second_a, first_a, first_b, first_c));
	minimum = std::min(minimum, pointSquaredDistance(second_b, first_a, first_b, first_c));
	minimum = std::min(minimum, pointSquaredDistance(second_c, first_a, first_b, first_c));
	const std::array<glm::dvec3, 3> first = {first_a, first_b, first_c};
	const std::array<glm::dvec3, 3> second = {second_a, second_b, second_c};
	for (int first_edge = 0; first_edge < 3; ++first_edge) {
		for (int second_edge = 0; second_edge < 3; ++second_edge) {
			minimum = std::min(
				minimum,
				segmentSquaredDistance(
					first[first_edge], first[(first_edge + 1) % 3],
					second[second_edge], second[(second_edge + 1) % 3]));
		}
	}
	return minimum;
}
