#include "geometry/service/SimplePolygonTriangulator.h"

#include "geometry/service/SimplePolygonValidator.h"

#include <cstddef>

namespace {

constexpr float kTriangulationTolerance = 1.0e-6f;

float cross_product(const glm::vec2 &first, const glm::vec2 &second)
{
	return first.x * second.y - first.y * second.x;
}

bool point_in_triangle(const glm::vec2 &point,
	                   const glm::vec2 &first,
	                   const glm::vec2 &second,
	                   const glm::vec2 &third)
{
	const float first_cross = cross_product(second - first, point - first);
	const float second_cross = cross_product(third - second, point - second);
	const float third_cross = cross_product(first - third, point - third);
	return first_cross >= -kTriangulationTolerance &&
	       second_cross >= -kTriangulationTolerance &&
	       third_cross >= -kTriangulationTolerance;
}

}

bool SimplePolygonTriangulator::triangulate(
	const std::vector<glm::vec2> &counter_clockwise_vertices,
	std::vector<glm::ivec3> *triangles,
	std::string *diagnostic) const
{
	if (triangles == nullptr) return false;
	triangles->clear();
	if (counter_clockwise_vertices.size() < 3 ||
	    SimplePolygonValidator::signedArea(counter_clockwise_vertices) <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile triangulation requires a canonical polygon.";
		}
		return false;
	}

	std::vector<std::size_t> polygon(counter_clockwise_vertices.size());
	for (std::size_t index = 0; index < polygon.size(); ++index) {
		polygon[index] = index;
	}

	std::size_t guard = 0;
	while (polygon.size() > 3 &&
	       guard++ < counter_clockwise_vertices.size() *
	                 counter_clockwise_vertices.size()) {
		bool removed_ear = false;
		for (std::size_t polygon_index = 0;
		     polygon_index < polygon.size();
		     ++polygon_index) {
			const std::size_t previous = polygon[
				(polygon_index + polygon.size() - 1) % polygon.size()];
			const std::size_t current = polygon[polygon_index];
			const std::size_t next = polygon[(polygon_index + 1) % polygon.size()];
			const glm::vec2 &first = counter_clockwise_vertices[previous];
			const glm::vec2 &second = counter_clockwise_vertices[current];
			const glm::vec2 &third = counter_clockwise_vertices[next];
			if (cross_product(second - first, third - second) <=
			    kTriangulationTolerance) {
				continue;
			}
			bool contains_vertex = false;
			for (std::size_t candidate : polygon) {
				if (candidate == previous || candidate == current || candidate == next) {
					continue;
				}
				if (point_in_triangle(counter_clockwise_vertices[candidate],
				                      first, second, third)) {
					contains_vertex = true;
					break;
				}
			}
			if (contains_vertex) continue;
			triangles->emplace_back(
				static_cast<int>(previous),
				static_cast<int>(current),
				static_cast<int>(next));
			polygon.erase(
				polygon.begin() + static_cast<std::ptrdiff_t>(polygon_index));
			removed_ear = true;
			break;
		}
		if (!removed_ear) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile polygon triangulation failed.";
			}
			triangles->clear();
			return false;
		}
	}
	if (polygon.size() == 3) {
		triangles->emplace_back(
			static_cast<int>(polygon[0]),
			static_cast<int>(polygon[1]),
			static_cast<int>(polygon[2]));
	}
	return triangles->size() + 2 == counter_clockwise_vertices.size();
}
