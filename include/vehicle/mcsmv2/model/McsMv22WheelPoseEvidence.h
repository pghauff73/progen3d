#pragma once

#include <glm/glm.hpp>

#include <string>
#include <utility>

class McsMv22WheelPoseEvidence
{
public:
	McsMv22WheelPoseEvidence(
		std::string wheel_identifier,
		double steer_degrees,
		double travel_metres,
		double camber_degrees,
		double toe_degrees,
		glm::dvec3 source_centre,
		glm::dmat4 source_transform)
		: wheel_identifier_(std::move(wheel_identifier)),
		  steer_degrees_(steer_degrees),
		  travel_metres_(travel_metres),
		  camber_degrees_(camber_degrees),
		  toe_degrees_(toe_degrees),
		  source_centre_(source_centre),
		  source_transform_(source_transform)
	{
	}

	const std::string &wheelIdentifier() const { return wheel_identifier_; }
	double steerDegrees() const { return steer_degrees_; }
	double travelMetres() const { return travel_metres_; }
	double camberDegrees() const { return camber_degrees_; }
	double toeDegrees() const { return toe_degrees_; }
	const glm::dvec3 &sourceCentre() const { return source_centre_; }
	const glm::dmat4 &sourceTransform() const { return source_transform_; }

private:
	std::string wheel_identifier_;
	double steer_degrees_ = 0.0;
	double travel_metres_ = 0.0;
	double camber_degrees_ = 0.0;
	double toe_degrees_ = 0.0;
	glm::dvec3 source_centre_{0.0};
	glm::dmat4 source_transform_{1.0};
};
