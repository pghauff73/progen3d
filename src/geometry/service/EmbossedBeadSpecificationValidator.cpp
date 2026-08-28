#include "geometry/service/EmbossedBeadSpecificationValidator.h"

#include "geometry/service/Profile2DFactory.h"
#include "geometry/service/SweepProfileSpecificationValidator.h"

#include <algorithm>
#include <cmath>
#include <utility>

std::shared_ptr<const EmbossedBeadShapeSpecification>
EmbossedBeadSpecificationValidator::validate(
	EmbossedBeadShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.width) || candidate.width <= 0.0f ||
	    !std::isfinite(candidate.depth) || candidate.depth <= 0.0f ||
	    !std::isfinite(candidate.shoulder_radius) ||
	    candidate.shoulder_radius < 0.0f ||
	    candidate.shoulder_radius * 2.0f >=
		    std::min(candidate.width, candidate.depth)) {
		if (diagnostic != nullptr) {
			*diagnostic = "EmbossedBead dimensions require positive width/depth and a shoulder radius smaller than half the minimum dimension.";
		}
		return {};
	}

	std::shared_ptr<const Profile2D> profile;
	if (candidate.shoulder_radius > 0.0f) {
		profile = Profile2DFactory(complexity_limits_).createRoundedRectangle(
			candidate.width,
			candidate.depth,
			candidate.shoulder_radius,
			1,
			diagnostic);
	}
	else {
		profile = Profile2DFactory(complexity_limits_).createRectangle(
			candidate.width, candidate.depth, diagnostic);
	}
	if (!profile) return {};

	const float side_offset = candidate.side == EmbossedBeadSide::Positive
		? candidate.depth * 0.5f
		: -candidate.depth * 0.5f;
	Profile2DCandidate offset_profile;
	for (const glm::vec2 &point : profile->outerLoop().points()) {
		offset_profile.outer_loop.emplace_back(point.x, point.y + side_offset);
	}

	SweepProfileShapeSpecificationCandidate sweep_candidate;
	sweep_candidate.profile = std::move(offset_profile);
	sweep_candidate.path_points = std::move(candidate.path_points);
	sweep_candidate.up_hint = candidate.up_hint;
	sweep_candidate.cap_policy = candidate.end_style == EmbossedBeadEndStyle::Closed
		? ExtrudeProfileCapPolicy::createAll()
		: ExtrudeProfileCapPolicy::createNone();
	sweep_candidate.detail_level = candidate.detail_level;
	std::shared_ptr<const SweepProfileShapeSpecification> sweep =
		SweepProfileSpecificationValidator(complexity_limits_).validate(
			std::move(sweep_candidate), diagnostic);
	if (!sweep) return {};

	const std::string side_name = candidate.side == EmbossedBeadSide::Positive
		? "positive"
		: "negative";
	const std::string end_name = candidate.end_style == EmbossedBeadEndStyle::Closed
		? "closed"
		: "open";
	return std::make_shared<const EmbossedBeadShapeSpecification>(
		sweep->profile(),
		sweep->pathPoints(),
		sweep->upHint(),
		sweep->capPolicy(),
		candidate.width,
		candidate.depth,
		candidate.shoulder_radius,
		candidate.side,
		candidate.end_style,
		ShapeSpecificationKey(
			"EmbossedBead:v1:" + sweep->key().canonicalValue() +
			":side=" + side_name + ":end=" + end_name),
		"EmbossedBead(width(" + std::to_string(candidate.width) +
			") depth(" + std::to_string(candidate.depth) +
			") shoulder(" + std::to_string(candidate.shoulder_radius) +
			") side(" + side_name + ") end(" + end_name + "))",
		candidate.detail_level);
}
