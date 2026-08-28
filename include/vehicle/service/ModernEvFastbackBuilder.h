#pragma once

#include "geometry/model/GeometryComplexityLimits.h"
#include "geometry/model/GeometryDetailLevel.h"
#include "vehicle/model/ModernVehicleAssembly.h"

#include <optional>
#include <string>

class ModernEvFastbackBuilder
{
public:
	explicit ModernEvFastbackBuilder(
		GeometryComplexityLimits complexity_limits = GeometryComplexityLimits())
		: complexity_limits_(complexity_limits)
	{
	}

	std::optional<ModernVehicleAssembly> buildAcceptanceVehicle(
		GeometryDetailLevel detail_level,
		std::string *diagnostic) const;

private:
	GeometryComplexityLimits complexity_limits_;
};
