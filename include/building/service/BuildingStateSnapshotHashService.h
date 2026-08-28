#pragma once

#include "building/model/BuildingStateSnapshot.h"

#include <cstdint>

class BuildingStateSnapshotHashService {
public:
	std::uint64_t calculate(const BuildingStateSnapshot &snapshot) const;
};
