#pragma once

#include "geometry/model/FormedPanelShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/service/LoftSpecificationValidator.h"

#include <memory>
#include <string>

struct FormedPanelShapeSpecificationCandidate
{
	std::vector<LoftSectionCandidate> sections;
	float thickness = 0.001f;
	ShellOffsetSide offset_side = ShellOffsetSide::Both;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class FormedPanelSpecificationValidator
{
public:
	explicit FormedPanelSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const FormedPanelShapeSpecification> validate(
		FormedPanelShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
