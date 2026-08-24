#include "geometry/service/SimplePolygonValidator.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPolygonTolerance = 1.0e-6f;

float cross_product(const glm::vec2 &first, const glm::vec2 &second)
{
	return first.x * second.y - first.y * second.x;
}

bool nearly_equal(const glm::vec2 &first, const glm::vec2 &second)
{
	const glm::vec2 difference = first - second;
	return glm::dot(difference, difference) <=
	       kPolygonTolerance * kPolygonTolerance;
}

int orientation(const glm::vec2 &first,
	            const glm::vec2 &second,
	            const glm::vec2 &third)
{
	const float value = cross_product(second - first, third - first);
	if (value > kPolygonTolerance) return 1;
	if (value < -kPolygonTolerance) return -1;
	return 0;
}

bool point_on_segment(const glm::vec2 &point,
	                  const glm::vec2 &first,
	                  const glm::vec2 &second)
{
	if (orientation(first, second, point) != 0) return false;
	return point.x >= std::min(first.x, second.x) - kPolygonTolerance &&
	       point.x <= std::max(first.x, second.x) + kPolygonTolerance &&
	       point.y >= std::min(first.y, second.y) - kPolygonTolerance &&
	       point.y <= std::max(first.y, second.y) + kPolygonTolerance;
}

bool segments_intersect(const glm::vec2 &first_start,
	                    const glm::vec2 &first_end,
	                    const glm::vec2 &second_start,
	                    const glm::vec2 &second_end)
{
	const int first_orientation = orientation(first_start, first_end, second_start);
	const int second_orientation = orientation(first_start, first_end, second_end);
	const int third_orientation = orientation(second_start, second_end, first_start);
	const int fourth_orientation = orientation(second_start, second_end, first_end);
	if (first_orientation != second_orientation &&
	    third_orientation != fourth_orientation) {
		return true;
	}
	return (first_orientation == 0 && point_on_segment(second_start, first_start, first_end)) ||
	       (second_orientation == 0 && point_on_segment(second_end, first_start, first_end)) ||
	       (third_orientation == 0 && point_on_segment(first_start, second_start, second_end)) ||
	       (fourth_orientation == 0 && point_on_segment(first_end, second_start, second_end));
}

}

float SimplePolygonValidator::signedArea(
	const std::vector<glm::vec2> &vertices)
{
	float doubled_area = 0.0f;
	for (std::size_t index = 0; index < vertices.size(); ++index) {
		const glm::vec2 &current = vertices[index];
		const glm::vec2 &next = vertices[(index + 1) % vertices.size()];
		doubled_area += current.x * next.y - next.x * current.y;
	}
	return 0.5f * doubled_area;
}

bool SimplePolygonValidator::normalize(
	const std::vector<glm::vec2> &input_vertices,
	std::vector<glm::vec2> *normalized_vertices,
	std::string *diagnostic) const
{
	if (normalized_vertices == nullptr) return false;
	*normalized_vertices = input_vertices;
	if (normalized_vertices->size() > 1 &&
	    nearly_equal(normalized_vertices->front(), normalized_vertices->back())) {
		normalized_vertices->pop_back();
	}
	if (normalized_vertices->size() < 3) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile polygons require at least three vertices.";
		}
		return false;
	}
	for (const glm::vec2 &vertex : *normalized_vertices) {
		if (!std::isfinite(vertex.x) || !std::isfinite(vertex.y)) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile polygon coordinates must be finite.";
			}
			return false;
		}
	}
	for (std::size_t index = 0; index < normalized_vertices->size(); ++index) {
		const glm::vec2 &previous = (*normalized_vertices)[
			(index + normalized_vertices->size() - 1) % normalized_vertices->size()];
		const glm::vec2 &current = (*normalized_vertices)[index];
		const glm::vec2 &next = (*normalized_vertices)[
			(index + 1) % normalized_vertices->size()];
		if (nearly_equal(current, next)) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile polygons cannot contain zero-length edges.";
			}
			return false;
		}
		if (std::fabs(cross_product(current - previous, next - current)) <=
		    kPolygonTolerance) {
			if (diagnostic != nullptr) {
				*diagnostic = "AxialProfile polygons cannot contain collinear vertex triples.";
			}
			return false;
		}
	}
	for (std::size_t first_edge = 0;
	     first_edge < normalized_vertices->size();
	     ++first_edge) {
		const std::size_t first_next =
			(first_edge + 1) % normalized_vertices->size();
		for (std::size_t second_edge = first_edge + 1;
		     second_edge < normalized_vertices->size();
		     ++second_edge) {
			const std::size_t second_next =
				(second_edge + 1) % normalized_vertices->size();
			if (first_edge == second_edge || first_next == second_edge ||
			    second_next == first_edge) {
				continue;
			}
			if (segments_intersect((*normalized_vertices)[first_edge],
			                       (*normalized_vertices)[first_next],
			                       (*normalized_vertices)[second_edge],
			                       (*normalized_vertices)[second_next])) {
				if (diagnostic != nullptr) {
					*diagnostic = "AxialProfile polygons must be non-self-intersecting.";
				}
				return false;
			}
		}
	}
	const float area = signedArea(*normalized_vertices);
	if (!std::isfinite(area) || std::fabs(area) <= kPolygonTolerance) {
		if (diagnostic != nullptr) {
			*diagnostic = "AxialProfile polygons must have non-zero area.";
		}
		return false;
	}
	if (area < 0.0f) {
		std::reverse(normalized_vertices->begin(), normalized_vertices->end());
	}
	return true;
}
