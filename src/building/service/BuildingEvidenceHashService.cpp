#include "building/service/BuildingEvidenceHashService.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace {

class BuildingHashAccumulator {
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

	void appendDouble(double value)
	{
		if (value == 0.0) value = 0.0;
		std::uint64_t bits = 0;
		static_assert(sizeof(bits) == sizeof(value), "Building hashing requires 64-bit doubles.");
		std::memcpy(&bits, &value, sizeof(bits));
		appendUnsigned(bits);
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

std::uint64_t BuildingEvidenceHashService::calculate(
	const BuildingEvidenceRecord &record) const
{
	BuildingHashAccumulator hash;
	hash.appendString("SMB-OMv2.1-BuildingEvidenceRecord-v1");
	hash.appendString(record.evidenceId().value());
	hash.appendUnsigned(static_cast<std::uint64_t>(record.kind()));
	hash.appendUnsigned(static_cast<std::uint64_t>(record.authority()));
	hash.appendString(record.sourceIdentifier());
	hash.appendString(record.summary());
	return hash.value();
}

std::uint64_t BuildingEvidenceHashService::calculate(
	const BuildingRequirementEvaluationRecord &record) const
{
	BuildingHashAccumulator hash;
	hash.appendString("SMB-OMv2.1-BuildingRequirementEvaluationRecord-v1");
	hash.appendString(record.requirementId().value());
	hash.appendUnsigned(static_cast<std::uint64_t>(record.status()));
	hash.appendString(record.measuredValue().description());
	hash.appendUnsigned(record.measuredValue().numericValue().has_value() ? 1u : 0u);
	if (record.measuredValue().numericValue().has_value()) {
		hash.appendDouble(*record.measuredValue().numericValue());
	}
	hash.appendString(record.requiredValue().description());
	hash.appendUnsigned(record.requiredValue().numericValue().has_value() ? 1u : 0u);
	if (record.requiredValue().numericValue().has_value()) {
		hash.appendDouble(*record.requiredValue().numericValue());
	}
	hash.appendDouble(record.tolerance());
	hash.appendUnsigned(record.evidenceReferences().size());
	for (const BuildingEvidenceReference &reference : record.evidenceReferences()) {
		hash.appendString(reference.evidenceId().value());
	}
	hash.appendUnsigned(record.diagnostics().size());
	for (const std::string &diagnostic : record.diagnostics()) hash.appendString(diagnostic);
	return hash.value();
}

BuildingEvidenceRecord BuildingEvidenceHashService::attachCalculatedHash(
	const BuildingEvidenceRecord &record) const
{
	return record.withEvidenceHash(calculate(record));
}

BuildingRequirementEvaluationRecord BuildingEvidenceHashService::attachCalculatedHash(
	const BuildingRequirementEvaluationRecord &record) const
{
	return record.withEvidenceHash(calculate(record));
}
