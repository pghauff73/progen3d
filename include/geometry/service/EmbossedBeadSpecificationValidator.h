#pragma once

#include "geometry/model/EmbossedBeadShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>

struct EmbossedBeadShapeSpecificationCandidate
{
	std::vector<glm::vec3> path_points;
	glm::vec3 up_hint{0.0f, 1.0f, 0.0f};
	float width = 0.03f;
	float depth = 0.006f;
	float shoulder_radius = 0.002f;
	EmbossedBeadSide side = EmbossedBeadSide::Positive;
	EmbossedBeadEndStyle end_style = EmbossedBeadEndStyle::Closed;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class EmbossedBeadSpecificationValidator
{
public:
	explicit EmbossedBeadSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const EmbossedBeadShapeSpecification> validate(
		EmbossedBeadShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
