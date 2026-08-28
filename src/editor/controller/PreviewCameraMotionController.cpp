#include "editor/controller/PreviewCameraMotionController.h"

#include <algorithm>
#include <cmath>

#include <glm/glm.hpp>

void PreviewCameraMotionController::advance(
	PreviewCameraController &camera,
	const PreviewNavigationInput &input,
	const PreviewInteractionSettings &settings,
	float delta_seconds) const
{
	if (!std::isfinite(delta_seconds) || delta_seconds <= 0.0f) {
		return;
	}

	const float clamped_delta_seconds = std::min(delta_seconds, 0.1f);
	glm::vec3 requested_direction(0.0f);
	if (input.move_forward) {
		requested_direction += camera.forwardDirection();
	}
	if (input.move_backward) {
		requested_direction -= camera.forwardDirection();
	}
	if (input.move_right) {
		requested_direction += camera.rightDirection();
	}
	if (input.move_left) {
		requested_direction -= camera.rightDirection();
	}
	if (input.move_up) {
		requested_direction += glm::vec3(0.0f, 1.0f, 0.0f);
	}
	if (input.move_down) {
		requested_direction -= glm::vec3(0.0f, 1.0f, 0.0f);
	}

	const float direction_length = glm::length(requested_direction);
	if (direction_length > 0.0001f) {
		const float velocity = std::max(settings.navigation_velocity, 0.0f);
		camera.translateTarget(
			(requested_direction / direction_length) * velocity * clamped_delta_seconds);
	}

	float rotation_direction = 0.0f;
	if (input.rotate_left) {
		rotation_direction += 1.0f;
	}
	if (input.rotate_right) {
		rotation_direction -= 1.0f;
	}
	if (rotation_direction != 0.0f) {
		const float rotation_velocity = std::max(settings.rotation_velocity_degrees, 0.0f);
		camera.rotateViewAroundPositionByDegrees(
			rotation_direction * rotation_velocity * clamped_delta_seconds);
	}
}
