#include "lighting/service/ShadowAssignmentService.h"

ShadowAssignmentPlan ShadowAssignmentService::assign(
	const PreviewLightCollection &lights) const
{
	ShadowAssignmentPlan plan;
	bool primary_backend_slot_assigned = false;
	for (const SceneLight &light : lights.lights()) {
		if (!light.enabled() || !light.shadow().enabled()) continue;
		++plan.requested_shadow_count;
		ShadowAssignmentRecord record;
		record.light_id = light.id();
		record.light_type = light.type();
		if (light.type() == SceneLightType::Directional &&
		    plan.directional_assignment_count < directional_shadow_budget) {
			record.storage = ShadowMapStorage::DirectionalCascadeArray;
			record.storage_index = static_cast<std::int32_t>(plan.directional_assignment_count++);
		}
		else if (light.type() == SceneLightType::Spot &&
		         plan.spot_assignment_count < spot_shadow_budget) {
			record.storage = ShadowMapStorage::SpotDepthTextureArray;
			record.storage_index = static_cast<std::int32_t>(plan.spot_assignment_count++);
		}
		else if (light.type() == SceneLightType::Point &&
		         plan.point_assignment_count < point_shadow_budget) {
			record.storage = ShadowMapStorage::PointDepthCubeArray;
			record.storage_index = static_cast<std::int32_t>(plan.point_assignment_count++);
		}
		if (record.storage == ShadowMapStorage::Unallocated) {
			++plan.deferred_shadow_count;
		}
		else {
			++plan.allocated_shadow_count;
			if (!primary_backend_slot_assigned) {
				record.storage = ShadowMapStorage::PrimaryDepthTexture;
				record.storage_index = 0;
				record.rendered_by_current_backend = true;
				primary_backend_slot_assigned = true;
			}
		}
		plan.assignments.push_back(std::move(record));
	}
	return plan;
}
