#include "building/service/BuildingStateSnapshotHashService.h"

#include <string>

namespace {

class BuildingStateSnapshotHashAccumulator {
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

std::uint64_t BuildingStateSnapshotHashService::calculate(
	const BuildingStateSnapshot &snapshot) const
{
	BuildingStateSnapshotHashAccumulator hash;
	hash.appendString("SMB-OMv2.1-BuildingStateSnapshot-v1");
	hash.appendString(snapshot.scenarioId().value());
	hash.appendUnsigned(snapshot.objectStates().size());
	for (const BuildingObjectStateRecord &state : snapshot.objectStates()) {
		hash.appendString(state.objectId().value());
		hash.appendUnsigned(static_cast<std::uint64_t>(state.placementState()));
		hash.appendUnsigned(static_cast<std::uint64_t>(state.operationalState()));
		hash.appendUnsigned(static_cast<std::uint64_t>(state.conditionState()));
		hash.appendUnsigned(static_cast<std::uint64_t>(state.complianceState()));
		hash.appendUnsigned(static_cast<std::uint64_t>(state.serviceAvailabilityState()));
	}
	return hash.value();
}
