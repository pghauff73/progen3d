#include "editor/controller/PreviewCameraController.h"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kMinimumDistance = 1.25f;
constexpr float kMaximumDistance = 80.0f;

glm::vec3 rotate_around_axis(const glm::vec3 &vector,
	                         const glm::vec3 &axis,
	                         float radians)
{
	const glm::vec3 normalized_axis = glm::normalize(axis);
	const float cosine = std::cos(radians);
	const float sine = std::sin(radians);
	return vector * cosine +
	       glm::cross(normalized_axis, vector) * sine +
	       normalized_axis * glm::dot(normalized_axis, vector) * (1.0f - cosine);
}

}

float &PreviewCameraController::scaleFactor() { return scale_factor_; }
float &PreviewCameraController::azimuthDegrees() { return azimuth_degrees_; }
float &PreviewCameraController::elevationDegrees() { return elevation_degrees_; }
float &PreviewCameraController::rollDegrees() { return roll_degrees_; }
float &PreviewCameraController::targetX() { return target_x_; }
float &PreviewCameraController::targetY() { return target_y_; }
float &PreviewCameraController::targetZ() { return target_z_; }
float &PreviewCameraController::distance() { return distance_; }

float PreviewCameraController::scaleFactor() const { return scale_factor_; }
float PreviewCameraController::azimuthDegrees() const { return azimuth_degrees_; }
float PreviewCameraController::elevationDegrees() const { return elevation_degrees_; }
float PreviewCameraController::rollDegrees() const { return roll_degrees_; }
float PreviewCameraController::targetX() const { return target_x_; }
float PreviewCameraController::targetY() const { return target_y_; }
float PreviewCameraController::targetZ() const { return target_z_; }
float PreviewCameraController::distance() const { return distance_; }

void PreviewCameraController::reset()
{
	scale_factor_ = 1.0f;
	azimuth_degrees_ = 0.0f;
	elevation_degrees_ = 0.0f;
	roll_degrees_ = 0.0f;
	target_x_ = 0.0f;
	target_y_ = 0.0f;
	target_z_ = 0.0f;
	distance_ = 5.0f;
}

void PreviewCameraController::orbitByPixels(float horizontal_pixels, float vertical_pixels)
{
	azimuth_degrees_ += horizontal_pixels * 0.35f;
	elevation_degrees_ = std::clamp(
		elevation_degrees_ + vertical_pixels * 0.35f,
		-89.0f,
		89.0f);
}

void PreviewCameraController::panByPixels(float horizontal_pixels,
	                                      float vertical_pixels,
	                                      float viewport_scale)
{
	const float azimuth_radians = -azimuth_degrees_ * kPi / 180.0f;
	const float right_x = std::sin(azimuth_radians);
	const float right_z = -std::cos(azimuth_radians);
	const float forward_x = -std::cos(azimuth_radians);
	const float forward_z = -std::sin(azimuth_radians);
	const float pan_scale =
		distance_ / (std::max(viewport_scale, 1.0f) * std::max(scale_factor_, 0.2f));
	const float delta_right = -horizontal_pixels * pan_scale;
	const float delta_forward = vertical_pixels * pan_scale;
	target_x_ += right_x * delta_right + forward_x * delta_forward;
	target_z_ += right_z * delta_right + forward_z * delta_forward;
}

void PreviewCameraController::zoomByWheel(float wheel_delta, float viewport_scale)
{
	(void)viewport_scale;
	if (wheel_delta == 0.0f) {
		return;
	}
	const float zoom_factor = std::pow(0.88f, wheel_delta);
	distance_ = std::clamp(distance_ * zoom_factor, kMinimumDistance, kMaximumDistance);
}

void PreviewCameraController::rollByDegrees(float degrees)
{
	if (!std::isfinite(degrees)) {
		return;
	}
	roll_degrees_ = std::remainder(roll_degrees_ + degrees, 360.0f);
}

void PreviewCameraController::rotateViewAroundPositionByDegrees(float degrees)
{
	if (!std::isfinite(degrees)) {
		return;
	}

	const glm::vec3 fixed_position = position();
	azimuth_degrees_ = std::remainder(azimuth_degrees_ + degrees, 360.0f);
	const glm::vec3 rotated_target = fixed_position + forwardDirection() * distance_;
	target_x_ = rotated_target.x;
	target_y_ = rotated_target.y;
	target_z_ = rotated_target.z;
}

