#include "building/service/BuildingObjectSemanticProfileHashService.h"

#include <cstdint>
#include <string>

namespace {

class ProfileHashAccumulator {
public:
	void appendByte(std::uint8_t byte)
	{
		hash_ ^= byte;
		hash_ *= 1099511628211ULL;
	}
	void appendUnsigned(std::uint64_t value)
	{
		for (int byte_index = 0; byte_index < 8; ++byte_index) {
			appendByte(static_cast<std::uint8_t>((value >> (byte_index * 8)) & 0xffu));
		}
	}
	void appendString(const std::string &value)
	{
		appendUnsigned(value.size());
		for (unsigned char character : value) appendByte(character);
	}
	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

} // namespace

std::uint64_t BuildingObjectSemanticProfileHashService::calculate(
	const BuildingObjectSemanticProfile &profile) const
{
	ProfileHashAccumulator hash;
	hash.appendString("SMB-OMv2.1-BuildingObjectSemanticProfile-v1");
	hash.appendString(profile.objectId().value());
	hash.appendString(profile.conceptId().value());
	hash.appendUnsigned(profile.roleIds().size());
	for (const BuildingRoleId &role_id : profile.roleIds()) hash.appendString(role_id.value());
	hash.appendUnsigned(profile.functionAllocationIds().size());
	for (const BuildingFunctionAllocationId &allocation_id :
	     profile.functionAllocationIds()) {
		hash.appendString(allocation_id.value());
	}
	hash.appendUnsigned(profile.servicePortIds().size());
	for (const BuildingServicePortId &port_id : profile.servicePortIds()) {
		hash.appendString(port_id.value());
	}
	hash.appendUnsigned(profile.requirementIds().size());
	for (const BuildingRequirementId &requirement_id : profile.requirementIds()) {
		hash.appendString(requirement_id.value());
	}
	hash.appendUnsigned(profile.scenarioIds().size());
	for (const BuildingScenarioId &scenario_id : profile.scenarioIds()) {
		hash.appendString(scenario_id.value());
	}
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().geometry()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().spatialBoundary()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().spatialInterfaces()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().functionalRoles()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().buildingFunctions()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().servicePorts()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().requirements()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().operationalState()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().conditionState()));
	hash.appendUnsigned(static_cast<std::uint64_t>(profile.applicability().scenarioParticipation()));
	hash.appendUnsigned(profile.evidenceReferences().size());
	for (const BuildingEvidenceReference &reference : profile.evidenceReferences()) {
		hash.appendString(reference.evidenceId().value());
	}
	hash.appendUnsigned(profile.profileRevision());
	return hash.value();
}

BuildingObjectSemanticProfile BuildingObjectSemanticProfileHashService::attachCalculatedHash(
	const BuildingObjectSemanticProfile &profile) const
{
	return profile.withProfileHash(calculate(profile));
}
