#pragma once

#include "lighting/model/LightingEvidenceRecord.h"
#include "lighting/model/PreviewLightCollection.h"

#include <cstddef>

class LightingEvidenceHashService
{
public:
	LightingEvidenceRecord calculate(const PreviewLightCollection &lights,
	                                 std::size_t fixture_count,
	                                 std::size_t circuit_count,
	                                 std::string preset_name) const;
};