void PreviewCameraController::translateTarget(const glm::vec3 &translation)
{
	if (!std::isfinite(translation.x) ||
	    !std::isfinite(translation.y) ||
	    !std::isfinite(translation.z)) {
		return;
	}
	target_x_ += translation.x;
	target_y_ += translation.y;
	target_z_ += translation.z;
}

glm::vec3 PreviewCameraController::position() const
{
	return glm::vec3(target_x_, target_y_, target_z_) - forwardDirection() * distance_;
}

glm::vec3 PreviewCameraController::forwardDirection() const
{
	const float azimuth_radians = -azimuth_degrees_ * kPi / 180.0f;
	const float elevation_radians = elevation_degrees_ * kPi / 180.0f;
	const glm::vec3 orbit_direction(
		std::cos(elevation_radians) * std::cos(azimuth_radians),
		std::sin(elevation_radians),
		std::cos(elevation_radians) * std::sin(azimuth_radians));
	return glm::normalize(-orbit_direction);
}

glm::vec3 PreviewCameraController::rightDirection() const
{
	const glm::vec3 forward = forwardDirection();
	glm::vec3 right = glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f));
	if (glm::length(right) <= 0.0001f) {
		right = glm::cross(forward, glm::vec3(0.0f, 0.0f, 1.0f));
	}
	right = glm::normalize(right);
	return rotate_around_axis(right, forward, roll_degrees_ * kPi / 180.0f);
}

glm::vec3 PreviewCameraController::upDirection() const
{
	return glm::normalize(glm::cross(rightDirection(), forwardDirection()));
}

void PreviewCameraController::orientTo(PreviewOrientation orientation)
{
	switch (orientation) {
	case PreviewOrientation::PositiveX:
		azimuth_degrees_ = 0.0f;
		elevation_degrees_ = 0.0f;
		roll_degrees_ = 0.0f;
		break;
	case PreviewOrientation::NegativeX:
		azimuth_degrees_ = 180.0f;
		elevation_degrees_ = 0.0f;
		roll_degrees_ = 0.0f;
		break;
	case PreviewOrientation::PositiveY:
		azimuth_degrees_ = 0.0f;
		elevation_degrees_ = 89.0f;
		roll_degrees_ = 0.0f;
		break;
	case PreviewOrientation::NegativeY:
		azimuth_degrees_ = 0.0f;
		elevation_degrees_ = -89.0f;
		roll_degrees_ = 0.0f;
		break;
	case PreviewOrientation::PositiveZ:
		azimuth_degrees_ = -90.0f;
		elevation_degrees_ = 0.0f;
		roll_degrees_ = 0.0f;
		break;
	case PreviewOrientation::NegativeZ:
		azimuth_degrees_ = 90.0f;
		elevation_degrees_ = 0.0f;
		roll_degrees_ = 0.0f;
		break;
	case PreviewOrientation::Isometric:
		azimuth_degrees_ = -45.0f;
		elevation_degrees_ = 35.2643897f;
		roll_degrees_ = 0.0f;
		break;
	}
}

void PreviewCameraController::fitToBounds(float center_x,
	                                      float center_y,
	                                      float center_z,
	                                      float half_extent_x,
	                                      float half_extent_y,
	                                      float half_extent_z,
	                                      float viewport_width,
	                                      float viewport_height,
	                                      const PreviewProjectionConfiguration &projection)
{
	target_x_ = center_x;
	target_y_ = center_y;
	target_z_ = center_z;
	const float scaled_x = half_extent_x * scale_factor_;
	const float scaled_y = half_extent_y * scale_factor_;
	const float scaled_z = half_extent_z * scale_factor_;
	float bounding_radius = std::sqrt(
		scaled_x * scaled_x + scaled_y * scaled_y + scaled_z * scaled_z);
	if (bounding_radius <= 0.001f) {
		bounding_radius = std::max({scaled_x, scaled_y, scaled_z, 0.75f});
	}
	const float aspect = std::max(viewport_width, 1.0f) / std::max(viewport_height, 1.0f);
	const float field_of_view_y = projection.verticalFieldOfViewRadians();
	const float field_of_view_x =
		2.0f * std::atan(std::tan(field_of_view_y * 0.5f) * std::max(aspect, 0.1f));
	const float minimum_half_field_of_view =
		std::max(0.18f, std::min(field_of_view_x, field_of_view_y) * 0.5f);
	const float fitted_distance =
		(bounding_radius / std::sin(minimum_half_field_of_view)) * 1.08f + 0.85f;
	distance_ = std::clamp(fitted_distance, kMinimumDistance, kMaximumDistance);
}
