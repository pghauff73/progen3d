#include "vehicle/parametric/service/ModernCarBodySectionExtractionService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace {

struct QuantizedPoint
{
	std::int64_t x = 0;
	std::int64_t y = 0;
	std::int64_t z = 0;

	bool operator<(const QuantizedPoint &other) const
	{
		return std::tie(x, y, z) < std::tie(other.x, other.y, other.z);
	}
};

QuantizedPoint quantize(const glm::dvec3 &point)
{
	constexpr double scale = 1000000.0;
	return {
		static_cast<std::int64_t>(std::llround(point.x * scale)),
		static_cast<std::int64_t>(std::llround(point.y * scale)),
		static_cast<std::int64_t>(std::llround(point.z * scale))};
}

void add_edge_intersection(
	const glm::dvec3 &first,
	const glm::dvec3 &second,
	double station,
	std::map<QuantizedPoint, glm::dvec3> *points)
{
	const double first_distance = first.z - station;
	const double second_distance = second.z - station;
	if ((first_distance < 0.0 && second_distance < 0.0) ||
	    (first_distance > 0.0 && second_distance > 0.0)) {
		return;
	}
	const double denominator = second.z - first.z;
	if (std::fabs(denominator) <= 1.0e-12) return;
	const double parameter = std::clamp(
		(station - first.z) / denominator, 0.0, 1.0);
	glm::dvec3 point = first + (second - first) * parameter;
	point.z = station;
	points->emplace(quantize(point), point);
}

} // namespace

GeneratedBodySectionSet ModernCarBodySectionExtractionService::extract(
	const ModernCarVariantDefinition &variant,
	const GeneratedBodyMesh &body_mesh) const
{
	std::vector<GeneratedBodySection> sections;
	if (!body_mesh.mesh()) return GeneratedBodySectionSet(std::move(sections));
	const std::size_t section_count = variant.body().bodySectionNetwork().sectionCount();
	if (section_count == 0u) return GeneratedBodySectionSet(std::move(sections));
	const double rear = -variant.package().length() * 0.5 + 0.05;
	const double front = variant.package().length() * 0.5 - 0.05;
	sections.reserve(section_count);

	for (std::size_t section_index = 0u; section_index < section_count;
	     ++section_index) {
		const double interpolation = section_count == 1u
			? 0.5
			: static_cast<double>(section_index) /
				static_cast<double>(section_count - 1u);
		const double station = rear + (front - rear) * interpolation;
		std::map<QuantizedPoint, glm::dvec3> unique_points;
		for (const glm::ivec3 &face : body_mesh.mesh()->faces) {
			const std::array<glm::dvec3, 3> vertices{{
				glm::dvec3(body_mesh.mesh()->vertices[static_cast<std::size_t>(face.x)]),
				glm::dvec3(body_mesh.mesh()->vertices[static_cast<std::size_t>(face.y)]),
				glm::dvec3(body_mesh.mesh()->vertices[static_cast<std::size_t>(face.z)])}};
			add_edge_intersection(vertices[0], vertices[1], station, &unique_points);
			add_edge_intersection(vertices[1], vertices[2], station, &unique_points);
			add_edge_intersection(vertices[2], vertices[0], station, &unique_points);
		}
		std::vector<glm::dvec3> points;
		points.reserve(unique_points.size());
		for (const auto &entry : unique_points) points.push_back(entry.second);
		if (!points.empty()) {
			glm::dvec2 centre(0.0);
			for (const glm::dvec3 &point : points) centre += glm::dvec2(point.x, point.y);
			centre /= static_cast<double>(points.size());
			std::sort(points.begin(), points.end(), [&](const glm::dvec3 &first,
			                                               const glm::dvec3 &second) {
				const double first_angle =
					std::atan2(first.y - centre.y, first.x - centre.x);
				const double second_angle =
					std::atan2(second.y - centre.y, second.x - centre.x);
				if (first_angle != second_angle) return first_angle < second_angle;
				return glm::length(glm::dvec2(first.x, first.y) - centre) <
				       glm::length(glm::dvec2(second.x, second.y) - centre);
			});
		}
		sections.emplace_back(
			"section_" + std::to_string(section_index), station, std::move(points));
	}
	return GeneratedBodySectionSet(std::move(sections));
}
