#include "geometry/service/EdgeFlangeSpecificationValidator.h"

#include "geometry/service/SweepProfileSpecificationValidator.h"

#include <glm/gtc/constants.hpp>

#include <cmath>
#include <utility>

std::shared_ptr<const EdgeFlangeShapeSpecification>
EdgeFlangeSpecificationValidator::validate(
	EdgeFlangeShapeSpecificationCandidate candidate,
	std::string *diagnostic) const
{
	if (!std::isfinite(candidate.width) || candidate.width <= 0.0f ||
	    !std::isfinite(candidate.thickness) || candidate.thickness <= 0.0f ||
	    !std::isfinite(candidate.angle_degrees) ||
	    std::fabs(candidate.angle_degrees) > 180.0f ||
	    !std::isfinite(candidate.bend_radius) || candidate.bend_radius < 0.0f) {
		if (diagnostic != nullptr) {
			*diagnostic = "EdgeFlange requires positive width/thickness, a finite angle within [-180,180], and a non-negative bend radius.";
		}
		return {};
	}

	const float side_sign = candidate.side == EdgeFlangeSide::Positive ? 1.0f : -1.0f;
	const float angle = glm::radians(candidate.angle_degrees * side_sign);
	const float cosine = std::cos(angle);
	const float sine = std::sin(angle);
	auto rotate = [cosine, sine](const glm::vec2 &point) {
		return glm::vec2(
			point.x * cosine - point.y * sine,
			point.x * sine + point.y * cosine);
	};
	const float half_thickness = candidate.thickness * 0.5f;
	Profile2DCandidate flange_profile;
	flange_profile.outer_loop = {
		rotate(glm::vec2(0.0f, -half_thickness)),
		rotate(glm::vec2(candidate.width, -half_thickness)),
		rotate(glm::vec2(candidate.width, half_thickness)),
		rotate(glm::vec2(0.0f, half_thickness))};

	SweepProfileShapeSpecificationCandidate sweep_candidate;
	sweep_candidate.profile = std::move(flange_profile);
	sweep_candidate.path_points = std::move(candidate.boundary_path);
	sweep_candidate.up_hint = candidate.up_hint;
	sweep_candidate.cap_policy = ExtrudeProfileCapPolicy::createAll();
	sweep_candidate.detail_level = candidate.detail_level;
	std::shared_ptr<const SweepProfileShapeSpecification> sweep =
		SweepProfileSpecificationValidator(complexity_limits_).validate(
			std::move(sweep_candidate), diagnostic);
	if (!sweep) return {};

	const std::string side_name = candidate.side == EdgeFlangeSide::Positive
		? "positive"
		: "negative";
	return std::make_shared<const EdgeFlangeShapeSpecification>(
		sweep->profile(),
		sweep->pathPoints(),
		sweep->upHint(),
		candidate.width,
		candidate.thickness,
		candidate.angle_degrees,
		candidate.bend_radius,
		candidate.side,
		ShapeSpecificationKey(
			"EdgeFlange:v1:" + sweep->key().canonicalValue() +
			":side=" + side_name +
			":bend=" + std::to_string(candidate.bend_radius)),
		"EdgeFlange(width(" + std::to_string(candidate.width) +
			") thickness(" + std::to_string(candidate.thickness) +
			") angle(" + std::to_string(candidate.angle_degrees) +
			") bend(" + std::to_string(candidate.bend_radius) +
			") side(" + side_name + "))",
		candidate.detail_level);
}
