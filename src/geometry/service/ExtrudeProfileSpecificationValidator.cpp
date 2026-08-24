#include "geometry/service/ExtrudeProfileSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

float canonical_float(float value)
{
	return value == 0.0f ? 0.0f : value;
}

std::string float_bits(float value)
{
	value = canonical_float(value);
	static_assert(sizeof(float) == sizeof(std::uint32_t),
	              "Shape keys require 32-bit float storage.");
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

void append_loop_key(std::ostringstream *key,
	                 const ProfileLoop2D &loop)
{
	*key << "[";
	for (const glm::vec2 &point : loop.points()) {
		*key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
	*key << "]";
}

void append_loop_text(std::ostringstream *text,
	                  const ProfileLoop2D &loop)
{
	*text << "polygon(";
	for (std::size_t point_index = 0;
	     point_index < loop.points().size();
	     ++point_index) {
		if (point_index != 0) *text << " ";
		const glm::vec2 &point = loop.points()[point_index];
		*text << point.x << " " << point.y;
	}
	*text << ")";
}

}

std::shared_ptr<const ExtrudeProfileShapeSpecification>
ExtrudeProfileSpecificationValidator::validate(
	ExtrudeProfileShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.depth)) {
		if (diagnostic != nullptr) {
			*diagnostic = "ExtrudeProfile depth must be finite.";
		}
		return {};
	}
	if (candidate.depth <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "ExtrudeProfile depth must be greater than zero.";
		}
		return {};
	}

	std::shared_ptr<const Profile2D> profile =
		Profile2DValidator(complexity_limits_).validate(
			std::move(candidate.profile), diagnostic);
	if (!profile) return {};

	std::ostringstream key;
	key << "ExtrudeProfile:v1:depth=" << float_bits(candidate.depth)
	    << ":cap=" << candidate.cap_policy.canonicalText() << ":outer=";
	append_loop_key(&key, profile->outerLoop());
	for (const ProfileLoop2D &inner_loop : profile->innerLoops()) {
		key << ":hole=";
		append_loop_key(&key, inner_loop);
	}

	std::ostringstream canonical_text;
	canonical_text << "Extrude(profile(";
	append_loop_text(&canonical_text, profile->outerLoop());
	for (const ProfileLoop2D &inner_loop : profile->innerLoops()) {
		canonical_text << " hole(";
		append_loop_text(&canonical_text, inner_loop);
		canonical_text << ")";
	}
	canonical_text << ") depth(" << candidate.depth << ") cap("
	               << candidate.cap_policy.canonicalText() << "))";

	return std::make_shared<const ExtrudeProfileShapeSpecification>(
		*profile,
		candidate.depth,
		candidate.cap_policy,
		ShapeSpecificationKey(key.str()),
		canonical_text.str(),
		candidate.detail_level);
}
