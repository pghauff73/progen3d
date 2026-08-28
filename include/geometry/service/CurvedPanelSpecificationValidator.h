#pragma once

#include "geometry/model/CurvedPanelShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>

struct CurvedPanelShapeSpecificationCandidate
{
	float width = 0.0f;
	float height = 0.0f;
	float horizontal_curvature = 0.0f;
	float vertical_curvature = 0.0f;
	float thickness = 0.0f;
	int horizontal_segments = 16;
	int vertical_segments = 8;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class CurvedPanelSpecificationValidator
{
public:
	explicit CurvedPanelSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const CurvedPanelShapeSpecification> validate(
		CurvedPanelShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
