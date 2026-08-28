#pragma once

#include <glm/glm.hpp>

class SpatialPoseUncertainty {
public:
	SpatialPoseUncertainty() = default;
	SpatialPoseUncertainty(glm::vec3 translation_standard_deviation,
	                       glm::vec3 rotation_standard_deviation_degrees)
		: translation_standard_deviation_(translation_standard_deviation),
		  rotation_standard_deviation_degrees_(rotation_standard_deviation_degrees) {}

	const glm::vec3 &translationStandardDeviation() const
	{
		return translation_standard_deviation_;
	}

	const glm::vec3 &rotationStandardDeviationDegrees() const
	{
		return rotation_standard_deviation_degrees_;
	}

	bool isZero() const
	{
		return translation_standard_deviation_ == glm::vec3(0.0f) &&
		       rotation_standard_deviation_degrees_ == glm::vec3(0.0f);
	}

private:
	glm::vec3 translation_standard_deviation_{0.0f};
	glm::vec3 rotation_standard_deviation_degrees_{0.0f};
};
