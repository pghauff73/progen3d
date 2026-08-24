#include "vehicle/mcsmv2/service/VehicleSurfaceDomainEvaluationService.h"

#include <algorithm>
#include <cmath>

namespace {

bool pointInLoop(
	const glm::dvec2 &point,
	const VehicleSurfaceDomainLoop &loop)
{
	const std::vector<VehicleSurfaceCoordinate> &coordinates = loop.coordinates();
	bool inside = false;
	double previous_u = coordinates[coordinates.size() - 2u].longitudinalParameter();
	double previous_v = coordinates[coordinates.size() - 2u].periodicParameter();
	for (std::size_t index = 0u; index + 1u < coordinates.size(); ++index) {
		const double current_u = coordinates[index].longitudinalParameter();
		const double current_v = coordinates[index].periodicParameter();
		const bool crosses = ((current_v > point.y) != (previous_v > point.y)) &&
			(point.x <
			 (previous_u - current_u) * (point.y - current_v) /
				 (previous_v - current_v + 1.0e-18) +
			 current_u);
		if (crosses) inside = !inside;
		previous_u = current_u;
		previous_v = current_v;
	}
	return inside;
}

glm::dvec2 blendPeriodicCoordinates(
	const glm::dvec2 &first,
	const glm::dvec2 &second,
	const glm::dvec2 &third,
	const glm::dvec3 &weights)
{
	const double longitudinal = weights.x * first.x +
		weights.y * second.x + weights.z * third.x;
	const double first_angle = first.y * 2.0 * 3.14159265358979323846;
	const double second_angle = second.y * 2.0 * 3.14159265358979323846;
	const double third_angle = third.y * 2.0 * 3.14159265358979323846;
	const double cosine = weights.x * std::cos(first_angle) +
		weights.y * std::cos(second_angle) +
		weights.z * std::cos(third_angle);
	const double sine = weights.x * std::sin(first_angle) +
		weights.y * std::sin(second_angle) +
		weights.z * std::sin(third_angle);
	double periodic = std::atan2(sine, cosine) /
		(2.0 * 3.14159265358979323846);
	if (periodic < 0.0) periodic += 1.0;
	return glm::dvec2(std::clamp(longitudinal, 0.0, 1.0), periodic);
}

} // namespace

bool VehicleSurfaceDomainEvaluationService::contains(
	const VehicleSurfaceDomain &domain,
	const glm::dvec2 &surface_coordinate) const
{
	for (const VehicleSurfaceDomainLoop &loop : domain.loops()) {
		if (pointInLoop(surface_coordinate, loop)) return true;
	}
	return false;
}

glm::dvec2 VehicleSurfaceDomainEvaluationService::interpolateFaceCoordinate(
	const glm::ivec3 &face,
	const glm::dvec3 &barycentric_coordinates,
	const std::vector<glm::dvec2> &vertex_surface_coordinates) const
{
	return blendPeriodicCoordinates(
		vertex_surface_coordinates.at(static_cast<std::size_t>(face.x)),
		vertex_surface_coordinates.at(static_cast<std::size_t>(face.y)),
		vertex_surface_coordinates.at(static_cast<std::size_t>(face.z)),
		barycentric_coordinates);
}

glm::dvec2 VehicleSurfaceDomainEvaluationService::calculateFaceCentroidCoordinate(
	const glm::ivec3 &face,
	const std::vector<glm::dvec2> &vertex_surface_coordinates) const
{
	return interpolateFaceCoordinate(
		face, glm::dvec3(1.0 / 3.0), vertex_surface_coordinates);
}
