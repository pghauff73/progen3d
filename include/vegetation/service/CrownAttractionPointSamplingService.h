#pragma once

#include "vegetation/model/CrownAttractionPointSamplingResult.h"
#include "vegetation/model/CrownVolumeSpecification.h"

#include <cstddef>
#include <cstdint>

class CrownAttractionPointSamplingService
{
public:
	CrownAttractionPointSamplingResult sample(
		const CrownVolumeSpecification &volume,
		std::size_t point_count,
		std::uint64_t deterministic_seed) const;
};
