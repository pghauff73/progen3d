#pragma once

#include <glm/glm.hpp>

class VehicleReferenceFrameTransformationService
{
public:
	glm::dvec3 transformPoint(
		const glm::dvec3 &source_point,
		const glm::dmat4 &source_to_target_matrix) const;

	glm::dmat4 transformRigidBodyPose(
		const glm::dmat4 &source_pose,
		const glm::dmat4 &source_to_target_matrix) const;
};
