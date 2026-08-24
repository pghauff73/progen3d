#pragma once

#include "vehicle/model/VehicleMvp26Architecture.h"

#include <cstdint>

class VehicleMvp26DeterministicHashService
{
public:
	std::uint64_t calculate(const VehicleMvp26Architecture &architecture) const;
};
