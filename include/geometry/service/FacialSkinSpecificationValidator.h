#pragma once

#include "geometry/model/FacialSkinShapeSpecification.h"

#include <memory>
#include <string>

struct FacialSkinShapeSpecificationCandidate
{
	glm::vec3 size{0.16f, 0.21f, 0.045f};
	int horizontal_segments = 48;
	int vertical_segments = 64;
	FacialExpressionControls controls;
};

class FacialSkinSpecificationValidator
{
public:
	std::shared_ptr<const FacialSkinShapeSpecification> validate(
		const FacialSkinShapeSpecificationCandidate &candidate,
		std::string *diagnostic) const;
};
