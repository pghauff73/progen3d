#pragma once

#include "editor/model/PreviewProjectionConfiguration.h"

#include <glm/glm.hpp>

enum class PreviewOrientation {
	PositiveX,
	NegativeX,
	PositiveY,
	NegativeY,
	PositiveZ,
	NegativeZ,
	Isometric
};

class PreviewCameraController
{
public:
	float &scaleFactor();
	float &azimuthDegrees();
	float &elevationDegrees();
	float &rollDegrees();
	float &targetX();
	float &targetY();
	float &targetZ();
	float &distance();

	float scaleFactor() const;
	float azimuthDegrees() const;
	float elevationDegrees() const;
	float rollDegrees() const;
	float targetX() const;
	float targetY() const;
	float targetZ() const;
	float distance() const;

	void reset();
	void orbitByPixels(float horizontal_pixels, float vertical_pixels);
	void panByPixels(float horizontal_pixels, float vertical_pixels, float viewport_scale);
	void zoomByWheel(float wheel_delta, float viewport_scale);
	void rollByDegrees(float degrees);
	void rotateViewAroundPositionByDegrees(float degrees);
	void translateTarget(const glm::vec3 &translation);
	glm::vec3 position() const;
	glm::vec3 forwardDirection() const;
	glm::vec3 rightDirection() const;
	glm::vec3 upDirection() const;
	void orientTo(PreviewOrientation orientation);
	void fitToBounds(float center_x,
	                 float center_y,
	                 float center_z,
	                 float half_extent_x,
	                 float half_extent_y,
	                 float half_extent_z,
	                 float viewport_width,
	                 float viewport_height,
	                 const PreviewProjectionConfiguration &projection);

private:
	float scale_factor_ = 1.0f;
	float azimuth_degrees_ = 0.0f;
	float elevation_degrees_ = 0.0f;
	float roll_degrees_ = 0.0f;
	float target_x_ = 0.0f;
	float target_y_ = 0.0f;
	float target_z_ = 0.0f;
	float distance_ = 5.0f;
};
