#pragma once

#include "vehicle/model/VehicleMvp25Architecture.h"

#include <cstdint>

class VehicleMvp25DeterministicHashService
{
public:
	std::uint64_t calculate(const VehicleMvp25Architecture &architecture) const;
	std::uint64_t calculateResidualHash(
		const VehicleFitResidualReport &report) const;
};
