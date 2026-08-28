#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/service/Profile2DValidator.h"

#include <memory>
#include <string>

struct ExtrudeProfileShapeSpecificationCandidate
{
	Profile2DCandidate profile;
	float depth = 1.0f;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class ExtrudeProfileSpecificationValidator
{
public:
	explicit ExtrudeProfileSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const ExtrudeProfileShapeSpecification> validate(
		ExtrudeProfileShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
