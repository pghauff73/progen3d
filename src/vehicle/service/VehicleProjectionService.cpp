#include "vehicle/service/VehicleProjectionService.h"

#include <glm/geometric.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {

float squared_distance(const glm::vec2 &first, const glm::vec2 &second)
{
	const glm::vec2 difference = first - second;
	return glm::dot(difference, difference);
}

float nearest_polyline_distance_squared(
	const glm::vec2 &point,
	const std::vector<glm::vec2> &polyline)
{
	if (polyline.empty()) return std::numeric_limits<float>::infinity();
	if (polyline.size() == 1u) return squared_distance(point, polyline.front());
	float nearest = std::numeric_limits<float>::infinity();
	for (std::size_t index = 1u; index < polyline.size(); ++index) {
		const glm::vec2 start = polyline[index - 1u];
		const glm::vec2 end = polyline[index];
		const glm::vec2 segment = end - start;
		const float length_squared = glm::dot(segment, segment);
		const float parameter = length_squared > 1.0e-12f
			? std::clamp(glm::dot(point - start, segment) / length_squared, 0.0f, 1.0f)
			: 0.0f;
		nearest = std::min(
			nearest, squared_distance(point, start + segment * parameter));
	}
	return nearest;
}

} // namespace

std::optional<glm::vec2> VehicleProjectionService::projectPoint(
	const VehicleCameraModel &camera,
	const glm::vec3 &world_point) const
{
	const glm::vec4 camera_point =
		camera.worldToCamera() * glm::vec4(world_point, 1.0f);
	if (!std::isfinite(camera_point.x) || !std::isfinite(camera_point.y) ||
	    !std::isfinite(camera_point.z)) {
		return std::nullopt;
	}
	if (camera.projection() == VehicleCameraProjection::Orthographic) {
		if (!std::isfinite(camera.orthographicScale()) ||
		    std::fabs(camera.orthographicScale()) < 1.0e-8f) {
			return std::nullopt;
		}
		return camera.principalPoint() +
		       glm::vec2(camera_point.x, camera_point.y) /
			       camera.orthographicScale();
	}
	const float depth = std::fabs(camera_point.z);
	if (!std::isfinite(camera.focalLength()) || camera.focalLength() <= 0.0f ||
	    depth < 1.0e-8f) {
		return std::nullopt;
	}
	const glm::vec2 dimensions = camera.imageDimensions();
	if (dimensions.x <= 0.0f || dimensions.y <= 0.0f) return std::nullopt;
	return camera.principalPoint() + glm::vec2(
		camera.focalLength() * camera_point.x / depth / dimensions.x,
		camera.focalLength() * camera_point.y / depth / dimensions.y);
}

float VehicleProjectionService::calculateLandmarkError(
	const VehicleLandmark &landmark,
	const VehicleObservationSet &observations) const
{
	float weighted_error = 0.0f;
	float total_weight = 0.0f;
	for (const VehicleLandmarkObservation &observation : landmark.observations()) {
		const VehicleViewObservation *view =
			observations.findByCamera(observation.cameraIdentifier());
		if (view == nullptr) continue;
		const std::optional<glm::vec2> projected =
			projectPoint(view->camera(), landmark.position());
		if (!projected.has_value()) continue;
		const float uncertainty = std::max(observation.uncertainty(), 1.0e-5f);
		const float weight = std::max(observation.confidence(), 0.0f) /
		                     (uncertainty * uncertainty);
		weighted_error += weight * squared_distance(*projected, observation.imagePoint());
		total_weight += weight;
	}
	return total_weight > 0.0f ? weighted_error / total_weight
	                          : std::numeric_limits<float>::infinity();
}

float VehicleProjectionService::calculateSilhouetteError(
	const SilhouetteConstraint &constraint,
	const VehicleObservationSet &observations) const
{
	const VehicleViewObservation *view = observations.findByCamera(
		constraint.cameraIdentifier());
	if (view == nullptr || constraint.observedSilhouette().empty()) {
		return std::numeric_limits<float>::infinity();
	}
	float error = 0.0f;
	std::size_t projected_count = 0u;
	for (const glm::vec3 &point : constraint.modelBoundaryPoints()) {
		const std::optional<glm::vec2> projected = projectPoint(view->camera(), point);
		if (!projected.has_value()) continue;
		error += nearest_polyline_distance_squared(
			*projected, constraint.observedSilhouette());
		++projected_count;
	}
	return projected_count > 0u
		? constraint.weight() * error / static_cast<float>(projected_count)
		: std::numeric_limits<float>::infinity();
}

float VehicleProjectionService::calculateCharacterLineError(
	const AutomotiveCharacterCurve &curve,
	const VehicleObservationSet &observations) const
{
	if (curve.controlPoints().empty()) return std::numeric_limits<float>::infinity();
	float error = 0.0f;
	std::size_t comparison_count = 0u;
	for (const std::string &observation_identifier : curve.observationIdentifiers()) {
		for (const VehicleViewObservation &view : observations.observations()) {
			for (const CharacterLineObservation &observation : view.characterLines()) {
				if (observation.identifier() != observation_identifier ||
				    observation.cameraIdentifier() != view.camera().identifier()) {
					continue;
				}
				for (const glm::vec3 &point : curve.controlPoints()) {
					const std::optional<glm::vec2> projected =
						projectPoint(view.camera(), point);
					if (!projected.has_value()) continue;
					const float uncertainty = std::max(observation.uncertainty(), 1.0e-5f);
					const float weight = std::max(observation.confidence(), 0.0f) /
					                     (uncertainty * uncertainty);
					error += weight * nearest_polyline_distance_squared(
						*projected, observation.imagePoints());
					++comparison_count;
				}
			}
		}
	}
	return comparison_count > 0u
		? error / static_cast<float>(comparison_count)
		: std::numeric_limits<float>::infinity();
}
