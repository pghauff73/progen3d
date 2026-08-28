#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/HostedOpeningShapeSpecification.h"
#include "geometry/service/Profile2DValidator.h"

#include <memory>
#include <string>

struct HostedOpeningShapeSpecificationCandidate
{
	std::vector<glm::vec2> host_boundary;
	std::vector<std::vector<glm::vec2>> opening_boundaries;
	float depth = 0.001f;
	ExtrudeProfileCapPolicy cap_policy = ExtrudeProfileCapPolicy::createAll();
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class HostedOpeningSpecificationValidator
{
public:
	explicit HostedOpeningSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const HostedOpeningShapeSpecification> validate(
		HostedOpeningShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
