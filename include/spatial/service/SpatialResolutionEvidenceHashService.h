#pragma once

#include "spatial/model/SpatialResolutionRecord.h"

#include <cstdint>

class SpatialResolutionEvidenceHashService {
public:
	std::uint64_t calculate(const SpatialResolutionRecord &record) const;
	SpatialResolutionRecord attachCalculatedHash(
		const SpatialResolutionRecord &record) const;
};
