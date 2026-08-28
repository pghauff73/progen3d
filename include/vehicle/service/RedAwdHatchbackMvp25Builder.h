#pragma once

#include "geometry/model/GeometryDetailLevel.h"
#include "vehicle/model/ModernVehicleAssembly.h"

#include <optional>
#include <string>

class RedAwdHatchbackMvp25Builder
{
public:
	std::optional<ModernVehicleAssembly> build(
		GeometryDetailLevel detail_level,
		std::string *diagnostic = nullptr) const;

private:
	VehicleMvp25Architecture createArchitecture() const;
};
