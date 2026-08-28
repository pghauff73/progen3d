#pragma once

#include "vehicle/mcsmv2/model/ModernCarSemanticVariant.h"
#include "vehicle/mcsmv2/model/VehicleSemanticSurface.h"

#include <cstddef>
#include <vector>

class VehicleSemanticSurfaceGenerationService
{
public:
	VehicleSemanticSurface generate(
		const ModernCarSemanticVariant &variant,
		std::size_t longitudinal_section_count = 72u,
		std::size_t half_section_sample_count = 34u) const;

	std::vector<glm::dvec3> createSourceSectionRing(
		const ModernCarSemanticVariant &variant,
		double source_x,
		std::size_t half_section_sample_count = 34u) const;
};
