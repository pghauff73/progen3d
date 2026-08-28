#include "geometry/service/FoldedProfileSpecificationValidator.h"

#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepProfileSpecificationValidator.h"

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

}

std::shared_ptr<const FoldedProfileShapeSpecification>
FoldedProfileSpecificationValidator::validate(
	FoldedProfileShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.thickness) || candidate.thickness <= 0.0f ||
	    !std::isfinite(candidate.extrusion_depth) ||
	    candidate.extrusion_depth <= 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "FoldedProfile thickness and extrusion depth must be finite and positive.";
		}
		return {};
	}
	std::string profile_diagnostic;
	auto strip_profile = Profile2DFactory(complexity_limits_).createRectangle(
		candidate.thickness, candidate.extrusion_depth, &profile_diagnostic);
	if (!strip_profile) {
		if (diagnostic != nullptr) *diagnostic = profile_diagnostic;
		return {};
	}
	SweepProfileShapeSpecificationCandidate sweep_candidate;
	sweep_candidate.profile.outer_loop = strip_profile->outerLoop().points();
	for (const glm::vec2 &point : candidate.fold_path) {
		sweep_candidate.path_points.emplace_back(point.x, point.y, 0.0f);
	}
	sweep_candidate.up_hint = glm::vec3(0.0f, 0.0f, 1.0f);
	sweep_candidate.cap_policy = candidate.cap_policy;
	sweep_candidate.detail_level = candidate.detail_level;
	if (!SweepProfileSpecificationValidator(complexity_limits_).validate(
			sweep_candidate, diagnostic)) {
		return {};
	}

	std::ostringstream key;
	key << "FoldedProfile:v1:t=" << float_bits(candidate.thickness)
	    << ":depth=" << float_bits(candidate.extrusion_depth)
	    << ":cap=" << candidate.cap_policy.canonicalText() << ":path=";
	for (const glm::vec2 &point : candidate.fold_path) {
		key << float_bits(point.x) << "," << float_bits(point.y) << ";";
	}
	std::ostringstream canonical_text;
	canonical_text << "FoldedProfile(thickness(" << candidate.thickness
	               << ") depth(" << candidate.extrusion_depth << ") path(";
	for (const glm::vec2 &point : candidate.fold_path) {
		canonical_text << point.x << " " << point.y << " ";
	}
	canonical_text << ") cap(" << candidate.cap_policy.canonicalText() << "))";

	return std::make_shared<const FoldedProfileShapeSpecification>(
		std::move(candidate.fold_path),
		candidate.thickness,
		candidate.extrusion_depth,
		candidate.cap_policy,
		ShapeSpecificationKey(key.str()),
		canonical_text.str(),
		candidate.detail_level);
}
