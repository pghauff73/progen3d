#include "vehicle/service/VehicleReferenceFrameTransformationService.h"

#include <glm/gtc/matrix_inverse.hpp>

glm::dvec3 VehicleReferenceFrameTransformationService::transformPoint(
	const glm::dvec3 &source_point,
	const glm::dmat4 &source_to_target_matrix) const
{
	return glm::dvec3(source_to_target_matrix * glm::dvec4(source_point, 1.0));
}

glm::dmat4 VehicleReferenceFrameTransformationService::transformRigidBodyPose(
	const glm::dmat4 &source_pose,
	const glm::dmat4 &source_to_target_matrix) const
{
	return source_to_target_matrix * source_pose *
		glm::inverse(source_to_target_matrix);
}
