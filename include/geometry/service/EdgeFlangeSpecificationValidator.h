#pragma once

#include "geometry/model/EdgeFlangeShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>

struct EdgeFlangeShapeSpecificationCandidate
{
	std::vector<glm::vec3> boundary_path;
	glm::vec3 up_hint{0.0f, 1.0f, 0.0f};
	float width = 0.02f;
	float thickness = 0.001f;
	float angle_degrees = 90.0f;
	float bend_radius = 0.002f;
	EdgeFlangeSide side = EdgeFlangeSide::Positive;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class EdgeFlangeSpecificationValidator
{
public:
	explicit EdgeFlangeSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const EdgeFlangeShapeSpecification> validate(
		EdgeFlangeShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
