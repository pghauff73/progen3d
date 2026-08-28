#pragma once

#include "vehicle/model/VehicleBodySpecification.h"
#include "vehicle/model/VehicleJoint.h"
#include "vehicle/model/VehicleValidationReport.h"

#include <glm/glm.hpp>

#include <vector>

class VehicleKinematicValidationService
{
public:
	VehicleValidationReport validateJoints(
		const std::vector<VehicleJoint> &joints) const;

	VehicleValidationReport validateWheelMovementEnvelope(
		const WheelArchSpecification &wheel_arch,
		float tire_section_width,
		float steering_degrees,
		float vertical_travel) const;

	glm::mat4 calculateWheelStateTransform(
		glm::vec3 wheel_center,
		float steering_degrees,
		float vertical_travel,
		float wheel_rotation_degrees) const;
};
