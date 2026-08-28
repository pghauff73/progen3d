#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class LightingEvidenceRecord
{
public:
	std::size_t scene_light_count = 0;
	std::size_t enabled_light_count = 0;
	std::size_t visible_light_count = 0;
	std::size_t shadowed_light_count = 0;
	std::size_t shadow_budget_assignment_count = 0;
	std::size_t deferred_shadow_count = 0;
	std::size_t gpu_light_record_count = 0;
	std::size_t lighting_upload_bytes = 0;
	std::size_t fixture_count = 0;
	std::size_t circuit_count = 0;
	std::size_t forward_plus_activation_threshold = 24;
	bool forward_plus_recommended = false;
	std::uint64_t state_hash = 0;
	std::string preset_name;
};
