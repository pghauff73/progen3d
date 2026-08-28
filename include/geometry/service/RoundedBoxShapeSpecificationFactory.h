#pragma once

#include "geometry/model/ExtrudeProfileShapeSpecification.h"
#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "geometry/model/RoundedBoxSpecification.h"

#include <memory>
#include <string>

class RoundedBoxShapeSpecificationFactory
{
public:
	explicit RoundedBoxShapeSpecificationFactory(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::shared_ptr<const ExtrudeProfileShapeSpecification> create(
		const RoundedBoxSpecification &specification,
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
