#pragma once

#include "geometry/model/AxisAlignedBounds.h"
#include "vegetation/model/CrownVolumeSpecification.h"

#include <glm/glm.hpp>

#include <string>

class CrownVolumeContainmentService
{
public:
	bool validate(
		const CrownVolumeSpecification &volume,
		std::string *diagnostic = nullptr) const;
	bool contains(
		const CrownVolumeSpecification &volume,
		const glm::vec3 &point) const;
	AxisAlignedBounds bounds(const CrownVolumeSpecification &volume) const;
};
