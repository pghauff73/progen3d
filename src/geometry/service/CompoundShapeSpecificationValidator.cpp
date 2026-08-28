#include "geometry/service/CompoundShapeSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

bool finite(const glm::mat4 &matrix)
{
	for (int column = 0; column < 4; ++column) {
		for (int row = 0; row < 4; ++row) {
			if (!std::isfinite(matrix[column][row])) return false;
		}
	}
	return true;
}

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

}

std::shared_ptr<const CompoundShapeSpecification>
CompoundShapeSpecificationValidator::validate(
	CompoundShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (candidate.parts.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "CompoundShape requires at least one child part.";
		}
		return {};
	}
	if (candidate.parts.size() > complexity_limits_.maximumInstanceArrayCount()) {
		if (diagnostic != nullptr) {
			*diagnostic = "CompoundShape exceeds the configured child part limit.";
		}
		return {};
	}

	std::vector<CompoundShapePartSpecification> parts;
	parts.reserve(candidate.parts.size());
	std::ostringstream key;
	key << "CompoundShape:v1";
	for (std::size_t part_index = 0;
	     part_index < candidate.parts.size();
	     ++part_index) {
		CompoundShapePartCandidate &part = candidate.parts[part_index];
		if (part.purpose.empty()) {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape child part purpose cannot be empty.";
			}
			return {};
		}
		if (!part.shape) {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape child part '" + part.purpose +
				              "' requires a shape specification.";
			}
			return {};
		}
		if (!finite(part.local_transform)) {
			if (diagnostic != nullptr) {
				*diagnostic = "CompoundShape child part '" + part.purpose +
				              "' has a non-finite local transform.";
			}
			return {};
		}
		key << ":part=" << part_index << "," << part.purpose << ","
		    << part.shape->key().canonicalValue() << ",matrix=";
		for (int column = 0; column < 4; ++column) {
			for (int row = 0; row < 4; ++row) {
				key << float_bits(part.local_transform[column][row]) << ",";
			}
		}
		parts.emplace_back(
			std::move(part.purpose),
			std::move(part.shape),
			part.local_transform);
	}

	return std::make_shared<const CompoundShapeSpecification>(
		std::move(parts),
		ShapeSpecificationKey(key.str()),
		"CompoundShape(parts(" + std::to_string(candidate.parts.size()) + "))",
		candidate.detail_level);
}
