#include "vehicle/service/VehicleKinematicValidationService.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

namespace {

bool finite(const glm::vec3 &value)
{
	return std::isfinite(value.x) && std::isfinite(value.y) &&
	       std::isfinite(value.z);
}

} // namespace

VehicleValidationReport VehicleKinematicValidationService::validateJoints(
	const std::vector<VehicleJoint> &joints) const
{
	VehicleValidationReport report;
	for (const VehicleJoint &joint : joints) {
		const bool requires_axis = joint.type() != VehicleJointType::Fixed;
		if (joint.jointIdentifier().empty() ||
		    joint.sourceObjectIdentifier().empty() ||
		    joint.sourceInterfaceIdentifier().empty() ||
		    joint.targetObjectIdentifier().empty() ||
		    joint.targetInterfaceIdentifier().empty() || !finite(joint.axis()) ||
		    (requires_axis && glm::length(joint.axis()) <= 1.0e-6f) ||
		    !std::isfinite(joint.minimumState()) ||
		    !std::isfinite(joint.maximumState()) ||
		    !std::isfinite(joint.currentState()) ||
		    joint.minimumState() > joint.maximumState() ||
		    joint.currentState() < joint.minimumState() - 1.0e-5f ||
		    joint.currentState() > joint.maximumState() + 1.0e-5f) {
			report.addIssue(VehicleValidationIssue(
				VehicleDiagnosticCode::InvalidJoint,
				"Vehicle joint '" + joint.jointIdentifier() +
					"' has an invalid axis, endpoint, limit, or current state.",
				{joint.sourceObjectIdentifier(), joint.targetObjectIdentifier()}));
		}
	}
	return report;
}

VehicleValidationReport
VehicleKinematicValidationService::validateWheelMovementEnvelope(
	const WheelArchSpecification &wheel_arch,
	float tire_section_width,
	float steering_degrees,
	float vertical_travel) const
{
	VehicleValidationReport report;
	if (!std::isfinite(tire_section_width) || tire_section_width <= 0.0f ||
	    !std::isfinite(steering_degrees) ||
	    std::fabs(steering_degrees) > 30.0f + 1.0e-5f ||
	    !std::isfinite(vertical_travel)) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::InvalidJoint,
			"Wheel movement validation requires a positive tire width, steering within +/-30 degrees, and finite travel."));
		return report;
	}
	const float steering_extra =
		std::fabs(std::sin(glm::radians(steering_degrees))) *
		tire_section_width * 0.5f;
	const float required_radius =
		wheel_arch.tireRadius() + std::fabs(vertical_travel) + steering_extra * 0.15f;
	if (required_radius > wheel_arch.archRadius() + 1.0e-5f) {
		report.addIssue(VehicleValidationIssue(
			VehicleDiagnosticCode::TravelCollision,
			"Wheel swept envelope exceeds arch '" + wheel_arch.identifier() +
				"' at the requested steering and suspension state."));
	}
	return report;
}

glm::mat4 VehicleKinematicValidationService::calculateWheelStateTransform(
	glm::vec3 wheel_center,
	float steering_degrees,
	float vertical_travel,
	float wheel_rotation_degrees) const
{
	glm::mat4 transform = glm::translate(
		glm::mat4(1.0f), wheel_center + glm::vec3(0.0f, vertical_travel, 0.0f));
	transform = glm::rotate(
		transform, glm::radians(steering_degrees), glm::vec3(0.0f, 1.0f, 0.0f));
	transform = glm::rotate(
		transform, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	return glm::rotate(
		transform, glm::radians(wheel_rotation_degrees),
		glm::vec3(0.0f, 1.0f, 0.0f));
}
