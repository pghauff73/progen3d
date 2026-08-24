#pragma once

#include "geometry/model/RigidTransformValidationReport.h"

#include <glm/glm.hpp>

class RigidTransformValidationService
{
public:
	RigidTransformValidationReport validate(
		const glm::dmat4 &transform,
		double tolerance = 1.0e-9) const;
};
