#pragma once

#include "lighting/model/PreviewLightCollection.h"
#include "lighting/model/ShadowAssignmentPlan.h"

class ShadowAssignmentService
{
public:
	static constexpr std::size_t directional_shadow_budget = 1;
	static constexpr std::size_t spot_shadow_budget = 4;
	static constexpr std::size_t point_shadow_budget = 2;

	ShadowAssignmentPlan assign(const PreviewLightCollection &lights) const;
};
