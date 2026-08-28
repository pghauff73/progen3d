#include "geometry/service/ProfileCapTriangulator.h"

#include "geometry/service/SimplePolygonValidator.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace {

constexpr float kTriangulationTolerance = 1.0e-6f;

float cross_product(const glm::vec2 &first, const glm::vec2 &second)
{
	return first.x * second.y - first.y * second.x;
}

bool approximately_equal(const glm::vec2 &first, const glm::vec2 &second)
{
	const glm::vec2 difference = first - second;
	return glm::dot(difference, difference) <=
	       kTriangulationTolerance * kTriangulationTolerance;
}

int orientation(const glm::vec2 &first,
	            const glm::vec2 &second,
	            const glm::vec2 &third)
{
	const float value = cross_product(second - first, third - first);
	if (value > kTriangulationTolerance) return 1;
	if (value < -kTriangulationTolerance) return -1;
	return 0;
}

bool point_on_segment(const glm::vec2 &point,
	                  const glm::vec2 &first,
	                  const glm::vec2 &second)
{
	if (orientation(first, second, point) != 0) return false;
	return point.x >= std::min(first.x, second.x) - kTriangulationTolerance &&
	       point.x <= std::max(first.x, second.x) + kTriangulationTolerance &&
	       point.y >= std::min(first.y, second.y) - kTriangulationTolerance &&
	       point.y <= std::max(first.y, second.y) + kTriangulationTolerance;
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

bool point_is_in_material(const glm::vec2 &point, const Profile2D &profile)
{
	if (locate_point(point, profile.outerLoop().points()) != PointLocation::Inside) {
		return false;
	}
	for (const ProfileLoop2D &hole : profile.innerLoops()) {
		if (locate_point(point, hole.points()) != PointLocation::Outside) return false;
	}
	return true;
}

bool edge_shares_endpoint(const glm::vec2 &edge_start,
	                      const glm::vec2 &edge_end,
	                      const glm::vec2 &bridge_start,
	                      const glm::vec2 &bridge_end)
{
	return approximately_equal(edge_start, bridge_start) ||
	       approximately_equal(edge_end, bridge_start) ||
	       approximately_equal(edge_start, bridge_end) ||
	       approximately_equal(edge_end, bridge_end);
}

bool bridge_is_visible(const glm::vec2 &bridge_start,
	                   const glm::vec2 &bridge_end,
	                   const std::vector<glm::vec2> &current_polygon,
	                   const Profile2D &profile)
{
	if (approximately_equal(bridge_start, bridge_end)) return false;
	for (std::size_t edge_index = 0;
	     edge_index < current_polygon.size();
	     ++edge_index) {
		const glm::vec2 &edge_start = current_polygon[edge_index];
		const glm::vec2 &edge_end =
			current_polygon[(edge_index + 1) % current_polygon.size()];
		if (edge_shares_endpoint(
				edge_start, edge_end, bridge_start, bridge_end)) {
			continue;
		}
		if (segments_intersect_or_touch(
				bridge_start, bridge_end, edge_start, edge_end)) {
			return false;
		}
	}
	for (const ProfileLoop2D &hole : profile.innerLoops()) {
		const std::vector<glm::vec2> &points = hole.points();
		for (std::size_t edge_index = 0; edge_index < points.size(); ++edge_index) {
			const glm::vec2 &edge_start = points[edge_index];
			const glm::vec2 &edge_end = points[(edge_index + 1) % points.size()];
			if (edge_shares_endpoint(
					edge_start, edge_end, bridge_start, bridge_end)) {
				continue;
			}
			if (segments_intersect_or_touch(
					bridge_start, bridge_end, edge_start, edge_end)) {
				return false;
			}
		}
	}
	return point_is_in_material((bridge_start + bridge_end) * 0.5f, profile);
}

bool merge_hole(const Profile2D &profile,
	            const ProfileLoop2D &hole,
	            std::vector<glm::vec2> *current_polygon,
	            std::string *diagnostic)
{
	float best_distance = std::numeric_limits<float>::max();
	std::size_t best_outer_index = 0;
	std::size_t best_hole_index = 0;
	bool found_bridge = false;
	for (std::size_t outer_index = 0;
	     outer_index < current_polygon->size();
	     ++outer_index) {
		for (std::size_t hole_index = 0;
		     hole_index < hole.points().size();
		     ++hole_index) {
			const glm::vec2 &outer_point = (*current_polygon)[outer_index];
			const glm::vec2 &hole_point = hole.points()[hole_index];
			if (!bridge_is_visible(
					hole_point, outer_point, *current_polygon, profile)) {
				continue;
			}
			const glm::vec2 difference = outer_point - hole_point;
			const float distance = glm::dot(difference, difference);
			if (!found_bridge || distance < best_distance - kTriangulationTolerance ||
			    (std::fabs(distance - best_distance) <= kTriangulationTolerance &&
			     std::pair<std::size_t, std::size_t>(outer_index, hole_index) <
			         std::pair<std::size_t, std::size_t>(best_outer_index, best_hole_index))) {
				found_bridge = true;
				best_distance = distance;
				best_outer_index = outer_index;
				best_hole_index = hole_index;
			}
		}
	}
	if (!found_bridge) {
		if (diagnostic != nullptr) {
			*diagnostic = "Profile cap triangulation could not find a visible bridge for an inner loop.";
		}
		return false;
	}

	std::vector<glm::vec2> merged;
	merged.reserve(current_polygon->size() + hole.points().size() + 2u);
	for (std::size_t index = 0; index <= best_outer_index; ++index) {
		merged.push_back((*current_polygon)[index]);
	}
	for (std::size_t offset = 0; offset <= hole.points().size(); ++offset) {
		merged.push_back(
			hole.points()[(best_hole_index + offset) % hole.points().size()]);
	}
	merged.push_back((*current_polygon)[best_outer_index]);
	for (std::size_t index = best_outer_index + 1;
	     index < current_polygon->size();
	     ++index) {
		merged.push_back((*current_polygon)[index]);
	}
	*current_polygon = std::move(merged);
	return true;
}

bool point_strictly_inside_triangle(const glm::vec2 &point,
	                                const glm::vec2 &first,
	                                const glm::vec2 &second,
	                                const glm::vec2 &third)
{
	if (approximately_equal(point, first) || approximately_equal(point, second) ||
	    approximately_equal(point, third)) {
		return false;
	}
	const float first_cross = cross_product(second - first, point - first);
	const float second_cross = cross_product(third - second, point - second);
	const float third_cross = cross_product(first - third, point - third);
	return first_cross > kTriangulationTolerance &&
	       second_cross > kTriangulationTolerance &&
	       third_cross > kTriangulationTolerance;
}

bool erase_redundant_vertex(std::vector<glm::vec2> *polygon)
{
	for (std::size_t index = 0; index < polygon->size(); ++index) {
		const glm::vec2 &previous =
			(*polygon)[(index + polygon->size() - 1) % polygon->size()];
		const glm::vec2 &current = (*polygon)[index];
		const glm::vec2 &next = (*polygon)[(index + 1) % polygon->size()];
		if (approximately_equal(previous, current) ||
		    approximately_equal(current, next) ||
		    (std::fabs(cross_product(current - previous, next - current)) <=
		         kTriangulationTolerance &&
		     point_on_segment(current, previous, next))) {
			polygon->erase(polygon->begin() + static_cast<std::ptrdiff_t>(index));
			return true;
		}
	}
	return false;
}

bool ear_clip(std::vector<glm::vec2> polygon,
	          std::vector<ProfileTriangle2D> *triangles,
	          std::string *diagnostic)
{
	std::size_t guard = 0;
	while (polygon.size() > 3 && guard++ < polygon.size() * polygon.size() * 4u) {
		bool removed_ear = false;
		for (std::size_t index = 0; index < polygon.size(); ++index) {
			const std::size_t previous_index =
				(index + polygon.size() - 1) % polygon.size();
			const std::size_t next_index = (index + 1) % polygon.size();
			const glm::vec2 &first = polygon[previous_index];
			const glm::vec2 &second = polygon[index];
			const glm::vec2 &third = polygon[next_index];
			if (cross_product(second - first, third - second) <=
			    kTriangulationTolerance) {
				continue;
			}
			bool contains_vertex = false;
			for (std::size_t candidate_index = 0;
			     candidate_index < polygon.size();
			     ++candidate_index) {
				if (candidate_index == previous_index || candidate_index == index ||
				    candidate_index == next_index) {
					continue;
				}
				if (point_strictly_inside_triangle(
						polygon[candidate_index], first, second, third)) {
					contains_vertex = true;
					break;
				}
			}
			if (contains_vertex) continue;
			triangles->push_back({first, second, third});
			polygon.erase(polygon.begin() + static_cast<std::ptrdiff_t>(index));
			removed_ear = true;
			break;
		}
		if (!removed_ear && !erase_redundant_vertex(&polygon)) {
			if (diagnostic != nullptr) {
				*diagnostic = "Profile cap triangulation failed after deterministic hole bridging.";
			}
			return false;
		}
	}
	if (polygon.size() == 3 &&
	    cross_product(polygon[1] - polygon[0], polygon[2] - polygon[1]) >
	        kTriangulationTolerance) {
		triangles->push_back({polygon[0], polygon[1], polygon[2]});
	}
	return !triangles->empty();
}

}

