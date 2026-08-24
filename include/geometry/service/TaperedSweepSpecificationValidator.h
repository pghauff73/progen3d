#pragma once

#include "geometry/model/ExtrudeProfileCapPolicy.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/TaperedSweepShapeSpecification.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

struct TaperedSweepShapeSpecificationCandidate
{
	std::vector<glm::vec3> path_points;
	std::vector<float> radii;
	glm::vec3 up_hint{0.0f, 0.0f, 1.0f};
	int circumferential_segments = 12;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class TaperedSweepSpecificationValidator
{
public:
	explicit TaperedSweepSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const TaperedSweepShapeSpecification> validate(
		TaperedSweepShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};

