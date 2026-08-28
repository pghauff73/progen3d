#include "geometry/service/HostedOpeningSpecificationValidator.h"

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

void append_loop_key(std::ostringstream *key, const ProfileLoop2D &loop)
{
	for (const glm::vec2 &point : loop.points()) {
		*key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
}

}

std::shared_ptr<const HostedOpeningShapeSpecification>
HostedOpeningSpecificationValidator::validate(
	HostedOpeningShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (candidate.opening_boundaries.empty()) {
		if (diagnostic != nullptr) {
			*diagnostic = "HostedOpening requires at least one opening boundary.";
		}
		return {};
	}
	if (!std::isfinite(candidate.depth) || candidate.depth <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "HostedOpening host depth must be finite and greater than zero.";
		}
		return {};
	}

	std::shared_ptr<const Profile2D> profile =
		Profile2DValidator(complexity_limits_).validate(
			Profile2DCandidate{
				std::move(candidate.host_boundary),
				std::move(candidate.opening_boundaries)},
			diagnostic);
	if (!profile) return {};

	std::ostringstream key;
	key << "HostedOpening:v1:depth=" << float_bits(candidate.depth)
	    << ":cap=" << candidate.cap_policy.canonicalText() << ":host=";
	append_loop_key(&key, profile->outerLoop());
	for (const ProfileLoop2D &opening : profile->innerLoops()) {
		key << ":opening=";
		append_loop_key(&key, opening);
	}
	return std::make_shared<const HostedOpeningShapeSpecification>(
		*profile,
		candidate.depth,
		candidate.cap_policy,
		profile->innerLoops().size(),
		ShapeSpecificationKey(key.str()),
		"HostedOpening(openings(" +
			std::to_string(profile->innerLoops().size()) +
			") depth(" + std::to_string(candidate.depth) + "))",
		candidate.detail_level);
}
