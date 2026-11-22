#include "geometry/service/FacialSkinAdmissibility.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

glm::dvec3 closestTrianglePoint(const glm::dvec3 &point,
	                          const glm::dvec3 &first, const glm::dvec3 &second,
	                          const glm::dvec3 &third)
{
	const glm::dvec3 edge_one = second - first, edge_two = third - first;
	const glm::dvec3 from_first = point - first;
	const double d1 = glm::dot(edge_one, from_first), d2 = glm::dot(edge_two, from_first);
	if (d1 <= 0.0 && d2 <= 0.0) return first;
	const glm::dvec3 from_second = point - second;
	const double d3 = glm::dot(edge_one, from_second), d4 = glm::dot(edge_two, from_second);
	if (d3 >= 0.0 && d4 <= d3) return second;
	const double third_region = d1 * d4 - d3 * d2;
	if (third_region <= 0.0 && d1 >= 0.0 && d3 <= 0.0)
		return first + (d1 / (d1 - d3)) * edge_one;
	const glm::dvec3 from_third = point - third;
	const double d5 = glm::dot(edge_one, from_third), d6 = glm::dot(edge_two, from_third);
	if (d6 >= 0.0 && d5 <= d6) return third;
	const double second_region = d5 * d2 - d1 * d6;
	if (second_region <= 0.0 && d2 >= 0.0 && d6 <= 0.0)
		return first + (d2 / (d2 - d6)) * edge_two;
	const double first_region = d3 * d6 - d5 * d4;
	if (first_region <= 0.0 && d4 - d3 >= 0.0 && d5 - d6 >= 0.0)
		return second + ((d4 - d3) / ((d4 - d3) + (d5 - d6))) * (third - second);
	const double inverse = 1.0 / (first_region + second_region + third_region);
	return first + second_region * inverse * edge_one + third_region * inverse * edge_two;
}

double squaredDistanceToBox(const glm::dvec3 &point, const glm::dvec3 &first,
	                       const glm::dvec3 &second, const glm::dvec3 &third)
{
	const glm::dvec3 minimum = glm::min(first, glm::min(second, third));
	const glm::dvec3 maximum = glm::max(first, glm::max(second, third));
	const glm::dvec3 closest = glm::clamp(point, minimum, maximum);
	return glm::dot(point - closest, point - closest);
}

} // namespace

FacialSkinAdmissibility::FacialSkinAdmissibility(
	const Mesh &neutral_mesh, const std::vector<MeshSurfaceTag> &surface_tags,
	const FacialSkinLandmarks &landmarks, const std::vector<bool> &fixed_vertices,
	double characteristic_length)
	: faces_(neutral_mesh.faces), vertex_count_(neutral_mesh.vertices.size()),
	  left_globe_center_(landmarks.left_eyeball_center),
	  right_globe_center_(landmarks.right_eyeball_center),
	  globe_radius_(landmarks.eyeball_radius), characteristic_length_(characteristic_length)
{
	if (surface_tags.size() != faces_.size() || fixed_vertices.size() != vertex_count_)
		throw std::invalid_argument("FacialSkin admissibility requires aligned surface tags and attachments.");
	for (std::size_t index = 0; index < faces_.size(); ++index) {
		const auto &face = faces_[index];
		const glm::dvec3 first(neutral_mesh.vertices[face.x]);
		const glm::dvec3 second(neutral_mesh.vertices[face.y]);
		const glm::dvec3 third(neutral_mesh.vertices[face.z]);
		const glm::dvec3 normal = glm::cross(second - first, third - first);
		const double length = glm::length(normal);
		if (!std::isfinite(length) || length <= 0.0)
			throw std::invalid_argument("FacialSkin admissibility requires nondegenerate neutral triangles.");
		reference_normals_.push_back(normal / length);
		moving_faces_.push_back(!fixed_vertices[face.x] || !fixed_vertices[face.y] || !fixed_vertices[face.z]);
	}
}

bool FacialSkinAdmissibility::accepts(const std::vector<glm::dvec3> &positions) const
{
	last_rejection_.clear();
	const auto reject = [this](std::string message) { last_rejection_ = std::move(message); return false; };
	if (positions.size() != vertex_count_) return reject("vertex count changed");
	for (const auto &point : positions) {
		if (!std::isfinite(point.x) || !std::isfinite(point.y) || !std::isfinite(point.z)) return reject("nonfinite position");
	}
	const double minimum_cross = 1.0e-10 * characteristic_length_ * characteristic_length_;
	const double radius_squared = globe_radius_ * globe_radius_;
	for (std::size_t index = 0; index < faces_.size(); ++index) {
		if (!moving_faces_[index]) continue;
		const auto &face = faces_[index];
		const auto &first = positions[face.x], &second = positions[face.y], &third = positions[face.z];
		const glm::dvec3 normal = glm::cross(second - first, third - first);
		if (glm::length(normal) <= minimum_cross) return reject("collapsed triangle " + std::to_string(index));
		if (glm::dot(normal, reference_normals_[index]) <= minimum_cross)
			return reject("neutral-normal hemisphere limit at triangle " + std::to_string(index));
		for (const auto &center : {left_globe_center_, right_globe_center_}) {
			if (squaredDistanceToBox(center, first, second, third) > radius_squared) continue;
			if (glm::length(closestTrianglePoint(center, first, second, third) - center) < globe_radius_)
				return reject("eye globe contact at triangle " + std::to_string(index));
		}
	}
	return true;
}

double FacialSkinAdmissibility::minimumGlobeClearance(const std::vector<glm::dvec3> &positions) const
{
	double minimum = std::numeric_limits<double>::infinity();
	if (positions.size() != vertex_count_) return -minimum;
	for (const auto &face : faces_) {
		for (const auto &center : {left_globe_center_, right_globe_center_}) {
			minimum = std::min(minimum, glm::length(closestTrianglePoint(center,
				positions[face.x], positions[face.y], positions[face.z]) - center) - globe_radius_);
		}
	}
	return minimum;
}
