#include "spatial/service/SpatialResolutionEvidenceHashService.h"

#include <cstdint>
#include <cstring>
#include <string>

namespace {

class EvidenceHashAccumulator {
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

	void appendSigned(std::int64_t value)
	{
		appendUnsigned(static_cast<std::uint64_t>(value));
	}

	void appendFloat(float value)
	{
		if (value == 0.0f) value = 0.0f;
		std::uint32_t bits = 0;
		static_assert(sizeof(bits) == sizeof(value), "Evidence hashing requires 32-bit floats.");
		std::memcpy(&bits, &value, sizeof(bits));
		appendUnsigned(bits);
	}

	void appendString(const std::string &value)
	{
		appendUnsigned(value.size());
		for (unsigned char character : value) appendByte(character);
	}

	void appendMatrix(const glm::mat4 &matrix)
	{
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) appendFloat(matrix[column][row]);
		}
	}

	void appendVector(const glm::vec3 &vector)
	{
		appendFloat(vector.x);
		appendFloat(vector.y);
		appendFloat(vector.z);
	}

	std::uint64_t value() const { return hash_; }

private:
	std::uint64_t hash_ = 14695981039346656037ULL;
};

} // namespace

std::uint64_t SpatialResolutionEvidenceHashService::calculate(
	const SpatialResolutionRecord &record) const
{
	EvidenceHashAccumulator hash;
	hash.appendString("SMB-OMv2-SpatialResolutionRecord-v2");
	hash.appendString(record.constraintId().value());
	hash.appendString(record.sourceObjectId().value());
	hash.appendString(record.targetObjectId().value());
	hash.appendMatrix(record.initialWorldTransform());
	hash.appendMatrix(record.finalWorldTransform());
	hash.appendUnsigned(static_cast<std::uint64_t>(record.algorithm()));
	hash.appendSigned(record.broadPhaseStepCount());
	hash.appendSigned(record.refinementIterationCount());
	hash.appendSigned(record.collisionQueryCount());
	hash.appendFloat(record.tolerance());
	hash.appendVector(record.contactPoint());
	hash.appendVector(record.contactNormal());
	hash.appendFloat(record.resultingClearance());
	hash.appendFloat(record.residualError());
	hash.appendUnsigned(static_cast<std::uint64_t>(record.status()));
	hash.appendUnsigned(record.warnings().size());
	for (const std::string &warning : record.warnings()) hash.appendString(warning);
	return hash.value();
}

SpatialResolutionRecord SpatialResolutionEvidenceHashService::attachCalculatedHash(
	const SpatialResolutionRecord &record) const
{
	return record.withEvidenceHash(calculate(record));
}
