#include "vehicle/parametric/service/ModernCarObservationGenerationService.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <tuple>
#include <vector>

namespace {

double cross_product(
	const glm::dvec2 &origin,
	const glm::dvec2 &first,
	const glm::dvec2 &second)
{
	return (first.x - origin.x) * (second.y - origin.y) -
	       (first.y - origin.y) * (second.x - origin.x);
}

std::vector<glm::dvec2> convex_hull(std::vector<glm::dvec2> points)
{
	std::sort(points.begin(), points.end(), [](const glm::dvec2 &first,
	                                          const glm::dvec2 &second) {
		return std::tie(first.x, first.y) < std::tie(second.x, second.y);
	});
	points.erase(std::unique(points.begin(), points.end(), [](const glm::dvec2 &first,
	                                                         const glm::dvec2 &second) {
		return std::fabs(first.x - second.x) <= 1.0e-9 &&
		       std::fabs(first.y - second.y) <= 1.0e-9;
	}), points.end());
	if (points.size() <= 2u) return points;
	std::vector<glm::dvec2> hull;
	hull.reserve(points.size() * 2u);
	for (const glm::dvec2 &point : points) {
		while (hull.size() >= 2u &&
		       cross_product(hull[hull.size() - 2u], hull.back(), point) <= 0.0) {
			hull.pop_back();
		}
		hull.push_back(point);
	}
	const std::size_t lower_size = hull.size();
	for (auto point = points.rbegin(); point != points.rend(); ++point) {
		while (hull.size() > lower_size &&
		       cross_product(hull[hull.size() - 2u], hull.back(), *point) <= 0.0) {
			hull.pop_back();
		}
		hull.push_back(*point);
	}
	if (!hull.empty()) hull.pop_back();
	return hull;
}

std::vector<glm::dvec2> normalized_projection(
	const Mesh &mesh,
	const glm::dvec2 &minimum,
	const glm::dvec2 &maximum,
	const std::function<glm::dvec2(const glm::dvec3 &)> &project)
{
	const glm::dvec2 span = glm::max(maximum - minimum, glm::dvec2(1.0e-12));
	std::vector<glm::dvec2> points;
	points.reserve(mesh.vertices.size());
	for (const glm::vec3 &vertex : mesh.vertices) {
		const glm::dvec2 normalized =
			glm::clamp((project(glm::dvec3(vertex)) - minimum) / span,
			           glm::dvec2(0.0), glm::dvec2(1.0));
		points.push_back(normalized);
	}
	return convex_hull(std::move(points));
}

} // namespace

GeneratedObservationSet ModernCarObservationGenerationService::generate(
	const ModernCarVariantDefinition &variant,
	const GeneratedBodyMesh &body_mesh) const
{
	std::vector<GeneratedVehicleObservation> observations;
	if (!body_mesh.mesh()) return GeneratedObservationSet(std::move(observations));
	const double half_width = variant.package().width() * 0.5;
	const double half_length = variant.package().length() * 0.5;
	const double height = variant.package().height();
	const Mesh &mesh = *body_mesh.mesh();

	auto add_observation = [&](const std::string &name,
	                          GeneratedObservationView view,
	                          glm::dvec3 direction,
	                          glm::dvec3 up,
	                          glm::dvec2 minimum,
	                          glm::dvec2 maximum,
	                          const std::function<glm::dvec2(const glm::dvec3 &)> &project) {
		observations.emplace_back(
			"MCP_OMv1." + variant.identifier() + ".Camera." + name,
			view,
			direction,
			up,
			normalized_projection(mesh, minimum, maximum, project));
	};
	add_observation(
		"front", GeneratedObservationView::Front,
		{0.0, 0.0, -1.0}, {0.0, 1.0, 0.0},
		{-half_width, 0.0}, {half_width, height},
		[](const glm::dvec3 &point) { return glm::dvec2(point.x, point.y); });
	add_observation(
		"rear", GeneratedObservationView::Rear,
		{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0},
		{-half_width, 0.0}, {half_width, height},
		[](const glm::dvec3 &point) { return glm::dvec2(-point.x, point.y); });
	add_observation(
		"left", GeneratedObservationView::Left,
		{1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
		{-half_length, 0.0}, {half_length, height},
		[](const glm::dvec3 &point) { return glm::dvec2(-point.z, point.y); });
	add_observation(
		"right", GeneratedObservationView::Right,
		{-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0},
		{-half_length, 0.0}, {half_length, height},
		[](const glm::dvec3 &point) { return glm::dvec2(point.z, point.y); });
	add_observation(
		"top", GeneratedObservationView::Top,
		{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0},
		{-half_length, -half_width}, {half_length, half_width},
		[](const glm::dvec3 &point) { return glm::dvec2(point.z, point.x); });
	return GeneratedObservationSet(std::move(observations));
}
