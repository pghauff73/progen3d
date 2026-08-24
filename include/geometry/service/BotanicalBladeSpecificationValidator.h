#pragma once

#include "geometry/model/BotanicalBladeShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>

enum class BotanicalBladeKind
{
	Leaf,
	Petal
};

struct BotanicalBladeShapeSpecificationCandidate
{
	BotanicalBladeKind kind = BotanicalBladeKind::Leaf;
	BotanicalBladeProfile profile = BotanicalBladeProfile::Elliptic;
	float length = 0.10f;
	float maximum_width = 0.05f;
	float longitudinal_curvature = 0.0f;
	float camber = 0.0f;
	float twist_degrees = 0.0f;
	float thickness = 0.001f;
	float width_power = 1.0f;
	int longitudinal_segments = 12;
	int lateral_segments = 4;
	GeometryDetailLevel detail_level = GeometryDetailLevel::ConstructionDetail;
};

class BotanicalBladeSpecificationValidator
{
public:
	explicit BotanicalBladeSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const BotanicalBladeShapeSpecification> validate(
		BotanicalBladeShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};

