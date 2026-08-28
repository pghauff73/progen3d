#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vegetation/model/PlantFruitSpecification.h"
#include "vegetation/model/PlantOrganGeometrySource.h"

#include <optional>
#include <string>

class FruitShellGeometryFactory
{
public:
	explicit FruitShellGeometryFactory(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::optional<PlantOrganGeometrySource> create(
		const PlantFruitSpecification &specification,
		GeometryDetailLevel detail_level,
		std::string *diagnostic = nullptr) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
