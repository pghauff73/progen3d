#pragma once

#include "building/model/BuildingObjectSemanticProfile.h"

#include <cstdint>

class BuildingObjectSemanticProfileHashService {
public:
	std::uint64_t calculate(const BuildingObjectSemanticProfile &profile) const;
	BuildingObjectSemanticProfile attachCalculatedHash(
		const BuildingObjectSemanticProfile &profile) const;
};
