#pragma once

#include "geometry/model/CompoundShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"

#include <memory>
#include <string>
#include <vector>

struct CompoundShapePartCandidate
{
	std::string purpose;
	std::shared_ptr<const ShapeSpecification> shape;
	glm::mat4 local_transform{1.0f};
};

struct CompoundShapeSpecificationCandidate
{
	std::vector<CompoundShapePartCandidate> parts;
	GeometryDetailLevel detail_level = GeometryDetailLevel::FastenersAndSeals;
};

class CompoundShapeSpecificationValidator
{
public:
	explicit CompoundShapeSpecificationValidator(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const CompoundShapeSpecification> validate(
		CompoundShapeSpecificationCandidate candidate,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
