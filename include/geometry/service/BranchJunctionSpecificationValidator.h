#pragma once

#include "geometry/model/BranchJunctionShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>
#include <vector>

struct BranchJunctionShapeSpecificationCandidate
{
	float core_radius = 0.0f;
	float bulge_scale = 1.15f;
	std::vector<BranchJunctionArmSpecification> arms;
	int circumferential_segments = 12;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class BranchJunctionSpecificationValidator
{
public:
	explicit BranchJunctionSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const BranchJunctionShapeSpecification> validate(
		BranchJunctionShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
