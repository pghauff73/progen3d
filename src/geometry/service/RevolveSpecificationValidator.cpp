#include "geometry/service/RevolveSpecificationValidator.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <utility>

namespace {

std::string float_bits(float value)
{
	if (value == 0.0f) value = 0.0f;
	std::uint32_t bits = 0;
	std::memcpy(&bits, &value, sizeof(bits));
	std::ostringstream text;
	text << std::hex << std::setw(8) << std::setfill('0') << bits;
	return text.str();
}

bool validate_nonnegative_radius(
	const ProfileLoop2D &loop,
	const std::string &loop_name,
	std::string *diagnostic)
{
	for (const glm::vec2 &point : loop.points()) {
		if (point.x < 0.0f) {
			if (diagnostic != nullptr) {
				*diagnostic = loop_name +
				              " must use nonnegative radial coordinates.";
			}
			return false;
		}
	}
	return true;
}

}

std::shared_ptr<const RevolveShapeSpecification>
RevolveSpecificationValidator::validate(
	RevolveShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.start_degrees) ||
	    !std::isfinite(candidate.sweep_degrees) ||
	    candidate.sweep_degrees <= 0.0f ||
	    candidate.sweep_degrees > 360.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "Revolve requires a finite start and a sweep in (0, 360].";
		}
		return {};
	}
	if (candidate.angular_segments < 3) {
		if (diagnostic != nullptr) {
			*diagnostic = "Revolve requires at least three angular segments.";
		}
		return {};
	}

	std::shared_ptr<const Profile2D> profile =
		Profile2DValidator(complexity_limits_).validate(
			std::move(candidate.radial_profile), diagnostic);
	if (!profile) return {};
	if (!validate_nonnegative_radius(
			profile->outerLoop(), "Revolve outer profile", diagnostic)) {
		return {};
	}
	for (std::size_t hole_index = 0;
	     hole_index < profile->innerLoops().size();
	     ++hole_index) {
		if (!validate_nonnegative_radius(
				profile->innerLoops()[hole_index],
				"Revolve hole " + std::to_string(hole_index),
				diagnostic)) {
			return {};
		}
	}

	std::ostringstream key;
	key << "Revolve:v1:start=" << float_bits(candidate.start_degrees)
	    << ":sweep=" << float_bits(candidate.sweep_degrees)
	    << ":segments=" << candidate.angular_segments
	    << ":cap=" << candidate.angular_cap_policy.canonicalText()
	    << ":outer=";
	for (const glm::vec2 &point : profile->outerLoop().points()) {
		key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
	for (const ProfileLoop2D &inner_loop : profile->innerLoops()) {
		key << ":hole=";
		for (const glm::vec2 &point : inner_loop.points()) {
			key << float_bits(point.x) << "," << float_bits(point.y) << ";";
		}
	}

	std::ostringstream canonical_text;
	canonical_text << "Revolve(start(" << candidate.start_degrees
	               << ") angle(" << candidate.sweep_degrees
	               << ") segments(" << candidate.angular_segments
	               << ") cap(" << candidate.angular_cap_policy.canonicalText()
	               << "))";

	return std::make_shared<const RevolveShapeSpecification>(
		*profile,
		candidate.start_degrees,
		candidate.sweep_degrees,
		candidate.angular_segments,
		candidate.angular_cap_policy,
		ShapeSpecificationKey(key.str()),
		canonical_text.str(),
		candidate.detail_level);
}
