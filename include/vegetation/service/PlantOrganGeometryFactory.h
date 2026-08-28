#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vegetation/model/PlantOrganGeometrySource.h"
#include "vegetation/model/PlantSpeciesSpecification.h"
#include "vegetation/model/VegetationOrganType.h"

#include <optional>
#include <string>

class PlantOrganGeometryFactory
{
public:
	explicit PlantOrganGeometryFactory(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::optional<PlantOrganGeometrySource> create(
		const PlantSpeciesSpecification &species,
		VegetationOrganType organ_type,
		GeometryDetailLevel detail_level,
		std::string *diagnostic = nullptr) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
