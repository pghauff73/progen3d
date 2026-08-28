#include "geometry/service/PolygonContainmentAnalyzer.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kContainmentTolerance = 1.0e-6f;

float cross_product(const glm::vec2 &first, const glm::vec2 &second)
{
	return first.x * second.y - first.y * second.x;
}

int orientation(const glm::vec2 &first,
	            const glm::vec2 &second,
	            const glm::vec2 &third)
{
	const float value = cross_product(second - first, third - first);
	if (value > kContainmentTolerance) return 1;
	if (value < -kContainmentTolerance) return -1;
	return 0;
}

bool point_on_segment(const glm::vec2 &point,
	                  const glm::vec2 &first,
	                  const glm::vec2 &second)
{
	if (orientation(first, second, point) != 0) return false;
	return point.x >= std::min(first.x, second.x) - kContainmentTolerance &&
	       point.x <= std::max(first.x, second.x) + kContainmentTolerance &&
	       point.y >= std::min(first.y, second.y) - kContainmentTolerance &&
	       point.y <= std::max(first.y, second.y) + kContainmentTolerance;
}

bool segments_intersect_or_touch(const glm::vec2 &first_start,
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

enum class PointLocation
{
	Outside,
	Inside,
	Boundary
};

PointLocation locate_point(const glm::vec2 &point,
	                       const std::vector<glm::vec2> &polygon)
{
	bool inside = false;
	for (std::size_t current = 0, previous = polygon.size() - 1;
	     current < polygon.size(); previous = current++) {
		const glm::vec2 &first = polygon[previous];
		const glm::vec2 &second = polygon[current];
		if (point_on_segment(point, first, second)) return PointLocation::Boundary;
		const bool crosses = ((second.y > point.y) != (first.y > point.y)) &&
			(point.x < (first.x - second.x) * (point.y - second.y) /
			           (first.y - second.y) + second.x);
		if (crosses) inside = !inside;
	}
	return inside ? PointLocation::Inside : PointLocation::Outside;
}

bool all_points_inside(const std::vector<glm::vec2> &contained,
	                   const std::vector<glm::vec2> &container)
{
	for (const glm::vec2 &point : contained) {
		if (locate_point(point, container) != PointLocation::Inside) return false;
	}
	return true;
}

}

PolygonContainmentRelationship PolygonContainmentAnalyzer::analyze(
	const std::vector<glm::vec2> &first,
	const std::vector<glm::vec2> &second) const
{
	for (std::size_t first_edge = 0; first_edge < first.size(); ++first_edge) {
		const glm::vec2 &first_start = first[first_edge];
		const glm::vec2 &first_end = first[(first_edge + 1) % first.size()];
		for (std::size_t second_edge = 0;
		     second_edge < second.size();
		     ++second_edge) {
			if (segments_intersect_or_touch(
				    first_start,
				    first_end,
				    second[second_edge],
				    second[(second_edge + 1) % second.size()])) {
				return PolygonContainmentRelationship::CrossingOrTouching;
			}
		}
	}
	if (all_points_inside(second, first)) {
		return PolygonContainmentRelationship::FirstContainsSecond;
	}
	if (all_points_inside(first, second)) {
		return PolygonContainmentRelationship::SecondContainsFirst;
	}
	return PolygonContainmentRelationship::Disjoint;
}
