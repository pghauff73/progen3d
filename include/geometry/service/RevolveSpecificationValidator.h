#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/RevolveShapeSpecification.h"
#include "geometry/service/Profile2DValidator.h"

#include <memory>
#include <string>

struct RevolveShapeSpecificationCandidate
{
	Profile2DCandidate radial_profile;
	float start_degrees = 0.0f;
	float sweep_degrees = 360.0f;
	int angular_segments = 32;
	ExtrudeProfileCapPolicy angular_cap_policy =
		ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class RevolveSpecificationValidator
{
public:
	explicit RevolveSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const RevolveShapeSpecification> validate(
		RevolveShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