bool ProfileCapTriangulator::triangulate(
	const Profile2D &profile,
	std::vector<ProfileTriangle2D> *triangles,
	std::string *diagnostic) const
{
	if (triangles == nullptr) return false;
	triangles->clear();
	std::vector<glm::vec2> merged_polygon = profile.outerLoop().points();
	std::vector<const ProfileLoop2D *> holes;
	holes.reserve(profile.innerLoops().size());
	for (const ProfileLoop2D &hole : profile.innerLoops()) holes.push_back(&hole);
	std::sort(
		holes.begin(), holes.end(), [](const ProfileLoop2D *first, const ProfileLoop2D *second) {
			auto rightmost_x = [](const ProfileLoop2D *loop) {
				float value = std::numeric_limits<float>::lowest();
				for (const glm::vec2 &point : loop->points()) value = std::max(value, point.x);
				return value;
			};
			const float first_x = rightmost_x(first);
			const float second_x = rightmost_x(second);
			if (first_x != second_x) return first_x > second_x;
			return first->points().size() < second->points().size();
		});
	for (const ProfileLoop2D *hole : holes) {
		if (!merge_hole(profile, *hole, &merged_polygon, diagnostic)) return false;
	}
	if (SimplePolygonValidator::signedArea(merged_polygon) <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Profile cap triangulation produced a non-positive merged polygon.";
		}
		return false;
	}
	return ear_clip(std::move(merged_polygon), triangles, diagnostic);
}
