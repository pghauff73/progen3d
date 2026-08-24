#include "lighting/service/LightingEvidenceHashService.h"

#include "lighting/model/GpuLightRecord.h"
#include "lighting/service/ShadowAssignmentService.h"

#include <algorithm>
#include <cstring>
#include <utility>

namespace {

class LightingHashAccumulator
{
public:
	void appendByte(unsigned char value)
	{
		hash_ ^= static_cast<std::uint64_t>(value);
		hash_ *= 1099511628211ull;
	}

	void appendString(const std::string &value)
	{
		for (unsigned char character : value) appendByte(character);
		appendByte(0xffu);
	}

	template <typename Value>
	void appendValue(const Value &value)
	{
		const unsigned char *bytes = reinterpret_cast<const unsigned char *>(&value);
		for (std::size_t index = 0; index < sizeof(Value); ++index) appendByte(bytes[index]);
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 1469598103934665603ull;
};

}

LightingEvidenceRecord LightingEvidenceHashService::calculate(
	const PreviewLightCollection &lights,
	std::size_t fixture_count,
	std::size_t circuit_count,
	std::string preset_name) const
{
	LightingEvidenceRecord evidence;
	evidence.scene_light_count = lights.lights().size();
	evidence.fixture_count = fixture_count;
	evidence.circuit_count = circuit_count;
	evidence.preset_name = std::move(preset_name);
	const ShadowAssignmentPlan shadow_plan = ShadowAssignmentService().assign(lights);
	evidence.shadow_budget_assignment_count = shadow_plan.allocated_shadow_count;
	evidence.deferred_shadow_count = shadow_plan.deferred_shadow_count;
	LightingHashAccumulator hash;
	for (const SceneLight &light : lights.lights()) {
		hash.appendString(light.id().value());
		hash.appendString(light.name());
		hash.appendValue(light.type());
		hash.appendValue(light.position());
		hash.appendValue(light.orientation());
		hash.appendValue(light.enabled());
		hash.appendValue(light.visible());
		hash.appendValue(light.emission().linearRgb());
		hash.appendValue(light.emission().colorTemperatureKelvin());
		hash.appendValue(light.emission().usesColorTemperature());
		hash.appendValue(light.emission().intensity());
		hash.appendValue(light.emission().intensityUnit());
		hash.appendValue(light.exposureCompensation());
		hash.appendValue(light.controlExposureCompensation());
		hash.appendValue(light.shadow().enabled());
		hash.appendValue(light.shadow().resolution());
		if (light.enabled()) ++evidence.enabled_light_count;
		if (light.visible()) ++evidence.visible_light_count;
		if (light.enabled() && light.shadow().enabled()) ++evidence.shadowed_light_count;
	}
	evidence.gpu_light_record_count = std::min(
		evidence.enabled_light_count, PreviewLightCollection::maximum_visible_lights);
	evidence.forward_plus_recommended =
		evidence.gpu_light_record_count > evidence.forward_plus_activation_threshold;
	evidence.lighting_upload_bytes = evidence.gpu_light_record_count * sizeof(GpuLightRecord);
	hash.appendString(evidence.preset_name);
	hash.appendValue(evidence.fixture_count);
	hash.appendValue(evidence.circuit_count);
	hash.appendValue(evidence.shadow_budget_assignment_count);
	hash.appendValue(evidence.deferred_shadow_count);
	hash.appendValue(evidence.forward_plus_recommended);
	evidence.state_hash = hash.value();
	return evidence;
}
