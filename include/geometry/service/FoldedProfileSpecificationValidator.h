#pragma once

#include "geometry/model/FoldedProfileShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>

struct FoldedProfileShapeSpecificationCandidate
{
	std::vector<glm::vec2> fold_path;
	float thickness = 0.001f;
	float extrusion_depth = 1.0f;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class FoldedProfileSpecificationValidator
{
public:
	explicit FoldedProfileSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const FoldedProfileShapeSpecification> validate(
		FoldedProfileShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
