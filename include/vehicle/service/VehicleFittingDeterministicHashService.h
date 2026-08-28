#pragma once

#include "vehicle/model/VehicleFittingArchitecture.h"

#include <cstdint>

class VehicleFittingDeterministicHashService
{
public:
	std::uint64_t calculate(
		const VehicleFittingArchitecture &architecture) const;
};
