#pragma once

#include "lighting/model/LightId.h"
#include "lighting/model/SceneLightType.h"

#include <cstddef>
#include <cstdint>
#include <vector>

enum class ShadowMapStorage
{
	PrimaryDepthTexture,
	DirectionalCascadeArray,
	SpotDepthTextureArray,
	PointDepthCubeArray,
	Unallocated
};

class ShadowAssignmentRecord
{
public:
	LightId light_id;
	SceneLightType light_type = SceneLightType::Point;
	ShadowMapStorage storage = ShadowMapStorage::Unallocated;
	std::int32_t storage_index = -1;
	bool rendered_by_current_backend = false;
};

class ShadowAssignmentPlan
{
public:
	const ShadowAssignmentRecord *find(const LightId &light_id) const
	{
		for (const ShadowAssignmentRecord &record : assignments) {
			if (record.light_id == light_id) return &record;
		}
		return nullptr;
	}

	std::vector<ShadowAssignmentRecord> assignments;
	std::size_t requested_shadow_count = 0;
	std::size_t allocated_shadow_count = 0;
	std::size_t deferred_shadow_count = 0;
	std::size_t directional_assignment_count = 0;
	std::size_t spot_assignment_count = 0;
	std::size_t point_assignment_count = 0;
};
